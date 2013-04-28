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

/* MULTITHREAD SAFE: Called holding the transformer lock */
MME_ERROR mme_command_send (mme_transformer_t *transformer, MME_Command_t *command)
{
  ICS_ERROR            err;
  MME_ERROR            res = MME_INTERNAL_ERROR;

  MME_CommandStatus_t *status = &command->CmdStatus;	/* Caller's CommandStatus struct */

  mme_msg_t           *msg = NULL;
  
  mme_command_t       *cmd;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif

  MME_ASSERT(_ICS_OS_MUTEX_HELD(&transformer->tlock));

  /* 
   * Allocate a local command desc
   */
  cmd = mme_cmd_alloc(transformer);

  if (cmd == NULL)
  {
    res = MME_NOMEM;
    goto error_release;
  }

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif

  /* Fill out Caller's CommandStatus (must be done before pack to embed CmdId) */
  status->State = MME_COMMAND_FAILED;
  status->CmdId = cmd->cmdId;

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif

  /* Pack the MME_Command (and CmdStatus) into a msg buffer */
  res = mme_command_pack(transformer, command, &msg);
  if (res != MME_SUCCESS)
    goto error_freeCmd;

  MME_ASSERT(msg);

  MME_PRINTF(MME_DBG_COMMAND,
	     "transformer %p Sending buf %p [CmdId 0x%x] size %d to port 0x%x\n",
	     transformer, msg->buf, status->CmdId, msg->size, transformer->receiverPort);

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* Fill out the local command desc */
  cmd->command = command;
  cmd->msg     = msg;
  cmd->state   = _MME_COMMAND_RUNNING;

  /* Fill the caller's CommandStatus state as *EXECUTING* */
  status->State = MME_COMMAND_EXECUTING;
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  /* Send pre-packed message off to Receiver task
   * NB: As soon as this is sent the cmd desc becomes live and
   * may be completed in IRQ context
   */
  err = ICS_msg_send(transformer->receiverPort, msg->buf, _MME_MSG_CACHE_FLAGS, msg->size, 0 /* flags */);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_COMMAND,
		"transformer %p Failed to send buf %p to port 0x%x\n",
		transformer, msg->buf, transformer->receiverPort);
    
    res = MME_TRANSFORMER_NOT_RESPONDING;
    goto error_freeMsg;
  }
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  return MME_SUCCESS;

error_freeMsg:
  mme_msg_free(msg); /* Takes/drops MME state lock */
  cmd->msg = NULL;

error_freeCmd:
  mme_cmd_free(transformer, cmd);

error_release:
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* MME_SendCommand()
 * Send a transformer command to a transformer instance
 */
MME_ERROR MME_SendCommand (MME_TransformerHandle_t handle, MME_Command_t *command)
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

  MME_PRINTF(MME_DBG_COMMAND, "handle 0x%x command %p\n",
	     handle, command);
  
  if (handle == 0)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }
  
  if (command == NULL || command->StructSize != sizeof(MME_Command_t))
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  MME_PRINTF(MME_DBG_COMMAND, "command %p [CmdCode %d CmdEnd %d]\n",
	     command, command->CmdCode, command->CmdEnd);
  
  if (command->CmdCode > MME_SEND_BUFFERS)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }
  
  if (command->CmdEnd > MME_COMMAND_END_RETURN_WAKE)
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

  MME_PRINTF(MME_DBG_COMMAND, "Located transformer %p callback %p\n", 
	     transformer, transformer->callback);

  /* Detect illegal use of a NULL callback and a requested notify event */
  if ((transformer->callback == NULL) && (command->CmdEnd == MME_COMMAND_END_RETURN_NOTIFY))
  {
    res = MME_INVALID_HANDLE;
    goto error_release;
  }

  /* This function drops the transformer lock */
  res = mme_command_send(transformer, command);
  if (res != MME_SUCCESS)
    goto error;

  MME_PRINTF(MME_DBG_COMMAND, "Issued command %p CmdId 0x%x\n",
	     command, command->CmdStatus.CmdId);
  
  return MME_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

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
