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

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

#include <embx.h>	/* EMBX API definitions */

#include <embxshm.h>	/* EMBX SHM API definitions */

#include "_embx_sys.h"

extern EMBXSHM_MailboxConfig_t  *embx_transportConfig;

/*
 * Simple 'shim' layer to wrap EMBX API around the ICS API
 *
 * Provides all original EMBX APIs from Multicom 3.x except
 * the Object handling ones
 *
 */

/*
 * Local global variables
 */
EMBX_BOOL embx_initialised = EMBX_FALSE;

EMBX_ERROR EMBX_Init (void)
{
  if (embx_initialised)
    return EMBX_ALREADY_INITIALIZED;

  embx_initialised = EMBX_TRUE;

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_Deinit (void)
{
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  EMBX_CloseTransport(0);

  EMBX_UnregisterTransport(0);

  ICS_cpu_term(0);

  embx_initialised = EMBX_FALSE;

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_ModifyTuneable (EMBX_Tuneable_t key, EMBX_UINT value)
{
  /* XXXX Not supported */
  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_CreatePort (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
  ICS_ERROR err;

  ICS_UINT ndescs;
  
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* Inherit Port ndescs from transport config freeListSize if present */
  ndescs = (embx_transportConfig ? embx_transportConfig->freeListSize : 128);

  err = ICS_port_alloc(portName, NULL, NULL, ndescs, 0, port);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_Connect (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *portp)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_port_lookup(portName, 0 /*flags*/, ICS_TIMEOUT_IMMEDIATE, portp);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_ConnectBlock (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *portp)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_port_lookup(portName, ICS_BLOCK, ICS_TIMEOUT_INFINITE, portp);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_ConnectBlockTimeout (EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *portp,
				     EMBX_UINT timeout)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_port_lookup(portName, ICS_BLOCK, timeout, portp);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_ClosePort (EMBX_PORT port)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_port_free(port, 0);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_InvalidatePort (EMBX_PORT port)
{
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_Receive (EMBX_PORT port, EMBX_RECEIVE_EVENT *recv)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* NB: ICS_MSG_DESC must match EMBX_RECEIVE_EVENT exactly */
  ICS_ASSERT(sizeof(ICS_MSG_DESC) == sizeof(EMBX_RECEIVE_EVENT));

  err = ICS_msg_recv(port, (ICS_MSG_DESC *)recv, ICS_TIMEOUT_IMMEDIATE);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_ReceiveBlock (EMBX_PORT port, EMBX_RECEIVE_EVENT *recv)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* NB: ICS_MSG_DESC must match EMBX_RECEIVE_EVENT exactly */
  ICS_ASSERT(sizeof(ICS_MSG_DESC) == sizeof(EMBX_RECEIVE_EVENT));

  err = ICS_msg_recv(port, (ICS_MSG_DESC *)recv, ICS_TIMEOUT_INFINITE);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_ReceiveBlockTimeout (EMBX_PORT port, EMBX_RECEIVE_EVENT *recv, EMBX_UINT timeout)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* NB: ICS_MSG_DESC must match EMBX_RECEIVE_EVENT exactly */
  ICS_ASSERT(sizeof(ICS_MSG_DESC) == sizeof(EMBX_RECEIVE_EVENT));

  err = ICS_msg_recv(port, (ICS_MSG_DESC *)recv, timeout);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_SendMessage (EMBX_PORT port, EMBX_VOID *buffer, EMBX_UINT size)
{
  ICS_ERROR err;

  ICS_MEM_FLAGS mflags = embx_heapFlags;	/* XXXX Should reflect original owner ? */

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_msg_send(port, buffer, mflags, size, 0 /* flags */);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_Offset (EMBX_TRANSPORT tp, EMBX_VOID *address, EMBX_INT *offset)
{
  ICS_ERROR     err;

  ICS_OFFSET    paddr;
  ICS_MEM_FLAGS mflags;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_region_virt2phys(address, &paddr, &mflags);

  if (err == ICS_SUCCESS)
  {
    *offset = (EMBX_INT) paddr;
  }

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_Address (EMBX_TRANSPORT tp, EMBX_INT offset, EMBX_VOID **addressp)
{
  ICS_ERROR err;

  ICS_MEM_FLAGS mflags = embx_heapFlags;	/* XXXX Should reflect original owner ? */

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  err = ICS_region_phys2virt(offset, sizeof(ICS_UINT) /* size */, mflags, addressp);

  return EMBX_ERROR_CODE(err);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
