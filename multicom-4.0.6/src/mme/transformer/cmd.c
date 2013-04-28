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

void mme_cmd_init (mme_transformer_t *transformer)
{
  int i;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;

  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* Initialise the Transformer cmd descs */
  transformer->numCmds        = 0;
  INIT_LIST_HEAD(&transformer->issuedCmds);
  INIT_LIST_HEAD(&transformer->freeCmds);

  /* Add all the Command descs to the freelist */
  for (i = 0; i < _MME_MAX_COMMANDS; i++)
  {
    mme_command_t *cmd = &transformer->cmds[i];

    /* Initialise each cmd desc */
    cmd->state = _MME_COMMAND_IDLE;
    
    /* Stamp the first Id/Version into the CmdId */
    cmd->cmdId = _MME_CMD_ID(i, 0);

    /* Initialise the cmd wait event */
    (void) _ICS_OS_EVENT_INIT(&cmd->block);

    list_add_tail(&cmd->list, &transformer->freeCmds);
  }
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
}

void mme_cmd_term (mme_transformer_t *transformer)
{
  int i;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;

 _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* There must be no live commands */
  MME_assert(transformer->numCmds == 0);
  MME_assert(list_empty(&transformer->issuedCmds));

  /* Destroy all cmds */
  for (i = 0; i < _MME_MAX_COMMANDS; i++)
  {
    mme_command_t *cmd = &transformer->cmds[i];
    
    _ICS_OS_EVENT_DESTROY(&cmd->block);
  }

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
}

/* Display all currently issued commands
 *
 * Called holding the transformer lock
 */
void mme_cmd_dump (mme_transformer_t *transformer)
{
  mme_command_t *cmd;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;

  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* Iterate over all issued command descs */
  list_for_each_entry(cmd, &transformer->issuedCmds, list)
  {
    MME_EPRINTF(MME_DBG_COMMAND,
		"transformer %p cmd %p command %p (0x%x) msg %p\n",
		transformer, cmd, cmd->command, cmd->command->CmdStatus.CmdId, cmd->msg);
  }
#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
}

/* 
 * Allocate a local command desc for the given transformer.
 * Adds to the issued list and updates the cmd counter
 *
 * Returns NULL if no more cmd descs are available
 *
 * Called holding the transformer lock
 */
mme_command_t *mme_cmd_alloc (mme_transformer_t *transformer)
{
  mme_command_t *cmd;
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif
  MME_ASSERT(_ICS_OS_MUTEX_HELD(&transformer->tlock));

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  /* Grab a command desc */
  if (list_empty(&transformer->freeCmds))
  {
    /* We have run out of Command descs (see _MME_MAX_COMMANDS) */

    MME_EPRINTF(MME_DBG_COMMAND,
		"transformer %p Failed to allocate a cmd desc : %d in use\n",
		transformer, transformer->numCmds);

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
    return NULL;
  }
  
  /* Pop the head of the freelist */
  cmd = list_first_entry(&transformer->freeCmds, mme_command_t, list);


  list_del_init(&cmd->list);

  /* Enqueue onto the list of issued Commands and account for it */
  list_add_tail(&cmd->list, &transformer->issuedCmds);
  transformer->numCmds++;

  /* Assert that nobody has fiddled with this */
  MME_ASSERT(cmd->state   == _MME_COMMAND_IDLE);
  MME_ASSERT(cmd->command == NULL);
  MME_ASSERT(cmd->msg     == NULL);

  MME_PRINTF(MME_DBG_COMMAND,
	     "transformer %p allocated cmd %p numCmds %d\n",
	     transformer, cmd, transformer->numCmds);

  MME_ASSERT(transformer->numCmds <= _MME_MAX_COMMANDS);

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  return cmd;
}

/* Free a command desc back to the given transformer
 * Removes from the issued list and adds to the freelist, updating the cmd counter
 *
 * MULTITHREAD SAFE: Called holding the transformer lock
 */
void mme_cmd_free (mme_transformer_t *transformer, mme_command_t *cmd)
{
#ifdef CONFIG_SMP
  ICS_MEM_FLAGS iflags;
#endif
  MME_ASSERT(_ICS_OS_MUTEX_HELD(&transformer->tlock));

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_ENTER(&mme_state->mmespinLock, iflags);
#endif
  MME_PRINTF(MME_DBG_COMMAND, "transformer %p Free cmd %p cmdId 0x%x numCmds %d\n",
	     transformer, cmd, cmd->cmdId, transformer->numCmds);

  /* Remove Cmd from issued list and account for it */
  MME_ASSERT(transformer->numCmds > 0);
  list_del(&cmd->list);
  transformer->numCmds--;

  /* Associated msg should have already been freed off */
  MME_ASSERT(cmd->msg == NULL);

  /* Clear down Cmd state */
  cmd->state   = _MME_COMMAND_IDLE;
  cmd->command = NULL;  

  /* Increment cmdId version for next use */
  cmd->cmdId = _MME_CMD_ID(_MME_CMD_INDEX(cmd->cmdId), _MME_CMD_VERSION(cmd->cmdId)+1);

  /* Free command back to head of freelist */
  list_add(&cmd->list, &transformer->freeCmds);

#ifdef CONFIG_SMP
  _ICS_OS_SPINLOCK_EXIT(&mme_state->mmespinLock, iflags);
#endif
  return;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
