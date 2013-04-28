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

static const MME_Command_t pingCommandTemplate = { sizeof(MME_Command_t),
						   MME_PING,			/* PING the transformer */
						   MME_COMMAND_END_RETURN_WAKE,	/* Uses MME_WaitCommand() */
						   _MME_HIGH_PRIORITY,		/* High priority command */
						   0,				/* Num in bufs */
						   0,				/* Num out bufs */
						   NULL,			/* Buffer ptr */
						   {				/* Command status */
						     0,				/*   CmdId */
						     MME_COMMAND_IDLE,		/*   State */
						     0,				/*   ProcessedTime */
						     MME_SUCCESS,		/*   Error */
						     0,				/*   Num params */
						     NULL			/*   Params */
						   },
						   0,				/* Num params */
						   NULL				/* Params */
};

/*
 * Create and issue a PING command to the target transformer
 * Then wait the specified timeout for it to be acknowledged
 *
 * MULTITHREAD SAFE: Called holding the transformer lock which is dropped
 * before exit
 */
static
MME_ERROR pingTransformer (mme_transformer_t *transformer, MME_Time_t timeout)
{
  MME_ERROR res;

  MME_Command_t pingCommand = pingCommandTemplate;
  MME_Event_t event;

  /* This call drops the lock */
  res = mme_command_send(transformer, &pingCommand);
  if (res != MME_SUCCESS)
    return res;

  /* Now wait for the PING response or a timeout 
   *
   * NB: In the case of a timeout we assume the remote is dead and free off
   * the internal command desc.
   */
  res = MME_WaitCommand(transformer->handle, pingCommand.CmdStatus.CmdId, &event, timeout);

  MME_PRINTF(MME_DBG_COMMAND, "wait CmdId 0x%x timeout 0x%x returned %d\n",
	     pingCommand.CmdStatus.CmdId, timeout, res);
  
  return res;
}

MME_ERROR MME_PingTransformer (MME_TransformerHandle_t handle, MME_Time_t timeout)
{
  MME_ERROR          res = MME_INTERNAL_ERROR;

  mme_transformer_t *transformer;

  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }

  MME_PRINTF(MME_DBG_COMMAND, "handle 0x%x timeout 0x%x\n", handle, timeout);

  /* Validate parameters */
  if (handle == 0)
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

  /* This call drops the transformer lock */
  return pingTransformer(transformer, timeout);

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* MME_IsStillAlive()
 * Ping a remote transformer to see if it's still running
 * Sets alive to be non zero if it replies, zero otherwise
 */
MME_ERROR MME_IsStillAlive (MME_TransformerHandle_t handle, MME_UINT *alivep)
{
  MME_ERROR res;
  
  /* Ping the transformer using a 'fixed' timeout value */
  res = MME_PingTransformer(handle, _MME_COMMAND_TIMEOUT);

  *alivep = (res == MME_SUCCESS);

  return res;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

