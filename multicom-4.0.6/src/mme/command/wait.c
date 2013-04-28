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

static
void freeCmd (mme_transformer_t *transformer, mme_command_t *cmd)
{
    /* Re-acquire lock so we can free off cmd desc */
    _ICS_OS_MUTEX_TAKE(&transformer->tlock);

    MME_PRINTF(MME_DBG_COMMAND, "Freeing off cmd %p\n", cmd);

#if 0
    /* XXXX Also need to handle the race where there are multiple completions */
    /* XXXX But this can also occur when commands are aborted */
    MME_ASSERT(!_ICS_OS_EVENT_READY(&cmd->block));
#endif

    /* Free off the associated msg buffer */
    MME_ASSERT(cmd->msg);
    mme_msg_free(cmd->msg); /* Takes/drops MME state lock */
    cmd->msg = NULL;

    /* Free off the internal command desc */
    mme_cmd_free(transformer, cmd);
    
    _ICS_OS_MUTEX_RELEASE(&transformer->tlock);
    
    return;
}

/*
 * MULTITHREAD SAFE: Called holding the transformer lock 
 */
static
MME_ERROR waitCommand (mme_transformer_t *transformer, MME_CommandId_t cmdId, MME_Event_t *eventp, MME_UINT timeout)
{
  ICS_ERROR            err;

  MME_Command_t       *command;
  mme_command_t       *cmd;
  MME_UINT             idx;

  /* Find the Command array idx for this CmdId */
  idx = _MME_CMD_INDEX(cmdId);
  MME_ASSERT(idx >=0 && idx < _MME_MAX_COMMANDS);
  
  cmd = &transformer->cmds[idx];

  MME_PRINTF(MME_DBG_COMMAND, "cmd %p[%d] state %d\n",
	     cmd, idx, cmd->state);

  /* The Command should be running/completed */
  if (cmd->state == _MME_COMMAND_IDLE)
  {
    MME_EPRINTF(MME_DBG_COMMAND, "transformer %p cmdId 0x%x cmd %p state = %d incorrect\n",
		transformer, cmdId, cmd, cmd->state);

    _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

    return MME_INVALID_HANDLE;
  }
  
  MME_ASSERT(cmd->command);
  command = cmd->command;

  MME_PRINTF(MME_DBG_COMMAND, "cmd %p[%d] command %p CmdEnd %d\n",
	     cmd, idx, command, command->CmdEnd);

  /* We will never wake up if this isn't the case */
  if (command->CmdEnd != MME_COMMAND_END_RETURN_WAKE)
  {
    _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

    return MME_INVALID_HANDLE;
  }

  /* Should be still on the issued command list */
  MME_ASSERT(!list_empty(&cmd->list));

  /* Don't hold lock whilst we block */
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  /* Convert timeout value (MME_TIMEOUT_INFINITE != ICS_TIMEOUT_INFINITE) */
  if (timeout == MME_TIMEOUT_INFINITE)
    timeout = ICS_TIMEOUT_INFINITE;
  
  /* Now block waiting on the cmd event (interruptible if blocking for ever) */
  err = _ICS_OS_EVENT_WAIT(&cmd->block, timeout, timeout == ICS_TIMEOUT_INFINITE);
  if (err != ICS_SUCCESS)
  {
    /* Must free off cmd (assumes it will never be completed normally) */
    freeCmd(transformer, cmd);

    /* Fill out command desc and event status */
    command->CmdStatus.State = MME_COMMAND_FAILED;
    command->CmdStatus.Error = MME_COMMAND_TIMEOUT;

    *eventp = MME_TRANSFORMER_TIMEOUT;

    /* Translate ICS error code */
    return ((err == ICS_SYSTEM_TIMEOUT) ? MME_COMMAND_TIMEOUT :
	    (err == ICS_SYSTEM_INTERRUPT) ? MME_SYSTEM_INTERRUPT : MME_ICS_ERROR);
  }

  /*
   * Wakeup
   *
   * transformer cannot have been freed as there are
   * outstanding commands
   */
  /* Assert that the CmdStatus has been updated */
  MME_ASSERT(command->CmdStatus.State == MME_COMMAND_COMPLETED ||
	     command->CmdStatus.State == MME_COMMAND_FAILED);

  /* Return event code to caller */
  *eventp = cmd->event;

  /* Only free off the command if this is a completion event
   */
  if (cmd->event == MME_COMMAND_COMPLETED_EVT || cmd->event == MME_TRANSFORMER_TIMEOUT)
  {
    freeCmd(transformer, cmd);
  }

  /* Return MME_COMMAND_TIMEOUT for that specific return event */
  return (*eventp == MME_TRANSFORMER_TIMEOUT) ? MME_COMMAND_TIMEOUT : MME_SUCCESS;
}

/* MME_WaitCommand()
 *
 * Block waiting for an issued MME Command to complete
 */
MME_ERROR MME_WaitCommand (MME_TransformerHandle_t handle, MME_CommandId_t cmdId, MME_Event_t *eventp,
			   MME_Time_t timeout)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer;

  /* 
   * Sanity check the arguments
   */
  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  MME_PRINTF(MME_DBG_COMMAND, "handle 0x%x cmdId 0x%x eventp %p timeout 0x%x\n",
	     handle, cmdId, eventp, timeout);
  
  if (handle == 0 || cmdId == 0)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  if (eventp == NULL)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  /* Lookup the transformer instance (takes lock on success) */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  res = waitCommand(transformer, cmdId, eventp, timeout);
  if (res != MME_SUCCESS)
    goto error;

  MME_PRINTF(MME_DBG_COMMAND, "transformer %p Successfully waited for CmdId 0x%x event %d\n",
	     transformer, cmdId, *eventp);
  
  return MME_SUCCESS;

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
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
