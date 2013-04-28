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

#ifndef _ICS_MSG_H
#define _ICS_MSG_H

typedef ICS_HANDLE ICS_MSG_EVENT;

/* Maximum amount of inline data for receive event desc (always ICS_CACHELINE_SIZE multiples) */
#define ICS_MSG_INLINE_DATA	96

/*
 * Valid ICS_msg_send flags
 */
typedef enum
{
  ICS_MSG_CONNECT = 0x1,	/* Automatically connect to target CPU if not connected */

} ICS_MSG_SEND_FLAGS;

/*
 * Region/Buffer flags
 */
typedef enum
{
  ICS_INLINE       = 0x01,	/* Data is inline */

  ICS_CACHED       = 0x02,	/* Cached memory */
  ICS_UNCACHED     = 0x04,	/* Uncached memory */
  ICS_WRITE_BUFFER = 0x08,	/* Write buffering (uncached memory only) */

  ICS_PHYSICAL     = 0x10,	/* Physical memory address */

} ICS_MEM_FLAGS;

/*
 * ICS_msg_recv return structure
 */
typedef struct ics_msg_desc
{
  ICS_OFFSET        data;
  ICS_SIZE	    size;
  ICS_MEM_FLAGS	    mflags;
  ICS_UINT	    srcCpu;
   
  ICS_CHAR	    payload[ICS_MSG_INLINE_DATA];

} ICS_MSG_DESC;

/*
 * - Send and Receive
 */
ICS_EXPORT ICS_ERROR ICS_msg_recv   (ICS_PORT port, ICS_MSG_DESC *rdesc, ICS_LONG timeout);
ICS_EXPORT ICS_ERROR ICS_msg_post   (ICS_PORT port, ICS_MSG_DESC *rdesc, ICS_MSG_EVENT *eventp);

ICS_EXPORT ICS_ERROR ICS_msg_cancel (ICS_MSG_EVENT handle);
ICS_EXPORT ICS_ERROR ICS_msg_wait   (ICS_MSG_EVENT handle, ICS_LONG timeout);
ICS_EXPORT ICS_ERROR ICS_msg_test   (ICS_MSG_EVENT handle, ICS_BOOL *ready);

ICS_EXPORT ICS_ERROR ICS_msg_send   (ICS_PORT port, ICS_VOID *buffer, ICS_MEM_FLAGS mflags, ICS_SIZE size, ICS_UINT flags);

#endif /* _ICS_MSG_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
