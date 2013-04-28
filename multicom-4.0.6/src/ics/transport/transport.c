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

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */


/* Generic transport implementation 
 *
 * Currently just call the SHM implementation directly
 * perhaps one day replace this with a per cpu lookup routing table
 * where different channel transports per cpu can be used
 */

#include "_ics_shm.h"	/* SHM transport specific headers */

ICS_ERROR ics_transport_init (ICS_UINT cpuNum, ICS_ULONG cpuMask)
{
  return ics_shm_init(cpuNum, cpuMask);
}

void ics_transport_term ()
{
  ics_shm_term();
}

ICS_ERROR ics_transport_start ()
{
  ics_shm_start();

  return ICS_SUCCESS;
}

void ics_transport_stop ()
{
  ics_shm_stop();
}

ICS_ERROR ics_transport_connect (ICS_UINT cpuNum, ICS_UINT timeout)
{
  return ics_shm_connect(cpuNum, timeout);
}

ICS_ERROR ics_transport_disconnect (ICS_UINT cpuNum, ICS_UINT flags)
{
  return ics_shm_disconnect(cpuNum, flags);
}

void ics_transport_watchdog (ICS_ULONG timestamp)
{
  ics_shm_watchdog(timestamp);
}

ICS_ULONG ics_transport_watchdog_query (ICS_UINT cpuNum)
{
  return ics_shm_watchdog_query(cpuNum);
}

void ics_transport_debug_msg (ICS_CHAR *msg, ICS_UINT len)
{
  ics_shm_debug_msg(msg, len);
}

ICS_ERROR ics_transport_debug_dump (ICS_UINT cpuNum)
{
  return ics_shm_debug_dump(cpuNum);
}

ICS_ERROR ics_transport_debug_copy (ICS_UINT cpuNum, ICS_CHAR *buf, ICS_SIZE bufSize, ICS_INT *bytesp)
{
  return ics_shm_debug_copy(cpuNum, buf, bufSize, bytesp);
}

ICS_ERROR ics_transport_stats_update (ICS_STATS_HANDLE handle, ICS_STATS_VALUE value, ICS_STATS_TIME timestamp)
{
  return ics_shm_stats_update(handle, value, timestamp);
}

ICS_ERROR ics_transport_stats_add (const ICS_CHAR *name, ICS_STATS_TYPE type, ICS_STATS_HANDLE *handlep)
{
  return ics_shm_stats_add(name, type, handlep);
}

ICS_ERROR ics_transport_stats_remove (ICS_STATS_HANDLE handle)
{
  return ics_shm_stats_remove(handle);
}

ICS_ERROR ics_transport_stats_sample (ICS_UINT cpuNum, ICS_STATS_ITEM *stats, ICS_UINT *nstatsp)
{
  return ics_shm_stats_sample(cpuNum, stats, nstatsp);
}

ICS_ERROR ics_transport_cpu_version (ICS_UINT cpuNum, ICS_ULONG *versionp)
{
  return ics_shm_cpu_version(cpuNum, versionp);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
