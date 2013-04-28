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


/* MME_TermTransformer()
 * Terminate a transformer instance
 */
MME_ERROR MME_TermTransformer (MME_TransformerHandle_t handle)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer;
  
  mme_msg_t         *msg;
  mme_manager_msg_t *message;

  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  if (handle == 0)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }
    
  MME_PRINTF(MME_DBG_MANAGER, "handle 0x%x\n", handle);

  /* Lookup the transformer instance (takes lock on success) */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  /* No commands must be pending on the companion side */
  if (transformer->numCmds > 0)
  {
    MME_EPRINTF(MME_DBG_TRANSFORMER,
		"transformer %p : %d commands still running\n",
		transformer, transformer->numCmds);

#ifdef MME_DEBUG
    mme_cmd_dump(transformer);
#endif

    res = MME_COMMAND_STILL_EXECUTING;
    goto error_release;
  }

  /* Allocate an MME meta data control message (takes/drops MME state lock) */
  msg = mme_msg_alloc(sizeof(*msg));
  if (msg == NULL)
  {
    res = MME_NOMEM;
    goto error_release;
  }
  message = (mme_manager_msg_t *) msg->buf;

  message->u.term.handle = transformer->receiverHandle;

  MME_PRINTF(MME_DBG_MANAGER, "Sending term transformer message %p to 0x%x\n",
	     message, transformer->mgrPort);

  /* Send message and wait for reply */
  res = mme_manager_send(transformer->mgrPort, _MME_MANAGER_TRANS_TERM, message, sizeof(*message));
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT, "manager msg failed transformer %p mgrPort 0x%x : %d\n",
		transformer, transformer->mgrPort, res);
    goto error_free;
  }

  MME_assert(message->type == _MME_MANAGER_TRANS_TERM);

  /* Decode reply */
  res = message->res;
  if (res != MME_SUCCESS)
  {
    goto error_free;
  }

  /* Terminate transformer task and free off structure (destroys lock) */
  mme_transformer_term(transformer);

  /* Free off the MME meta data desc */
  mme_msg_free(msg);

  MME_PRINTF(MME_DBG_MANAGER, "Successfully terminated transformer : %p\n",
	     transformer);

  return MME_SUCCESS;

error_free:
  /* Free off the MME meta data desc */
  mme_msg_free(msg); /* takes/drops MME state lock */

error_release:
  _ICS_OS_MUTEX_RELEASE(&transformer->tlock);

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* MME_KillTransformer()
 * Terminate a transformer instance, without communicating 
 * with the remote processor, which we assume has crashed
 */
MME_ERROR MME_KillTransformer (MME_TransformerHandle_t handle)
{
  MME_ERROR          res;

  mme_transformer_t *transformer;
  
  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  if (handle == 0)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }
    
  MME_PRINTF(MME_DBG_MANAGER, "handle 0x%x\n", handle);

  /* Lookup the transformer instance (takes lock on success) */
  transformer = mme_transformer_instance(handle);
  if (transformer == NULL)
  {
    res = MME_INVALID_HANDLE;
    goto error;
  }

  /* No commands must be pending on the companion side 
   * They must all be killed before we can terminate
   */
  if (transformer->numCmds > 0)
  {
    MME_EPRINTF(MME_DBG_TRANSFORMER,
		"transformer %p : %d commands still running\n",
		transformer, transformer->numCmds);

#ifdef MME_DEBUG
    mme_cmd_dump(transformer);
#endif

    /* Issue kill for all outstanding commands (cf MME_KillCommandAll) */
    mme_transformer_kill(transformer);

    res = MME_COMMAND_STILL_EXECUTING;
    goto error_release;
  }

  /* Terminate transformer task and free off structure (destroys lock) */
  mme_transformer_term(transformer);

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
