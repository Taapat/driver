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
 * EMBX_Mailbox.h
 *
 *
 * An API to manage the hardware mailbox resources.
 */

#ifndef _EMBX_MAILBOX_H
#define _EMBX_MAILBOX_H

#include <embx/embx_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Represents with a local or remote mailbox register. */
typedef struct EMBX_Mailbox EMBX_Mailbox_t; 

/* Mechanism to control initialization */
typedef enum EMBX_Mailbox_Flags {
	/* select while four enable/status set is the local one */
	EMBX_MAILBOX_FLAGS_SET1  = 0x01,
	EMBX_MAILBOX_FLAGS_SET2  = 0x02,

	/* this mailbox supports spinlocks */
	EMBX_MAILBOX_FLAGS_LOCKS = 0x04,

	/* handle mailboxes that are not always addressable */
	EMBX_MAILBOX_FLAGS_PASSIVE = 0x08,
	EMBX_MAILBOX_FLAGS_ACTIVATE = 0x10,

	/* aliases to match documentation in ST40 Sys. Arch. Manual */
	EMBX_MAILBOX_FLAGS_ST20 = EMBX_MAILBOX_FLAGS_SET1,
	EMBX_MAILBOX_FLAGS_ST40 = EMBX_MAILBOX_FLAGS_SET2
} EMBX_Mailbox_Flags_t;



EMBX_ERROR EMBX_Mailbox_Init(void);
EMBX_VOID  EMBX_Mailbox_Deinit(void);
EMBX_ERROR EMBX_Mailbox_Register(void *pMailbox, int intNumber, int intLevel, EMBX_Mailbox_Flags_t flags); 
EMBX_VOID  EMBX_Mailbox_Deregister(void *pMailbox);

EMBX_ERROR EMBX_Mailbox_Alloc(void (*handler)(void *), void *param, EMBX_Mailbox_t **pMailbox);
EMBX_ERROR EMBX_Mailbox_Synchronize(EMBX_Mailbox_t *local, EMBX_UINT token, EMBX_Mailbox_t **pRemote);
EMBX_VOID  EMBX_Mailbox_Free(EMBX_Mailbox_t *mailbox);
EMBX_ERROR EMBX_Mailbox_UpdateInterruptHandler(EMBX_Mailbox_t *mailbox, void (*handler)(void *), void *param);

EMBX_VOID  EMBX_Mailbox_InterruptEnable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptDisable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptClear(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptRaise(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);

EMBX_UINT  EMBX_Mailbox_StatusGet(EMBX_Mailbox_t *mailbox);
EMBX_VOID  EMBX_Mailbox_StatusSet(EMBX_Mailbox_t *mailbox, EMBX_UINT value);
EMBX_VOID  EMBX_Mailbox_StatusMask(EMBX_Mailbox_t *mailbox, EMBX_UINT set, EMBX_UINT clear);

#ifdef __cplusplus
}
#endif

#endif /* _EMBX_MAILBOX_H */
