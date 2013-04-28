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

#ifndef _EMBXSHM_H
#define _EMBXSHM_H

/*
 * DEPRECTED EMBX SHM header file.
 *
 * Supplied to provide compatibility with Multicom 3.x
 */

#include <embx/embx_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EMBXSHM_MAX_CPUS 8

typedef struct
{
    EMBX_CHAR  name[EMBX_MAX_TRANSPORT_NAME+1]; /* The name of this transport */
    EMBX_UINT  cpuID;                           /* Local CPU ID (0 == master) */
    EMBX_UINT  participants[EMBXSHM_MAX_CPUS];  /* 1 == participating in this transport */
    EMBX_UINT  pointerWarp;                     /* Offset when pointer warping */
                                                /* LocalAddr + offset == BusAddr */
                                                /* BusAddr - offset == LocalAddr */
    EMBX_UINT  maxPorts;                        /* Max ports supported (0 == no limit) */
    EMBX_UINT  maxObjects;                      /* Number of shared objects to support */
    EMBX_UINT  freeListSize;                    /* Number of pre-allocated free nodes per port */
    EMBX_VOID *sharedAddr;                      /* Address of base of shared memory */
    EMBX_UINT  sharedSize;                      /* Size of shared memory */
    EMBX_VOID *warpRangeAddr;                   /* Primary warp range base address */
    EMBX_UINT  warpRangeSize;                   /* Primary warp range size */
    EMBX_VOID *warpRangeAddr2;                  /* Secondary Warp range base address */
    EMBX_UINT  warpRangeSize2;                  /* Secondary Warp range size */
} EMBXSHM_MailboxConfig_t;

EMBX_Transport_t *EMBXSHM_mailbox_factory (EMBX_VOID *param);
EMBX_Transport_t *EMBXSHMC_mailbox_factory (EMBX_VOID *param);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBXSHM_H */


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
