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

static
ICS_ERROR mapDebugLog (ics_shm_seg_t *shm, ics_shm_debug_log_t **debugLogp)
{
  ics_shm_debug_log_t *debugLog;
  ICS_SIZE             debugLogSize;
  ICS_OFFSET           paddr;
  
  /* Get the base paddr and size of the debugLog from the SHM segment */
  paddr        = shm->debugLog;
  debugLogSize = shm->debugLogSize;
  
  ICS_assert(paddr);
  ICS_assert(debugLogSize);

  /* Map in the debug log */
  if ((debugLog = _ICS_OS_MMAP(paddr, debugLogSize, ICS_TRUE)) == NULL)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to map paddr 0x%x size %zi\n",
		paddr, debugLogSize);

    return ICS_ENOMEM;
  }

  ICS_PRINTF(ICS_DBG_INIT, "mapped paddr 0x%x size %d @ %p\n",
	     paddr, debugLogSize, debugLog);
  
  /* Make sure we have no stale cache entries for this segment */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, debugLogSize);

  /* Return mapped debugLog segment to caller */
  *debugLogp = debugLog;

  return ICS_SUCCESS;
}

static
ICS_ERROR unmapDebugLog (ics_shm_debug_log_t *debugLog, ICS_SIZE debugLogSize)
{
  /* PURGE cache before unmapping */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, debugLogSize);
  
  return (_ICS_OS_MUNMAP(debugLog) ? ICS_SYSTEM_ERROR : ICS_SUCCESS);
}

static
ICS_ERROR mapShmSeg (ICS_OFFSET paddr, ics_shm_seg_t **shmp, ICS_SIZE *segSizep)
{
  ics_shm_seg_t *shm;
  ICS_SIZE       segSize;
  
  /* Map in all of the shared memory region */
  segSize = ALIGNUP(sizeof(ics_shm_seg_t), ICS_PAGE_SIZE);
  if ((shm = _ICS_OS_MMAP(paddr, segSize, _ICS_SHM_CTRL_CACHED)) == NULL)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to map paddr 0x%x size %zi\n",
		paddr, segSize);
    
    _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);

    return ICS_ENOMEM;
  }

  ICS_PRINTF(ICS_DBG_INIT, "mapped paddr 0x%x size %d @ %p\n",
	     paddr, segSize, shm);
  
  /* Make sure we have no stale cache entries for this segment */
  if (_ICS_SHM_CTRL_CACHED)
    _ICS_OS_CACHE_PURGE(shm, paddr, segSize);

  /* Return segment and size to caller */
  *shmp = shm;
  *segSizep = segSize;

  return ICS_SUCCESS;
}

static
ICS_ERROR unmapShmSeg (ics_shm_seg_t *shm, ICS_OFFSET paddr)
{
  ICS_SIZE segSize = ALIGNUP(sizeof(ics_shm_seg_t), ICS_PAGE_SIZE);

  /* PURGE cache before unmapping */
  if (_ICS_SHM_CTRL_CACHED)
    _ICS_OS_CACHE_PURGE(shm, paddr, segSize);

  return (_ICS_OS_MUNMAP(shm) ? ICS_SYSTEM_ERROR : ICS_SUCCESS);
}

static
ICS_ERROR unmapMboxes (ics_shm_cpu_t *cpu)
{
  int i;
  
  for (i = 0; i < _ICS_MAX_MAILBOXES; i++)
  {
    /* Unmap any remote mailbox mappings */
    if (cpu->rmbox[i])
      _ICS_OS_MUNMAP((void *)cpu->rmbox[i]);
  }

  return ICS_SUCCESS;
}

static
ICS_ERROR findShmSeg (ICS_UINT cpuNum, ICS_UINT version, ICS_UINT timeout, ICS_UINT *versionp, ICS_OFFSET *paddrp)
{
  ics_mailbox_t *rmbox = NULL;
  ICS_OFFSET     paddr = ICS_BAD_OFFSET;

  ICS_UINT       value;

  _ICS_OS_TIME   start;

  /*
   * Loop scanning the remote mailboxes looking for the SHM segment of the
   * matching CPU.
   */
  start = _ICS_OS_TIME_NOW();
  do
  {
    ICS_UINT value, mask;
    
    /* Find the remote mailbox for the target cpu
     *
     * We look for a matching mbox which has the correct cpu number
     * and the special SHM token to prevent false matches against
     * the other mboxes which may just have channel enable bits set
     */
    value = _ICS_MAILBOX_SHM_HANDLE(0, 0 /* ignore version */, cpuNum);

    /* Mask out version number (and paddr) */
    mask = ~((_ICS_MAILBOX_SHM_VER_MASK << _ICS_MAILBOX_SHM_VER_SHIFT) | _ICS_MAILBOX_SHM_PADDR_MASK);

    if (ics_mailbox_find(value	/* token */,
			 mask	/* mask */, 
			 &rmbox) == ICS_SUCCESS)
    {
      ICS_UINT ver;
      
      /* Read the Mailbox enable register */
      value = ics_mailbox_enable_get(rmbox);
      
      /* Decode the SHM segment version number */
      ver   = _ICS_MAILBOX_SHM_VER(value);
      
      ICS_PRINTF(ICS_DBG_INIT, "Found rmbox %p enable=0x%x ver=%d\n",
		 rmbox, value, ver);

      /* If initial connect or matching SHM segment version then done */
      if (version == 0 || ver == version)
	break;

      /* Found correct cpuNum, but wrong version. Return an error rather than
       * spin in here waiting for it to reboot
       */
      ICS_EPRINTF(ICS_DBG_INIT, "cpu %d Found rmbox %p enable=0x%x ver=%d wanted version %d\n",
		    cpuNum, rmbox, value, ver, version);
      return ICS_NOT_INITIALIZED;
    }

    /* Cannot find the remote mailbox, loop until we do or timeout */
    _ICS_OS_TASK_DELAY(1);
    
  } while (!_ICS_OS_TIME_EXPIRED(start, timeout));
  
  if (rmbox == NULL)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "cpuNum %d version %d timeout %d: Failed to find remote mbox\n",
		cpuNum, version, timeout);

    /* Dump out all the mailboxes */
    ics_mailbox_dump();

    return ICS_SYSTEM_TIMEOUT;
  }

  /* Read the Mailbox enable register */
  value = ics_mailbox_enable_get(rmbox);

  ICS_PRINTF(ICS_DBG_INIT, "Found rmbox %p enable=0x%x\n",
	     rmbox, value);
  
  /* Extract the shared memory address from the mailbox 
   * Masking out any lower bits which we use for control info
   */
  paddr = _ICS_MAILBOX_SHM_PADDR(value);
  
  /* Don't allow page 0 as the paddr */
  ICS_assert(paddr);

  /* Return SHM segment paddr to caller */
  *paddrp = paddr;

  /* Return version to caller */
  *versionp = _ICS_MAILBOX_SHM_VER(value);

  ICS_PRINTF(ICS_DBG_INIT, "rmbox %p enable=0x%x paddr 0x%x version %d\n",
	     rmbox, value, paddr, *versionp);

  return ICS_SUCCESS;
}

/*
 * Connect to another CPU by scanning the remote mailboxes
 * looking for the correct cpu id. Once found the rmbox will also
 * have the physical address of its shared memory region.
 *
 * We map this segment into our virtual address space and then 
 * extract further information from within it
 *
 * MULTITHREAD SAFE: Uses the shm_state lock
 */
ICS_ERROR ics_shm_connect (ICS_UINT cpuNum, ICS_UINT timeout)
{
  ICS_ERROR        err = ICS_SUCCESS;
  ics_shm_cpu_t   *cpu;
  
  ics_shm_seg_t   *shm;
  ICS_OFFSET       paddr;
  ICS_SIZE         segSize;
  ics_shm_debug_log_t *debugLog;

  ICS_assert(ics_shm_state);

  ICS_PRINTF(ICS_DBG_INIT, "cpuNum %d timeout %d\n",
	     cpuNum, timeout);

  /* Take state lock to protect cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
  
  cpu = &ics_shm_state->cpu[cpuNum];
  
  if (cpu->shm != NULL)
  {
    _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);

    /* Already connected */
    return ICS_SUCCESS;
  }

  if (ics_shm_state->cpuNum == cpuNum)
  {
    int i;

    /* Loopback: Connecting back to ourselves, just grab our local state info */
    cpu->shm     = ics_shm_state->shm;
    cpu->shmSize = ics_shm_state->shmSize;
    cpu->paddr   = ics_shm_state->paddr;
    for (i = 0; i < _ICS_MAX_MAILBOXES; i++)
      cpu->rmbox[i]  = ics_shm_state->mbox[i];
    cpu->debugLog = ics_shm_state->debugLog;
  }
  else
  {
    int i;
    
    ICS_UINT version;

    /* Find the corresponding remote SHM segment paddr */
    if ((err = findShmSeg(cpuNum, cpu->version, timeout, &version, &paddr)) != ICS_SUCCESS)
      goto error_release;

    ICS_assert(paddr);

    /* Map in the SHM segment for this CPU */
    if ((err = mapShmSeg(paddr, &shm, &segSize)) != ICS_SUCCESS)
      goto error_release;
    
    ICS_assert(shm);
    ICS_assert(segSize);

    /* Map in the CPU's debug log */
    if ((err = mapDebugLog(shm, &debugLog)) != ICS_SUCCESS)
      goto error_unmap;

    ICS_assert(debugLog);

    /* Initialise the cpu descriptor */
    cpu->shm      = shm;
    cpu->shmSize  = segSize;
    cpu->paddr    = paddr;
    cpu->debugLog = debugLog;

    for (i = 0; i < _ICS_MAX_MAILBOXES; i++)
    {
      if (shm->mbox[i])
      {
	/* Need to map these physical mailbox addresses into our address space (uncached) */
	/* Normally this will match a fixed identity mapping that is already in place and
	 * so won't actually create a new vmem mapping, but it is necessary for sh4 29-bit mode
	 */
	cpu->rmbox[i] = (ics_mailbox_t *) _ICS_OS_MMAP(shm->mbox[i], _ICS_OS_PAGESIZE, ICS_FALSE);
	ICS_ASSERT(cpu->rmbox[i]);
      }
      else
	cpu->rmbox[i] = NULL;
    }
    
    /* Stash the version number we found in the Mailbox register */
    cpu->version = version;
  }
    
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
  
  ICS_PRINTF(ICS_DBG_INIT,
	     "Connected to cpu %d rmbox %p shm %p version %d debugLog %p logSize %d\n",
	     cpuNum, cpu->rmbox[_ICS_MAILBOX_PADDR], cpu->shm, cpu->version, cpu->debugLog, cpu->debugLog->size);

  return ICS_SUCCESS;

error_unmap:
  _ICS_OS_MUNMAP(shm);

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
  
  ICS_EPRINTF(ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Disconnect from a CPU
 * Unmap and clear down any state for the target CPU
 *
 * MULTITHREAD SAFE: Uses the shm_state lock and also
 * the SPINLOCK to protect against ISR context callers of ics_channel_send
 *
 */
ICS_ERROR ics_shm_disconnect (ICS_UINT cpuNum, ICS_UINT flags)
{
  ICS_ERROR        err = ICS_SUCCESS;
  ics_shm_cpu_t   *cpu;

  ics_shm_seg_t   *shm;

  unsigned long    iflags;

  ICS_assert(ics_shm_state);

  ICS_PRINTF(ICS_DBG_INIT, "cpuNum %d\n",
	     cpuNum);

  /* Take state lock to protect cpu table */
  _ICS_OS_MUTEX_TAKE(&ics_shm_state->shmLock);
  
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->shm == NULL)
  {
    _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
    
    /* Already disconnected */
    return ICS_SUCCESS;
  }

  /* Protect against any channel callback handlers (calling ics_channel_send) */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->shmSpinLock, iflags);

  /* Save this for below */
  shm = cpu->shm;

  /* Clear this whilst holding the spinlock */
  cpu->shm = NULL;

  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->shmSpinLock, iflags);
  
  /* Are we disconnecting from a remote cpu ? */
  if (ics_shm_state->cpuNum != cpuNum)
  {
    /* Unmap the debugLog memory */
    if ((err = unmapDebugLog(cpu->debugLog, shm->debugLogSize)) != ICS_SUCCESS)
      goto error_release;
    
    /* Unmap the SHM segment */
    if ((err = unmapShmSeg(shm, cpu->paddr)) != ICS_SUCCESS)
      goto error_release;

    /* Unmap the remote mboxes */
    if ((err = unmapMboxes(cpu)) != ICS_SUCCESS)
      goto error_release;
  }

  /* Clear rest of CPU state */
  cpu->shmSize = 0;
  cpu->debugLog = NULL;
  _ICS_OS_MEMSET(cpu->rmbox, 0, sizeof(cpu->rmbox));
  
  /* Bump version (and wrap) ready for reboot */
  if (flags & ICS_CPU_DEAD)
    cpu->version = (++cpu->version & _ICS_MAILBOX_SHM_VER_MASK);

  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
  
  return ICS_SUCCESS;

error_release:
  _ICS_OS_MUTEX_RELEASE(&ics_shm_state->shmLock);
  
  ICS_EPRINTF(ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

