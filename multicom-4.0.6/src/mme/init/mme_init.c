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

#include <mme.h>	/* External defines and prototypes */

#include "_mme_sys.h"	/* Internal defines and prototypes */

/* 
 * Main MME initialisation code
 */

/* Global MME state */
mme_state_t *mme_state = NULL;

/*
 * Create and start the Manager task 
 *
 * This replaces having to call MME_Run()
 */
static
MME_ERROR createManagerTask (void)
{
  ICS_ERROR err;

  char taskName[_ICS_OS_MAX_TASK_NAME];

  MME_assert(mme_state);

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, _MME_MANAGER_TASK_NAME);

  /* Create a high priority thread for servicing these messages */
  err = _ICS_OS_TASK_CREATE(mme_manager_task, NULL, _MME_MANAGER_TASK_PRIORITY, taskName, &mme_state->managerTask);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(ICS_DBG_INIT, "Failed to create Manager task %s : %s (%d)\n", taskName,
		ics_err_str(err), err);

    return MME_INTERNAL_ERROR;
  }

  (void)_ICS_OS_EVENT_INIT(&mme_state->managerExit);

  MME_PRINTF(ICS_DBG_INIT, "created Manager task %p\n", mme_state->managerTask);

  return MME_SUCCESS;
}

/* Create and Initialise the MME manager task & port */
static
MME_ERROR initManager (void)
{
  MME_ERROR res;
  ICS_ERROR err;

  char      managerName[ICS_PORT_MAX_NAME+1];
  int       n;

  MME_assert(mme_state);

  /* Create the Global Manager port for this CPU
   */
  n = sprintf(managerName, _MME_MANAGER_PORT_NAME, mme_state->cpuNum);
  MME_assert(n < ICS_PORT_MAX_NAME);

  /* Port name will be registered with the nameserver */
  err = ICS_port_alloc(managerName, NULL, NULL, _MME_MANAGER_PORT_NSLOTS, 0, &mme_state->managerPort);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT, "Failed to create Manager port '%s' : %s (%d)\n",
		managerName,
		ics_err_str(err), err);
    
    res = MME_INTERNAL_ERROR;
    goto error;
  }

  MME_assert(mme_state->managerPort != ICS_INVALID_HANDLE_VALUE);

  /* Create the Manager task */
  err = createManagerTask();
  if (err != ICS_SUCCESS)
  {
    /* Failed to create the Manager task */
    MME_EPRINTF(MME_DBG_INIT, "Failed to create Manager task : %s (%d)\n", 
		ics_err_str(err), err);
    
    res = MME_INTERNAL_ERROR;
    goto error_close;
  }

  MME_PRINTF(MME_DBG_INIT, "managerPort 0x%x\n",
	     mme_state->managerPort);

  return MME_SUCCESS;

error_close:
  ICS_port_free(mme_state->managerPort, 0);
  mme_state->managerPort = ICS_INVALID_HANDLE_VALUE;

error:
  MME_PRINTF(MME_DBG_INIT, "Failed : %s (%d)\n",
	     MME_Error_Str(res), res);

  return res;
}

/* 
 * Create a heap to act as a Buffer pool for MME_DataBuffer (and MME Meta data) allocations
 */
static
MME_ERROR initHeap (MME_UINT heapSize)
{
  ICS_ERROR err;
  
  ICS_HEAP  heap;

  ICS_REGION rgnCached, rgnUncached;
  
  MME_assert(mme_state->heap == ICS_INVALID_HANDLE_VALUE);

  MME_PRINTF(MME_DBG_INIT, "Creating ICS heap of size %d\n", heapSize);

  err = ics_heap_create(NULL, heapSize, 0, &heap);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT, "Failed to create ICS heap of size %d : %s (%d)\n",
		heapSize,
		ics_err_str(err), err);
    return MME_NOMEM;
  }
  
  MME_PRINTF(MME_DBG_INIT, "Adding cached region %p 0x%x %d\n",
	     ics_heap_base(heap, ICS_CACHED),
	     ics_heap_pbase(heap),
	     ics_heap_size(heap));

  /* Add the heap region to ICS specifying a cached mapping */
  err = ICS_region_add(ics_heap_base(heap, ICS_CACHED),
		       ics_heap_pbase(heap),
		       ics_heap_size(heap),
		       ICS_CACHED,
		       mme_state->cpuMask,
		       &rgnCached);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT,
		"Failed to add region base %p pbase 0x%x size %d cpuMask 0x%lx : %s (%d)\n",
		ics_heap_base(heap, ICS_CACHED),
		ics_heap_pbase(heap),
		ics_heap_size(heap),
		mme_state->cpuMask,
		ics_err_str(err), err);
    ics_heap_destroy(heap, 0);
    
    return MME_ICS_ERROR;
  }

#if defined (__sh__) || defined(__st200__)
  MME_PRINTF(MME_DBG_INIT, "Adding uncached region %p 0x%x %d\n",
	     ics_heap_base(heap, ICS_UNCACHED),
	     ics_heap_pbase(heap),
	     ics_heap_size(heap));

  /* Add the heap region to ICS specifying an uncached mapping */
  err = ICS_region_add(ics_heap_base(heap, ICS_UNCACHED),
		       ics_heap_pbase(heap),
		       ics_heap_size(heap),
		       ICS_UNCACHED,
		       mme_state->cpuMask,
		       &rgnUncached);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT,
		"Failed to add region base %p pbase 0x%x size %d cpuMask 0x%lx : %s (%d)\n",
		ics_heap_base(heap, ICS_UNCACHED),
		ics_heap_pbase(heap),
		ics_heap_size(heap),
		mme_state->cpuMask,
		ics_err_str(err), err);

    ICS_region_remove(rgnCached, 0);

    ics_heap_destroy(heap, 0);
    
    return MME_ICS_ERROR;
  }
#endif /* defined (__sh__) || defined(__st200__) */

  /* Stash heap info */
  mme_state->heap         = heap;
  mme_state->heapSize     = heapSize;
  mme_state->heapCached   = rgnCached;
  mme_state->heapUncached = rgnUncached;

  MME_PRINTF(MME_DBG_INIT, "Success: heap %p cached rgn 0x%x uncached rgn 0x%x\n",
	     heap, rgnCached, rgnUncached);

  return MME_SUCCESS;
}

static
void mme_watchdog_callback (ICS_WATCHDOG handle, ICS_VOID *param, ICS_UINT cpu)
{
  MME_assert(mme_state);
  MME_assert(mme_state->watchdog);
  
  MME_PRINTF(MME_DBG_INIT, "Watchdog callback param %p handle %x cpu %d\n",
	     param, (ICS_UINT)handle, cpu);
  
  /* Locate and terminate/kill all transformers associated with the failed CPU */
  mme_transformer_cpu_down(cpu);

  /* And now re-prime the watchdog */
  (void) ICS_watchdog_reprime(mme_state->watchdog, cpu);
}

static
MME_ERROR initWatchdog (void)
{
  ICS_ERROR err;

  /* Install a watchdog handler for all CPUs in the bitmask */
  err = ICS_watchdog_add(mme_watchdog_callback, NULL, mme_state->cpuMask, 0, &mme_state->watchdog);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT, "Failed to install ICS watchdog cpuMask 0x%lx : %s (%d)\n",
		mme_state->cpuMask,
		ics_err_str(err), err);

    return MME_ICS_ERROR;
  }

  return MME_SUCCESS;
}

/* mme_init()
 * Initialize MME 
 */
MME_ERROR mme_init (void)
{
  ICS_ERROR err;
  MME_ERROR res;

  ICS_UINT  cpuNum;
  ICS_ULONG cpuMask;

  ICS_ULONG initial = _ICS_OS_MEM_USED();

  /* You cannot call initialize multiple times from the same cpu */
  if (mme_state)
  {
    res = MME_DRIVER_ALREADY_INITIALIZED;
    goto error;
  }
  
  /* Query the ICS subsystem */
  err = ICS_cpu_info(&cpuNum, &cpuMask);
  if (err != ICS_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT,
		"ICS is not initialised : %s (%d)\n",
		ics_err_str(err), err);

    /* ICS must be up and running */
    res = MME_INVALID_TRANSPORT;
    goto error;
  }

  /* Allocate the local state data structure */
  _ICS_OS_ZALLOC(mme_state, sizeof(*mme_state));
  if (mme_state == NULL)
  {
    res = MME_NOMEM;
    goto error;
  }

  /* Create/initialise a multithreaded lock */
  if (!_ICS_OS_MUTEX_INIT(&mme_state->lock))
  {
    MME_EPRINTF(MME_DBG_INIT,
		"state %p MUTEX_INIT %p failed\n",
		mme_state, &mme_state->lock);

    res = MME_INTERNAL_ERROR;
    goto error_term;
  }

  /* Initialise the MME state */
  mme_state->cpuNum   = cpuNum;
  mme_state->cpuMask  = cpuMask;

  /* Initialise the registered transformer list */
  INIT_LIST_HEAD(&mme_state->regTrans);

  /* Initialise MME spinlock */
  _ICS_OS_SPINLOCK_INIT(&mme_state->mmespinLock);

  /* Initialise the MME manager task & port */
  res = initManager();
  if (res != MME_SUCCESS)
    goto error_term;

  /* Allocate the MME buffer pool */
  res = initHeap(_MME_BUF_POOL_SIZE);
  if (res != MME_SUCCESS)
    goto error_term;

  /* Initialise the MME message (meta data) cache */
  res = mme_msg_init();
  if (res != MME_SUCCESS)
    goto error_term;

  /* Install a watchdog trigger to handle CPU failures */
  res = initWatchdog();
  if (res != MME_SUCCESS)
    goto error_term;
  
  MME_PRINTF(MME_DBG, "Succeeded on cpu %d mme_state %p\n", 
	     mme_state->cpuNum, mme_state);

  MME_PRINTF(MME_DBG, "Memory usage : OS used %ld (+%ld) avail %ld\n",
	     _ICS_OS_MEM_USED(), _ICS_OS_MEM_USED() - initial,
	     _ICS_OS_MEM_AVAIL());

  return MME_SUCCESS;

error_term:
  MME_Term();

error:
  MME_EPRINTF(MME_DBG_INIT,
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  return res;
}

/* MME_Init()
 * Initialize MME 
 */
MME_ERROR MME_Init (void)
{
  MME_ERROR res;
  MME_UINT  retrycount=_MME_INIT_MAX_ATTEMPT;

  /* Attempt to initialize MME for _MME_INIT_MAX_ATTEMPT times */
  /* Each MME init attempt takes upto 30 seconds,i.e 4 attempts[4X30 = 120sec] */
  do
  {
    res = mme_init();
    if(res!= MME_SUCCESS)
    {
      retrycount--;
      MME_PRINTF(MME_DBG,"mme_init : remaing retry attepmts=%d \n",retrycount);
    }
  }while((res != MME_SUCCESS) && (retrycount));
  /* return mme init status on SUCCESS or if */
  return res;
}

/* MME_HostInit() - DEPRECATED
 * 
 */
MME_ERROR MME_HostInit (void)
{
  return MME_SUCCESS;
}

/* MME_HostTerm() - DEPRECATED
 * 
 */
MME_ERROR MME_HostTerm(void)
{
  return MME_SUCCESS;
}

/* MME_RegisterTransport() - DEPRECATED
 * 
 */
MME_ERROR MME_RegisterTransport (const char *name)
{
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  if (name == NULL)
    return MME_INVALID_ARGUMENT;

  return MME_SUCCESS;
}

/* MME_DeregisterTransport() - DEPRECATED
 * 
 */
MME_ERROR MME_DeregisterTransport (const char *name)
{
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  if (name == NULL)
    return MME_INVALID_ARGUMENT;

  return MME_SUCCESS;
}


/* MME_HostRegisterTransport() - DEPRECATED
 * 
 */
MME_ERROR MME_HostRegisterTransport (const char *name)
{
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  if (name == NULL)
    return MME_INVALID_ARGUMENT;

  return MME_SUCCESS;
}

/* MME_HostDeregisterTransport() - DEPRECATED
 * 
 */
MME_ERROR MME_HostDeregisterTransport (const char *name)
{
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  if (name == NULL)
    return MME_INVALID_ARGUMENT;

  return MME_SUCCESS;
}

/* MME_Run()
 * Run the MME message loop on a companion CPU
 *
 * DEPRECATED in MME4
 *
 * Provided here for backward compatibility only
 * Simply blocks on an event awating MME shutdown
 */
MME_ERROR MME_Run (void)
{
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;
  
  _ICS_OS_EVENT_WAIT(&mme_state->managerExit, ICS_TIMEOUT_INFINITE, ICS_TRUE);

  return MME_SUCCESS;
}


/*
 * MME4 API
 *
 * Return a pointer to the MME version string
 *
 * This string takes the form: 
 *
 *   {major number}.{minor number}.{patch number} : [text]
 *
 * That is, a major, minor and release number, separated by
 * decimal points, and optionally followed by a space and a printable text string.
 */
const char *MME_Version (void)
{
  static const ICS_CHAR * version_string = __MULTICOM_VERSION__" ("__TIME__" "__DATE__")";
  
  return version_string;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
