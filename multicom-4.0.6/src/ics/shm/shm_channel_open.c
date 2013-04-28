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
 * Map a sender channel onto a target cpu and channel handle
 *
 * This maps in the FIFO memory (paddr) of the remote/target CPU FIFO 
 * so that subsequent sends can write directly into it
 *
 */
ICS_ERROR ics_shm_channel_open (ICS_HANDLE handle, ics_shm_channel_send_t **schannelp)
{
  ICS_ERROR               err;

  ics_shm_channel_send_t *schannel;
  ICS_SIZE                chnSize;

  ics_shm_cpu_t          *cpu;
  ics_chn_t              *chn;
  void                   *fifo;

  int type, cpuNum, ver, idx;
#ifdef CONFIG_SMP
  unsigned long  iflags;
#endif

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);
  ICS_ASSERT(schannelp);

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT, "handle 0x%x schannelp %p\n", handle, schannelp);

  /* Decode the channel handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);
  
  /* Check the target channel handle */
  if (type != _ICS_TYPE_CHANNEL || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_CHANNELS)
  {
    return ICS_HANDLE_INVALID;
  }

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT, "cpuNum %d idx %x version %d\n", cpuNum, idx, ver);

  /* Check that the target cpu is mapped in (NB: not holding state lock) */
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->shm == NULL)
    /* Connect to CPU if it is currently not mapped in */
    ics_shm_connect(cpuNum, _ICS_CONNECT_TIMEOUT);

#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
#else
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif

  /* Check that the target cpu is mapped (now holding state lock) */
  if (cpu->shm == NULL)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "cpuNum %d not mapped\n", cpuNum);

    /* Target cpu is not connected */
    err = ICS_NOT_CONNECTED;
    goto error_release;
  }
  
  /* Get a handle onto the target cpu channel */
  chn = &cpu->shm->chn[idx];

  /* Invalidate cacheline to refresh info */
  /* Have to use PURGE as this could be a loopback send */
  _ICS_OS_CACHE_PURGE(chn, chn->paddr, sizeof(*chn));

  if (chn->base == 0)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "cpuNum %d chn %p idx %x not initialised\n", cpuNum, chn, idx);
    
    /* Channel is not initialised */
    err = ICS_NOT_INITIALIZED;
    goto error_release;
  }

  if ((chn->version & _ICS_HANDLE_VER_MASK) != ver)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "chn %p idx %x ver %d != handle 0x%x ver %d\n",
		chn, idx, (chn->version & _ICS_HANDLE_VER_MASK), handle, ver);
    
    /* Channel version mismatch */
    err = ICS_INVALID_ARGUMENT;
    goto error_release;
  }

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT,
	     "chn %p[%d] version %d paddr %lx ns %d ss %d owner %p fptr %x bptr %x\n",
	     chn, idx, chn->version, chn->paddr, chn->ns, chn->ss,
	     chn->fifo.owner, chn->fifo.fptr, chn->fifo.bptr);
  
  /* These should all have been (re)initialized in the consumer 
   * XXXX But in the case of a companion reboot, this may not be true
   */
  ICS_assert(chn->ns >= 2);
  ICS_assert(chn->ss);
  ICS_assert(chn->fifo.fptr == chn->fifo.bptr);	/* XXXX Should be empty, but maybe not 0 */
  /* ICS_assert(chn->fifo.owner == NULL); */

  /* Calculate size of channel FIFO (page aligned and multiples of whole pages) */
  chnSize = chn->ns * chn->ss;
  chnSize = ALIGNUP(chnSize, ICS_PAGE_SIZE);

#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Map the remote FIFO memory (paddr) into our address space */
  fifo = _ICS_OS_MMAP(chn->base, chnSize, _ICS_SHM_FIFO_CACHED);
#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  if (fifo == NULL)
  {
    ICS_EPRINTF(ICS_DBG_CHN,
		"Failed to map FIFO cpu %d idx %x base 0x%x size %d\n",
		cpuNum, idx, chn->base, chnSize);
    
    err = ICS_SYSTEM_ERROR;
    goto error_release;
  }

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT, "mapped fifo %p base 0x%x ns %d ss %d size %d\n",
	     fifo, chn->base, chn->ns, chn->ss, chnSize);

  /* Destroy any old cache mappings */
  /* Have to use PURGE as this could be a loopback send 
   * and hence valid unwritten data could still be in the cache
   */
  if (_ICS_SHM_FIFO_CACHED)
    _ICS_OS_CACHE_PURGE(fifo, chn->base, chnSize);


#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  /* Allocate a Send channel desc */
  _ICS_OS_ZALLOC(schannel, sizeof(*schannel));
#ifdef CONFIG_SMP
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);
#endif
  if (schannel == NULL)
  {
    err = ICS_ENOMEM;
    goto error_unmap;
  }

  /* Fill out desc */
  schannel->cpuNum   = cpuNum;
  schannel->idx      = idx;
  schannel->handle   = handle;
  schannel->fifo     = fifo;
  schannel->fifoSize = chnSize;

  /* Flag that the SHM chn desc is in use */
  chn->fifo.owner = schannel;
  /* Flush out change */
  _ICS_OS_CACHE_FLUSH(&chn->fifo.owner, chn->paddr + offsetof(ics_chn_t, fifo.owner), sizeof(chn->fifo.owner));

  /* Return schannel handle to caller */
  *schannelp = schannel;

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT, "schannel %p base 0x%x fifo %p\n",
	     schannel, chn->base, schannel->fifo);

#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#else
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif
  return ICS_SUCCESS;

error_unmap:
  _ICS_OS_MUNMAP(fifo);

error_release:
#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#else
  /* aquire spinlock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
#endif

  ICS_EPRINTF(ICS_DBG_CHN, " Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Free a sender channel
 *
 */
ICS_ERROR ics_shm_channel_close (ics_shm_channel_send_t *schannel)
{
  ICS_ERROR               err;

  ics_shm_cpu_t          *cpu;
  ics_chn_t              *chn;

  void                   *fifo;
  unsigned long           iflags;

  ICS_ASSERT(ics_shm_state);
  ICS_ASSERT(ics_shm_state->shm);

  /* Check this send channel is valid (may have been freed back to malloc heap) */
  if (schannel == NULL || schannel->fifo == NULL || !ICS_PAGE_ALIGNED(schannel->fifoSize))
    return ICS_INVALID_ARGUMENT;

  ICS_PRINTF(ICS_DBG_CHN|ICS_DBG_INIT, "schannel %p fifo %p handle 0x%x\n",
	     schannel, schannel->fifo, schannel->handle);
  
#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
#endif
  /* Get a handle onto the target cpu desc */
  cpu = &ics_shm_state->cpu[schannel->cpuNum];
  if (cpu->shm == NULL)
  {
    ICS_EPRINTF(ICS_DBG_CHN, "cpuNum %d not mapped in\n", schannel->cpuNum);

    /* Target cpu is no longer connected */
    err = ICS_NOT_CONNECTED;
    goto error_release;
  }

  /* Protect against any channel handler callbacks (calling ics_channel_send) */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);

  /* Get a handle onto the target cpu channel */
  chn = &cpu->shm->chn[schannel->idx];


  /* Check that we own it! */
  ICS_assert(chn->fifo.owner == schannel);

  /* Clear SHM owner field */
  chn->fifo.owner = NULL;

  /* Update SHM structure */
  _ICS_OS_CACHE_PURGE(chn, chn->paddr, sizeof(*chn));

  /* Save this for later */
  fifo = schannel->fifo;

  /* Clear down the desc */
  _ICS_OS_MEMSET(schannel, 0, sizeof(*schannel));

  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);

  /* Unmap the channel FIFO */
  (void) _ICS_OS_MUNMAP(fifo);
  
  /* Free off the channel desc memory */
  _ICS_OS_FREE(schannel);

#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#endif

  return ICS_SUCCESS;

error_release:
#ifndef CONFIG_SMP
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
#endif

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

