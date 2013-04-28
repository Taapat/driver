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
static
MME_ERROR abortCommand (mme_transformer_t *transformer, MME_CommandId_t cmdId)
{
  ICS_ERROR            err;

  mme_command_t       *cmd;
  int                  idx;

  mme_receiver_msg_t   message;
  mme_abort_t         *abort = &message.abort;

  /* Find the Command array idx for this CmdId */
  idx = _MME_CMD_INDEX(cmdId);
  
  /* Sanity check CmdId */
  if (_MME_CMD_HANDLE(cmdId) != _MME_CMD_HANDLE_VAL || idx < 0 || idx >= _MME_MAX_COMMANDS)
    return MME_INVALID_ARGUMENT;

  cmd = &transformer->cmds[idx];

  /* The Command should still be running at least */
  if (cmd->state != _MME_COMMAND_RUNNING)
    return MME_INVALID_ARGUMENT;
  
  /* Should be still on the issued command list */
  MME_ASSERT(!list_empty(&cmd->list));

  MME_PRINTF(MME_DBG_COMMAND, "Attempting to abort cmd %p cmdId 0x%x\n", cmd, cmd->cmdId);

  /* Fill out the abort message desc (on stack) */
  abort->type  = _MME_RECEIVER_ABORT;
  abort->cmdId = cmdId;

  /* Send INLINE message off to Receiver task */
  err = ICS_msg_send(transformer->receiverPort, &message, ICS_INLINE, sizeof(message), 0 /* flags */);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_COMMAND,
		"transformer %p Failed to send abort message %p to port 0x%x\n",
		transformer, &message, transformer->receiverPort);
    
    return MME_TRANSFORMER_NOT_RESPONDING;
  }

  /* There is no direct response from the receiver */
  return MME_SUCCESS;
}

/* MME_AbortCommand()
 * Attempt to abort a submitted command 
 */
MME_ERROR MME_AbortCommand (MME_TransformerHandle_t handle, MME_CommandId_t cmdId)
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

  MME_PRINTF(MME_DBG_COMMAND, "handle 0x%x cmdId 0x%x\n",
	     handle, cmdId);
  
  if (handle == 0 || cmdId == 0)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  /* Lookup the transformer instance (takes lock on success) */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }
  
  res = abortCommand(transformer, cmdId);

  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  if (res != MME_SUCCESS)
    goto error;

  MME_PRINTF(MME_DBG_COMMAND, "transformer %p Successfully aborted CmdId 0x%x\n",
	     transformer, cmdId);
  
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
