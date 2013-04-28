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

static
void termManager (void)
{
  MME_assert(mme_state);

  /* Closing this port will wake the manager task */
  if (mme_state->managerPort != ICS_INVALID_HANDLE_VALUE)
  {
    MME_PRINTF(MME_DBG_INIT, "Closing manager port %p\n", mme_state->managerPort);
    
    ICS_port_free(mme_state->managerPort, 0);
    mme_state->managerPort = ICS_INVALID_HANDLE_VALUE;

    /* Drop state lock so manager task can exit */
    _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

    _ICS_OS_TASK_DESTROY(&mme_state->managerTask);

    /* Re-acquire lock for rest of termination */
    _ICS_OS_MUTEX_TAKE(&mme_state->lock);

    /* XXXX is this safe wrt MME_Run() exiting ? */
    _ICS_OS_EVENT_DESTROY(&mme_state->managerExit);
  }

  return;
}

static
void termHeap (void)
{
  ICS_ERROR err;

  if (mme_state->heapSize == 0)
    return;
  
  MME_assert(mme_state->heap != ICS_INVALID_HANDLE_VALUE);

  /* Remove the CACHED & UNCACHED mappings of the heap */
  err = ICS_region_remove(mme_state->heapCached, 0);
  MME_ASSERT(err == ICS_SUCCESS);

#if defined (__sh__) || defined(__st200__)
  err = ICS_region_remove(mme_state->heapUncached, 0);
  MME_ASSERT(err == ICS_SUCCESS);
#endif /* defined (__sh__) || defined(__st200__) */

  /* Now destroy the heap itself */
  err = ics_heap_destroy(mme_state->heap, 0);
  MME_ASSERT(err == ICS_SUCCESS);

  mme_state->heap         = ICS_INVALID_HANDLE_VALUE;
  mme_state->heapSize     = 0;
  mme_state->heapCached   = ICS_INVALID_HANDLE_VALUE;
  mme_state->heapUncached = ICS_INVALID_HANDLE_VALUE;

  return;
}

/* MME_Term()
 * Terminate MME. Stop all threads and release all memory
 */
MME_ERROR MME_Term (void)
{
  MME_ERROR res;
  
  int i;
  
  if (!mme_state)
    return MME_DRIVER_NOT_INITIALIZED;

  MME_PRINTF(MME_DBG_INIT, "Shutting down on cpu %d\n", mme_state->cpuNum);

  _ICS_OS_MUTEX_TAKE(&mme_state->lock);

  /* Check for any locally instantiated transformers */
  for (i = 0; i < _MME_TRANSFORMER_INSTANCES; i++)
  {
    if (mme_state->insTrans[i])
    {
      res = MME_HANDLES_STILL_OPEN;
      goto error_release;
    }
  }

  /* Terminate all the execution tasks
   * This can fail if there are still (remotely) instantiated transformers
   */
  res = mme_execution_term();
  if (res != MME_SUCCESS)
  {
    MME_EPRINTF(MME_DBG_INIT, "Failed to terminate execution tasks : %s (%d) \n",
		MME_Error_Str(res), res);
    goto error_release;
  }

  /* Remove watchdog callback */
  if (mme_state->watchdog)
    ICS_watchdog_remove(mme_state->watchdog);

  /* Stop the manager task and close its port */
  termManager();
      
  /* Deregister all local transformers */
  mme_register_term();
  
  /* Release all the MME meta data msgs */
  mme_msg_term();

  /* Destroy the MME heap */
  termHeap();

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);

  _ICS_OS_MUTEX_DESTROY(&mme_state->lock);

  MME_PRINTF(MME_DBG_INIT, "Successfully terminated MME mme_state %p\n", mme_state);

  /* Finally zero and free off the mme_state */
  _ICS_OS_MEMSET(mme_state, 0, sizeof(*mme_state));
  _ICS_OS_FREE(mme_state);
  mme_state = NULL;

  return MME_SUCCESS;

error_release:

  MME_EPRINTF(MME_DBG_INIT,
	      "Failed : %s (%d)\n",
	      MME_Error_Str(res), res);

  _ICS_OS_MUTEX_RELEASE(&mme_state->lock);
  
  return res;
}


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
