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

#ifndef _EMBX_TYPES_H
#define _EMBX_TYPES_H

#include <stddef.h>			/* size_t */

/*
 * Basic data types
 */
typedef void           EMBX_VOID;
typedef char           EMBX_CHAR;
typedef unsigned char  EMBX_UCHAR;
typedef unsigned char  EMBX_BYTE;
typedef short          EMBX_SHORT;
typedef unsigned short EMBX_USHORT;
typedef int            EMBX_INT;
typedef unsigned int   EMBX_UINT;
typedef long           EMBX_LONG;
typedef unsigned long  EMBX_ULONG;
typedef int            EMBX_BOOL;
typedef unsigned long  EMBX_OFFSET;
typedef size_t         EMBX_SIZE;

#define EMBX_FALSE 0
#define EMBX_TRUE  (!EMBX_FALSE)

/*
 * Opaque 32-bit handles
 */
typedef EMBX_UINT     EMBX_HANDLE;
typedef EMBX_HANDLE   EMBX_PORT;
typedef EMBX_HANDLE   EMBX_HEAP;
typedef EMBX_HANDLE   EMBX_FACTORY;			/* DEPRECATED */
typedef EMBX_HANDLE   EMBX_TRANSPORT;			/* DEPRECATED */

#define EMBX_INVALID_HANDLE_VALUE ((EMBX_HANDLE)0)


/*
 * EMBX TIMEOUT support
 */
#define EMBX_TIMEOUT_INFINITE	(0xfffffff)

/*
 * Status codes
 */
typedef enum
{
  /* Public API status codes */
  EMBX_SUCCESS = 0,
  EMBX_DRIVER_NOT_INITIALIZED,
  EMBX_ALREADY_INITIALIZED,
  EMBX_NOMEM,
  EMBX_INVALID_ARGUMENT,
  EMBX_INVALID_PORT,
  EMBX_INVALID_STATUS,
  EMBX_INVALID_TRANSPORT,
  EMBX_TRANSPORT_INVALIDATED,
  EMBX_TRANSPORT_CLOSED,
  EMBX_PORTS_STILL_OPEN,
  EMBX_PORT_INVALIDATED,
  EMBX_PORT_CLOSED,
  EMBX_PORT_NOT_BIND,
  EMBX_ALREADY_BIND,
  EMBX_CONNECTION_REFUSED,
  EMBX_SYSTEM_INTERRUPT,
  EMBX_SYSTEM_ERROR,
  EMBX_INCOHERENT_MEMORY,
  EMBX_SYSTEM_TIMEOUT,

} EMBX_ERROR;


typedef enum {
  EMBX_TUNEABLE_THREAD_STACK_SIZE,
  EMBX_TUNEABLE_THREAD_PRIORITY,
  EMBX_TUNEABLE_MAILBOX_PRIORITY,	/* WinCE Mailbox IST thread */
    
  EMBX_TUNEABLE_MAX
} EMBX_Tuneable_t;


/* Maximum amount of inline data for receive event desc */
#define EMBX_MAX_INLINE_DATA	96

/*
 * Memory/Buffer flags
 */
typedef enum
{
  EMBX_INLINE       = 0x01,	/* Data is inline */

  EMBX_CACHED       = 0x02,	/* Cached memory */
  EMBX_UNCACHED     = 0x04,	/* Uncached memory */
  EMBX_WRITE_BUFFER = 0x08,	/* Write buffering (uncached memory only) */

} EMBX_MEM_FLAGS;

/*
 * EMBX_Receive[Block] return structure
 */
typedef struct
{
  EMBX_VOID         *data;
  EMBX_SIZE          size;
  EMBX_MEM_FLAGS     flags;
  EMBX_UINT	     srcCpu;
   
  EMBX_CHAR          payload[EMBX_MAX_INLINE_DATA];

} EMBX_RECEIVE_EVENT;

/* Maximum length of an EMBX Port name */
#define EMBX_MAX_PORT_NAME      31


/* DEPRECATED */
#define EMBX_MAX_TRANSPORT_NAME 31

/* DEPRECATED */
typedef struct
{
    EMBX_CHAR  name[EMBX_MAX_TRANSPORT_NAME+1];
    EMBX_BOOL  isInitialized;
    EMBX_BOOL  usesZeroCopy;
    EMBX_BOOL  allowsPointerTranslation;
    EMBX_BOOL  allowsMultipleConnections;
    EMBX_UINT  maxPorts;
    EMBX_UINT  nrOpenHandles;
    EMBX_UINT  nrPortsInUse;
    EMBX_VOID *memStart;
    EMBX_VOID *memEnd;
    EMBX_INT   reserved[4];
} EMBX_TPINFO;

/* DEPRECATED */
typedef struct EMBX_Transport_s EMBX_Transport_t;
typedef EMBX_Transport_t *EMBX_TransportFactory_fn(EMBX_VOID *);

#endif /* _EMBX_TYPES_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
