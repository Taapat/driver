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

ics_shm_state_t *ics_shm_state;	/* Global SHM state */


static
ICS_ERROR allocMailbox (ICS_UINT num)
{
  ICS_ERROR      err;

  ics_mailbox_t *mbox;
  ICS_OFFSET     paddr;

  ICS_PRINTF(ICS_DBG_INIT, "attach handler to mbox [%d]\n", num);

  /* Allocate a local mailbox for the channels,
   * and associate a callback with it
   */
  err = ics_mailbox_alloc(ics_shm_channel_handler, (void *) num, &mbox);
  if (err != ICS_SUCCESS)
    return err;
  
  /* Stash the mbox address into the state structure */
  ics_shm_state->mbox[num] = mbox;

#if !defined(__KERNEL__) || !defined(CONFIG_32BIT)
  {
    ICS_MEM_FLAGS mflags;

    /* Convert mbox addr to physical */
    err = _ICS_OS_VIRT2PHYS((void *)mbox, &paddr, &mflags);
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_INIT, "Failed to convert mbox %p to paddr : %d\n",
		  mbox, err);
      return err;
    }
  }
#else
  /* XXXX Why doesn't VIRT2PHYS work on 32-bit Linux ? */
  /* MULTICOM_32BIT_SUPPORT: Just use the mbox address */
  paddr = (ICS_OFFSET)mbox;
#endif

  /* Also publish the physical mbox address in the SHM segment */
  ics_shm_state->shm->mbox[num] = paddr;

  ICS_PRINTF(ICS_DBG_INIT, "published mbox[%d] 0x%x paddr 0x%x\n", num, mbox, paddr);

  return ICS_SUCCESS;
}

/*
 * Allocate and initialise the local SoC mailboxes
 */
static
ICS_ERROR initMailboxes (void)
{
  ICS_ERROR      err;

  ICS_assert(ics_shm_state);

  ICS_PRINTF(ICS_DBG_INIT, "allocate mbox [%d]\n", _ICS_MAILBOX_PADDR);

  /* Allocate the control mailbox where we publish the pAddr */
  err = allocMailbox(_ICS_MAILBOX_PADDR);
  if (err != ICS_SUCCESS)
    return err;
  
  ICS_PRINTF(ICS_DBG_INIT, "allocate mbox [%d]\n", _ICS_MAILBOX_CHN_32);

  /* Allocate a local mailbox for the channels,
   * and associate a callback with it
   */
  err = allocMailbox(_ICS_MAILBOX_CHN_32);
  if (err != ICS_SUCCESS)
    return err;

  /* XXXX Repeat above for _ICS_MAILBOX_CHN_64 etc ...*/
  ICS_assert(_ICS_MAX_CHANNELS <= 32);

  return ICS_SUCCESS;
}

static
void termMailboxes (void)
{
  int i;
  
  ICS_assert(ics_shm_state);

  /* Free any allocated mailboxes */
  for (i = 0; i < _ICS_MAX_MAILBOXES; i++)
  {
    if (ics_shm_state->mbox[i])
      ics_mailbox_free(ics_shm_state->mbox[i]);
  }

  /* Now deregister all mailboxes */
  ics_mailbox_term();
}

/*
 * Allocate and initialise the per cpu SHM control
 */
static
ICS_ERROR initShm (ICS_UINT cpuNum, ICS_ULONG cpuMask)
{
  ICS_ERROR      err = ICS_SUCCESS;

  void          *mem;
  size_t         segSize;
  ics_shm_seg_t *shm;

  ICS_OFFSET     paddr;
  ICS_MEM_FLAGS  mflags;

  ICS_PRINTF(ICS_DBG_INIT, "cpuNum %d cpuMask 0x%lx\n",
	     cpuNum, cpuMask);
  /* 
   * Calculate how much memory to create (allocate whole numbers of pages)
   */
  segSize = ALIGNUP(sizeof(ics_shm_seg_t), ICS_PAGE_SIZE);

  /* Allocate a chunk of physically contiguous memory
   * that can be mapped into other CPUs memory systems.
   */
  if ((mem = _ICS_OS_CONTIG_ALLOC(segSize, ICS_PAGE_SIZE)) == NULL)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to allocate SHM segment %d bytes\n",
		segSize);

    return ICS_ENOMEM;
  }

  ICS_assert(ICS_PAGE_ALIGNED(mem));

  ICS_PRINTF(ICS_DBG_INIT, "mem %p segSize %d\n", mem, segSize);

  /* Convert the shm segment address to a physical one */
  err = _ICS_OS_VIRT2PHYS(mem, &paddr, &mflags);
  if (err != ICS_SUCCESS)
    goto error_free;

  ICS_assert(paddr);
  ICS_assert(ICS_PAGE_ALIGNED(paddr));
  ICS_assert(!(paddr & ~_ICS_MAILBOX_SHM_PADDR_MASK));
  
  /* Clear down the SHM segment */
  _ICS_OS_MEMSET(mem, 0, segSize);

  /* Initialise the SHM segment */
  ((ics_shm_seg_t *)mem)->version = ICS_VERSION_CODE;
  ((ics_shm_seg_t *)mem)->cpuNum  = cpuNum;

  /* Flush SHM segment from the cache */
  _ICS_OS_CACHE_FLUSH(mem, paddr, segSize);

  if (!_ICS_SHM_CTRL_CACHED)
  {
    /* Uncached SHM ctrl */
    if ((shm = _ICS_OS_MMAP(paddr, segSize, _ICS_SHM_CTRL_CACHED)) == NULL)
    {
      ICS_EPRINTF(ICS_DBG_INIT, "Failed to map paddr 0x%x size %zi\n",
		  paddr, segSize);
      
      goto error_free;
    }
  }
  else
    /* Cached SHM ctrl */
    shm = mem;

  /* Publish the debugLog address via SHM */
  if (ics_shm_state->debugLog)
  {
    ics_shm_debug_log_t *debugLog = ics_shm_state->debugLog;

    ICS_PRINTF(ICS_DBG_INIT, "debugLog %p -> 0x%x size %d\n",
	       debugLog, debugLog->paddr, debugLog->size);

    ICS_assert(debugLog->size);

    /* Export the debugLog address/size to the SHM segment */
    shm->debugLog     = debugLog->paddr;
    shm->debugLogSize = debugLog->size;

    /* Flush any cache entries */
    _ICS_OS_CACHE_FLUSH(debugLog, debugLog->paddr, debugLog->size);
  }

  /* Flush SHM segment from cache */
  _ICS_OS_CACHE_FLUSH(shm, paddr, ics_shm_state->shmSize);

  /* Stash all the local state info */
  ics_shm_state->cpuNum    = cpuNum;
  ics_shm_state->cpuMask   = cpuMask;
  ics_shm_state->mem       = mem;
  ics_shm_state->shm       = shm;
  ics_shm_state->shmSize   = segSize;
  ics_shm_state->paddr     = paddr;

  return ICS_SUCCESS;

error_free:
  _ICS_OS_CONTIG_FREE(mem);

  return err;
}

static
void destroy_shm (void)
{
  ICS_assert(ics_shm_state->mem);

  /* Unmap as necessary */
  if (ics_shm_state->shm != ics_shm_state->mem)
    _ICS_OS_MUNMAP(ics_shm_state->shm);

  /* Free off the SHM contiguous memory */
  _ICS_OS_CONTIG_FREE(ics_shm_state->mem);
}

void ics_shm_term (void)
{
  if (!ics_shm_state)
    return;

  ICS_PRINTF(ICS_DBG_INIT, "Shutting down: shm_state %p\n", ics_shm_state);

  if (ics_shm_state->mbox)
    termMailboxes();

  if (ics_shm_state->shm)
    destroy_shm();

  if (ics_shm_state->debugLog)
    ics_shm_debug_term();

  _ICS_OS_FREE(ics_shm_state);
  ics_shm_state = NULL;

  return;
}

/*
 * Main initialisation of the ICS SHM system
 *
 * Allocates the primary control state, SHM segments
 * and mailboxes etc.
 * But does not publish the SHM address into the mailbox
 * (which is instead done by ICS_start)
 * so as to allow further initialisation to be done before
 * other CPUs can start injecting messages.
 */
ICS_ERROR ics_shm_init (ICS_UINT cpuNum, ICS_ULONG cpuMask)
{
  ICS_ERROR err;
  
  /* You cannot call initialize multiple times on the same CPU */
  if (ics_shm_state)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "cpuNum %d is already initialised\n", cpuNum);

    return ICS_ALREADY_INITIALIZED;
  }

  /* Allocate the local state data structure */
  _ICS_OS_ZALLOC(ics_shm_state, sizeof(ics_shm_state_t));
  if (ics_shm_state == NULL)
  {
    return ICS_ENOMEM;
  }
 
  /* Get the debug logging system going as soon as possible */
  err = ics_shm_debug_init();
  if (err != ICS_SUCCESS)
    goto error_free;

  /* Debug log now available */
  ICS_PRINTF(ICS_DBG_INIT, "cpuNum %d cpuMask 0x%lx\n",
	     cpuNum, cpuMask);

  /* Create/initialise a multithreaded lock */
  if (!_ICS_OS_MUTEX_INIT(&ics_shm_state->shmLock))
  {
    err = ICS_SYSTEM_ERROR;

    goto error_term;
  }

  /* Initialise the IRQ/SMP lock */
  _ICS_OS_SPINLOCK_INIT(&ics_shm_state->shmSpinLock);

  /* Alloc/Initialise the SHM cpu components */
  err = initShm(cpuNum, cpuMask);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "initShm failed : %d\n", err);

    goto error_term;
  }

  /* Initialise the hw mailbox system */
  err = ics_mailbox_init();
  if (err != ICS_SUCCESS && err != ICS_ALREADY_INITIALIZED)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "mailbox init failed : %d\n", err);

    goto error_term;
  }

  /* Alloc/Initialise the SoC maiboxes */
  if ((err = initMailboxes()) != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "initMailboxes failed : %d\n", err);
    
    goto error_term;
  }

  ICS_PRINTF(ICS_DBG_INIT, "shm_state %p shm %p\n", 
	     ics_shm_state, ics_shm_state->shm);

  return ICS_SUCCESS;

error_term:
  ics_shm_term();
  
error_free:
  _ICS_OS_FREE(ics_shm_state);
  ics_shm_state = NULL;

  ICS_EPRINTF(ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

/*
 * Make this cpu visible to all others by publishing its
 * SHM segment address in the mailbox
 * This will allow remote cpus to send messages to our
 * FIFOs and generate interrrupts
 */
ICS_ERROR ics_shm_start (void)
{
  ICS_UINT ver = 0;
  ICS_UINT value;
  
  if (!ics_shm_state || !ics_shm_state->shm)
    return ICS_NOT_INITIALIZED;

  /* Read current Mailbox value */
  value = ics_mailbox_enable_get(ics_shm_state->mbox[_ICS_MAILBOX_PADDR]);

  /* Check for a CPU reboot case */
  if (_ICS_MAILBOX_SHM_TOKEN(value) == _ICS_MAILBOX_SHM_TOKEN_VALUE &&
      _ICS_MAILBOX_SHM_CPU(value)   == ics_shm_state->cpuNum)
  {
    /* Read and increment version */
    ver = _ICS_MAILBOX_SHM_VER(value)+1;
  }
  ICS_PRINTF(ICS_DBG_INIT, "value %x paddr %x token %d ver %d cpu %d\n",
	     value,
	     _ICS_MAILBOX_SHM_PADDR(value),
	     _ICS_MAILBOX_SHM_TOKEN(value),
	     _ICS_MAILBOX_SHM_VER(value),
	     _ICS_MAILBOX_SHM_CPU(value));

  /* Encode the <paddr,token,ver,cpu> into Mailbox value */
  value = _ICS_MAILBOX_SHM_HANDLE(ics_shm_state->paddr, ver, ics_shm_state->cpuNum);

  ICS_PRINTF(ICS_DBG_INIT, "Publish paddr %x ver %d cpu %d value 0x%x to mbox %p\n",
	     ics_shm_state->paddr, ver, ics_shm_state->cpuNum,
	     value, ics_shm_state->mbox[_ICS_MAILBOX_PADDR]);

  /* Publish <paddr,ver,cpu> in one of our local mailboxes  */
  ics_mailbox_enable_set(ics_shm_state->mbox[_ICS_MAILBOX_PADDR], value);

  return ICS_SUCCESS;
}

void ics_shm_stop (void)
{
  ics_mailbox_t value;
  
  ICS_assert(ics_shm_state);
  ICS_assert(ics_shm_state->mbox);

  /* Read current Mailbox value */
  value = ics_mailbox_enable_get(ics_shm_state->mbox[_ICS_MAILBOX_PADDR]);
  
  /* Clear our local mailbox, but keep <ver> for reboot  */
  ics_mailbox_enable_set(ics_shm_state->mbox[_ICS_MAILBOX_PADDR],
			 value & (_ICS_MAILBOX_SHM_VER_MASK << _ICS_MAILBOX_SHM_VER_SHIFT));

  /* XXXX Also Disable all channel mbox interrupts ??? */

  return;
}

ICS_ERROR ics_shm_cpu_version (ICS_UINT cpuNum, ICS_ULONG *versionp)
{
  ics_shm_cpu_t *cpu;

  if (!ics_shm_state || !ics_shm_state->shm)
    return ICS_NOT_INITIALIZED;
 
  if (cpuNum >= _ICS_MAX_CPUS)
    return ICS_INVALID_ARGUMENT;
    
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->shm == NULL)
    return ICS_NOT_CONNECTED;

  *versionp = cpu->shm->version;

  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
