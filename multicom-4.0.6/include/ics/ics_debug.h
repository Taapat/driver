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

#ifndef _ICS_DEBUG_H
#define _ICS_DEBUG_H

typedef enum 
{
  ICS_DBG          = 0x0000,	/* Always displayed */

  ICS_DBG_ERR      = 0x0001,	/* Display all error paths */
  ICS_DBG_INIT     = 0x0002,	/* Initialisation operations */
  ICS_DBG_CHN      = 0x0004,	/* Channel operations */
  ICS_DBG_MAILBOX  = 0x0008,	/* Harware mailboxes */

  ICS_DBG_MSG      = 0x0010, 	/* Message API */
  ICS_DBG_ADMIN    = 0x0020,	/* Admin messages and control */
  ICS_DBG_PORT     = 0x0040, 	/* Port protocol */


  ICS_DBG_NSRV     = 0x0100,	/* Nameserver actions */
  ICS_DBG_WATCHDOG = 0x0200,	/* Watchdog handler */
  ICS_DBG_STATS    = 0x0400,	/* Statistics logging */


  ICS_DBG_HEAP     = 0x1000,	/* Heap actions */
  ICS_DBG_REGION   = 0x2000,	/* Memory region control */
  ICS_DBG_LOAD     = 0x4000,	/* Companion firmware loader */
  ICS_DBG_DYN      = 0x8000	/* Dynamic module loader */

} ICS_DBG_FLAGS;

typedef enum 
{
  ICS_DBG_STDOUT   = 0x01,	/* Write msg to stdout */
  ICS_DBG_STDERR   = 0x02,	/* Write msgs to stderr */
  ICS_DBG_LOG      = 0x04	/* Write msgs to cyclic log */

} ICS_DBG_CHAN;

ICS_EXPORT const ICS_CHAR *ics_err_str (ICS_ERROR err);
ICS_EXPORT ICS_ERROR ICS_debug_dump (ICS_UINT cpuNum);
ICS_EXPORT ICS_ERROR ICS_debug_copy (ICS_UINT cpuNum, ICS_CHAR *buf, ICS_SIZE bufSize, ICS_INT *bytesp);

ICS_EXPORT void ics_debug_flags (ICS_UINT flags);
ICS_EXPORT void ics_debug_chan (ICS_UINT chan);

ICS_EXPORT void ICS_debug_printf (const char *fmt, ...);

#endif /* _ICS_DEBUG_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
