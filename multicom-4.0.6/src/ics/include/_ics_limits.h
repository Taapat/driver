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

#ifndef _ICS_LIMITS_SYS_H
#define _ICS_LIMITS_SYS_H

/*
 * ICS Compile time tuneables
 */

#define _ICS_MAX_CPUS			8	/* Maximum # of CPUs supported (see also _ICS_HANDLE_CPU_MASK) */
#define _ICS_MAX_CHANNELS		32	/* Maximum # of channels per CPU */

#define _ICS_MAX_MAILBOXES		4	/* Maximum mailboxes per CPU */

#define _ICS_INIT_PORTS			16	/* Initial # of local Ports per CPU */
#define _ICS_MAX_PORTS			256	/* Maximum # of local Ports per CPU */
#define _ICS_MAX_REGIONS		48	/* Maximum # of mapped regions per CPU */
#define _ICS_MAX_DYN_MOD		16	/* Maximum # of dynamic modules per CPU */

#define _ICS_CONNECT_TIMEOUT		30000	/* Inter-cpu connection timeout (in ms) */
#ifdef ICS_DEBUG
#define _ICS_COMMS_TIMEOUT		10000	/* Inter-cpu communications timeout (in ms) */
#else
#define _ICS_COMMS_TIMEOUT		1000	/* Inter-cpu communications timeout (in ms) */
#endif

#define _ICS_MSG_NSLOTS			256	/* Number of msg slots per CPU channel FIFO */

#define _ICS_NSRV_NUM_ENTRIES		128	/* Default initial name table size */

#define _ICS_WATCHDOG_INTERVAL		100	/* Watchdog timeout/timer interval in ms */
#define _ICS_WATCHDOG_FAILURES		2	/* Number of timeout intervals before failure report */

#define _ICS_MAX_PATHNAME_LEN	       	63	/* Maximum pathname length (see also ICS_MSG_INLINE_DATA) */

#define _ICS_STATS_INTERVAL		1000	/* Stats helper timeout/timer interval in ms */

#endif /* _ICS_LIMITS_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
