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

/* this is additional internal command state used to indicate that the command
 * is subject to an internal error and no buffer updates are required (and may
 * even be harmful).
 */
#define MME_COMMAND_DIED_EVT	0xdead

/* 
 * Notify the Command client about an Event for the given Command
 */
MME_ERROR mme_receiver_notify (mme_receiver_t *receiver, MME_Event_t event, mme_transform_t *transform,
			       MME_ERROR res)
{
  ICS_ERROR            err;
  
  MME_CommandStatus_t *status = &transform->command.CmdStatus;
  
  MME_PRINTF(MME_DBG_RECEIVER, "receiver %p event %d cmdId %08x res=%d\n",
	     receiver, event, status->CmdId, res);
  
  MME_ASSERT(receiver);
  MME_ASSERT(transform);
  MME_ASSERT(transform->type == _MME_RECEIVER_TRANSFORM);

  /* Return supplied error code */
  status->Error = res;

  /* Clear error code */
  res = MME_SUCCESS;

  switch (event)
  {
  case MME_COMMAND_COMPLETED_EVT:
    res = mme_command_repack(receiver, transform);
    if (status->Error == MME_SUCCESS)
      status->Error = res;

    /*FALLTHRU*/
    
  case MME_COMMAND_DIED_EVT:
    /* Update the command status structure and hand everything back to the Command client */
    status->State = ((status->Error == MME_SUCCESS) ? MME_COMMAND_COMPLETED : MME_COMMAND_FAILED);
    
    MME_PRINTF(MME_DBG_RECEIVER, "receiver %p sending reply transform %p port 0x%x\n",
	       receiver, transform, transform->replyPort);
    
    /* Send back original message buffer */
    err = ICS_msg_send(transform->replyPort, transform, _MME_MSG_CACHE_FLAGS, transform->size, 0 /* flags */);
    if (err != ICS_SUCCESS)
    {
      MME_EPRINTF(MME_DBG_RECEIVER, "receiver %p Failed to send notify transform %p port 0x%x : %s (%d)\n",
		  receiver, transform, transform->replyPort,
		  ics_err_str(err), err);
      res = MME_ICS_ERROR;
      goto error;
    }

    break;

  case MME_DATA_UNDERFLOW_EVT:
  case MME_NOT_ENOUGH_MEMORY_EVT:
  {
    mme_starvation_t starvation;

    MME_PRINTF(MME_DBG_RECEIVER, "receiver %p sending starvation reply transform %p port 0x%x\n",
	       receiver, transform, transform->replyPort);
    
    /* Send back a starvation message to the Client */
    starvation.type  = _MME_RECEIVER_STARVATION;
    starvation.event = event;

    /* Copy back whole of CommandStatus struct */
    _ICS_OS_MEMCPY(&starvation.status, &transform->command.CmdStatus, sizeof(MME_CommandStatus_t));

    /* Send back an INLINE message */
    err = ICS_msg_send(transform->replyPort, &starvation, ICS_INLINE, sizeof(starvation), 0 /* flags */);
    if (err != ICS_SUCCESS)
    {
      MME_EPRINTF(MME_DBG_RECEIVER, "receiver %p Failed to send notify starvation %p port 0x%x : %s (%d)\n",
		  receiver, &starvation, transform->replyPort,
		  ics_err_str(err), err);
      res = MME_ICS_ERROR;
      goto error;
    }
    break;
  }
  
  default:
    MME_assert(0);
    break;
  }
  
  MME_PRINTF(MME_DBG_RECEIVER, "receiver %p notified client Error=%d : %s (%d)\n",
	     receiver, status->Error,
	     MME_Error_Str(res), res);
  
  return res;
  
error:
  MME_EPRINTF(MME_DBG_RECEIVER, "Failed receiver %p : %s (%d)\n",
	      receiver,
	      MME_Error_Str(res), res);

  return res;

}

MME_ERROR MME_NotifyHost (MME_Event_t event, MME_Command_t *command, MME_ERROR res)
{
  mme_receiver_t      *receiver;

  mme_transform_t     *transform;

  MME_PRINTF(MME_DBG_RECEIVER, "event %d command %p res %d\n",
	     event, command, res);

  if (command == NULL)
    return MME_INVALID_ARGUMENT;
 
  if (event < MME_COMMAND_COMPLETED_EVT || event > MME_NOT_ENOUGH_MEMORY_EVT)
    return MME_INVALID_ARGUMENT;

  /* Convert the supplied Command address back to the original transform message */
  transform = (mme_transform_t *) ((unsigned long)command - offsetof(mme_transform_t, command));

  MME_ASSERT(transform->type == _MME_RECEIVER_TRANSFORM);

  /* Now get the receiver handle */
  receiver = (mme_receiver_t *) transform->receiverHandle;

#ifdef ICS_DEBUG
  {
    MME_CommandStatus_t *status  = &command->CmdStatus;
    MME_UINT             idx;
    
    idx = _MME_CMD_INDEX(status->CmdId);
    
    /* Extra Safety checks */
    MME_ASSERT(_MME_CMD_HANDLE(status->CmdId) == _MME_CMD_HANDLE_VAL);
    MME_ASSERT(idx < _MME_MAX_COMMANDS);
    MME_ASSERT(transform == receiver->transforms[idx]);
  }
#endif
  
  return mme_receiver_notify(receiver, event, transform, res);
}

static
MME_ERROR handleTransformMsg (mme_receiver_t *receiver, mme_transform_t *transform)
{
  MME_ERROR            res     = MME_INTERNAL_ERROR;

  MME_Command_t       *command = &transform->command;
  MME_CommandStatus_t *status  = &command->CmdStatus;

  MME_UINT             idx;

  idx = _MME_CMD_INDEX(status->CmdId);
  
  /* Safety checks */
  MME_ASSERT(_MME_CMD_HANDLE(status->CmdId) == _MME_CMD_HANDLE_VAL);
  MME_ASSERT(idx < _MME_MAX_COMMANDS);

  /* Remember this transform address so we can cancel it later if required */
  receiver->transforms[idx] = transform;

  switch (command->CmdCode)
  {
  case MME_SET_PARAMS:
#if 0
    /* XXXX Multicom3: altering the DueTime for parameter setting was not in the spec ... */
    command->DueTime = 0;
#endif

    /*FALLTHRU*/

  case MME_PING:
    /* Handle PING like a normal command so it can detect execution loop hangups */
  case MME_TRANSFORM:
  {
    MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	       "receiver %p Enqueue transform %p CmdCode %d DueTime 0x%x\n",
	       receiver, transform, command->CmdCode, command->DueTime);
    
    /* Enqueue the transform using its DueTime as the priority */
    res = mme_messageq_enqueue(&receiver->msgQ, transform, command->DueTime);
    if (res == MME_SUCCESS)
    {
      /* Receiver must have an associated exec task */
      MME_ASSERT(receiver->exec);

      /* Receiver must be on an exec list */
      MME_ASSERT(!list_empty(&receiver->list));

      MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		 "receiver %p wake exec %p\n",
		 receiver, receiver->exec);

      /* Poke the associated Execution task to inform it of new work */
      _ICS_OS_EVENT_POST(&receiver->exec->event);
    }
    
    break;
  }
  case MME_SEND_BUFFERS:
  {
    /*
     * Unpack and Execute the transform call
     */
    res = mme_command_unpack(receiver, transform);
    if (res == MME_SUCCESS)
    {
      MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		 "receiver %p Calling processCommand fn %p ctx %p command %p\n",
		 receiver, receiver->funcs.processCommand, receiver->context, command);
      
      /* Execute the Transformer command */
      res = (receiver->funcs.processCommand) (receiver->context, command);
    }
    
    break;
  }

  default:
    MME_EPRINTF(MME_DBG_RECEIVER, "Incorrect transform %p Command %p code %d\n", 
		transform, command, command->CmdCode);
    
    res = MME_INTERNAL_ERROR;
    break;
  }

  /*
   * Report any failures to the Command client, avoiding the repack
   * which is not necessary in the error case anyway
   */
  if (res != MME_SUCCESS && res != MME_TRANSFORM_DEFERRED)
  {
    MME_EPRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		"receiver %p Failed to process transform %p : %s (%d)\n",
		receiver, transform, 
		MME_Error_Str(res), res);
    
    /* Send an error reply to Command client */
    mme_receiver_notify(receiver, MME_COMMAND_DIED_EVT, transform, res);
  }
  
  return res;
}


static
MME_ERROR handleAbortMsg (mme_receiver_t *receiver, mme_abort_t *abort)
{
  MME_ERROR        res = MME_INTERNAL_ERROR;

  mme_transform_t *transform;
  MME_UINT         idx;

  MME_PRINTF(MME_DBG_RECEIVER, "receiver %p abort %p cmdId 0x%x\n",
	     receiver, abort, abort->cmdId);

  idx = _MME_CMD_INDEX(abort->cmdId);
  
  /* Safety checks */
  MME_ASSERT(_MME_CMD_HANDLE(abort->cmdId) == _MME_CMD_HANDLE_VAL);
  MME_ASSERT(idx < _MME_MAX_COMMANDS);

  /* Lookup the transform message address of this Cmd idx */
  transform = receiver->transforms[idx];

  /* This should never be NULL */
  MME_assert(transform);

  /* First attempt to dequeue the transform from the execution lists */
  res = mme_messageq_remove(&receiver->msgQ, transform);
  if (res != MME_SUCCESS)
  {
    /* Failed to find it on the execution list, so have to call the transformer */
    res = (receiver->funcs.abort) (receiver->context, abort->cmdId);
  }
  
  if (res == MME_SUCCESS)
  {
    /* report the successful abortion, using the original transform message buffer */
    res = mme_receiver_notify(receiver, MME_COMMAND_DIED_EVT /* avoid repack */, transform, MME_COMMAND_ABORTED);
    
    MME_assert(MME_SUCCESS == res); /* cannot recover */
  }
  
  return res;
}

/*
 * Per Transformer instantiation receiver task
 */
void mme_receiver (void *param)
{
  MME_ERROR       res = MME_INTERNAL_ERROR;

  mme_receiver_t *receiver = (mme_receiver_t *) param;
  ICS_PORT        port;
  
  MME_assert(receiver);
  
  MME_assert(receiver->port != ICS_INVALID_HANDLE_VALUE);

  port = receiver->port;

  /* Loop forever, servicing requests */
  while (1)
  {
    ICS_ERROR     err;
    ICS_MSG_DESC  rdesc;

    mme_receiver_msg_t *message;
    
    /* 
     * Post a blocking receive
     */
    err = ICS_msg_recv(port, &rdesc, ICS_TIMEOUT_INFINITE);

    if (err != ICS_SUCCESS)
    {
      /* Don't report PORT_CLOSED as errors */
      if (err == ICS_PORT_CLOSED)
	res = MME_SUCCESS;
      else
      {
	MME_EPRINTF(MME_DBG_RECEIVER, "ICS_msg_receive returned error : %s (%d)\n", 
		    ics_err_str(err), err);
	res = MME_ICS_ERROR;
      }

      /* Get outta here */
      break;
    }

    MME_assert(rdesc.data);
    MME_assert(rdesc.size);

    message = (mme_receiver_msg_t *) rdesc.data;

    MME_assert(rdesc.size >= sizeof(mme_receiver_msg_t));

    switch (message->type)
    {
    case _MME_RECEIVER_TRANSFORM:
      res = handleTransformMsg(receiver, &message->transform);
      break;

    case _MME_RECEIVER_ABORT:
      MME_assert(rdesc.mflags & ICS_INLINE);
      res = handleAbortMsg(receiver, &message->abort);
      break;

    default:
      MME_EPRINTF(MME_DBG_RECEIVER, "port 0x%x message %p size %d unknown req 0x%x\n",
		  port, rdesc.data, rdesc.size, message->type);

      /* FATAL error */
      MME_assert(0);
      break;
    }

    MME_PRINTF(MME_DBG_RECEIVER, "completed req 0x%x res %d\n", message->type, res);
  }
  
  MME_PRINTF(MME_DBG_RECEIVER, "exit : %s (%d)\n",
	     MME_Error_Str(res), res);
}

/* STATISTICS: Cmd counter callback */
void mme_receiver_stats (ICS_STATS_HANDLE handle, void *param)
{
  mme_receiver_t *receiver = (mme_receiver_t *) param;
  
  ICS_stats_update(handle, receiver->cmds, _ICS_OS_TIME_TICKS2MSEC(_ICS_OS_TIME_NOW()));

  return;
}

MME_ERROR mme_receiver_create (mme_transformer_reg_t *treg, MME_UINT paramsSize, MME_GenericParams_t params,
			       mme_receiver_t **receiverp)
{
  MME_ERROR        res = MME_INTERNAL_ERROR;
  
  ICS_ERROR        err;
  char             taskName[_ICS_OS_MAX_TASK_NAME];
  
  mme_receiver_t  *receiver;

  MME_ASSERT(receiverp);

  MME_PRINTF(MME_DBG_RECEIVER, "treg %p paramsSize %d params %p\n",
	     treg, paramsSize, params);

  /* Create a new Receiver struct */
  _ICS_OS_ZALLOC(receiver, sizeof(*receiver));
  if (receiver == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }

  /* Create the command transform array */
  _ICS_OS_ZALLOC(receiver->transforms, sizeof(*receiver->transforms)*_MME_MAX_COMMANDS);
  if (receiver->transforms == NULL)
  {
    res = MME_NOMEM;
    goto error_free;
  }
  
  MME_PRINTF(MME_DBG_INIT, "Calling initTransformer %p paramsSize %d params %p context %p\n",
	     treg->funcs.initTransformer, paramsSize, params, &receiver->context);

  /* 
   * Call the Transformer Init function, returning a context handle
   * 
   * Can return a user generated error so do this first
   */
  res = (treg->funcs.initTransformer) (paramsSize, params, &receiver->context);
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_RECEIVER, "initTransformer(%d, %p) function returned failure : %d\n",
		paramsSize, paramsSize, res);

    goto error_free;
  }

  /* Create a new Port for Transform messages */
  err = ICS_port_alloc(NULL, NULL, NULL, _MME_MAX_COMMANDS+1, 0, &receiver->port);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_RECEIVER, "Failed to create receiver port : %s (%d)\n",
		ics_err_str(err), err);
    
    res = MME_ICS_ERROR;
    goto error_free;
  }

  MME_PRINTF(MME_DBG_RECEIVER, "Created receive port 0x%x\n", receiver->port);

  /* Initialise rest of the Receiver struct */
  INIT_LIST_HEAD(&receiver->list);

  /* Initialise a message queue for this receiver */
  res = mme_messageq_init(&receiver->msgQ, _MME_MAX_COMMANDS+1);
  if (res != MME_SUCCESS)
    goto error_close;
  
  /* STATISTICS */
  {
    ICS_CHAR name[ICS_STATS_MAX_NAME+1];
    
    /* Generate a unique name for this statistic (using 16 bits of receiver addr) */
    snprintf(name, ICS_STATS_MAX_NAME+1, "%04hx_%s", (ICS_UINT)receiver, treg->name);
    
    ICS_stats_add(name, ICS_STATS_COUNTER|ICS_STATS_RATE, mme_receiver_stats, receiver, &receiver->stats);
  }

  /* Copy over the Registered transformer function pointers (for faster/safer access) */
  receiver->funcs = treg->funcs;

  /* Create a per transformer task to handle
   * Send Buffer and Abort requests
   */
  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, _MME_RECEIVER_TASK_NAME, (MME_UINT)receiver);
  
  err = _ICS_OS_TASK_CREATE(mme_receiver, receiver, _MME_RECEIVER_TASK_PRIORITY, taskName, &receiver->task);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_RECEIVER, "Failed to create receiver %p task : %s (%d)\n",
		receiver,
		ics_err_str(err), err);

    res = MME_INTERNAL_ERROR;
    goto error_close;
  }

  MME_PRINTF(MME_DBG_INIT, "Created Receiver %p task %p context %p\n", 
	     receiver, receiver->task.task, receiver->context);

  /* Pass back allocated structure */
  *receiverp = receiver;

  return MME_SUCCESS;

error_close:
  ICS_port_free(receiver->port, 0);

error_free:
  if (receiver->transforms)
    _ICS_OS_FREE(receiver->transforms);
 
  _ICS_OS_MEMSET(receiver, 0, sizeof(*receiver));
  _ICS_OS_FREE(receiver);
  
error:
  MME_EPRINTF(MME_DBG_RECEIVER, "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/*
 * Attempt to terminate a receiver by calling the Registered termTransformer callback
 */
MME_ERROR mme_receiver_term (mme_receiver_t *receiver)
{
  MME_ERROR  res;

  MME_UINT   prio;
  
  MME_assert(receiver);

  /* There must be no outstanding commands (should have been enforced on the host) */
  MME_assert(mme_messageq_peek(&receiver->msgQ, &prio) == MME_INVALID_HANDLE);

  /* Try to terminate the transformer */
  res = (receiver->funcs.termTransformer) (receiver->context);

  MME_PRINTF(MME_DBG_RECEIVER, "termTransformer callback returned : %d\n",  res);
  
  /* XXXX On success do we need to change receiver state to prevent further command executions ?
   * The Manager task is a higher priority than any of the execution ones, but could something
   * happen in the ISR ??
   */

  return res;
}

/* 
 * Free off a receiver desc, closing its Ports and deleting the task
 * It has already been unlinked from the execution task
 */
void mme_receiver_free (mme_receiver_t *receiver)
{
  MME_assert(receiver);

  /* Should no longer be on the execution list */
  MME_assert(list_empty(&receiver->list));

  /* Closing port will cause task to exit */
  if (receiver->port != ICS_INVALID_HANDLE_VALUE)
    ICS_port_free(receiver->port, 0);

  _ICS_OS_TASK_DESTROY(&receiver->task);

  /* Release the message queue */
  mme_messageq_term(&receiver->msgQ);

  /* STATISTICS */
  ICS_stats_remove(receiver->stats);

  /* Free transform ptr array */
  _ICS_OS_FREE(receiver->transforms);

  /* Zero and free off the memory (to be safe against accidental reuse of handle) */
  _ICS_OS_MEMSET(receiver, 0, sizeof(*receiver));
  _ICS_OS_FREE(receiver);
  
  return;
}

/* Return the time of the lowest DueTime message */
MME_ERROR mme_receiver_due_time (mme_receiver_t *receiver, MME_Time_t *timep)
{
  MME_ERROR           res;
  
  MME_UINT            time;

  MME_ASSERT(receiver);
  MME_ASSERT(timep);
  
  res = mme_messageq_peek(&receiver->msgQ, &time);
  if (res != MME_SUCCESS)
    return res;

  /* Return time value back to caller */
  *timep = (MME_Time_t) time;

  return MME_SUCCESS;
}

/* Process a single transform on the supplied receiver 
 *
 * Called from one of the execution loops when it has determined the
 * receiver which has the lowest DueTime message waiting
 */
MME_ERROR mme_receiver_process (mme_receiver_t *receiver)
{
  MME_ERROR           res = MME_INTERNAL_ERROR;
  MME_Event_t         evt = MME_COMMAND_COMPLETED_EVT;

  void               *message;
  mme_transform_t    *transform;

  MME_Command_t      *command;

  MME_ASSERT(receiver);
  
  MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	     "receiver %p process command\n",
	     receiver);

  /* Dequeue the next transform (highest priority) */
  res = mme_messageq_dequeue(&receiver->msgQ, &message);
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		"receiver %p Failed to dequeue transform : %d\n",
		receiver, res);
    goto error;
  }

  MME_ASSERT(message);
  transform = (mme_transform_t *) message;

  MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	     "dequeued transform %p type 0x%x\n",
	     transform, transform->type);
  
  /* We only expect TRANSFORM Command messages here */
  MME_ASSERT(transform->type == _MME_RECEIVER_TRANSFORM);

  /* Now examine the transform command */
  command = &transform->command;

  if (command->CmdCode == MME_PING)
  {
    /* Don't do any processing for MME_PING requests */
    res = MME_SUCCESS;
  }
  else
  {
    /*
     * Unpack and Execute the transform call
     */
    res = mme_command_unpack(receiver, transform);
    if (res == MME_SUCCESS)
    {
      MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		 "Calling processCommand fn %p ctx %p command %p CmdCode %d\n",
		 receiver->funcs.processCommand, receiver->context, 
		 command, command->CmdCode);
      
      /* Execute the Transformer command */
      res = (receiver->funcs.processCommand) (receiver->context, command);
      
      /* STATISTICS */
      receiver->cmds++;
    }
    else
    {
      MME_EPRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
		  "receiver %p Failed to unpack transform %p : %d\n",
		  receiver, transform, res);
      
      /* MME_COMMAND_DIED_EVT suppresses the client side transform reconversion */
      evt = MME_COMMAND_DIED_EVT;
    }
  }

  if (res != MME_TRANSFORM_DEFERRED) 
  {
    /* Inform client of command completion */
    res = mme_receiver_notify(receiver, evt, transform, res);
    MME_assert(MME_SUCCESS == res); /* cannot recover */
  }
  
  MME_PRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	     "receiver %p Completed command msg %p : %d\n",
	     receiver, transform, res);

  return res;

error:
  MME_EPRINTF(MME_DBG_RECEIVER|MME_DBG_COMMAND,
	      "Failed receiver %p : %s (%d)\n",
	      receiver,
	      MME_Error_Str(res), res);
  
  return res;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
