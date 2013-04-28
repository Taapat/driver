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

/*
 * Complete a transform command, updating the Client's MME_Command structure 
 * including performing the Client callback
 *
 * MULTITHREAD SAFE: Called holding the transformer lock (in task context)
 * This can also be called in IRQ context if we are not using a callback task
 */
static
MME_ERROR handleTransform (mme_transformer_t *transformer, mme_transform_t *transform)
{
  MME_Command_t       *command;

  mme_command_t       *cmd;
  MME_UINT             idx;
#ifdef CONFIG_SMP
  ICS_OFFSET    paddr;
  ICS_MEM_FLAGS mflags;
  ICS_MEM_FLAGS iflags;
#endif

#ifdef CONFIG_SMP
  /* Get the physical address */
  _ICS_OS_VIRT2PHYS(&transform->command.CmdStatus, &paddr, &mflags);
  /* Commit all updates to memory */
  if(ICS_CACHED == mflags)
     _ICS_OS_CACHE_PURGE(&transform->command.CmdStatus, paddr, sizeof(transform->command.CmdStatus));
#endif
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif

  MME_PRINTF(MME_DBG_COMMAND,
	     "transformer %p transform %p CmdId 0x%x\n",
	     transformer, transform, transform->command.CmdStatus.CmdId);

  /* Find the Command array idx for this CmdId */
  idx = _MME_CMD_INDEX(transform->command.CmdStatus.CmdId);
  MME_ASSERT(idx < _MME_MAX_COMMANDS);
  
  cmd = &transformer->cmds[idx];

  MME_ASSERT(cmd->state == _MME_COMMAND_RUNNING || cmd->state == _MME_COMMAND_KILLED);
  MME_ASSERT(!list_empty(&cmd->list));

  MME_PRINTF(MME_DBG_COMMAND, "cmd %p [%d] cmdId 0x%x/0x%x\n", 
	     cmd, idx,
	     cmd->cmdId, transform->command.CmdStatus.CmdId);

  /* Check the CmdId still matches the issued message */
  MME_ASSERT(cmd->cmdId == transform->command.CmdStatus.CmdId);

  /* Locate the Client's original MME_Command_t address */
  MME_ASSERT(cmd->command);
  command = cmd->command;

  /* Also check that the Client hasn't re-used their MME_Command desc */
  MME_ASSERT(cmd->cmdId == command->CmdStatus.CmdId);

  /* Unpack the completed transform, updating the client's 
   * MME_Command/MME_CommandStatus and MME_DataBuffer structures
   */
  mme_command_complete(command, transform);

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif

  /* Complete the command by doing a callback or posting an event (drops lock) */
  return mme_command_callback(transformer, MME_COMMAND_COMPLETED_EVT, cmd);
}

/*
 * Handle starvation events (MME_DATA_UNDERFLOW_EVT || MME_NOT_ENOUGH_MEMORY_EVT) 
 * on the transformer client, including performing the Client callback
 *
 * MULTITHREAD SAFE: Called holding the transformer lock (in task context)
 * This can also be called in IRQ context if we are not using a callback task
 */
static 
MME_ERROR handleStarvation (mme_transformer_t *transformer, mme_starvation_t *starvation)
{
  MME_UINT             idx;
  mme_command_t       *cmd;

  MME_Command_t       *command;
  MME_CommandStatus_t *status;
  
  MME_Event_t          event;
  
  /* Find the right idx for this CmdId. */
  idx = _MME_CMD_INDEX(starvation->status.CmdId);
  MME_ASSERT(idx < _MME_MAX_COMMANDS);
  
  cmd = &transformer->cmds[idx];
  MME_ASSERT(cmd->command);

  /* Copy the status from the message to the client's MME_CommandStatus_t */
  command = cmd->command;
  status  = &command->CmdStatus;

  event                 = starvation->event;
  status->Error         = starvation->status.Error;
  status->ProcessedTime = starvation->status.ProcessedTime;
  status->State         = starvation->status.State;
  
  MME_PRINTF(MME_DBG_COMMAND,
	     "Starvation event %d CmdId %08x Command %p\n",
	     event, starvation->status.CmdId, command); 

  MME_ASSERT(event == MME_DATA_UNDERFLOW_EVT || event == MME_NOT_ENOUGH_MEMORY_EVT);

  /* Do a callback or post an event (drops lock) */
  return mme_command_callback(transformer, event, cmd);
}


/*
 * Handle kill command message on the transformer client
 *
 * MULTITHREAD SAFE: Called holding the transformer lock (in task context)
 * This can also be called in IRQ context if we are not using a callback task
 */
static 
MME_ERROR handleKill (mme_transformer_t *transformer, mme_kill_t *kill)
{
  MME_UINT             idx;
  mme_command_t       *cmd;

  MME_Command_t       *command;
  MME_CommandStatus_t *status;
  
  MME_Event_t          event;
  
  /* Find the right idx for this CmdId. */
  idx = _MME_CMD_INDEX(kill->status.CmdId);
  MME_ASSERT(idx < _MME_MAX_COMMANDS);
  
  cmd = &transformer->cmds[idx];

  /* Check the CmdId still matches the issued one */
  if (kill->status.CmdId != cmd->cmdId)
  {
    /* This can happen if the command being killed was stuck
     * in the transformer replyPort message queue and then got completed
     * after the kill was issued
     */
    MME_EPRINTF(MME_DBG_COMMAND,
		"Kill cmd %p [%d] CmdId 0x%08x/0x%08x already free\n",
		cmd, idx, cmd->cmdId, kill->status.CmdId);

    /* Must drop lock when called from task context */
    if (!_ICS_OS_TASK_INTERRUPT())
      _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

    /* Silently ignore this error */
    return MME_SUCCESS;
  }
  
  /* Copy the status from the message to the client's MME_CommandStatus_t */
  command = cmd->command;
  status  = &command->CmdStatus;

  status->Error         = kill->status.Error;
  status->State         = kill->status.State;

  event                 = kill->event;
  
  MME_ASSERT(event == MME_TRANSFORMER_TIMEOUT);

  MME_PRINTF(MME_DBG_COMMAND,
	     "Kill event %d CmdId %08x Command %p\n",
	     event, kill->status.CmdId, command); 

  /* Do a callback or post an event (drops lock) */
  return mme_command_callback(transformer, event, cmd);
}


/*
 * Main code to handle transformer Client side messages
 * Can be called from the transformer helper task, or directly in
 * IRQ context for callback based transformers.
 *
 * MULTITHREAD SAFE: Should be called holding the transformer tlock when
 * in task context. Otherwise called from IRQ context when no callback
 * task is being used
 */
static
MME_ERROR handleMsg (mme_transformer_t *transformer, ICS_MSG_DESC *rdesc)
{
  MME_ERROR           res = MME_INTERNAL_ERROR;

  mme_receiver_msg_t *message;
  
  MME_assert(rdesc->data);
  MME_assert(rdesc->size);
  
  /* The receiver task returns the original message buffer */
  message = (mme_receiver_msg_t *) rdesc->data;

  switch (message->type)
  {
  case _MME_RECEIVER_TRANSFORM:
  {
    MME_ASSERT(!(rdesc->mflags & ICS_INLINE));
    MME_assert(rdesc->size >= sizeof(mme_transform_t));
    
    MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	       "transformer %p received reply message %p type 0x%x\n",
	       transformer, message, message->type);

    res = handleTransform(transformer, &message->transform);
    
    break;
  }
  case _MME_RECEIVER_STARVATION:
  {
    MME_assert(rdesc->mflags & ICS_INLINE);
    MME_assert(rdesc->size == sizeof(mme_starvation_t));
    
    res = handleStarvation(transformer, &message->starvation);
    
    break;
  }
  case _MME_RECEIVER_KILL:
  {
    MME_assert(rdesc->mflags & ICS_INLINE);
    MME_assert(rdesc->size == sizeof(mme_kill_t));
    
    res = handleKill(transformer, &message->kill);
    
    break;
  }
    
  default:
    MME_EPRINTF(MME_DBG_COMMAND, "Incorrect message %p type 0x%x\n", message, message->type);
    MME_assert(0);
  }
  
  MME_ASSERT(res == ICS_SUCCESS);
  
  MME_PRINTF(MME_DBG_COMMAND, "completed message %p type 0x%x res %d\n", message, message->type, res);

  return res;
}

/*
 * Per Transformer Client side instantiation task
 *
 * Blocks on the per transformer replyPort waiting 
 * for completion messages from companion
 */
void mme_transformer (void *param)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer = (mme_transformer_t *) param;
  
  ICS_PORT           port;

  MME_assert(transformer);
  MME_assert(transformer->replyPort != ICS_INVALID_HANDLE_VALUE);

  port = transformer->replyPort;

  MME_PRINTF(MME_DBG_TRANSFORMER, "transformer %p starting : port 0x%x\n",
	     transformer, port);

  /* Loop forever, servicing transformer command responses
   * from the receiver task 
   */
  while (1)
  {
    ICS_ERROR     err;
    ICS_MSG_DESC  rdesc;

    /* 
     * Post a blocking receive
     */
    err = ICS_msg_recv(port, &rdesc, ICS_TIMEOUT_INFINITE);

    if (err != ICS_SUCCESS)
    {
      /* Don't report PORT_CLOSED as errors */
      if (err == ICS_PORT_CLOSED)
      {
	res = MME_SUCCESS;
      }
      else
      {
	MME_EPRINTF(MME_DBG_RECEIVER, "ICS_msg_receive returned error : %s (%d)\n", 
		    ics_err_str(err), err);

	res = MME_ICS_ERROR;
      }
      
      /* Error - get outta here */
      break;
    }

    /* Must hold mutex before call */
    _ICS_OS_MUTEX_TAKE(&transformer->tlock);

    /* Handle new message */
    res = handleMsg(transformer, &rdesc);

    MME_ASSERT(res == ICS_SUCCESS);	/* XXXX handle more gracefully ? */

  } /* while (1) */

  MME_PRINTF(MME_DBG_TRANSFORMER, "exit : %s (%d)\n",
	     MME_Error_Str(res), res);
}


/*
 * MME transformer client side ICS port callback code for cases where a helper Task is not being used
 */
ICS_ERROR mme_transformer_callback (ICS_PORT port, void *param, ICS_MSG_DESC *rdesc)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer = (mme_transformer_t *) param;
  
  /* Handle new message */
  res = handleMsg(transformer, rdesc);

  MME_ASSERT(res == ICS_SUCCESS);	/* XXXX handle more gracefully ? */

  return ICS_SUCCESS;
}

/*
 * Find a free transformer instance idx
 *
 * Returns -1 if non found
 *
 * MULTITHREAD SAFE: Called holding the mme_state lock
 */
static
int findFreeTrans (void)
{
  int i;
  
  MME_assert(mme_state);

  /* XXXX Should this be a dynamic table ? */
  for (i = 0; i < _MME_TRANSFORMER_INSTANCES; i++)
  {
    if (mme_state->insTrans[i] == NULL)
    {
      return i;
    }
  }

  /* No free slot found */
  return -1;
}

/*
 * Lookup a local transformer instance via its handle
 * 
 * MULTITHREAD SAFE: Returns holding the transformer lock on success
 */
mme_transformer_t *
mme_transformer_instance (MME_TransformerHandle_t handle)
{
  int type, cpuNum, ver, idx;
   
  mme_transformer_t *transformer;

  /* Decode transformer handle */
  _MME_DECODE_HDL(handle, type, cpuNum, ver, idx);
  
  /* Check the target port */
  if (type != _MME_TYPE_TRANSFORMER|| cpuNum != mme_state->cpuNum || idx >= _MME_TRANSFORMER_INSTANCES)
  {
    return NULL;
  }

  /* _ICS_OS_MUTEX_TAKE(&mme_state->lock); */
  
  transformer = mme_state->insTrans[idx];
  if (transformer == NULL) {
    /* _ICS_OS_MUTEX_RELASE(&mme_state->lock); */
    return NULL;
  }
  
  _ICS_OS_MUTEX_TAKE(&transformer->tlock);

  /* _ICS_OS_MUTEX_RELASE(&mme_state->lock); */
  
  /* Check transformer is still valid whilst holding the lock */
  if (transformer->handle != handle)
  {
    _ICS_OS_MUTEX_RELEASE(&transformer->tlock);
    
    return NULL;
  }

  /* MULTITHREAD SAFE: Returns holding the transformer lock */

  return transformer;
}

/*
 * Allocate a data structure to hold all the local state for
 * an instantiated transformer instances. Also creates the callback
 * task for issuing Command completion callbacks
 */
MME_ERROR
mme_transformer_create (MME_TransformerHandle_t receiverHandle, ICS_PORT receiverPort, ICS_PORT mgrPort, 
			MME_GenericCallback_t callback, void *callbackData, mme_transformer_t **transformerp,
			int createTask)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  ICS_ERROR          err;
  char               taskName[_ICS_OS_MAX_TASK_NAME];

  int                idx;
  mme_transformer_t *transformer;

  MME_PRINTF(MME_DBG_TRANSFORMER, "Create rhandle 0x%x rport 0x%x mgrPort 0x%x\n",
	     receiverHandle, receiverPort, mgrPort);

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);

  /* First try and find a free transformer instance slot */
  idx = findFreeTrans();
  if (idx == -1)
  {
    res = MME_NOMEM;
    goto error;
  }
    
  _ICS_OS_ZALLOC(transformer, sizeof(*transformer));
  if (transformer == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }

  /* Allocate the command desc array */
  _ICS_OS_ZALLOC(transformer->cmds, sizeof(*transformer->cmds)*_MME_MAX_COMMANDS);
  if (transformer->cmds == NULL)
  {
    res = MME_NOMEM;
    goto error_free;
  }

  if (!_ICS_OS_MUTEX_INIT(&transformer->tlock))
  {
    res = MME_ICS_ERROR;
    goto error_free;
  }

  if (createTask)
  {
    /* Create a new Port for transform reply messages (with one slot for each possible message reply) */
    err = ICS_port_alloc(NULL, NULL, NULL, _MME_MAX_COMMANDS+1, 0, &transformer->replyPort);
    if (err != ICS_SUCCESS)
    {
      MME_EPRINTF(MME_DBG_TRANSFORMER, "Failed to create ICS port : %s (%d)\n",
		  ics_err_str(err), err);
      
      res = MME_INTERNAL_ERROR;
      goto error_free;
    }

    /* Create a per transformer task to handle
     * Command completion callbacks
     */
    snprintf(taskName, _ICS_OS_MAX_TASK_NAME, _MME_TRANSFORMER_TASK_NAME, (MME_UINT)transformer);
    err = _ICS_OS_TASK_CREATE(mme_transformer, transformer, _MME_TRANSFORMER_TASK_PRIORITY,
			      taskName, &transformer->task);
    if (err != ICS_SUCCESS)
    {
      res = MME_INTERNAL_ERROR;
      goto error_close;
    }
  }
  else
  {
    /* 
     * No task required, handle completion messages in the IRQ
     */
    
    /* Create a new anonymous Callback Port for transform reply messages */
    err = ICS_port_alloc(NULL, 
			 mme_transformer_callback, transformer,
			 ICS_PORT_MIN_NDESC,
			 0, /* flags */
			 &transformer->replyPort);
    if (err != ICS_SUCCESS)
    {
      MME_EPRINTF(MME_DBG_TRANSFORMER, "Failed to create ICS callback port : %s (%d)\n",
		  ics_err_str(err), err);
      
      res = MME_INTERNAL_ERROR;
      goto error_free;
    }
  }

  MME_PRINTF(MME_DBG_TRANSFORMER, "Created reply port 0x%x\n", transformer->replyPort);

  /* Initialise the rest of the transformer structure */
  transformer->mgrPort        = mgrPort;
  transformer->receiverPort   = receiverPort;
  transformer->receiverHandle = receiverHandle;
  transformer->callback       = callback;
  transformer->callbackData   = callbackData;

  /* Initialise all the local cmd descs */
  mme_cmd_init(transformer);

  /* Generate the transformer handle */
  transformer->handle = _MME_HANDLE(_MME_TYPE_TRANSFORMER, mme_state->cpuNum, 0 /*version*/, idx);

  /* Finally add the transformer to the instance table */
  mme_state->insTrans[idx] = transformer;
  
  /* Return transformer struct to caller */
  *transformerp = transformer;

  MME_PRINTF(MME_DBG_TRANSFORMER, "Successfully created transformer : %p 0x%x\n",
	     transformer, transformer->handle);

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  return MME_SUCCESS;

error_close:
  ICS_port_free(transformer->replyPort, 0);

error_free:
  if (transformer->cmds)
    _ICS_OS_FREE(transformer->cmds);

  _ICS_OS_FREE(transformer);

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

error:
  MME_EPRINTF(MME_DBG_TRANSFORMER, "exit : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}
 
/* Terminate a transformer instance, closing its port and freeing off
 * the associated task and memory
 *
 * MULTITHREAD SAFE: Called holding the transformer lock
 */
void
mme_transformer_term (mme_transformer_t *transformer)
{
  int idx;
  
  MME_assert(transformer);
  MME_assert(transformer->numCmds == 0);

  MME_PRINTF(MME_DBG_TRANSFORMER, "Terminate transformer %p : port 0x%x task %p\n",
	     transformer, transformer->replyPort, transformer->task);
  
  /* Get the transformer table index */
  idx = _MME_HDL2IDX(transformer->handle);
  
  /* Remove transformer instance from lookup table */
  MME_ASSERT(idx >= 0 && idx < _MME_TRANSFORMER_INSTANCES);
  MME_ASSERT(mme_state->insTrans[idx] == transformer);
  mme_state->insTrans[idx] = NULL;

  /* This should cause the transformer task to exit */
  if (transformer->replyPort != ICS_INVALID_HANDLE_VALUE)
    ICS_port_free(transformer->replyPort, 0);

  /* Terminate the transformer task if there is one */
  if (transformer->task.task)
    _ICS_OS_TASK_DESTROY(&transformer->task);

  /* Release all cmd resources */
  mme_cmd_term(transformer);

  /* Zero and free off the memory (to be safe against accidental reuse of handle) */
  _ICS_OS_MEMSET(transformer->cmds, 0, sizeof(*transformer->cmds)*_MME_MAX_COMMANDS);
  _ICS_OS_FREE(transformer->cmds);

  /* Mark the transformer as invalid (whilst still holding the lock) */
  transformer->handle = 0;

  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  _ICS_OS_MUTEX_DESTROY(&transformer->tlock);

  _ICS_OS_FREE(transformer);

  return;
}

void
mme_transformer_cpu_down (ICS_UINT cpuNum)
{
  int i;
  
  mme_transformer_t *transformer;

  /*
   * Search through all the instantiated transformers and kill
   * any which are hosted on the failed CPU
   */
  
  /* Check for any locally instantiated transformers */
  for (i = 0; i < _MME_TRANSFORMER_INSTANCES; i++)
  {
    if ((transformer = mme_state->insTrans[i]))
    {
      ICS_UINT portCpu;
      
      _ICS_OS_MUTEX_TAKE(&transformer->tlock);
      
      /* Is this transformer hosted on the failed CPU ? */
      if ((ICS_port_cpu(transformer->mgrPort, &portCpu) == ICS_SUCCESS) && (portCpu == cpuNum))
      {
	MME_PRINTF(MME_DBG_TRANSFORMER, "Kill transformer %p port 0x%x cpu %d\n",
		   transformer, transformer->mgrPort, cpuNum);

	mme_transformer_kill(transformer);
      }
      
      _ICS_OS_MUTEX_RELEASE(&transformer->tlock);
    }
  }
} 

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
