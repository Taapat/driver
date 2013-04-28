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

#ifndef _ICS_SHM_SYS_H
#define _ICS_SHM_SYS_H

#include "_ics_shm_lock.h"
#include "_ics_shm_debug.h"
#include "_ics_shm_channel.h"

#define _ICS_SHM_CTRL_CACHED	ICS_TRUE			/* TRUE causes SHM ctrl to be cached */
#define _ICS_SHM_FIFO_CACHED	ICS_TRUE			/* TRUE causes FIFO data to be cached */

/*
 * Global ICS shm state
 */

extern struct ics_shm_state	*ics_shm_state;

/* SHM: ICS per CPU Shared Memory segment */
typedef struct ics_shm_seg
{
  ics_shm_spinlock_t 		segLock;			/* Inter-cpu spinlock */

  /* CACHELINE aligned */
  ics_chn_t			chn[_ICS_MAX_CHANNELS];		/* The ICS SHM channel descs */

  /* CACHELINE aligned */
  ICS_UINT			version;			/* ICS protocol version # */
  
  ICS_UINT			cpuNum;				/* Owning cpu # */
  
  ICS_OFFSET			mbox[_ICS_MAX_MAILBOXES];	/* Mailbox addresses (paddr) */

  volatile ICS_ULONG		watchdogTs;			/* Watchdog time stamp */

  ICS_SIZE			debugLogSize;			/* Size (bytes) debugLog */
  ICS_OFFSET			debugLog;			/* Cyclic debug logging area (paddr) */

  ICS_STATS_ITEM		stats[ICS_STATS_MAX_ITEMS];	/* STATISTICS array */

} ics_shm_seg_t;

/* Local: Per CPU SHM info */
typedef struct ics_shm_cpu
{
  ics_shm_seg_t			*shm;				/* The mapped SHM segment of the cpu */
  ICS_SIZE 			 shmSize;			/* Size of the mapped SHM segment */
  ICS_OFFSET			 paddr;				/* Physical address used for cache flush/purge */

  ics_shm_debug_log_t           *debugLog;			/* Mapped in debugLog for this cpu */

  ics_mailbox_t			*rmbox[_ICS_MAX_MAILBOXES];	/* The remote mbox mappings for the cpu */

  ICS_UINT			 version;			/* Incarnation number */

} ics_shm_cpu_t;

/* Local: Primary ICS SHM state */
typedef struct ics_shm_state 
{
  _ICS_OS_MUTEX			 shmLock;			/* OS Lock to protect this structure */

  _ICS_OS_SPINLOCK		 shmSpinLock;			/* IRQ/Spinlock to protect from the chan handler IRQ */

  ICS_UINT		 	 cpuNum;			/* The local cpu's number */
  ICS_ULONG			 cpuMask;			/* Bitmask of present cpus */

  ics_mailbox_t			*mbox[_ICS_MAX_MAILBOXES];	/* The local mboxes for our cpu */
  ICS_UINT			 lastIdx[_ICS_MAX_MAILBOXES];	/* Used to ensure round-robin servicing */

  void				*mem;				/* Allocate SHM ctrl segment base */
  struct ics_shm_seg		*shm;				/* The local cpu's SHM ctrl segment */
  ICS_SIZE			 shmSize;			/* Total size of the SHM segment */
  ICS_OFFSET			 paddr;				/* Phys address of the SHM segment */

  _ICS_OS_SPINLOCK		 debugLock;			/* Protect the debugLog (local) */
  ics_shm_debug_log_t           *debugLog;			/* Local debugLog for this cpu */
  
  ics_shm_cpu_t			 cpu[_ICS_MAX_CPUS];		/* SHM Connected cpus */
  ics_shm_channel_t		 channel[_ICS_MAX_CHANNELS];	/* Local SHM Channel descriptors */

} ics_shm_state_t;

/*
 * Exported Internal APIs
 */
ICS_EXPORT ICS_ERROR ics_shm_init (ICS_UINT cpuNum, ICS_ULONG cpuMask);
ICS_EXPORT void      ics_shm_term (void);
ICS_EXPORT ICS_ERROR ics_shm_start (void);
ICS_EXPORT void      ics_shm_stop (void);

ICS_EXPORT ICS_ERROR ics_shm_connect (ICS_UINT cpuNum, ICS_UINT timeout);
ICS_EXPORT ICS_ERROR ics_shm_disconnect (ICS_UINT cpuNum, ICS_UINT flags);

ICS_EXPORT void      ics_shm_watchdog (ICS_ULONG timestamp);
ICS_EXPORT ICS_UINT  ics_shm_watchdog_query (ICS_UINT cpuNum);

ICS_EXPORT ICS_ERROR ics_shm_stats_add (const ICS_CHAR *name, ICS_STATS_TYPE type, ICS_STATS_HANDLE *handlep);
ICS_EXPORT ICS_ERROR ics_shm_stats_remove (ICS_STATS_HANDLE handle);
ICS_EXPORT ICS_ERROR ics_shm_stats_update (ICS_STATS_HANDLE handle, ICS_STATS_VALUE value, ICS_STATS_TIME timestamp);
ICS_EXPORT ICS_ERROR ics_shm_stats_sample (ICS_UINT cpuNum, ICS_STATS_ITEM *stats, ICS_UINT *nstatsp);

ICS_EXPORT ICS_ERROR ics_shm_cpu_version (ICS_UINT cpuNum, ICS_ULONG *versionp);

#endif /* _ICS_SHM_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
