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

#ifndef _ICS_SHM_DEBUG_H
#define _ICS_SHM_DEBUG_H

/*
 * Datastructure and defines for the cyclic logging buffer
 *
 * The buffer is managed as a cyclic array of fixed length lines
 * Newly logged strings are written to the line at the fptr line offset
 * The oldest logged lines are found at the bptr line offset
 *
 * fptr == bptr indicates an empty buffer
 *
 * When the buffer overflows the overflow count is incremented and the
 * bptr is moved forwards, hence erasing the oldest line.
 *
 */

#if defined(ICS_DEBUG) || defined(ICS_DEBUG_FLAGS)
#define _ICS_DEBUG_LOG_SIZE	(64*1024)	/* Total size of debugLog segment (510 lines @ 128 chars) */
#else
#define _ICS_DEBUG_LOG_SIZE	(8*1024)	/* Total size of debugLog segment (62 lines @ 128 chars) */
#endif

#define _ICS_DEBUG_NUM_LINES	((_ICS_DEBUG_LOG_SIZE-offsetof(ics_shm_debug_log_t, log))/_ICS_DEBUG_LINE_SIZE)

typedef struct ics_shm_debug_log
{
  ics_shm_spinlock_t		dbgLock;			/* Inter-cpu SHM spinlock */

  /* CACHELINE aligned */
  volatile ICS_UINT		fptr;				/* Insert here (line #) */
  volatile ICS_UINT		bptr;				/* Extract from here (line #) */
  volatile ICS_UINT		overflow;			/* Number of lines dropped (shared) */
  ICS_UINT			numLines;			/* Number of lines in cyclic buffer */
  ICS_SIZE			size;				/* Size of the cyclic buffer (bytes) */

  ICS_OFFSET			paddr;				/* Needed for cache flush/purge */

  ICS_CHAR			_pad[_ICS_CACHELINE_PAD(4,2)];

  /* CACHELINE aligned */
  ICS_CHAR			log[1][_ICS_DEBUG_LINE_SIZE];	/* Cyclic buffer (variable num lines) */
} ics_shm_debug_log_t;

/* Internal message logging APIs */
ICS_ERROR ics_shm_debug_init (void);
ICS_ERROR ics_shm_debug_term (void);

void      ics_shm_debug_msg (ICS_CHAR *msg, ICS_UINT len);
ICS_ERROR ics_shm_debug_dump (ICS_UINT cpuNum);
ICS_ERROR ics_shm_debug_copy (ICS_UINT cpuNum, ICS_CHAR *buf, ICS_SIZE bufSize, ICS_INT *bytesp);

#endif /* _ICS_SHM_DEBUG_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

