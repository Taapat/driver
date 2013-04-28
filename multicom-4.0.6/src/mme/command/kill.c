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
 * Lookup and kill the specified command
 *
 * We do this by generating a new inline kill reply message.
 * This makes it much easier to handle the locking issues
 *
 * MULTITHREAD SAFE: Called holding the transformer lock
 */
static
MME_ERROR killCommand (mme_transformer_t *transformer, mme_command_t *cmd)
{
  ICS_ERROR       err;

  mme_kill_t      kill;

  /* Check the internal command is still running */
  if (cmd->state != _MME_COMMAND_RUNNING)
    return MME_INVALID_ARGUMENT;

  /* Sanity checks */
  MME_ASSERT(!list_empty(&cmd->list));
  MME_ASSERT(cmd->command);

  MME_PRINTF(MME_DBG_COMMAND, "cmd %p cmdId 0x%x command %p CmdId 0x%x msg %p\n",
	     cmd, cmd->cmdId, cmd->command, cmd->command->CmdStatus.CmdId, cmd->msg);

  /* Check that the Client hasn't re-used/released their MME_Command desc */
  MME_assert(cmd->cmdId == cmd->command->CmdStatus.CmdId);

  /* Flag the internal command as killed so we don't repeat this operation */
  cmd->state = _MME_COMMAND_KILLED;

  /* signal the user command as failed/aborted */
  kill.type                   = _MME_RECEIVER_KILL;
  kill.event                  = MME_TRANSFORMER_TIMEOUT; /* Special event code */
  kill.status.CmdId           = cmd->cmdId;
  kill.status.State           = MME_COMMAND_FAILED;
  kill.status.Error           = MME_COMMAND_ABORTED;
  
  /* Send inline abort message back to our own replyPort */
  err = ICS_msg_send(transformer->replyPort, &kill, ICS_INLINE, sizeof(kill), 0 /* flags */);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_COMMAND,
		"transformer %p Failed to send kill msg to own port 0x%x\n",
		transformer, transformer->replyPort);
   
    /* XXXX Can we recover ? */
    return MME_ICS_ERROR;
  }

  return MME_SUCCESS;
}

/* MME_KillCommand()
 * Abort a command without communicating with the remote processor. Should only be used 
 * when we know the transformer processor has definitely crashed
 */
MME_ERROR MME_KillCommand (MME_TransformerHandle_t handle, MME_CommandId_t cmdId)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  int                idx;

  mme_command_t     *cmd;
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
  
  if (handle == 0)
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

  /* Find the Command array idx for this CmdId */
  idx = _MME_CMD_INDEX(cmdId);
  
  /* Sanity check CmdId */
  if (_MME_CMD_HANDLE(cmdId) != _MME_CMD_HANDLE_VAL || idx < 0 || idx >= _MME_MAX_COMMANDS)
  {
    res = MME_INVALID_ARGUMENT;
    goto error_release;
  }

  cmd = &transformer->cmds[idx];

  /* Check that the cmdId still matches */
  if (cmd->cmdId != cmdId)
  {
    res = MME_INVALID_ARGUMENT;
    goto error_release;
  }

  res = killCommand(transformer, cmd);

  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

  return res;

error_release:
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* 
 * Mark a transformer as killed and
 * issue a kill request against all outstanding commands
 *
 * MULTITHREAD SAFE: Called holding transformer lock
 */
void mme_transformer_kill (mme_transformer_t *transformer)
{
  mme_command_t  *cmd;

  if (transformer->numCmds == 0)
  {
    MME_ASSERT(list_empty(&transformer->issuedCmds));
    return;
  }
  
  /* Kill all outstanding issued commands (asynchronous) */
  list_for_each_entry(cmd, &transformer->issuedCmds, list)
  {
    (void) killCommand(transformer, cmd);
  }
}


/* MME_KillCommandAll()
 * Abort all commands without communicating with the remote processor. Should only be used 
 * when we know the transformer processor has definitely crashed
 */
MME_ERROR MME_KillCommandAll (MME_TransformerHandle_t handle)
{
  MME_ERROR       res;

  mme_transformer_t *transformer;

  /* 
   * Sanity check the arguments
   */
  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  MME_PRINTF(MME_DBG_COMMAND, "handle 0x%x\n", handle);
  
  if (handle == 0)
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

  /* Issue kill against all outstanding commands */
  mme_transformer_kill(transformer);

  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

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
