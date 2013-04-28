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

#include "_ics_shm.h"	/* SHM specific headers */

/* 
 * Write a message string to the cyclic debug log buffer
 * 
 * An Inter-cpu spinlock is used to make shared accesses to this structure
 * safe, but careful cache invalidating and flushing is still required
 */
void ics_shm_debug_msg (ICS_CHAR *msg, ICS_UINT len)
{
  ics_shm_debug_log_t *debugLog;
  ICS_CHAR            *base;

  unsigned long        flags;

  if (!ics_shm_state || !ics_shm_state->debugLog)
    return;

  /* Local OS lock */
  _ICS_OS_SPINLOCK_ENTER(&ics_shm_state->debugLock, flags);

  debugLog = ics_shm_state->debugLog;
  
  /* Take Inter-cpu SHM lock */
  _ICS_SHM_LOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Refresh all of control structure */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, offsetof(ics_shm_debug_log_t, log));
  
  /* Calculate insertion point */
  base = &debugLog->log[debugLog->fptr][0];

  /* Don't overflow line, and leave space for '\0' */
  if (len > _ICS_DEBUG_LINE_SIZE-1)
    len = _ICS_DEBUG_LINE_SIZE-1;

  /* Copy string into line buffer, truncating it as necessary */
  (void) strncpy(base, msg, len);

  /* Always terminate the string */
  base[len] = '\0';

  /* Flush newly written line to memory */
  _ICS_OS_CACHE_FLUSH(base, debugLog->paddr + offsetof(ics_shm_debug_log_t, log[debugLog->fptr][0]), len);

  /* Move on and wrap the fptr */
  debugLog->fptr = (debugLog->fptr+1 == _ICS_DEBUG_NUM_LINES) ? 0 : debugLog->fptr+1;

  /* Check for the log becoming full */
  if (debugLog->fptr == debugLog->bptr)
  {
    /* Overflow detected. Move on the bptr, so we can overwrite the oldest lines */
    debugLog->bptr = (debugLog->bptr+1 == _ICS_DEBUG_NUM_LINES) ? 0 : debugLog->bptr+1;
    
    /* Record the overflow condition */
    debugLog->overflow++;
  }

  /* Write all of the control structure back to memory */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, offsetof(ics_shm_debug_log_t, log));

  /* Drop Inter-cpu lock */
  _ICS_SHM_UNLOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Drop local OS lock */
  _ICS_OS_SPINLOCK_EXIT(&ics_shm_state->debugLock, flags);
}

/* Dump out the supplied log to stdout
 *
 * Moves on the log bptr accordingly
 *
 * These logs are held in cached shared memory and hence
 * careful flushing and invalidating of cache lines is required.
 */
static
int debugDump (ICS_UINT cpuNum, ics_shm_debug_log_t *debugLog)
{
  int nl = 0;
  
  ICS_UINT bptr, fptr;

  /* Take the Inter-cpu SHM lock */
  _ICS_SHM_LOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Refresh the whole debugLog and all its lines */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, debugLog->size);

  /* Sample the current log state */
  bptr = debugLog->bptr;
  fptr = debugLog->fptr;

  _ICS_OS_PRINTF("Dumping cpu %d debug log %p [%d/%d] lines lost = %d\n", 
		 cpuNum, debugLog, bptr, fptr, debugLog->overflow);

  /* Dump out all messages between the bptr and the fptr */
  while (bptr != fptr)
  {
    ICS_CHAR *base = &debugLog->log[bptr][0];
    int len;
    
    _ICS_OS_PRINTF("[%d.%03d] ", cpuNum, bptr);

    /* Concatenate lines which don't end in '\n' */
    do 
    {
      base = &debugLog->log[bptr][0];
      len = strlen(base);
    
      _ICS_OS_PRINTF("%s", base);

      /* Update back pointer (consumes line) */
      bptr = (bptr+1 == debugLog->numLines) ? 0 : bptr+1;

      /* Don't wrap past end of buffer */
      if (bptr == fptr)
	break;

    } while (base[len-1] != '\n');

    nl++;

    /* Prevent the watchdog expiring due to slow PRINTF */
    /* XXXX Perhaps we should hook up a timer callback instead ? */
    ics_shm_watchdog(_ICS_OS_TIME_NOW());
  }
  
  /* Update control */
  debugLog->bptr = bptr;
  debugLog->overflow = 0;

  /* Write all of the control structure back to memory */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, offsetof(ics_shm_debug_log_t, log));

  _ICS_SHM_UNLOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Return the number of lines displayed */
  return nl;
}

ICS_ERROR ics_shm_debug_dump (ICS_UINT cpuNum)
{
  ICS_INT nl;
  ics_shm_cpu_t *cpu;

  if (!ics_shm_state || !ics_shm_state->shm)
    return ICS_NOT_INITIALIZED;
 
  if (cpuNum >= _ICS_MAX_CPUS)
    return ICS_INVALID_ARGUMENT;
    
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->debugLog == NULL)
    return ICS_NOT_CONNECTED;

  nl = debugDump(cpuNum, cpu->debugLog);

  return ICS_SUCCESS;
}

/* Copy out the debug log to a buffer
 *
 * Moves on the log bptr accordingly
 *
 * These logs are held in cached shared memory and hence
 * careful flushing and invalidating of cache lines is required.
 */
static
int debugCopy (ICS_UINT cpuNum, ics_shm_debug_log_t *debugLog, ICS_CHAR *buf, const ICS_SIZE bufSize,
	       ICS_INT *bytesp)
{
  ICS_ERROR err = ICS_SUCCESS;

  ICS_UINT bptr, fptr;

  ICS_UINT offset = 0;
  
  /* Take the Inter-cpu SHM lock */
  _ICS_SHM_LOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Refresh the whole debugLog and all its lines */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, debugLog->size);

  /* Sample the current log state */
  bptr = debugLog->bptr;
  fptr = debugLog->fptr;

  /* Dump out all messages between the bptr and the fptr */
  while ((bptr != fptr) && ((bufSize - offset) >= _ICS_DEBUG_LINE_SIZE))
  {
    ICS_CHAR *base = &debugLog->log[bptr][0];
    int len;

    /* Prefix line with bptr */
    offset += sprintf(buf+offset, "[%03d] ", bptr);

    /* Concatenate lines which don't end in '\n' */
    do 
    {
      base = &debugLog->log[bptr][0];
      len = strlen(base);

      /* Check for buffer overflow */
      if ((offset + len) >= bufSize)
	break;

      /* write line into supplied buffer */
      offset += sprintf(buf+offset, "%s", base);

      /* Update back pointer (consumes line) */
      bptr = (bptr+1 == debugLog->numLines) ? 0 : bptr+1;

      /* Don't wrap past end of buffer */
      if ((bptr == fptr))
	break;

    } while (base[len-1] != '\n');
  }

  ICS_ASSERT(offset <= bufSize);

  /* Detect and signal buffer overflow */
  if (bptr != fptr)
    err = ICS_ENOMEM;

  /* Update control */
  debugLog->bptr = bptr;
  debugLog->overflow = 0;

  /* Write all of the control structure back to memory */
  _ICS_OS_CACHE_PURGE(debugLog, debugLog->paddr, offsetof(ics_shm_debug_log_t, log));

  _ICS_SHM_UNLOCK(&debugLog->dbgLock, debugLog->paddr + offsetof(ics_shm_debug_log_t, dbgLock));

  /* Return the number of bytes copied */
  *bytesp = offset;

  return err;
}

ICS_ERROR ics_shm_debug_copy (ICS_UINT cpuNum, ICS_CHAR *buf, const ICS_SIZE bufSize, ICS_INT *bytesp)
{
  ICS_ERROR err;

  ics_shm_cpu_t *cpu;

  if (!ics_shm_state || !ics_shm_state->shm)
    return ICS_NOT_INITIALIZED;
 
  if (cpuNum >= _ICS_MAX_CPUS)
    return ICS_INVALID_ARGUMENT;
    
  cpu = &ics_shm_state->cpu[cpuNum];
  if (cpu->debugLog == NULL)
    return ICS_NOT_CONNECTED;

  err = debugCopy(cpuNum, cpu->debugLog, buf, bufSize, bytesp);

  return err;
}


ICS_ERROR ics_shm_debug_init (void)
{
  ics_shm_debug_log_t *debugLog;
  ICS_SIZE             logSize;

  ICS_ERROR            err;
  ICS_OFFSET           paddr;
  ICS_MEM_FLAGS        mflags;

  ICS_assert(ics_shm_state);
  ICS_assert(_ICS_DEBUG_NUM_LINES >= 2);

  if (ics_shm_state->debugLog)
    return ICS_ALREADY_INITIALIZED;

  /* Allocate the debug log control and buffer 
   *
   * We allocate it from contiguous memory so it can be directly mapped
   * into other CPU's address space (e.g. on the host)
   */
  logSize = _ICS_DEBUG_LOG_SIZE;
  debugLog = _ICS_OS_CONTIG_ALLOC(logSize, ICS_PAGE_SIZE);

  if (debugLog == NULL)
  {
    ICS_EPRINTF(ICS_DBG_INIT, "Failed to allocate debugLog %d bytes\n",
		logSize);

    return ICS_ENOMEM;
  }

  /* Convert the address to a physical one */
  err = _ICS_OS_VIRT2PHYS(debugLog, &paddr, &mflags);
  if (err != ICS_SUCCESS)
  {
    _ICS_OS_CONTIG_FREE(debugLog);
    return err;
  }

  /* Initialise the debug log */
  _ICS_OS_MEMSET(debugLog, 0, logSize);
  
  debugLog->size     = logSize;
  debugLog->numLines = _ICS_DEBUG_NUM_LINES;
  debugLog->paddr    = paddr;

  /* Flush the debugLog structure to memory */
  _ICS_OS_CACHE_FLUSH(debugLog, paddr, logSize);

  _ICS_OS_SPINLOCK_INIT(&ics_shm_state->debugLock);
  ics_shm_state->debugLog = debugLog;

  return ICS_SUCCESS;
}

ICS_ERROR ics_shm_debug_term (void)
{
  ICS_assert(ics_shm_state);
  ICS_assert(ics_shm_state->debugLog);
  
  _ICS_OS_CONTIG_FREE(ics_shm_state->debugLog);
  ics_shm_state->debugLog = NULL;

  return ICS_SUCCESS;
}


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
