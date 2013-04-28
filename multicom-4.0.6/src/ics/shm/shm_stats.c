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

ICS_ERROR ics_shm_stats_update (ICS_STATS_HANDLE handle, ICS_STATS_VALUE value, ICS_STATS_TIME timestamp)
{
  ICS_UINT idx = (ICS_UINT) handle;
  ICS_STATS_ITEM *item;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);
  
  if (idx >= ICS_STATS_MAX_ITEMS)
    return ICS_HANDLE_INVALID;

  _ICS_SHM_LOCK(&ics_shm_state->shm->segLock, ics_shm_state->paddr + offsetof(ics_shm_seg_t, segLock));

  item = &ics_shm_state->shm->stats[idx];

  /* Save previous value and timestamp */
  item->value[1] = item->value[0];
  item->timestamp[1] = item->timestamp[0];
  
  /* Update stat value and timestamp */
  item->value[0]     = value;
  item->timestamp[0] = timestamp;

  /* Flush through updates to SHM memory */
  _ICS_OS_CACHE_FLUSH(item, ics_shm_state->paddr + offsetof(ics_shm_seg_t, stats[idx]), sizeof(*item));

  _ICS_SHM_UNLOCK(&ics_shm_state->shm->segLock, ics_shm_state->paddr + offsetof(ics_shm_seg_t, segLock));

  return ICS_SUCCESS;
}
  

ICS_ERROR ics_shm_stats_add (const ICS_CHAR *name, ICS_STATS_TYPE type, ICS_STATS_HANDLE *handlep)
{
  ICS_UINT idx;
  ICS_STATS_ITEM *item;

  ICS_ASSERT(ics_shm_state);
  
  if (ics_shm_state->shm == NULL)
    return ICS_NOT_INITIALIZED;

  if (name == NULL || strlen(name) == 0 || strlen(name) > ICS_STATS_MAX_NAME)
    return ICS_INVALID_ARGUMENT;

  /* Find a free stat slot */
  for (idx = 0; idx < ICS_STATS_MAX_ITEMS; idx++)
  {
    item = &ics_shm_state->shm->stats[idx];

    if (strlen(item->name) == 0)
      break;
  }
  
  if (idx >= ICS_STATS_MAX_ITEMS)
    return ICS_ENOMEM;

  /* 
   * Fill out/zero stat entry
   */
  item->value[0]     = item->value[1] = 0;
  item->timestamp[0] = item->timestamp[1] = 0;
  item->type         = type;
  strcpy(item->name, name);		/* Will also copy the '\0' */

  /* Flush through updates to SHM memory */
  _ICS_OS_CACHE_FLUSH(item, ics_shm_state->paddr + offsetof(ics_shm_seg_t, stats[idx]), sizeof(*item));

  /* Return item idx as the handle */
  *handlep = (ICS_STATS_HANDLE) idx;

  return ICS_SUCCESS;
}

ICS_ERROR ics_shm_stats_remove (ICS_STATS_HANDLE handle)
{
  ICS_UINT idx = (ICS_UINT) handle;

  ICS_STATS_ITEM *item;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);
  
  if (idx >= ICS_STATS_MAX_ITEMS)
    return ICS_HANDLE_INVALID;

  item = &ics_shm_state->shm->stats[idx];

  /* Clear stat entry */
  _ICS_OS_MEMSET(item, 0, sizeof(*item));

  /* Flush through updates */
  _ICS_OS_CACHE_FLUSH(item, ics_shm_state->paddr + offsetof(ics_shm_seg_t, stats[idx]), sizeof(*item));

  return ICS_SUCCESS;
}

ICS_ERROR ics_shm_stats_sample (ICS_UINT cpuNum, ICS_STATS_ITEM *stats, ICS_UINT *nstatsp)
{
  ics_shm_cpu_t *cpu;

  ICS_UINT idx, i;
  ICS_STATS_ITEM *item;

  ICS_ASSERT(ics_shm_state);

  if (cpuNum >= _ICS_MAX_CPUS)
    return ICS_INVALID_ARGUMENT;

  cpu = &ics_shm_state->cpu[cpuNum];

  if (cpu->shm == NULL)
    return ICS_NOT_CONNECTED;

  _ICS_SHM_LOCK(&ics_shm_state->shm->segLock, ics_shm_state->paddr + offsetof(ics_shm_seg_t, segLock));

  /* Purge from cache before read */
  _ICS_OS_CACHE_PURGE(&cpu->shm->stats[0], cpu->paddr + offsetof(ics_shm_seg_t, stats[0]), sizeof(cpu->shm->stats));

  for (i = 0, idx = 0; idx < ICS_STATS_MAX_ITEMS; idx++)
  {
    item = &cpu->shm->stats[idx];
    
    /* Only copy out valid entries */
    if (strlen(item->name))
    {
      _ICS_OS_MEMCPY(&stats[i++], item, sizeof(*item));
    }
  }

  _ICS_SHM_UNLOCK(&ics_shm_state->shm->segLock, ics_shm_state->paddr + offsetof(ics_shm_seg_t, segLock));

  /* Return number copied */
  *nstatsp = i;

  ICS_PRINTF(ICS_DBG_STATS, "cpuNum %d stats %p read nstats=%d\n", cpuNum, stats, *nstatsp);

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

/* vim:ts=2:sw=2: */

