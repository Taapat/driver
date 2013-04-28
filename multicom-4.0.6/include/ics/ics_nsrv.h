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

#ifndef _ICS_NSRV_H
#define _ICS_NSRV_H

/* Maximum length of an ICS Nameserver name (not including the '\0') */
#define ICS_NSRV_MAX_NAME			63

/* Maximum amount of object data that can be stored */
#define ICS_NSRV_MAX_DATA			8

typedef ICS_HANDLE   ICS_NSRV_HANDLE;

/*
 * Nameserver: Client side API functions
 */
ICS_EXPORT ICS_ERROR ICS_nsrv_add (const ICS_CHAR *name, ICS_VOID *data, ICS_SIZE size, ICS_UINT flags,
				   ICS_NSRV_HANDLE *handlep);
ICS_EXPORT ICS_ERROR ICS_nsrv_remove (ICS_NSRV_HANDLE handle, ICS_UINT flags);

ICS_EXPORT ICS_ERROR ICS_nsrv_lookup (const ICS_CHAR *name, ICS_UINT flags, ICS_LONG timeout, ICS_VOID *data, ICS_SIZE *sizep);

#endif /* _ICS_NSRV_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
