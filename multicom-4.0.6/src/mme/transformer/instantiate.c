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

/* MME_InitTransformer()
 * Instantiate a transformer instance
 */
MME_ERROR MME_InitTransformer (const char *name,
        		       MME_TransformerInitParams_t *params, 
                               MME_TransformerHandle_t *handlep)
{
  MME_ERROR               res;

  ICS_ERROR               err;
  ICS_PORT                mgrPort;
  ICS_SIZE                size;

  mme_msg_t              *msg;
  mme_manager_msg_t      *message;
  MME_UINT                messageSize;
  
  MME_TransformerHandle_t receiverHandle;
  ICS_PORT                receiverPort;

  mme_transformer_t      *transformer;

  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }
  
  MME_PRINTF(MME_DBG_MANAGER, "name '%s' params %p handle %p\n", (name ? name : "(NULL)"), params, handlep);

  /* Validate parameters */
  if (name == NULL || params == NULL || handlep == NULL ||
      params->StructSize != sizeof(MME_TransformerInitParams_t) ||
      (params->TransformerInitParamsSize && params->TransformerInitParams_p == NULL) ||
      strlen(name) > MME_MAX_TRANSFORMER_NAME)
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }
  
  /* Lookup transformer name in the ICS nameserver */
  err = ICS_nsrv_lookup(name, ICS_BLOCK, _MME_TRANSFORMER_TIMEOUT, &mgrPort, &size);
  if (err != ICS_SUCCESS)
  {
    /* Didn't find a matching nameserver entry */
    res = MME_UNKNOWN_TRANSFORMER;
    goto error;
  }

  MME_assert(size == sizeof(mgrPort));

  MME_PRINTF(MME_DBG_MANAGER, "name '%s' mgrPort 0x%x\n", name, mgrPort);

  MME_PRINTF(MME_DBG_MANAGER, "callback %p callbackData %p priority %d paramSize %d params_p  %p\n",
	     params->Callback, params->CallbackUserData,
	     params->Priority,
	     params->TransformerInitParamsSize,
	     params->TransformerInitParams_p);

  /* calculate the size of the init message */
  messageSize = sizeof(*message) + params->TransformerInitParamsSize;
  
  /* Allocate an MME meta data message (takes/drops MME state lock) */
  msg = mme_msg_alloc(messageSize);
  if (msg == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }
  message = (mme_manager_msg_t *) msg->buf;

  /* We now have the ICS Port handle of the manager task servicing the named transformer
   * Fill out an Init Transformer message and send it to the Port
   */
  strncpy(message->u.init.name, name, MME_MAX_TRANSFORMER_NAME);
  message->u.init.name[MME_MAX_TRANSFORMER_NAME] = '\0';	/* Always terminate string */
  message->u.init.priority = params->Priority;

  /* Copy in any extra parameter info */
  if (params->TransformerInitParamsSize)
  {
    MME_PRINTF(MME_DBG_MANAGER,
	       "Copy in %d ParamSize bytes from %p to %p\n",
	       params->TransformerInitParamsSize,
	       params->TransformerInitParams_p, message+1);

    /* XXXX BIG ENDIAN ISSUES */
    _ICS_OS_MEMCPY(message+1, params->TransformerInitParams_p, params->TransformerInitParamsSize);
  }
  
  MME_PRINTF(MME_DBG_MANAGER, "Sending init transformer message %p size %d to 0x%x\n",
	     message, messageSize, mgrPort);

  /* Send message and wait for reply */
  res = mme_manager_send(mgrPort, _MME_MANAGER_TRANS_INIT, message, messageSize);
  if (res != MME_SUCCESS)
    /* Some form of ICS communication error */
    goto error_free;

  /* Decode reply */
  res = message->res;
  if (res != MME_SUCCESS)
    /* Remote instantiation failed */
    goto error_free;

  MME_assert(message->type == _MME_MANAGER_TRANS_INIT);

  /* Extract the msg reply data */
  receiverHandle = message->u.init.handle;		/* Extract Receiver handle & Port */
  receiverPort   = message->u.init.port;

  MME_PRINTF(MME_DBG_MANAGER, "response: rhandle 0x%x rport 0x%x\n",
	     receiverHandle, receiverPort);

  MME_assert(receiverHandle != MME_INVALID_ARGUMENT);
  MME_assert(receiverPort != ICS_INVALID_HANDLE_VALUE);

  /* Create a transformer structure including the callback task */
  res = mme_transformer_create(receiverHandle, receiverPort, mgrPort, 
			       params->Callback, params->CallbackUserData,
			       &transformer, (params->Callback != NULL));
  if (res != MME_SUCCESS)
    /* Failed to create the local instantiation */
    goto error_term;

  /* Free off the MME meta data message */
  mme_msg_free(msg);

  MME_PRINTF(MME_DBG_MANAGER, "Successfully instantiated transformer '%s' : %p\n",
	     name, transformer);

  /* Return opaque handle to caller */
  *handlep = transformer->handle;

  return MME_SUCCESS;

error_term:
  /* Need to terminate the remote instantiated Transformer */
  message->u.term.handle = receiverHandle;
  (void) mme_manager_send(mgrPort, _MME_MANAGER_TRANS_TERM, message, sizeof(*message));

error_free:
  mme_msg_free(msg);

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "Failed name '%s' : %s (%d)\n",
	      (name ? name : "(NULL)"),
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
