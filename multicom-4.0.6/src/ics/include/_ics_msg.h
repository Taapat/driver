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

#ifndef _ICS_MSG_SYS_H
#define _ICS_MSG_SYS_H

#define _ICS_MSG_SSIZE			(sizeof(ics_msg_t))	/* Size of a message channel slot */

typedef struct ics_msg 
{
  ICS_UINT		srcCpu;		/* Sending cpu # */
  ICS_PORT       	port;		/* Target port # */
  ICS_OFFSET		data;		/* 'address' of data (if not ICS_INLINE) */
  ICS_SIZE		size;		/* Size of message */
  ICS_MEM_FLAGS		mflags;		/* Memory/Buffer flags */
  ICS_UINT		seqNo;		/* Message seqNo to aid debugging */    
  
  /* Pad to next cacheline boundary */
  ICS_CHAR		pad[_ICS_CACHELINE_PAD(4,2)];
  
  /* CACHELINE ALIGNED */
  ICS_CHAR		payload[ICS_MSG_INLINE_DATA];	/* Defined in ics_msg.h */
  
} ics_msg_t;

/* Per Channel message handler callback */
ICS_ERROR ics_message_handler (ICS_CHANNEL handle, void *p, void *b);

/* Exported internal APIs */
ICS_ERROR _ics_msg_send (ICS_PORT port, ICS_VOID *buf, ICS_MEM_FLAGS mflags, ICS_UINT size, ICS_UINT flags, 
			 ICS_UINT cpuNum);

#endif /* _ICS_MSG_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
