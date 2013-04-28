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

#ifndef _ICS_MAILBOX_H
#define _ICS_MAILBOX_H

/* Represents a local or remote mbox register. */
typedef volatile ICS_UINT ics_mailbox_t; 

/* Represents a hw lock element */
typedef volatile ICS_UINT ics_mailbox_lock_t;

#define _ICS_MAILBOX_NUM		(4)		/* Number of hw mboxes per mailbox */
#define _ICS_MAILBOX_LOCKS		(32)		/* Number of hw locks per mailbox */

/*
 * There are 4 hw mboxes per mailbox, divide them up for maximum utilisation 
 */
#define _ICS_MAILBOX_PADDR		(0)		/* Use this Mbox for publishing the SHM paddr */
#define _ICS_MAILBOX_CHN_32		(1)		/* Use this Mbox for the 1st 32 channels */
#define _ICS_MAILBOX_CHN_64		(2)		/* Use this Mbox for the 2nd 32 channels */

/* Encode the SHM <paddr,token,ver,cpu> into the 32-bit mbox register
 *
 * [31-12] SHM segment paddr (20-bit) 	ASSUMES minimum 4k ICS SHM segment alignment
 * [11-09] SHM Token         ( 3-bit)
 * [08-05] SHM Version       ( 4-bit)
 * [04-00] SHM CPU number    ( 5-bit)
 *
 */
#define _ICS_MAILBOX_SHM_PADDR_MASK	(0xfffff << 12)
#define _ICS_MAILBOX_SHM_TOKEN_MASK	(0x7)
#define _ICS_MAILBOX_SHM_TOKEN_SHIFT	(9)
#define _ICS_MAILBOX_SHM_VER_MASK	(0xf)
#define _ICS_MAILBOX_SHM_VER_SHIFT	(5)
#define _ICS_MAILBOX_SHM_CPU_MASK	(0x1f)
#define _ICS_MAILBOX_SHM_CPU_SHIFT	(0)

#define _ICS_MAILBOX_SHM_TOKEN_VALUE	(0x5)	/* Token to distinguish SHM segment mboxes */

/* Create a Mailbox value which encodes the <PADDR,TOK,VER,CPU> */
#define _ICS_MAILBOX_SHM_HANDLE(PADDR,VER,CPU)	(((PADDR) & _ICS_MAILBOX_SHM_PADDR_MASK) | \
						 (_ICS_MAILBOX_SHM_TOKEN_VALUE << _ICS_MAILBOX_SHM_TOKEN_SHIFT) | \
						 (((VER) & _ICS_MAILBOX_SHM_VER_MASK) << _ICS_MAILBOX_SHM_VER_SHIFT) | \
						 (((CPU) & _ICS_MAILBOX_SHM_CPU_MASK) << _ICS_MAILBOX_SHM_CPU_SHIFT))
/* Extract PADDR */
#define _ICS_MAILBOX_SHM_PADDR(VALUE)		((VALUE) & _ICS_MAILBOX_SHM_PADDR_MASK)

/* Extract TOKEN */
#define _ICS_MAILBOX_SHM_TOKEN(VALUE)		(((VALUE) >> _ICS_MAILBOX_SHM_TOKEN_SHIFT) & _ICS_MAILBOX_SHM_TOKEN_MASK)

/* Extract VER */
#define _ICS_MAILBOX_SHM_VER(VALUE)		(((VALUE) >> _ICS_MAILBOX_SHM_VER_SHIFT) & _ICS_MAILBOX_SHM_VER_MASK)

/* Extract CPU */
#define _ICS_MAILBOX_SHM_CPU(VALUE)		(((VALUE) >> _ICS_MAILBOX_SHM_CPU_SHIFT) & _ICS_MAILBOX_SHM_CPU_MASK)


/* Maps an mbox number and bit index onto the channel array */
#define _ICS_MAILBOX_MBOX2CHN(MBOX,IDX)	((MBOX-_ICS_MAILBOX_CHN_32)*32 + (IDX))
/* Map a channel number back to the correct mbox */
#define _ICS_MAILBOX_CHN2MBOX(CHN)	(_ICS_MAILBOX_CHN_32+((CHN)/(sizeof(ics_mailbox_t)*8)))
/* Map a channel number back to the correct mbox bit index */
#define _ICS_MAILBOX_CHN2IDX(CHN)	((CHN)%(sizeof(ics_mailbox_t)*8))

typedef void (*ICS_MAILBOX_FN)(ICS_VOID *param, ICS_UINT status);

/* Function prototypes */
ICS_ERROR ics_mailbox_init (void);
void      ics_mailbox_term (void);
ICS_ERROR ics_mailbox_register (void *pMailbox, int intNumber, int intLevel, unsigned long flags);
ICS_VOID  ics_mailbox_deregister (void *pMailbox);
ICS_ERROR ics_mailbox_alloc (ICS_MAILBOX_FN handler, void *param, ics_mailbox_t **pMailbox);
ICS_VOID  ics_mailbox_free (ics_mailbox_t *mailbox);
ICS_ERROR ics_mailbox_find (ICS_UINT token, ICS_UINT mask, ics_mailbox_t **pMailbox);
ICS_VOID  ics_mailbox_dump (void);
ICS_ERROR ics_mailbox_update_interrupt_handler (ics_mailbox_t *mailbox, ICS_MAILBOX_FN handler, void *param);

ICS_VOID ics_mailbox_interrupt_enable (ics_mailbox_t *mailbox, ICS_UINT bit);
ICS_VOID ics_mailbox_interrupt_disable (ics_mailbox_t *mailbox, ICS_UINT bit);
ICS_VOID ics_mailbox_interrupt_clear (ics_mailbox_t *mailbox, ICS_UINT bit);
ICS_VOID ics_mailbox_interrupt_raise (ics_mailbox_t *mailbox, ICS_UINT bit);

ICS_UINT ics_mailbox_enable_get (ics_mailbox_t *mailbox);
ICS_VOID ics_mailbox_enable_set (ics_mailbox_t *mailbox, ICS_UINT value);
ICS_VOID ics_mailbox_enable_mask (ics_mailbox_t *mailbox, ICS_UINT set, ICS_UINT clear);

ICS_UINT ics_mailbox_status_get (ics_mailbox_t *mailbox);
ICS_VOID ics_mailbox_status_set (ics_mailbox_t *mailbox, ICS_UINT value);
ICS_VOID ics_mailbox_status_mask (ics_mailbox_t *mailbox, ICS_UINT set, ICS_UINT clear);

/* HW lock management */
ICS_ERROR ics_mailbox_lock_alloc (ics_mailbox_lock_t **pLock);
ICS_VOID  ics_mailbox_lock_free (ics_mailbox_lock_t *spinlock);

/* Hardware lock acquire/release */
#define ics_mailbox_lock_take (lock)   do { ICS_UINT backoff = 1; while (*((volatile int *)lock)) { _ICS_BUSY_WAIT(backoff, 1024); } while(0)
#define ics_mailbox_lock_release (lock) (*((volatile int *) lock) = 0)

#endif /* _ICS_MAILBOX_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
