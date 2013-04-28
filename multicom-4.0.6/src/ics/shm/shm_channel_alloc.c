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

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */


#include "_ics_shm.h"	/* SHM transport specific headers */

/* 
 * Find first free channel index
 *
 * Returns -1 if non found
 *
 * MULTITHREAD SAFE: Must hold state lock
 *
 */
static
int findFreeChn (void)
{
  unsigned i;
  
  ICS_assert(ics_shm_state);
#ifndef CONFIG_SMP
  ICS_ASSERT(_ICS_OS_MUTEX_HELD(&ics_shm_state->shmLock));
#endif

  for (i = 0; i < _ICS_MAX_CHANNELS; i++)
  {
    /* Determine whether a channel is free or not from the fifo ptr */
    if (ics_shm_state->channel[i].fifo == NULL)
      return i;
  }
  
  /* Not found */
  return -1;
}

/*
 * Free off the local SHM channel at idx
 */
ICS_ERROR ics_shm_channel_free (ICS_HANDLE handle)
{
  ics_shm_channel_t *channel;
  ics_chn_t         *chn;

  int type, cpuNum, ver, chan;
#ifdef CONFIG_SMP
  unsigned long  iflags;
#endif

  /* Decode the channel handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, chan);
  
  /* Check the channel handle */
  if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || chan >= _ICS_MAX_CHANNELS)
  {
    return ICS_HANDLE_INVALID;
  }

  /* Must be a local channel */
  if (cpuNum != ics_state->cpuNum)
  {
    return ICS_HANDLE_INVALID;
  }
  
#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  ICS_assert(ics_shm_state->shm);
  ICS_assert(chan < _ICS_MAX_CHANNELS);
#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
#endif

  channel = &ics_shm_state->channel[chan];
  chn     = &ics_shm_state->shm->chn[chan];

  if (channel->fifo == NULL)
  {
#ifndef CONFIG_SMP
    _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#else
     /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    return ICS_INVALID_ARGUMENT;
  }

  ICS_ASSERT(ICS_PAGE_ALIGNED(channel->fifo));

  ICS_assert(chn->fifo.fptr == chn->fifo.bptr);		/* XXXX Should be empty */

  /* Clear the interrupt enable mask for this channel idx */
  ics_mailbox_interrupt_disable(ics_shm_state->mbox[_ICS_MAILBOX_CHN2MBOX(chan)], _ICS_MAILBOX_CHN2IDX(chan));

  /* Unmap FIFO if necessary */
  if (channel->fifo != channel->mem)
    _ICS_OS_MUNMAP(channel->fifo);

  /* Free off the SHM channel fifo memory (if we allocated it) */
  if (channel->umem == NULL)
  {
#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    _ICS_OS_CONTIG_FREE(channel->mem);
#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  }

  /* Destroy the channel mutex */
  ICS_ASSERT(!_ICS_OS_MUTEX_HELD(&channel->chanLock));
  _ICS_OS_MUTEX_DESTROY(&channel->chanLock);

  /* XXXX What happens if someone is blocked on this ? */
  _ICS_OS_EVENT_DESTROY(&channel->event);

  /* XXXX chn desc is reset during ics_chn_init() */

  /* Clear down local desc (effectively freeing it) */
  _ICS_OS_MEMSET(channel, 0, sizeof(*channel));
#ifdef CONFIG_SMP
  /* release spinlok */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#else
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#endif
  
  return ICS_SUCCESS;
}

/*
 * Allocate and initialise a new local SHM channel
 * Memory can be supplied by caller otherwise it will be allocated
 * from the OS contiguous memory allocator
 *
 * Returns allocated channel handle to caller in handlep
 */
ICS_ERROR ics_shm_channel_alloc (ICS_CHANNEL_CALLBACK callback,
				 void *param, 
				 ICS_VOID *umem, ICS_UINT nslots, ICS_UINT ssize, ICS_HANDLE *handlep)
{
  ICS_ERROR          err = ICS_SUCCESS;

  ics_shm_channel_t *channel;
  ics_chn_t         *chn;
  ICS_SIZE           chnSize;

  void              *mem;
  void              *fifo;
  ICS_OFFSET         paddr;
  ICS_MEM_FLAGS      mflags;

  int                chan;
#ifdef CONFIG_SMP
  unsigned long  iflags;
#endif

  ICS_assert(ics_shm_state->shm);

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
	     "callback %p param %p nslots %d ssize %d\n",
	     callback, param, nslots, ssize);
  
  ICS_assert(nslots >= 2);
  ICS_assert(ssize && ALIGNED(ssize, sizeof(int)));

  /* XXXX Do we have to enforce ICS_CACHELINE_ALIGNED ssize ??? */
/*   ICS_assert(ssize && ICS_CACHELINE_ALIGNED(ssize)); */

#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#else
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
#endif

  /* Find a free channel index */
  if ((chan = findFreeChn()) == -1)
  {
#ifdef CONFIG_SMP
  /* Release spinlock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
    /* Failed to find a free channel index */
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
		"callback %p param %p nslots %d ssize %d: Failed to find free channel\n",
		callback, param, nslots, ssize);
    
    err = ICS_ENOMEM;
    
    goto error_release;
  }

  /* Allocate the FIFO memory (page aligned and multiples of whole pages) */
  chnSize = nslots * ssize;
  chnSize = ALIGNUP(chnSize, ICS_PAGE_SIZE);

#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  if (umem == NULL)
  {
    /* Allocate the FIFO memory from contiguous OS pages */
    if ((mem = _ICS_OS_CONTIG_ALLOC(chnSize, ICS_PAGE_SIZE)) == NULL)
    {
      /* Failed to allocate the SHM Channel FIFO */
      ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
		  "Failed to allocate FIFO memory : size %d\n",
		  chnSize);
      
      err = ICS_ENOMEM;
      
      goto error_free;
    }
  }
  else
  {
    /* Use the user supplied memory as the fifo base */
    mem = umem;
    
    /* XXXX Perhaps we should check it is physically contiguous ? */
  }

  ICS_assert(ICS_PAGE_ALIGNED(mem));
  ICS_assert(ICS_PAGE_ALIGNED(chnSize));

#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif

#ifdef CONFIG_SMP
  /* Since we dropped spinlock while allocationg contigious memory, make sure you still have hold
   *  the free channel index*/
  if ((chan = findFreeChn()) == -1)
  {
    /* Release spinlock */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
    /* Failed to find a free channel index */
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
		   "callback %p param %p nslots %d ssize %d: Failed to find free channel\n",
		    callback, param, nslots, ssize);
    
    err = ICS_ENOMEM;
    goto error_free;
  }
#endif

  /* Grab handles to the local and SHM channel descs */
  channel = &ics_shm_state->channel[chan];
  chn     = &ics_shm_state->shm->chn[chan];

  /* MULTITHREAD SAFE lock */
  if (!_ICS_OS_MUTEX_INIT(&channel->chanLock))
  {
    err = ICS_ENOMEM;
    goto error_free;
  }
  
  /* An OS event for signalling and blocking on */
  if (!_ICS_OS_EVENT_INIT(&channel->event))
  {
    /* Failed to initialize the OS event */
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
	       "Failed to initialize event %p\n",
	       &channel->event);

    err = ICS_ENOMEM;
    goto error_mutex_free;
  }

  /* Convert the FIFO address to a physical one */
  err = _ICS_OS_VIRT2PHYS(mem, &paddr, &mflags);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialize the OS event */
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_CHN,
		"Failed to convert fifo addr %p to phyiscal : %s (%d)\n",
		mem,
		ics_err_str(err), err);
    
    goto error_mutex_free;
  }    

  /* Zap the FIFO to aid debugging */
  _ICS_OS_MEMSET(mem, 0x69, chnSize);
  _ICS_OS_CACHE_PURGE(mem, paddr, chnSize);

  if (!_ICS_SHM_FIFO_CACHED)
  {
    /* Uncached FIFO */
    fifo = _ICS_OS_MMAP(paddr, chnSize, ICS_FALSE /* uncached */);
    if (fifo == NULL)
    {
      ICS_EPRINTF(ICS_DBG_CHN,
		  "Failed to map FIFO paddr 0x%x size %d\n",
		  paddr, chnSize);
      
      err = ICS_SYSTEM_ERROR;
      goto error_event_free;
    }
  }
  else
  {
    /* Cached FIFO */
    fifo = mem;
  }

  /* Initialise the SHM channel desc */
  ics_chn_init(chn, paddr, nslots, ssize);

  /* Fill out the channel desc */
  channel->callback = callback;
  channel->param    = param;
  channel->umem     = umem;
  channel->mem      = mem;
  channel->fifo     = fifo;
  channel->bptr     = 0;

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_CHN, "channel %p chn %p chan %d version %d fifo %p[0x%x]\n",
	     channel, chn, chan, chn->version, fifo, paddr);
  
  ICS_assert(ics_shm_state->mbox[_ICS_MAILBOX_CHN2MBOX(chan)]);

  /* Enable the mbox interrupt mask for this channel idx */
  ics_mailbox_interrupt_enable(ics_shm_state->mbox[_ICS_MAILBOX_CHN2MBOX(chan)], _ICS_MAILBOX_CHN2IDX(chan));

  /* Generate a channel handle */
  *handlep = channel->handle = _ICS_HANDLE(_ICS_TYPE_CHANNEL, ics_state->cpuNum, chn->version, chan);

#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#else
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#endif

  return ICS_SUCCESS;

error_event_free:
  _ICS_OS_EVENT_DESTROY(&channel->event);

error_mutex_free:
#ifdef CONFIG_SMP
     /* release spinlok */
    _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  _ICS_OS_MUTEX_DESTROY(&channel->chanLock);

error_free:
  /* Free off the SHM channel fifo memory */
  if ((umem == NULL) && mem)
    _ICS_OS_CONTIG_FREE(mem);

error_release:
  
#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#endif

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

