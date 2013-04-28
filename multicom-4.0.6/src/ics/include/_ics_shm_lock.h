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

#ifndef _ICS_SHM_LOCK_H
#define _ICS_SHM_LOCK_H

/*
 * Cache line sized structure to be used to store a single marker value and reserve the
 * remainder of the cache line.
 */
typedef struct ics_shm_marker_s
{
  volatile ICS_UINT marker;
  
  char _padding[_ICS_CACHELINE_PAD(1, 0)];
} ics_shm_marker_t;

/*
 * A Shared Memory spinlock
 *
 * There is a cache line for each CPU declared
 */
typedef struct ics_shm_spinlock_s
{
  ics_shm_marker_t n[_ICS_MAX_CPUS];
} ics_shm_spinlock_t;

/*
 * Acquire a spinlock - code lifted from Multicom/EMBXSHM
 *
 * This is basically an extended Decker algorithm for multiple CPUS
 * Each CPU attempts to acquire the lock by writing an integer and
 * then checks to see if anyone else wanted it. If there is conflict then
 * it drops it's request and backs off and retries.
 * Here this algorithm has been tweaked to give priority to CPUs with the
 * lower ranks
 */
_ICS_OS_INLINE_FUNC_PREFIX
void ics_shm_spinlock (ics_shm_spinlock_t *lock, ICS_OFFSET paddr, ICS_UINT cpuNum)
{
  int i;

  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(ICS_CACHELINE_ALIGNED(lock));

retry:
  /* Catch memory corruption of lock struct */
  ICS_ASSERT(lock->n[cpuNum].marker == 0);

  lock->n[cpuNum].marker = 1;
  _ICS_OS_CACHE_FLUSH(&lock->n[cpuNum].marker, 
		      paddr + offsetof(ics_shm_spinlock_t, n[cpuNum].marker),
		      sizeof(int));

  /* Purge all of the lock area */
  _ICS_OS_CACHE_PURGE(lock, paddr, sizeof(*lock));

  for (i=0; i<cpuNum; i++)
  {
	if (lock->n[i].marker)
	{
	  /* Catch memory corruption of lock struct */
	  ICS_ASSERT(lock->n[i].marker == 1);

	  /* For cpu rank < cpuNum drop req and backoff */
	  lock->n[cpuNum].marker = 0;
	  _ICS_OS_CACHE_FLUSH(&lock->n[cpuNum].marker, 
			      paddr + offsetof(ics_shm_spinlock_t, n[cpuNum].marker),
			      sizeof(int));
		
	  do {
	    ICS_UINT backoff = 1;
	    _ICS_OS_CACHE_PURGE(&lock->n[i].marker, 
				paddr + offsetof(ics_shm_spinlock_t, n[i].marker),
				sizeof(int));
	    _ICS_BUSY_WAIT(backoff, 128);
	  } while (lock->n[i].marker);
		
	  goto retry;
	}
  }

  for (i=(cpuNum+1); i<_ICS_MAX_CPUS; i++)
  {
	while (lock->n[i].marker)
	{
	  ICS_UINT backoff = 1;

	  /* Catch memory corruption of lock struct */
	  ICS_ASSERT(lock->n[i].marker == 1);

	  /* For cpu rank > cpuNum keep req asserted and backoff */
	  _ICS_OS_CACHE_PURGE(&lock->n[i].marker, 
			      paddr + offsetof(ics_shm_spinlock_t, n[i].marker),
			      sizeof(int));
	  _ICS_BUSY_WAIT(backoff, 128);
	}
  }

}

_ICS_OS_INLINE_FUNC_PREFIX
void ics_shm_spinunlock (ics_shm_spinlock_t *lock, ICS_OFFSET paddr, ICS_UINT cpuNum)
{
  ICS_ASSERT(cpuNum < _ICS_MAX_CPUS);
  ICS_ASSERT(ICS_CACHELINE_ALIGNED(lock));
  
  /* Catch memory corruption (and double unlock) of lock struct */
  ICS_ASSERT(lock->n[cpuNum].marker == 1);

  lock->n[cpuNum].marker = 0;
  _ICS_OS_CACHE_FLUSH(&lock->n[cpuNum].marker, paddr, sizeof(int));
}

#define _ICS_SHM_LOCK(L, P) 	do { ics_shm_spinlock((L),(P),ics_state->cpuNum); } while (0)
#define _ICS_SHM_UNLOCK(L, P) 	do { ics_shm_spinunlock((L),(P),ics_state->cpuNum); } while (0)

#endif /* _ICS_SHM_LOCK_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
