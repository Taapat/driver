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

/* 
 * Main ICS initialisation code
 */

/* Global static ICS state */
ics_state_t *ics_state = NULL;


/*
 * Release all the per CPU state and channels etc
 */
static
void termCpus (void)
{
  int i;

  ICS_assert(ics_state);

  ICS_PRINTF(ICS_DBG_INIT, "Shutting down cpu %d\n", ics_state->cpuNum);

  for (i = 0; i < _ICS_MAX_CPUS; i++)
  {
    ics_cpu_t *cpu;
    
    cpu = &ics_state->cpu[i];

    /* Free off any allocated channels */
    if (cpu->localChannel != ICS_INVALID_HANDLE_VALUE)
      ICS_channel_free(cpu->localChannel, 0);
  }

}

/*
 * Allocate and initialise the per cpu data structures
 */
static
ICS_ERROR initCpus (void)
{
  ICS_ERROR err = ICS_SUCCESS;
  int       i;
  ICS_ULONG mask;

  ICS_assert(ics_state);

  /* We allocate a local message channel for each CPU that 
   * may send messages to us (including ourselves)
   *
   * NB: We always allocate a channel for each CPU even if not
   * present so that the channel id matches the CPU number
   */
  for (i = 0, mask = ics_state->cpuMask; mask; i++, mask >>= 1)
  {
    ics_cpu_t *cpu;
    
    cpu = &ics_state->cpu[i];

    /* Set an invalid handle in case of error */
    cpu->localChannel  = ICS_INVALID_HANDLE_VALUE;
    cpu->remoteChannel = ICS_INVALID_HANDLE_VALUE;

    /* Create a Channel of _ICS_MSG_NSLOTS each of _ICS_MSG_SSIZE */
    err = ICS_channel_alloc(ics_message_handler, cpu, 
			    NULL /* FIFO mem */,
			    _ICS_MSG_NSLOTS, _ICS_MSG_SSIZE,
			    0 /* flags */,
			    &cpu->localChannel);

    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_INIT,
		  "[%d] ICS_channel_alloc NSLOTS %d SSIZE %d failed\n",
		  i, _ICS_MSG_NSLOTS, _ICS_MSG_SSIZE);
      goto failure;
    }

    ICS_PRINTF(ICS_DBG_INIT, "[%d] NSLOTS %d SSIZE %d localChannel 0x%x\n",
	       i, _ICS_MSG_NSLOTS, _ICS_MSG_SSIZE, cpu->localChannel);
  }

  return ICS_SUCCESS;

failure:
  /* Tidy up */
  termCpus();

  ICS_EPRINTF(ICS_DBG_INIT, "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}


/*
 * Disconnect from all cpus in the bitmask
 *
 */
static
void disconnectCpus (void)
{
  ICS_ULONG mask;
  int       i;

  ICS_PRINTF(ICS_DBG_INIT, "cpuMask 0x%lx\n", ics_state->cpuMask);

  for (i = 0, mask = ics_state->cpuMask; mask; i++, mask >>= 1)
  {
    if (mask & 1)
    {
      ics_cpu_t *cpu;

      cpu = &ics_state->cpu[i];
      
      /* Disconnect any connected CPUs */
      if (cpu->state == _ICS_CPU_CONNECTED || cpu->state == _ICS_CPU_MAPPED)
      {
	ICS_PRINTF(ICS_DBG_INIT, "disconnecting cpu %d\n", i);
	ICS_cpu_disconnect(i, 0);
      }
    }
  }
}

/*
 * Make a connection to all CPUs in the bitmask
 *
 */
static
ICS_ERROR connectCpus (void)
{
  ICS_ERROR err = ICS_SUCCESS;

  ICS_ULONG mask;
  int       i;

  /* SLAVEs start their transports first so the MASTER can
   * connect to them all before they start sending it
   * requests
   */
  if (ics_state->cpuNum != _ICS_MASTER_CPU)
  {
    /* SLAVE: Now start the ICS transport allowing other CPUs to contact us */
    err = ics_transport_start();
    if (err != ICS_SUCCESS)
    {
      goto error;
    }
  }

  ICS_PRINTF(ICS_DBG_INIT, "cpuMask 0x%lx\n", ics_state->cpuMask); 
 
  /* Connect to CPUs in reverse order so that we connect
   * to the MASTER last. This ensures we are fully connected
   * to all peers before that completes
   */
  for (i = _ICS_MAX_CPUS-1, mask = (1UL << i); mask; i--, mask >>= 1)
  {
    if (ics_state->cpuMask & mask)
    {
      ics_cpu_t *cpu;

      cpu = &ics_state->cpu[i];

      /* Connect to CPU via ICS
       *
       * This connects to the remote cpu and allocates a channel for
       * sending messages to it
       */
      if ((err = ICS_cpu_connect(i, 0, _ICS_CONNECT_TIMEOUT)) != ICS_SUCCESS)
      {
	/* Failed to connect for some reason */
	goto error_disconnect;
      }
    }
  }

  /* SLAVEs should now all be connected to each other
   * and blocked waiting for the MASTER to start
   */
  if (ics_state->cpuNum == _ICS_MASTER_CPU)
  {
    /* MASTER: Now start the ICS transport allowing other CPUs to contact us */
    err = ics_transport_start();
    if (err != ICS_SUCCESS)
    {
      goto error_disconnect;
    }
  }

  return ICS_SUCCESS;

error_disconnect:
  /* Tidy up */
  for (i = 0, mask = ics_state->cpuMask; mask; i++, mask >>= 1)
  {
    if (mask & 1)
    {
      ics_cpu_t *cpu = &ics_state->cpu[i];
      
      /* Disconnect any connected CPUs */
      if (cpu->state == _ICS_CPU_CONNECTED || cpu->state == _ICS_CPU_MAPPED)
	ICS_cpu_disconnect(i, 0);
    }
  }

error:
  ICS_EPRINTF(ICS_DBG_INIT,
	      "Failed : %s (%d)\n",
	      ics_err_str(err), err);
  return err;
}


void ICS_cpu_term (ICS_UINT flags)
{
  if (ics_state == NULL)
    return;

  ICS_PRINTF(ICS_DBG_INIT, "Shutting down: state %p cpu %d flags 0x%x\n", 
	     ics_state, ics_state->cpuNum, flags);

  /* Stops any new connections */
  ics_transport_stop();

  ics_admin_term();

  if (ics_state->watchdog)
    ICS_watchdog_remove(ics_state->watchdog);

  ics_watchdog_term();

#ifdef ICS_DEBUG_MEM
  ICS_stats_remove(ics_state->statsMem);
#endif
  ICS_stats_remove(ics_state->statsMemSys);
  ICS_stats_remove(ics_state->statsCpu);

  ics_stats_term();

  ics_nsrv_term();

  disconnectCpus();

  /* Close and free off any remaining ports */
  ics_port_term();

  /* Remove any remaining regions */
  ics_region_term();
  
  /* Free up per CPU state */
  termCpus();

  /* Free off any cached events */
  ics_event_term();

  /* No more messages will be logged after this completes */
  ics_transport_term();

  ICS_PRINTF(ICS_DBG_INIT, "Success: Freeing off state %p\n", ics_state);

  _ICS_OS_FREE(ics_state);
  ics_state = NULL;

  /* Display any un-freed mmap allocations */
  ICS_assert(_ICS_DEBUG_MMAP_DUMP() == 0);

  /* Display any un-freed memory allocations */
  ICS_assert(_ICS_DEBUG_MEM_DUMP() == 0);

  return;
}

/* ICS watchdog : Called whenever a CPU failure is detected.
 *
 * Called from task context, whilst not holding the ICS state lock
 */
static
void ics_watchdog_callback (ICS_WATCHDOG handle, ICS_VOID *param, ICS_UINT cpu)
{
  ICS_EPRINTF(ICS_DBG_WATCHDOG, "Watchdog callback param %p handle %x cpu %d\n",
	      param, (ICS_UINT)handle, cpu);

  ICS_assert(ics_state);
  ICS_assert(ics_state->watchdog);

#ifdef ICS_DEBUG
  /* Dump out the log file of the failed CPU */
  if (ics_state->cpuNum == 0)
    ICS_debug_dump(cpu);
#endif

  /* Must disconnect from the failed CPU */
  (void) ICS_cpu_disconnect(cpu, ICS_CPU_DEAD);

  /* And now re-prime the watchdog */
  (void) ICS_watchdog_reprime(ics_state->watchdog, cpu);

  ICS_EPRINTF(ICS_DBG_WATCHDOG, "Disconnected cpu %d\n", cpu);
}


/*
 * Initialise the ICS system on this cpu
 *
 */
ICS_ERROR ics_cpu_init (ICS_UINT cpuNum, ICS_ULONG cpuMask, ICS_UINT flags)
{
  ICS_ERROR err = ICS_SUCCESS;
  ICS_UINT  validFlags = (ICS_INIT_CONNECT_ALL|ICS_INIT_WATCHDOG);

  ICS_ULONG initial = _ICS_OS_MEM_USED();

  /* You cannot call initialize multiple times from the same cpu */
  if (ics_state)
    return ICS_ALREADY_INITIALIZED;

  ICS_PRINTF(ICS_DBG, "cpuNum %d cpuMask 0x%lx flags 0x%x\n",
	     cpuNum, cpuMask, flags);

  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Initialise DEBUG memory allocation system (ICS_DEBUG_MEM) */
  _ICS_DEBUG_MEM_INIT();
  
  /* Allocate the local state data structure */
  _ICS_OS_ZALLOC(ics_state, sizeof(ics_state_t));
  if (ics_state == NULL)
  {
    err = ICS_ENOMEM;
    goto error;
  }

  /* Initialise the cpu state structures */
  ics_state->cpuNum  = cpuNum;

  /* Add in ourselves to the cpuMask */
  cpuMask |= (1UL << cpuNum);
  ics_state->cpuMask = cpuMask;

  /* Call the transport initialisation for this cpu */
  if ((err = ics_transport_init(cpuNum, cpuMask)) != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "cpu %d state %p transport init failed\n",
		cpuNum, ics_state);
    goto error_free;
  }

  /*
   * Debug log up and running now we can PRINTF 
   */
  ICS_PRINTF(ICS_DBG, "cpu %d ('%s') cpuMask 0x%lx ics_state %p\n", 
	     cpuNum, ics_cpu_name(cpuNum), cpuMask, ics_state);

  /* Initialise an IRQ/SMP lock */
  _ICS_OS_SPINLOCK_INIT(&ics_state->spinLock);

  /* Create/initialise a multithreaded lock */
  if (!_ICS_OS_MUTEX_INIT(&ics_state->lock))
  {
    ICS_EPRINTF(ICS_DBG_INIT,
		"cpu %d state %p MUTEX_INIT %p failed\n",
		cpuNum, ics_state, &ics_state->lock);

    err = ICS_SYSTEM_ERROR;
    goto error_termTransport;
  }

  /* Initialise the Port table */
  err = ics_port_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
    goto error_termTransport;

  /* Initialise the Region table */
  err = ics_region_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
    goto error_termPort;
  
  /* Initialise the event list */
  err = ics_event_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
    goto error_termRegion;

  /* Alloc/Initialise the local ICS CPU message channels */
  err = initCpus();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT,
		"cpu %d cpuMask 0x%lx state %p initCpus failed\n",
		ics_state->cpuNum, ics_state->cpuMask, ics_state);
    
    goto error_termEvent;
  }

  /* Create the admin port */
  err = ics_admin_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    goto error_termCpus;
  }

  /* Create and initialise the Nameserver client/server */
  err = ics_nsrv_init(cpuNum);
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise Nameserver */
    goto error_termAdmin;
  }

  /*
   * Initialise the Stats subsystem
   */
  err = ics_stats_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise the Stats subsystem */
    goto error_termNsrv;
  }

#ifdef ICS_DEBUG_MEM
  /* Register some stats handlers */
  err = ICS_stats_add("ICS memory usage", ICS_STATS_COUNTER, ics_stats_mem, NULL, &ics_state->statsMem);
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise the Stats subsystem */
    goto error_termStats;
  }
#endif

  err = ICS_stats_add("ICS OS memory usage", ICS_STATS_COUNTER, ics_stats_mem_sys, NULL, &ics_state->statsMemSys);
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise the Stats subsystem */
    goto error_termStats;
  }

  err = ICS_stats_add("ICS OS CPU load", ICS_STATS_PERCENT100, ics_stats_cpu, NULL, &ics_state->statsCpu);
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise the Stats subsystem */
    goto error_termStats;
  }

  /*
   * Create and initialise the Watchdog task
   */
  err = ics_watchdog_init();
  ICS_assert(err == ICS_SUCCESS);
  if (err != ICS_SUCCESS)
  {
    /* Failed to initialise the Watchdog task */
    goto error_termStats;
  }

  if ((flags & ICS_INIT_CONNECT_ALL))
  {
    /* 
     * Now connect to all the other cpus
     */
    err = connectCpus();
    ICS_assert(err == ICS_SUCCESS);
    if (err != ICS_SUCCESS)
    {
    /* Failed to connect to all cpus */
      goto error_termDisconnect;
    }
  }
  else
  {
    /* Always make a connection to ourself */
    err = ICS_cpu_connect(cpuNum, 0, _ICS_CONNECT_TIMEOUT);
    ICS_assert(err == ICS_SUCCESS);

    /* Now start the ICS transport allowing other CPUs to connect to us */
    err = ics_transport_start();
    if (err != ICS_SUCCESS)
    {
      goto error_termWatchdog;
    }
  }

  if ((flags & ICS_INIT_WATCHDOG))
  {
    /* Install a watchdog handler for all CPUs in the bitmask */
    err = ICS_watchdog_add(ics_watchdog_callback, NULL, ics_state->cpuMask, 0, &ics_state->watchdog);
    ICS_assert(err == ICS_SUCCESS);
    if (err != ICS_SUCCESS)
    {
      goto error_termDisconnect;
    }
  }

  ICS_PRINTF(ICS_DBG_INIT, "Completed successfully : cpu %d cpuMask 0x%x state %p\n",
	     ics_state->cpuNum, ics_state->cpuMask, ics_state);

  /* Log the memory usage at the end of init */
  ICS_PRINTF(ICS_DBG, "Memory usage : allocated %ld OS used %ld (+%ld) avail %ld\n",
	     _ICS_DEBUG_MEM_TOTAL(), 
	     _ICS_OS_MEM_USED(), _ICS_OS_MEM_USED() - initial,
	     _ICS_OS_MEM_AVAIL());

  return ICS_SUCCESS;

/*
 * Failure cases, terminate state in reverse order 
 */

error_termDisconnect:
  /* Stops any new connections */
  ics_transport_stop();

  disconnectCpus();

error_termWatchdog:
  ics_watchdog_term();

error_termStats:
  ics_stats_term();

error_termNsrv:
  ics_nsrv_term();

error_termAdmin:
  ics_admin_term();

error_termCpus:
  /* Free up per CPU state */
  termCpus();

error_termEvent:
  /* Free off any cached events */
  ics_event_term();

error_termRegion:
  /* Remove any remaining regions */
  ics_region_term();
  
error_termPort:
  /* Close and free off any remaining ports */
  ics_port_term();

error_termTransport:
  /* No more messages will be logged after this completes */
  ics_transport_term();

error_free:
  _ICS_OS_FREE(ics_state);
  ics_state = NULL;

error:
  ICS_EPRINTF(ICS_DBG_INIT,
	      "Failed : %s (%d)\n",
	      ics_err_str(err), err);

  return err;
}

ICS_ERROR ICS_cpu_init (ICS_UINT flags)
{
  ICS_UINT  validFlags = (ICS_INIT_CONNECT_ALL|ICS_INIT_WATCHDOG);
  ICS_INT   cpuNum;
  ICS_ULONG cpuMask;

  if (flags & ~validFlags)
    return ICS_INVALID_ARGUMENT;
  
  /* First convert our BSP CPU name into a logical CPU number */
  cpuNum = ics_cpu_self();
  if (cpuNum < 0)
    return ICS_NOT_INITIALIZED;

  /* Now get a bitmask of each CPU present */
  cpuMask = ics_cpu_mask();

  ICS_assert(cpuMask);				/* Should have at least one bit set */
  ICS_assert(cpuMask & 1);			/* Should have a Master cpu == 0 */
  ICS_assert(cpuMask & (1UL << cpuNum));	/* Should contain ourself */		

  /* Now initialise ICS for real */
  return ics_cpu_init(cpuNum, cpuMask, flags);
}

/*
 * Return the local CPU number and cpuMask to the caller
 * 
 * On success returns ICS_SUCCESS and cpuNump & cpuMaskp are updated 
 * with the local cpu number and cpu mask
 * 
 * Otherwise returns ICS_INVALID_ARGUMENT or ICS_NOT_INITIALIZED
 */
ICS_ERROR ICS_cpu_info (ICS_UINT *cpuNump, ICS_ULONG *cpuMaskp)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  if (cpuNump == NULL || cpuMaskp == NULL)
    return ICS_INVALID_ARGUMENT;

  *cpuNump  = ics_state->cpuNum;
  *cpuMaskp = ics_state->cpuMask;

  return ICS_SUCCESS;
}

ICS_ERROR ICS_cpu_version (ICS_UINT cpuNum, ICS_ULONG *versionp)
{
  if (ics_state == NULL)
    return ICS_NOT_INITIALIZED;
  
  if (cpuNum > _ICS_MAX_CPUS || versionp == NULL)
    return ICS_INVALID_ARGUMENT;

  return ics_transport_cpu_version(cpuNum, versionp);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
