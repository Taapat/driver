/**************************************************************
 * Copyright (C) 2010   STMicroelectronics. All Rights Reserved.
 * This file is part of the latest release of the Multicom4 project. This release 
 * is fully functional and provides all of the original MME functionality.This 
 * release  is now considered stable and ready for integration with other software 
 * components.

 * Multicom4 is a free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation 
 * version 2.

 * Multicom4 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along with Multicom4; 
 * see the file COPYING.  If not, write to the Free Software Foundation, 59 Temple Place - 
 * Suite 330, Boston, MA 02111-1307, USA.

 * Written by Multicom team at STMicroelectronics in November 2010.  
 * Contact multicom.support@st.com. 
**************************************************************/

/* 
 * 
 */ 

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */

/* Take the Client supplied command struct and marshall it into
 * a (SHM) buffer for transmission to the receiver task
 */
MME_ERROR mme_command_pack (mme_transformer_t *transformer, MME_Command_t *command, mme_msg_t **msgp)
{
  MME_ERROR              res = MME_NOMEM;

  union {
    mme_transform_t     *transform;
    MME_DataBuffer_t    *buffer;
    MME_ScatterPage_t   *page;
    char                *ch;
    int                  i;
    void                *p;
    void               **pp;
  } p, size;

  mme_msg_t             *msg;

  unsigned int           i, j;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif

  /* all argument checking should have been performed further up the food chain
   * so all the argument checking in this function can asserted
   */
  MME_ASSERT(transformer);
  MME_ASSERT(command);
  MME_ASSERT(msgp);

  MME_PRINTF(MME_DBG_COMMAND,
	     "transformer %p command %p NumInBufs %d NumOutBufs %d\n",
	     transformer, command, command->NumberInputBuffers, command->NumberOutputBuffers);

  /* determine the size of message we will need */
  size.i          = 0;
  size.transform += 1;							/* Include transform header */
  /* Array of MME_DataBuffer_t ptrs */ 
  size.pp        += command->NumberInputBuffers + command->NumberOutputBuffers;
  /* Array of MME_DataBuffer_t structs */
  size.buffer    += command->NumberInputBuffers + command->NumberOutputBuffers;
  for (i=0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++)
  {
    /* Add in MME_ScatterPage_t structs */
    size.page += command->DataBuffers_p[i]->NumberOfScatterPages;
  }
  size.i += command->CmdStatus.AdditionalInfoSize;			/* Add in CmdStatus extra stuff */
  size.i += command->ParamSize;						/* Add in Command extra stuff */

  MME_PRINTF(MME_DBG_COMMAND,
	     "allocating msg of size %d\n",
	     size.i);

  /* allocate the Transform message (takes/drops MME state lock) */
  msg = mme_msg_alloc(size.i);
  if (msg == NULL)
    return MME_NOMEM;

  MME_PRINTF(MME_DBG_COMMAND,
	     "allocated msg %p size %d\n",
	     msg, size.i);

  /* Begin the marshalling */
  p.p = msg->buf;
  
  /* populate the Transform message header */
  p.transform->type      = _MME_RECEIVER_TRANSFORM;
  p.transform->replyPort = transformer->replyPort;
  p.transform->size      = size.i;

  /* Now the transform bit */
  p.transform->receiverHandle = transformer->receiverHandle;  /* used for sanity check on companion */
  p.transform->command        = *command;		/* Copy all of Client MME_Command_t structure */

  p.transform += 1;		/* Skip over transform header */

  /* skip the (MME_DataBuffer_t *) array */
  p.pp += command->NumberInputBuffers + command->NumberOutputBuffers;

  /* copy all the MME_DataBuffers into our message */
  for (i=0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++)
  {
    MME_PRINTF(MME_DBG_COMMAND, "Copy DataBuffer %d at %p to %p\n",
	       i, command->DataBuffers_p[i], p.buffer);
    
    /* Copy whole of MME_DataBuffer_t structure */
    *p.buffer++ = *command->DataBuffers_p[i];
  }
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* copy the all MME_ScatterPages from all the MME_DataBuffers into our message,
   * converting the Page_p address into an ICS_OFFSET for each one
   */
  for (i=0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++)
  {
    MME_DataBuffer_t  *buf = command->DataBuffers_p[i];
    MME_ScatterPage_t *pages;
    
    ICS_MEM_FLAGS      mflags;
		
    pages   = p.page;
    p.page += buf->NumberOfScatterPages;	/* Skip over array of MME_ScatterPage_t */

    for (j=0; j < buf->NumberOfScatterPages; j++)
    {
      ICS_ERROR          err;
      ICS_OFFSET         hdl;

      MME_ScatterPage_t *page = &buf->ScatterPages_p[j];

      MME_PRINTF(MME_DBG_COMMAND, "Buf %d Copy Page %d at %p to %p\n", i, j, page, &pages[j]);

      /* copy the original MME_ScatterPage_t into the marshall buffer */
      pages[j] = *page;
      
      if ((page->FlagsIn & MME_DATA_PHYSICAL))
      {
	/* simple case as the address is already a physical one */
	MME_PRINTF(MME_DBG_COMMAND,
		   "Page_p %p (%s), Handle 0x%08x buf %d, page %d\n", 
		   page->Page_p, "PHYS", page->Page_p, i, j);
	continue;
      }

      /* Intercept NULL pointer addresses as soon as we can 
       * otherwise we will end up attempting to perform virt2phys on it
       *
       * But allow through zero sized pages as some drivers can generate these under normal conditions
       */
      if (page->Size == 0)
	continue;

      if (page->Page_p == NULL)
      {
	res = MME_INVALID_ARGUMENT;
	goto error_free;
      }

      /* Convert the Virtual address into an ICS_OFFSET using the registered regions
       */
      err = ICS_region_virt2phys(page->Page_p, &hdl, &mflags);
      if (err != ICS_SUCCESS)
      {
	/* XXXX Help! What can we do now as there is no ICS mapping for this Client page */
	MME_EPRINTF(MME_DBG_COMMAND, 
		    "Page_p %p Size %d Failed to convert addr err %d, buf %d, page %d\n", 
		    page->Page_p, page->Size, err, i, j);
	
	res = MME_EMBX_ERROR;
	goto error_free;
      }

      MME_PRINTF(MME_DBG_COMMAND,
		 "Page_p %p (%s) Handle 0x%08x buf %d, page %d\n", 
		 page->Page_p, 
		 (mflags & ICS_CACHED) ? "CACHED" : "UNCACHED",
		 hdl, i, j);

      /* Finally update the Page_p in the marshall buffer */
      pages[j].Page_p = (void *) hdl;

      /* Check the Cache flags and Purge page if necessary but allow Client
       * to flag this has already been done with MME_DATA_CACHE_COHERENT
       *
       * Purge both Input & Output buffers now so we don't need to do it
       * again for the Output buffers on Command completion
       */
      if ((mflags & ICS_CACHED) && !(page->FlagsIn & MME_DATA_CACHE_COHERENT))
      {
	_ICS_OS_CACHE_PURGE(page->Page_p, hdl, page->Size);
      }
      else if ((mflags & ICS_UNCACHED))
      {
	/* XXXX Multicom4 is this the behaviour we want ? */
	/* Set this flag to cause remote mapping to be uncached too */
	pages[j].FlagsIn |= MME_REMOTE_CACHE_COHERENT;
      }
    } /* for (j=0; j < buf->NumberOfScatterPages; j++) */

  } /* for (i=0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++) */
	
  /* XXXX: Need to use a macro to swap all args to little endian if on a big endian machine */

  if (command->CmdStatus.AdditionalInfoSize > 0)
  {
    _ICS_OS_MEMCPY(p.p, command->CmdStatus.AdditionalInfo_p, command->CmdStatus.AdditionalInfoSize);
    p.ch += command->CmdStatus.AdditionalInfoSize;
  }

  if (command->ParamSize > 0)
    _ICS_OS_MEMCPY(p.p, command->Param_p, command->ParamSize);

  /* Return allocated message desc to caller */
  *msgp = msg;
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif

  return MME_SUCCESS;

error_free:
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  mme_msg_free(msg);

  return res;
}

/* 
 * Reverse the marshalling done in mme_command_pack() to correctly
 * fill out the embedded MME_Command_t structure in a transform message.
 * This also converts all the ICS_OFFSET values back to local virtual addresses
 * an updates the ScatterPage Page_p pointer accordingly
 */
MME_ERROR mme_command_unpack (mme_receiver_t *receiver, mme_transform_t *transform)
{
  union {
    mme_transform_t     *transform;
    MME_DataBuffer_t    *buffer;
    MME_ScatterPage_t   *page;
    char                *ch;
    void                *p;
    void               **pp;
  } p;

  MME_Command_t         *command;
  MME_DataBuffer_t      *buf_p, **buf_pp;

  int                    i, j, nBufs;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif

  MME_ASSERT(transform);

  /* a few up front validation checks */
  p.transform = transform;
  MME_assert(p.transform->receiverHandle == (MME_TransformerHandle_t)receiver);

  /* extract the embedded Command pointer */
  command = &p.transform->command;
  MME_ASSERT(command->CmdCode <= MME_SEND_BUFFERS);

  MME_PRINTF(MME_DBG_COMMAND, "receiver %p transform %p NiB %d NoB %d\n", 
	     receiver, transform,
	     command->NumberInputBuffers, command->NumberOutputBuffers);
	   
  p.transform += 1;	/* Skip over the transform header */

  nBufs = command->NumberInputBuffers + command->NumberOutputBuffers;

  /* extract the data buffer pointers */
  buf_pp              = p.p;		/* Base of MME_DataBuffer_t ptr array */
  command->DataBuffers_p  = buf_pp;	/* Update Command DataBuffers_p */
  p.pp               += nBufs;		/* Skip over array of MME_DataBuffer_t ptrs */

  buf_p               = p.buffer; 	/* Base of MME_DataBuffer_t struct array */
  p.buffer           += nBufs;		/* Skip over array of MME_DataBuffer_t structs */
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  for (i=0; i < nBufs; i++)
  {
    int nPages               = buf_p[i].NumberOfScatterPages;
    MME_ScatterPage_t *pages = p.page;

    /* update the pointers to and in the data buffers */
    buf_pp[i] = &buf_p[i];
    buf_p[i].ScatterPages_p = pages;

    /* Now skip over the nPages ScatterPage array */
    p.page += nPages;

    MME_PRINTF(MME_DBG_COMMAND, "unpack buf %d NumberOfScatterPages %d\n", i, nPages);

    /* Convert the ICS_OFFSET to an address for each ScatterPage Page_p */
    for (j=0; j < nPages; j++)
    {
      ICS_ERROR     err;
      ICS_MEM_FLAGS mflags;
      ICS_OFFSET    hdl;

      /* Don't perform any translations on Pages flagged as being PHYSICAL addresses */
      /* Also don't bother if the size is 0 */
      if (((pages[j].FlagsOut|pages[j].FlagsOut) & MME_DATA_PHYSICAL) || (pages[j].Size == 0))
      {
	continue;
      }

      /* Derive the cache mapping flags from the FlagsIn bits
       * 
       * XXXX Multicom4
       * We have now borrowed this undocumented MME_REMOTE_CACHE_COHERENT
       * flag bit to indicate that the buffer should be translated to
       * an UNCACHED address in the Companion when set. Otherwise the
       * default MME_DataBuffer cache flags are used.
       */
      if (pages[j].FlagsIn & MME_REMOTE_CACHE_COHERENT)
	mflags = ICS_UNCACHED;
      else
	/* Use default MME_DataBuffer cache flags */
	mflags = _MME_BUF_CACHE_FLAGS;
      
      hdl = (ICS_OFFSET) pages[j].Page_p;
      
      MME_PRINTF(MME_DBG_COMMAND,
		 "buffer %d, page %d - FlagsIn 0x%x hdl 0x%08x\n",
		 i, j, pages[j].FlagsIn, hdl);
      
      err = ICS_region_phys2virt(hdl, pages[j].Size, mflags, &pages[j].Page_p);
      if (err != ICS_SUCCESS)
      {
	MME_EPRINTF(MME_DBG_COMMAND, 
		    "Page %p[%d] Size %d Failed to convert hdl 0x%x (%s) : %s (%d)\n",
		    &pages[j], j,
		    pages[j].Size,
		    hdl,
		    (mflags & ICS_CACHED) ? "CACHED" : "UNCACHED",
		    ics_err_str(err), err);
	MME_assert(err == ICS_SUCCESS);
      }

      MME_PRINTF(MME_DBG_COMMAND, 
		 "Page %p[%d] Converted hdl 0x%x to Page_p %p (%s)\n",
		 &pages[j], j, hdl,
		 pages[j].Page_p,
		 (mflags & ICS_CACHED) ? "CACHED" : "UNCACHED");

#if 0
      /* Only PURGE Input buffers */
      /* XXXX This optimisation is not safe as OutputBuffers are effectively rw */
      if (i >= command->NumberInputBuffers)
	continue;
#endif

      if (mflags & ICS_CACHED)
      {
	/* XXXX Do we need this ? What was the original use of MME_REMOTE_CACHE_COHERENT ? */
	if (!(pages[j].FlagsOut & MME_REMOTE_CACHE_COHERENT))
	{
	  MME_PRINTF(MME_DBG_COMMAND,
		     "purging CACHE %p paddr 0x%lx size %d\n",
		     pages[j].Page_p, hdl, pages[j].Size);
	  
	  /* Purge cache before recipient reads the buffer */
	  _ICS_OS_CACHE_PURGE(pages[j].Page_p, hdl, pages[j].Size);
	}
      }
    } /* for (j=0; j < nPages; j++) */
  }
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  /* Now the CmdStatus parametric information (if there is any) */
  if (command->CmdStatus.AdditionalInfoSize > 0)
  {
    command->CmdStatus.AdditionalInfo_p = p.p;
    p.ch += command->CmdStatus.AdditionalInfoSize;

    MME_PRINTF(MME_DBG_COMMAND,
	       "command %p CmdStatus AdditionalInfoSize %d AdditionalInfo_p %p\n",
	       command,
	       command->CmdStatus.AdditionalInfoSize,
	       command->CmdStatus.AdditionalInfo_p);
  }

  /* Finally the Command additional Parameter info */
  if (command->ParamSize > 0) 
  {
    command->Param_p = p.p;

    MME_PRINTF(MME_DBG_COMMAND,
	       "command %p ParamSize %d Param_p %p\n",
	       command,
	       command->ParamSize, command->Param_p);
  }

  return MME_SUCCESS;
}

/*
 * Called on the Companion once the transform has completed to walk the
 * MME_DataBuffers again and purge the cache of the ScatterPages so that
 * all changes are committed to memory before responding to the host
 */
MME_ERROR mme_command_repack (mme_receiver_t *receiver, mme_transform_t *transform)
{
  union {
    mme_transform_t     *transform;
    MME_DataBuffer_t    *buffer;
    MME_ScatterPage_t   *page;
    void               **pp;
  } p;

  MME_Command_t         *command;
  MME_DataBuffer_t      *buf_p;
  int                    i, j, nBufs;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif

  MME_ASSERT(receiver);
  MME_ASSERT(transform);

  MME_PRINTF(MME_DBG_COMMAND,
	     "receiver %p transform %p\n",
	     receiver, transform);

  /* a few up front validation checks */
  p.transform = transform;
  MME_assert(p.transform->receiverHandle == (MME_TransformerHandle_t)receiver);

  /* extract the command pointer */
  command = &p.transform->command;
  MME_assert(command->CmdCode <= MME_SEND_BUFFERS);

  p.transform += 1;

  nBufs = command->NumberInputBuffers + command->NumberOutputBuffers;

  /* skip the MME_DataBuffer_t sections */
  p.pp               += nBufs;		/* Skip over array of MME_DataBuffer_t ptrs */
  buf_p               = p.buffer; 	/* Base of MME_DataBuffer_t struct array */
  p.buffer           += nBufs;		/* Skip over array of MME_DataBuffer_t structs */
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* now iterate over all the MME_DataBuffer ScatterPages */
  for (i=0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++)
  {
    MME_ScatterPage_t *pages = p.page;

    /* Skip over array of MME_ScatterPage_t structs */
    p.page += buf_p[i].NumberOfScatterPages;

    /* Only purge Output buffers */
    if (i < command->NumberInputBuffers)
      continue;

    MME_PRINTF(MME_DBG_COMMAND, "repack buf %d NumberOfScatterPages %d\n", i, buf_p[i].NumberOfScatterPages);

    /* every Page must be updated so that it does not persist in the cache allowing
     * stale values to be read when the buffer is returned or reused
     */
    for (j=0; j < buf_p[i].NumberOfScatterPages; j++)
    {
      MME_ScatterPage_t *page = &pages[j];

#ifdef __KERNEL__      
      ICS_ERROR          err;
      ICS_OFFSET         paddr;
      ICS_MEM_FLAGS      mflags;
#endif
      
      MME_PRINTF(MME_DBG_COMMAND,
		 "repack buf %d page %d - Page_p %p FlagsIn 0x%x FlagsOut 0x%x\n",
		 i, j, page->Page_p, page->FlagsIn, page->FlagsOut);
      
      /* Don't need to do any virtual address lookup or cache flushing
       * for these cases
       */
      if ((page->FlagsOut & MME_DATA_CACHE_COHERENT) ||
	  (page->FlagsIn & MME_REMOTE_CACHE_COHERENT) ||	/* XXXX Multicom4 extension */
	  (page->FlagsIn & MME_DATA_TRANSIENT) ||
	  ((page->FlagsIn|page->FlagsOut) & MME_DATA_PHYSICAL) ||
	  (page->Size == 0))					/* ignore size 0 pages */
	continue;

      MME_PRINTF(MME_DBG_COMMAND, 
		 "Purge CACHED page %p size %d\n",
		 page->Page_p, page->Size);

#ifdef __KERNEL__      
      /* Need physical address for purge */
      err = ICS_region_virt2phys(page->Page_p, &paddr, &mflags);
      MME_ASSERT(err == ICS_SUCCESS);
#endif

      _ICS_OS_CACHE_PURGE(page->Page_p, paddr, page->Size);

    } /* for each ScatterPage */

  } /* for each DataBuffer */
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  return MME_SUCCESS;
}

/*
 * Called on the host as the transform message comes back
 * We need to walk the original MME_Command DataBuffers again
 * updating the OutputBuffer ScatterPage info which has been
 * filled in by the Companion
 *
 * MULTITHREAD SAFE: Called holding the transformer lock when in task context
 * This can be called in IRQ context if we are not using a callback task
 */
MME_ERROR mme_command_complete (MME_Command_t *command, mme_transform_t *transform)
{
  union {
    mme_transform_t     *transform;
    MME_DataBuffer_t    *buffer;
    MME_ScatterPage_t   *page;
    int                  i;
    void                *p;
    void               **pp;
  } p;
  
  unsigned int           i, j;
  
  MME_CommandStatus_t   *status;

  /* Sanity checks */
  MME_ASSERT(command);
  MME_ASSERT(command->StructSize = sizeof(MME_Command_t));

  MME_ASSERT(transform);

  /* a few up front validation checks */
  p.transform = transform;

  MME_assert(transform->command.NumberInputBuffers  == command->NumberInputBuffers);
  MME_assert(transform->command.NumberOutputBuffers == command->NumberOutputBuffers);

  /* Skip to the ScatterPages array */
  p.transform += 1;
  p.pp      += command->NumberInputBuffers + command->NumberOutputBuffers;
  p.buffer  += command->NumberInputBuffers + command->NumberOutputBuffers;

  MME_ASSERT(command->DataBuffers_p || ((command->NumberInputBuffers+command->NumberOutputBuffers) == 0));
  
  for (i = 0; i < (command->NumberInputBuffers + command->NumberOutputBuffers); i++)
  {
    MME_DataBuffer_t  *buf   = command->DataBuffers_p[i];
    MME_ScatterPage_t *pages = p.page;

    /* Sanity checks */
    MME_ASSERT(buf);
    MME_ASSERT(buf->StructSize == sizeof(MME_DataBuffer_t));
    MME_ASSERT(buf->ScatterPages_p);
    
    /* Skip over array of MME_ScatterPage_t structs */
    p.page += buf->NumberOfScatterPages;

    /* Update the OutputBuffer ScatterPages only */
    if (i < command->NumberInputBuffers)
      continue;

    for (j=0; j < buf->NumberOfScatterPages; j++)
    {
      /* Update caller's ScatterPages accordingly */
      buf->ScatterPages_p[j].BytesUsed = pages[j].BytesUsed;
      buf->ScatterPages_p[j].FlagsOut  = pages[j].FlagsOut;
    }
  }

  /* Deal with any CmdStatus parametric data */
  /* XXXX Should this use the message's AdditionalInfoSize and
   * hence allow it to be set to zero by the Companion ?
   */
  if (command->CmdStatus.AdditionalInfoSize > 0)
  {
    MME_ASSERT(command->CmdStatus.AdditionalInfo_p);
    _ICS_OS_MEMCPY(command->CmdStatus.AdditionalInfo_p, p.p, command->CmdStatus.AdditionalInfoSize);
  }

  /* Update the Client's CommandStatus structure */
  status                = &command->CmdStatus;
  status->Error         = transform->command.CmdStatus.Error;
  status->ProcessedTime = transform->command.CmdStatus.ProcessedTime;
  
  /* At the moment that we write the command state, a sampling client will know
   * we are finished with the command. The command has completed or failed. 
   */
  status->State         = transform->command.CmdStatus.State;

  MME_PRINTF(MME_DBG_COMMAND, 
	     "updated Status %p : Error %d ProcessedTime %d State %d\n",
	     status,
	     status->Error, status->ProcessedTime, status->State);

  return MME_SUCCESS;
}

/*
 * Perform the associated transformer callback or post an event
 * Can be called in either task or IRQ context
 * 
 * MULTITHREAD SAFE: Called holding the transformer lock (when in task context)
 * Drops the lock before exit
 */
MME_ERROR mme_command_callback (mme_transformer_t *transformer, MME_Event_t event, mme_command_t *cmd)
{
  MME_Command_t  *command;

  /* Locate the Client's original MME_Command_t address */
  command = cmd->command;
  MME_ASSERT(command);

  MME_PRINTF(MME_DBG_COMMAND,
	     "cmd %p event %d command %p [0x%x] CmdEnd %d\n",
	     cmd, event, command, command->CmdStatus.CmdId, command->CmdEnd);

  /*
   * Perform the Callback if the Client requested it
   */
  if ((command->CmdEnd == MME_COMMAND_END_RETURN_NOTIFY) || 
      (command->CmdEnd == MME_COMMAND_END_RETURN_NO_INFO))
  {
    MME_GenericCallback_t callback;
    void 		  *callbackData;

    /* We must not take this path from an IRQ */
    MME_ASSERT(!_ICS_OS_TASK_INTERRUPT());

    /* Stash these for later (as we drop the transformer lock) */
    callback     = transformer->callback;
    callbackData = transformer->callbackData;

    if (event == MME_COMMAND_COMPLETED_EVT || event == MME_TRANSFORMER_TIMEOUT)
    {
      /* Free off the associated msg buffer */
      MME_ASSERT(cmd->msg);
      mme_msg_free(cmd->msg); /* takes and releases mme_state lock */
      cmd->msg = NULL;
      
      /* Free off the local command desc */
      mme_cmd_free(transformer, cmd);
    }

    /* Drop the lock before calling the user callback */
    _ICS_OS_MUTEX_RELEASE(&transformer->tlock);
    
    /* Multicom3 comment
     * it would really make more sense to assert callback and make it illegal to send
     * commands with MME_COMMAND_END_RETURN_NOTIFY fail if callback is NULL. However this
     * would mean changing the behaviour of the API.
     */
    if (callback && (command->CmdEnd == MME_COMMAND_END_RETURN_NOTIFY))
    {
      MME_PRINTF(MME_DBG_COMMAND,
		 "Calling callback %p: event %d command %p callbackData %p\n",
		 callback,
		 MME_COMMAND_COMPLETED_EVT,
		 command, callbackData);

      (callback) (event, command, callbackData);
    }
  }
  else if (command->CmdEnd == MME_COMMAND_END_RETURN_WAKE)
  {
    /* MME4 API extension
     * Support for MME_WaitCommand() where the client can block waiting for
     * individual commands to complete.
     *
     * In this case they must set CmdEnd = MME_COMMAND_END_RETURN_WAKE which causes
     * an OS Event to be signalled on completion
     */
    MME_PRINTF(MME_DBG_COMMAND,
	       "Posting wake : command %p cmd %p event %p\n",
	       command, cmd, &cmd->block);

#if 0
    /* XXXX What can we do here if multiple notifications occur before wait ? */
    /* XXXX But this can also occur when commands are aborted */
    MME_ASSERT(!_ICS_OS_EVENT_READY(&cmd->block));
#endif

    /* Update the cmd event state so it can be passed back to user during wait */
    cmd->event = event;

    if (event == MME_COMMAND_COMPLETED_EVT || event == MME_TRANSFORMER_TIMEOUT)
    {
      /* Mark internal command as complete so we don't attempt to abort/kill it */
      cmd->state = _MME_COMMAND_COMPLETE;
    }

    /* Wake up blocked waiter. It will free off the local cmd desc */
    _ICS_OS_EVENT_POST(&cmd->block);

    /* Lock is only held when called from task context */
    if (!_ICS_OS_TASK_INTERRUPT())
      _ICS_OS_MUTEX_RELEASE(&transformer->tlock);
  }
  else
  {
    /* Unrecognised CmdEnd code (was checked in MME_SendCommand) */
    MME_assert(0);
  }

  return MME_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
