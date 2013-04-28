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

#ifndef _ICS_TYPES_H
#define _ICS_TYPES_H

#define ICS_EXPORT 			extern

/*
 * Basic data types
 */
typedef void           			ICS_VOID;
typedef char           			ICS_CHAR;
typedef unsigned char  			ICS_UCHAR;
typedef unsigned char  			ICS_BYTE;
typedef short          			ICS_SHORT;
typedef unsigned short 			ICS_USHORT;
typedef int            			ICS_INT;
typedef unsigned int   			ICS_UINT;
typedef long           			ICS_LONG;
typedef unsigned long  			ICS_ULONG;
typedef int            			ICS_BOOL;
typedef unsigned int   			ICS_UINT32;
/* Force alignment of this to maintain consistency between sh4 and ST231 compilers */
typedef unsigned long long __attribute__ ((aligned(8))) ICS_UINT64;

/* These could be 64-bit one day */
typedef size_t         			ICS_SIZE;
typedef unsigned long  			ICS_OFFSET;

/*
 * Opaque 32-bit handles
 */
typedef ICS_UINT32     			ICS_HANDLE;

#define ICS_INVALID_HANDLE_VALUE 	((ICS_HANDLE)0)

#define ICS_FALSE 			(0)
#define ICS_TRUE 			(!ICS_FALSE)

/* Value returned to indicate a bad ICS_OFFSET */
#define ICS_BAD_OFFSET  		((ICS_OFFSET)0xdeadface)

/* CPU bitmask representing all CPUs */
#define ICS_CPU_ALL			(ICS_UINT)(-1UL)

/* Standard timeout values */
#define ICS_TIMEOUT_INFINITE		(0x7fffffff)
#define ICS_TIMEOUT_IMMEDIATE		(0)

/* Various API bit flags */
#define ICS_NONBLOCK			(0U << 0)
#define ICS_BLOCK			   (1U << 0)
#define ICS_CPU_DEAD			(1U << 1)

/*
 * Status codes
 */
typedef enum
{
    /* Public API status codes */
    ICS_SUCCESS             = 0,

    ICS_NOT_INITIALIZED     = 1,
    ICS_ALREADY_INITIALIZED = 2,
    ICS_ENOMEM              = 3, 
    ICS_INVALID_ARGUMENT    = 4,
    ICS_HANDLE_INVALID      = 5,

    ICS_SYSTEM_INTERRUPT    = 10,
    ICS_SYSTEM_ERROR        = 11,
    ICS_SYSTEM_TIMEOUT	    = 12,

    ICS_NOT_CONNECTED       = 20,
    ICS_CONNECTION_REFUSED  = 21,

    ICS_EMPTY 	            = 30,
    ICS_FULL                = 31,

    ICS_PORT_CLOSED         = 40,

    ICS_NAME_NOT_FOUND      = 50,
    ICS_NAME_IN_USE         = 51,

} ICS_ERROR;

#endif /* _ICS_TYPES_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
