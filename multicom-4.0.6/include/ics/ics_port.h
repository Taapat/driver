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

#ifndef _ICS_PORT_H
#define _ICS_PORT_H

typedef ICS_HANDLE   ICS_PORT;

/* Maximum length of an ICS Port name */
#define ICS_PORT_MAX_NAME      		ICS_NSRV_MAX_NAME

/* Minimum number of descs in a Port mq (unless callback is used) */
#define ICS_PORT_MIN_NDESC		2

struct ics_msg_desc;

/* Callback function for Ports */
typedef ICS_ERROR (*ICS_PORT_CALLBACK) (ICS_PORT port, ICS_VOID *param, struct ics_msg_desc *rdesc);

/*
 * - Port management and lookup functions
 */
ICS_EXPORT ICS_ERROR ICS_port_alloc (const ICS_CHAR *portName, ICS_PORT_CALLBACK callback, ICS_VOID *param,
				     ICS_UINT ndesc, ICS_UINT flags, ICS_PORT *portp);
ICS_EXPORT ICS_ERROR ICS_port_free (ICS_PORT port, ICS_UINT flags);

ICS_EXPORT ICS_ERROR ICS_port_lookup (const ICS_CHAR *portName, ICS_UINT flags, ICS_LONG timeout, ICS_PORT *portp);

ICS_EXPORT ICS_ERROR ICS_port_cpu (ICS_PORT port, ICS_UINT *cpuNump);

#endif /* _ICS_PORT_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
