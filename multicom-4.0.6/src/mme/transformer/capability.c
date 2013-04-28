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

/* MME_GetTransformerCapability()
 * Obtain the capabilities of a transformer 
 */
MME_ERROR MME_GetTransformerCapability (const char *name, MME_TransformerCapability_t *capability)
{
  MME_ERROR                   res;

  ICS_ERROR                   err;
  ICS_PORT                    mgrPort;
  ICS_SIZE                    size;

  mme_msg_t                   *msg;
  mme_manager_msg_t           *message;
  MME_UINT                     messageSize;
  MME_TransformerCapability_t *messageCap;

  if (mme_state == NULL)
  {
    res = MME_DRIVER_NOT_INITIALIZED;
    goto error;
  }
  
  MME_PRINTF(MME_DBG_MANAGER, "name '%s' cap %p\n", (name ? name : "(NULL)"), capability);
 
  /* Validate parameters */
  if (name == NULL || capability == NULL ||
      capability->StructSize != sizeof(MME_TransformerCapability_t) ||
      (capability->TransformerInfoSize && capability->TransformerInfo_p == NULL))
  {
    res = MME_INVALID_ARGUMENT;
    goto error;
  }

  MME_PRINTF(MME_DBG_MANAGER, "  TransformerInfoSize %d TransformerInfo_p %p\n",
	     capability->TransformerInfoSize, capability->TransformerInfo_p);
 
  /* Lookup transformer name in the ICS nameserver */
  err = ICS_nsrv_lookup(name, ICS_NONBLOCK,_MME_TRANSFORMER_LOOKUP_TIMEOUT, &mgrPort, &size);
  if (err != ICS_SUCCESS)
  {
    /* Didn't find a matching nameserver entry */
    res = MME_UNKNOWN_TRANSFORMER;
    goto error;
  }

  MME_assert(size == sizeof(mgrPort));

  MME_PRINTF(MME_DBG_MANAGER, "name '%s' mgrPort 0x%x\n", name, mgrPort);

  /* calculate the size of the capability message */
  messageSize = sizeof(*message) + capability->TransformerInfoSize;

  /* Allocate an MME meta data message (takes/drops MME state lock) */
  msg = mme_msg_alloc(messageSize);
  if (msg == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }
  message = (mme_manager_msg_t *) msg->buf;
  messageCap = &message->u.get.capability;

  /* We now have the ICS Port handle of the manager task servicing the named transformer
   * Fill out a Get Capabilities message and send it to the Port
   */
  strncpy(message->u.get.name, name, MME_MAX_TRANSFORMER_NAME);
  message->u.get.name[MME_MAX_TRANSFORMER_NAME] = '\0';

  _ICS_OS_MEMCPY(messageCap, capability, sizeof(MME_TransformerCapability_t));

  MME_PRINTF(MME_DBG_MANAGER, "Sending get capability message %p size %d to 0x%x\n",
	     message, messageSize, mgrPort);

  /* Send message and wait for reply */
  res = mme_manager_send(mgrPort, _MME_MANAGER_GET_CAPABILITY, message, messageSize);
  if (res != MME_SUCCESS)
    goto error_free;

  MME_assert(message->type == _MME_MANAGER_GET_CAPABILITY);

  MME_PRINTF(MME_DBG_MANAGER, "Get capability message %p response res %d\n",
	     message, message->res);
  
  /* Decode reply */
  res = message->res;
  if (res != MME_SUCCESS)
    goto error_free;
  
  /* SUCCESS */

  /* Copy back the Capability structure (but preserve the original TransformerInfo_p) */
  _ICS_OS_MEMCPY(capability, messageCap, offsetof(MME_TransformerCapability_t, TransformerInfo_p));
  
  /* Copy back the TransformerInfo_p data */
  if (messageCap->TransformerInfoSize)
  {
    MME_assert(messageCap->TransformerInfoSize <= capability->TransformerInfoSize);

    MME_PRINTF(MME_DBG_MANAGER,
	       "Copy back %d TransformerInfo bytes from %p to %p\n",
	       capability->TransformerInfoSize,
	       message+1, capability->TransformerInfo_p);

    _ICS_OS_MEMCPY(capability->TransformerInfo_p, message+1, messageCap->TransformerInfoSize);
  }

  /* Free off MME meta data buffer */
  mme_msg_free(msg);

  return MME_SUCCESS;

error_free:
  mme_msg_free(msg);

error:
  MME_EPRINTF(MME_DBG_INIT, 
	      "(%s, %p) : Failed : %s (%d)\n",
	      (name ? name : "(NULL)"), capability,
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
