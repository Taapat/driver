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
 * Private header file for ICS implementation
 *
 */ 

#ifndef _ICS_SYS_H
#define _ICS_SYS_H

/* Calculate the number of bytes of structure padding needed to reach the next CACHELINE
 * given that (I) ICS_UINT and (O) ICS_OFFSET members are present in the structure
 */
#define _ICS_CACHELINE_PAD(I, O)	(ICS_CACHELINE_SIZE - ((I)*sizeof(ICS_UINT)+(O)*sizeof(ICS_OFFSET)))

/* Local private header files */
#include "_ics_os.h"
#include "_ics_debug.h"
#include "_ics_list.h"
#include "_ics_limits.h"
#include "_ics_util.h"
#include "_ics_debug.h"
#include "_ics_mailbox.h"
#include "_ics_connect.h"
#include "_ics_chn.h"
#include "_ics_mq.h"
#include "_ics_handle.h"
#include "_ics_msg.h"
#include "_ics_nsrv.h"
#include "_ics_event.h"
#include "_ics_port.h"
#include "_ics_region.h"
#include "_ics_admin.h"
#include "_ics_dyn.h"
#include "_ics_elf.h"
#include "_ics_stats.h"
#include "_ics_channel.h"
#include "_ics_transport.h"
#include "_ics_watchdog.h"

#define _ICS_MASTER_CPU		0			/* The CPU which acts as the ICS MASTER */

/*
 * Wait for some short period of time without accessing the bus
 *
 * Use an exponential backoff to reduce latency
 * (B) is the backoff variable which is updated each time called
 * (T) is a powerof2 limit for the maximum timeout period
 */
#define _ICS_BUSY_WAIT(B,T) do {	\
    volatile int c=0; 			\
    ICS_ASSERT(powerof2((T)));		\
    while (c<(B)) c++;			\
    (B) = (((B) << 1) | 1) & ((T)-1);	\
} while (0)

/* CPU connection state */
typedef enum ics_cpu_state
{
  _ICS_CPU_UNINITIALISED = 0,
  _ICS_CPU_MAPPED        = 1,
  _ICS_CPU_CONNECTED     = 2,
  _ICS_CPU_DISCONNECTING = 3,
  _ICS_CPU_DISCONNECTED  = 4,
  
} ics_cpu_state_t;

/* Per connected CPU state */  
typedef struct ics_cpu
{
  ics_cpu_state_t		 state;			/* CPU state */

  ICS_CHANNEL		 	 localChannel;		/* RX: Local ICS Channel handle for this cpu */

  ICS_CHANNEL		 	 remoteChannel;		/* TX: Remote ICS Channel handle for this cpu */
  ICS_CHANNEL_SEND               sendChannel;		/* TX: Send channel for this cpu */

  ICS_UINT			 txSeqNo;		/* TX: Outgoing message seqNo for this CPU */
  ICS_UINT			 rxSeqNo;		/* RX: Last incoming seqNo from this CPU */
  
  ICS_ULONG			 watchdogTs;		/* Latest watchdog timestamp */
  ICS_UINT			 watchdogFails;		/* Number of failures observed of updated timestamp */

} ics_cpu_t;

/* Primary ICS state */
typedef struct ics_state 
{
  ICS_UINT                       cpuNum;		/* The local cpu number */
  ICS_ULONG                      cpuMask;		/* Bitmask for each configured cpu */

  _ICS_OS_MUTEX                  lock;			/* Mutex kock to protect this structure */
  _ICS_OS_SPINLOCK		 spinLock;		/* IRQ/Spinlock to protect from the msg handler IRQ */

  ics_cpu_t			 cpu[_ICS_MAX_CPUS];	/* Connected cpus */
  ics_region_t			 rgn[_ICS_MAX_REGIONS];	/* Locally mapped regions */
  ics_dyn_mod_t			 dyn[_ICS_MAX_DYN_MOD]; /* Dynamically loaded modules */

  ics_port_t		       **port; 			/* Local port entries (dynamic table) */
  ICS_UINT     			 portEntries;		/* Size of current port table */

  _ICS_OS_TASK_INFO		 nsrvTask;		/* Nameserver task (only valid on master) */
  ICS_PORT			 nsrvPort;		/* Nameserver port */

  _ICS_OS_TASK_INFO		 adminTask;		/* Admin task */
  ICS_PORT			 adminPort;		/* Admin server port */

  _ICS_OS_TASK_INFO		 watchdogTask;		/* Watchdog task */
  _ICS_OS_EVENT			 watchdogEvent;		/* Watchdog task sleeps here */
  volatile ICS_ULONG		 watchdogMask;		/* Bitmask of monitored cpus */
  struct list_head		 watchdogCallback;	/* List of active watchdog callback handlers */

  ICS_WATCHDOG			 watchdog;		/* Internal watchdog handle */

  struct list_head		 events;		/* List of free event descs */
  ICS_UINT			 eventCount;		/* Number of allocated events */

  _ICS_OS_TASK_INFO		 statsTask;		/* Stats task */
  _ICS_OS_EVENT			 statsEvent;		/* Stats task sleeps here */
  struct list_head		 statsCallback;		/* List of registered stats handlers */

  ICS_STATS_HANDLE		 statsMem;		/* ICS Memory stats handle */
  ICS_STATS_HANDLE		 statsMemSys;		/* System Memory stats handle */
  ICS_STATS_HANDLE		 statsCpu;		/* CPU load stats handle */

} ics_state_t;

/* Global ICS state structure */
extern ics_state_t *ics_state;

#endif /* _ICS_SYS_H */
 
/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
