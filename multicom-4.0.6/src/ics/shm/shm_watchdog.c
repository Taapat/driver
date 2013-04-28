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


#include "_ics_shm.h"	/* SHM transport specific headers */

/*
 * Update our own watchdog timestamp in SHM
 */
void ics_shm_watchdog (ICS_ULONG timestamp)
{
  ICS_assert(ics_shm_state);
  ICS_assert(ics_shm_state->shm);

  /* Update our own watchdog timestamp in the SHM segment */
  ics_shm_state->shm->watchdogTs = timestamp;

  /* Write new value to memory */
  _ICS_OS_CACHE_PURGE(&ics_shm_state->shm->watchdogTs, 
		      ics_shm_state->paddr + offsetof(ics_shm_seg_t, watchdogTs),
		      sizeof(ics_shm_state->shm->watchdogTs));
}

/*
 * Return the last watchdog timestamp for a given cpu
 */
ICS_UINT ics_shm_watchdog_query (ICS_UINT cpuNum)
{
  ics_shm_cpu_t *cpu;

  ICS_assert(ics_shm_state);
  
  cpu = &ics_shm_state->cpu[cpuNum];

  ICS_assert(cpu->shm);

  /* About to read from SHM, so purge first */
  _ICS_OS_CACHE_PURGE(&cpu->shm->watchdogTs,
		      cpu->paddr + offsetof(ics_shm_seg_t, watchdogTs),
		      sizeof(cpu->shm->watchdogTs));

  return cpu->shm->watchdogTs;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

/* vim:ts=2:sw=2: */

