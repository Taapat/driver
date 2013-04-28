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

#include <ics.h>		/* External defines and prototypes */

#include "_ics_sys.h"		/* Internal defines and prototypes */

#include <embxmailbox.h>	/* EMBX API definitions */

#include "_embx_sys.h"

#define _EMBX_MAILBOX_SET2	0x100

/*
 * Simple 'shim' layer to wrap EMBX API around the ICS API
 *
 * Provides most of the original EMBX APIs from Multicom 3.x
 *
 */

EMBX_ERROR EMBX_Mailbox_Init (void)
{
  ICS_ERROR err;
  
  err = ics_mailbox_init();

  return EMBX_ERROR_CODE(err);
}

EMBX_VOID EMBX_Mailbox_Deinit (void)
{
  ics_mailbox_term();
}


EMBX_ERROR EMBX_Mailbox_Register (void *pMailbox, int intNumber, int intLevel, EMBX_Mailbox_Flags_t flags)
{
  ICS_ERROR err;

  /* The new ICS mailbox scheme treats the Mailbox IP as two distinct
   * mailboxes. The local ones being indicated by a non zero intNumber
   */
  
  if (flags & EMBX_MAILBOX_FLAGS_SET1)
  {
    /* We own the lower set */
    err = ics_mailbox_register(pMailbox, intNumber, intLevel, flags);

    /* Register upper set as remote */
    err = ics_mailbox_register(pMailbox + _EMBX_MAILBOX_SET2, 0, intLevel, flags);
  }
  else if (flags & EMBX_MAILBOX_FLAGS_SET2)
  {
    /* Register lower set as remote */
    err = ics_mailbox_register(pMailbox, 0, intLevel, flags);

    /* We own the upper set (offset _EMBX_MAILBOX_SET2) */
    err = ics_mailbox_register(pMailbox + _EMBX_MAILBOX_SET2, intNumber, intLevel, flags);
  }
  else
  {
    /* Register both sets as remote mailboxes */
    err = ics_mailbox_register(pMailbox, 0, intLevel, flags);
    err = ics_mailbox_register(pMailbox + _EMBX_MAILBOX_SET2, 0, intLevel, flags);
  }

  return EMBX_ERROR_CODE(err);
}

EMBX_VOID EMBX_Mailbox_Deregister (void *pMailbox)
{
  ics_mailbox_deregister(pMailbox);
  ics_mailbox_deregister(pMailbox+_EMBX_MAILBOX_SET2);

  return;
}

EMBX_ERROR EMBX_Mailbox_Alloc (void (*handler)(void *), void *param, EMBX_Mailbox_t **pMailbox)
{
  ICS_ERROR err;

  err = ics_mailbox_alloc((ICS_MAILBOX_FN)handler, param, (ics_mailbox_t **) pMailbox);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_Mailbox_Synchronize (EMBX_Mailbox_t *local, EMBX_UINT token, EMBX_Mailbox_t **pRemote)
{
  ICS_ERROR err;

  err = ics_mailbox_find(token, (ICS_UINT)-1 /* mask */, (ics_mailbox_t **) pRemote);

  return EMBX_ERROR_CODE(err);
}

EMBX_VOID  EMBX_Mailbox_Free (EMBX_Mailbox_t *mailbox)
{
  ics_mailbox_free((ics_mailbox_t *)mailbox);
}

EMBX_ERROR EMBX_Mailbox_UpdateInterruptHandler (EMBX_Mailbox_t *mailbox, void (*handler)(void *), void *param)
{
  ICS_ERROR err;

  err = ics_mailbox_update_interrupt_handler((ics_mailbox_t *)mailbox, (ICS_MAILBOX_FN)handler, param);

  return EMBX_ERROR_CODE(err);
}

EMBX_VOID  EMBX_Mailbox_InterruptEnable (EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
  ics_mailbox_interrupt_enable((ics_mailbox_t *)mailbox, bit);
}

EMBX_VOID  EMBX_Mailbox_InterruptDisable (EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
  ics_mailbox_interrupt_disable((ics_mailbox_t *)mailbox, bit);
}

EMBX_VOID  EMBX_Mailbox_InterruptClear (EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
  ics_mailbox_interrupt_clear((ics_mailbox_t *)mailbox, bit);
}

EMBX_VOID  EMBX_Mailbox_InterruptRaise (EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
  ics_mailbox_interrupt_raise((ics_mailbox_t *)mailbox, bit);
}

EMBX_UINT  EMBX_Mailbox_StatusGet (EMBX_Mailbox_t *mailbox)
{
  ICS_UINT value;
  
  value = ics_mailbox_status_get((ics_mailbox_t *)mailbox);
  
  return value;
}

EMBX_VOID  EMBX_Mailbox_StatusSet (EMBX_Mailbox_t *mailbox, EMBX_UINT value)
{
  ics_mailbox_status_set((ics_mailbox_t *)mailbox, value);
}

EMBX_VOID  EMBX_Mailbox_StatusMask (EMBX_Mailbox_t *mailbox, EMBX_UINT set, EMBX_UINT clear)
{
  ics_mailbox_status_mask((ics_mailbox_t *)mailbox, set, clear);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
