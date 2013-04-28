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

#ifndef _EMBX_H
#define _EMBX_H

#ifdef __WINCE__
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT extern
#endif

#include <embx/embx_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Function prototypes
 */

/*
 * - Generic
 */
DLLEXPORT EMBX_ERROR  EMBX_Init   (void);
DLLEXPORT EMBX_ERROR  EMBX_Deinit (void);
DLLEXPORT EMBX_ERROR  EMBX_ModifyTuneable (EMBX_Tuneable_t key, EMBX_UINT value);

/*
 * - Transports - DEPRECATED
 */
DLLEXPORT EMBX_ERROR EMBX_RegisterTransport   (EMBX_TransportFactory_fn *fn, EMBX_VOID *arg, EMBX_UINT argSize, EMBX_FACTORY *hFactory);
DLLEXPORT EMBX_ERROR EMBX_UnregisterTransport (EMBX_FACTORY hFactory);

DLLEXPORT EMBX_ERROR EMBX_FindTransport       (const EMBX_CHAR *name, EMBX_TPINFO *tpinfo);
DLLEXPORT EMBX_ERROR EMBX_GetFirstTransport   (EMBX_TPINFO *tpinfo);
DLLEXPORT EMBX_ERROR EMBX_GetNextTransport    (EMBX_TPINFO *tpinfo);

DLLEXPORT EMBX_ERROR EMBX_OpenTransport       (const EMBX_CHAR *name, EMBX_TRANSPORT *tp);
DLLEXPORT EMBX_ERROR EMBX_CloseTransport      (EMBX_TRANSPORT tp);
DLLEXPORT EMBX_ERROR EMBX_GetTransportInfo    (EMBX_TRANSPORT tp, EMBX_TPINFO *tpinfo);

/*
 * - Buffer Management
 */

DLLEXPORT EMBX_ERROR EMBX_Alloc (EMBX_TRANSPORT tp, EMBX_UINT size, EMBX_VOID **buffer);
DLLEXPORT EMBX_ERROR EMBX_Free  (EMBX_VOID *buffer);
DLLEXPORT EMBX_ERROR EMBX_Release (EMBX_TRANSPORT tp, EMBX_VOID *buffer);

DLLEXPORT EMBX_ERROR EMBX_GetBufferSize(EMBX_VOID *buffer, EMBX_UINT *size);

/*
 * - Object Management - DEPRECATED
 */

/*
 * - Ports
 */
DLLEXPORT EMBX_ERROR EMBX_CreatePort     (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_Connect        (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_ConnectBlock   (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_ConnectBlockTimeout (EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port, EMBX_UINT timeout);

DLLEXPORT EMBX_ERROR EMBX_ClosePort      (EMBX_PORT port);
DLLEXPORT EMBX_ERROR EMBX_InvalidatePort (EMBX_PORT port);

/*
 * - Send and Receive
 */
DLLEXPORT EMBX_ERROR EMBX_Receive      (EMBX_PORT port, EMBX_RECEIVE_EVENT *);
DLLEXPORT EMBX_ERROR EMBX_ReceiveBlock (EMBX_PORT port, EMBX_RECEIVE_EVENT *);
DLLEXPORT EMBX_ERROR EMBX_ReceiveBlockTimeout (EMBX_PORT port, EMBX_RECEIVE_EVENT *recev, EMBX_UINT timeout);

DLLEXPORT EMBX_ERROR EMBX_SendMessage  (EMBX_PORT port, EMBX_VOID *buffer,  EMBX_UINT size);


/*
 * - Pointer manipulation
 */
DLLEXPORT EMBX_ERROR EMBX_Offset  (EMBX_TRANSPORT tp, EMBX_VOID *address, EMBX_INT   *offset);
DLLEXPORT EMBX_ERROR EMBX_Address (EMBX_TRANSPORT tp, EMBX_INT   offset,  EMBX_VOID **address);


/*
 * EMBX Transport configuration variables
 */
DLLEXPORT EMBX_INT   embx_cpuNum;
DLLEXPORT EMBX_ULONG embx_cpuMask;
DLLEXPORT EMBX_UINT  embx_heapSize;
DLLEXPORT EMBX_UINT  embx_heapFlags;

/* 
 * EMBXSHM emulation
 */
DLLEXPORT EMBX_Transport_t *EMBXSHM_mailbox_factory (EMBX_VOID *param);
DLLEXPORT EMBX_Transport_t *EMBXSHMC_mailbox_factory (EMBX_VOID *param);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBX_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
