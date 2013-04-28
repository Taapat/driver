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
 * Client function to send a message to the MME Management task port of a CPU
 */
MME_ERROR mme_manager_send (ICS_PORT mgrPort, mme_manager_msg_type_t type, 
			    mme_manager_msg_t *msg, MME_UINT msgSize)
{
  MME_ERROR     res;

  ICS_ERROR     err;
  ICS_PORT      initReply;
  ICS_MSG_DESC  rdesc;

  ICS_UINT      flags = 0;

  MME_PRINTF(MME_DBG_MANAGER, "mgrPort 0x%x type 0x%x msg %p msgSize %d\n",
	     mgrPort, type, msg, msgSize);

  /* Create a temporary local port for the manager reply messages */
  err = ICS_port_alloc(NULL, NULL, NULL, ICS_PORT_MIN_NDESC, 0, &initReply);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to create a local port : %s (%d)\n",
		ics_err_str(err), err);
    
    res = MME_ICS_ERROR;
    goto error;
  }
  MME_assert(initReply != ICS_INVALID_HANDLE_VALUE);

  MME_PRINTF(MME_DBG_MANAGER, "Created reply port 0x%x\n", initReply);

  /* Fill out common msg desc fields */
  msg->type      = type;
  msg->replyPort = initReply;
  msg->res       = MME_INTERNAL_ERROR;

  /* For certain types of message we should attempt to
   * connect to the remote CPU if it is not connected
   */
  if (type == _MME_MANAGER_TRANS_INIT || type == _MME_MANAGER_GET_CAPABILITY)
  {
    flags |= ICS_MSG_CONNECT;
  }

  err = ICS_msg_send(mgrPort, msg, _MME_MSG_CACHE_FLAGS, msgSize, flags);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to send msg to port 0x%x : %s (%d)\n",
		mgrPort, ics_err_str(err), err);
    res = MME_TRANSFORMER_NOT_RESPONDING;
    goto error_closePort;
  }
  
  /* Block waiting for reply */
  err = ICS_msg_recv(initReply, &rdesc, _MME_TRANSFORMER_TIMEOUT);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_MANAGER, "Failed to recv msg on port 0x%x : %s (%d)\n",
		initReply, ics_err_str(err), err);

    res = (err == ICS_SYSTEM_TIMEOUT) ? MME_TRANSFORMER_NOT_RESPONDING : MME_INTERNAL_ERROR;
    goto error_closePort;
  }

  MME_PRINTF(MME_DBG_MANAGER, "Reply received : mflags 0x%x size %d data %p\n",
	     rdesc.mflags, rdesc.size, rdesc.data);

  /* Sanity check the reply */
  /*  MME_assert(rdesc.data == (ICS_OFFSET) msg); */
  if (!(rdesc.data == (ICS_OFFSET) msg)) {
 	  MME_EPRINTF(MME_DBG_MANAGER, "rdesc.data = 0x%lx (ICS_OFFSET) msg = 0x%lx\n", rdesc.data, msg);
  }
  MME_assert(rdesc.size == msgSize);
  MME_assert(rdesc.mflags == _MME_MSG_CACHE_FLAGS);

  /* Free off the local port */
  (void) ICS_port_free(initReply, 0);

  return MME_SUCCESS;

error_closePort:
  (void) ICS_port_free(initReply, 0);
  
error:
  return res;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
