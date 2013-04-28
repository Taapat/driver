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

#include <embx.h>	/* EMBX API definitions */

#include <embxshm.h>	/* EMBX SHM API definitions */

#include "_embx_sys.h"

/* 
 * Global variables 
 */
EMBX_INT   embx_cpuNum    = -1;
EMBX_ULONG embx_cpuMask   = 0;
EMBX_UINT  embx_heapSize  = 0;
EMBX_UINT  embx_heapFlags = 0;

/* EMBX_RegisterTransport info */
static EMBX_TransportFactory_fn *embx_transportFn;
EMBXSHM_MailboxConfig_t  *embx_transportConfig;

static ICS_HEAP embx_heap = ICS_INVALID_HANDLE_VALUE;
static ICS_REGION embx_rgnCached = ICS_INVALID_HANDLE_VALUE;
static ICS_REGION embx_rgnUncached = ICS_INVALID_HANDLE_VALUE;

/*
 * EMBX transport layer emulation
 *
 * Emulate the EMBX transport registration but only support
 * a single EMBXSHM transport. Save this config and decode it
 * so we can generate the cpuNum and cpuMask needed to call
 * ICS_init()
 */

EMBX_ERROR EMBX_RegisterTransport (EMBX_TransportFactory_fn *fn, EMBX_VOID *arg, EMBX_UINT arg_size,
				   EMBX_FACTORY *hFactory)
{
  unsigned char *pArgCopy;

  if (fn == NULL || arg == NULL || arg_size == 0 || hFactory == NULL)
  {
    return EMBX_INVALID_ARGUMENT;
  }

  /* EMBXSHM config allowed only */
  if (arg_size < sizeof(EMBXSHM_MailboxConfig_t))
  {
    return EMBX_INVALID_ARGUMENT;
  }
  
  /* Only allow a single Transport registration */
  if (embx_transportFn || embx_transportConfig)
  {
    return EMBX_ALREADY_INITIALIZED;
  }

  /* Copy the user supplied Transport config */
  pArgCopy = (unsigned char *)_ICS_OS_MALLOC(arg_size);
  if (pArgCopy == NULL)
  {
    return EMBX_NOMEM;
  }
  _ICS_OS_MEMCPY(pArgCopy, (unsigned char *)arg, arg_size);

  /* Stash the transport factory info for EMBX_Init() */
  embx_transportFn  = fn;
  embx_transportConfig = (EMBXSHM_MailboxConfig_t *)pArgCopy;

  /* Call registered transport (assume we have just one) 
   * This function will decode the supplied config struct and update
   * the embx config info such as embx_cpuNum, embx_cpuMask
   *
   * XXXX Havana seems to need this in order to work - why ??
   */
  (embx_transportFn)(embx_transportConfig);

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_UnregisterTransport (EMBX_FACTORY hFactory)
{
  if (embx_transportConfig == NULL)
    return EMBX_DRIVER_NOT_INITIALIZED;

  _ICS_OS_FREE(embx_transportConfig);
  embx_transportConfig = NULL;
  
  embx_transportFn = NULL;

  return EMBX_SUCCESS;
}

static
void fillTpinfo (EMBX_TPINFO *tpinfo)
{
  ICS_assert(embx_transportConfig);

  /* Fill out the caller's tpinfo */
  strncpy(tpinfo->name, embx_transportConfig->name, EMBX_MAX_TRANSPORT_NAME);
  tpinfo->isInitialized = EMBX_TRUE;
  tpinfo->usesZeroCopy  = EMBX_TRUE;
  tpinfo->allowsPointerTranslation = EMBX_TRUE;
  tpinfo->allowsPointerTranslation = EMBX_TRUE;
  tpinfo->maxPorts = embx_transportConfig->maxPorts;
  tpinfo->nrOpenHandles = 0;
  tpinfo->nrPortsInUse  = 0;
  tpinfo->memStart = NULL;
  tpinfo->memEnd   = NULL;
}

EMBX_ERROR EMBX_FindTransport (const EMBX_CHAR *name, EMBX_TPINFO *tpinfo)
{
  if ((name == 0) || (tpinfo == 0))
  {
    return EMBX_INVALID_ARGUMENT;
  }

  /* Is there a transport registered ? */
  if (embx_transportConfig == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* Does the name match the registered the transport name ? */
  if (strcmp(name, embx_transportConfig->name))
    return EMBX_INVALID_TRANSPORT;
   
  /* Fill out the TPINFO */
  fillTpinfo(tpinfo);

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_GetFirstTransport (EMBX_TPINFO *tpinfo)
{
  if (embx_transportConfig == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* Fill out the TPINFO */
  fillTpinfo(tpinfo);

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_GetNextTransport (EMBX_TPINFO *tpinfo)
{
  /* There can be only one */
  return EMBX_INVALID_TRANSPORT;
}

EMBX_ERROR EMBX_OpenTransport (const EMBX_CHAR *name, EMBX_TRANSPORT *tp)
{
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* Is there a transport registered ? */
  if (embx_transportConfig == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* Does the name match the registered the transport name ? */
  if (strcmp(name, embx_transportConfig->name))
    return EMBX_INVALID_TRANSPORT;

  /* Call registered transport (assume we have just one) 
   * This function will decode the supplied config struct and update
   * the embx config info such as embx_cpuNum, embx_cpuMask
   */
  (embx_transportFn)(embx_transportConfig);

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_CloseTransport (EMBX_TRANSPORT htp)
{
  /* Is there a transport registered ? */
  if (embx_transportConfig == NULL)
    return EMBX_INVALID_TRANSPORT;

  if (embx_heap)
    ics_heap_destroy(embx_heap, 0);
  
  if (embx_rgnCached)
    ICS_region_remove(embx_rgnCached, 0);

  if (embx_rgnUncached)
    ICS_region_remove(embx_rgnUncached, 0);

  /* Reset these for re-use */
  embx_heap = ICS_INVALID_HANDLE_VALUE;
  embx_rgnCached = ICS_INVALID_HANDLE_VALUE;
  embx_rgnUncached = ICS_INVALID_HANDLE_VALUE;

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_GetTransportInfo (EMBX_TRANSPORT htp, EMBX_TPINFO *tpinfo)
{
  /* Is there a transport registered ? */
  if (embx_transportConfig == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* Fill out the TPINFO */
  fillTpinfo(tpinfo);

  return EMBX_SUCCESS;
}

/*
 * Decode and interpret the EMXBSHM config, calling ics_init()
 * and then creating the EMBX SHM heap and registering it as a region
 *
 * In theory it could also register the Warp ranges
 */
static
EMBX_ERROR transportConfig (EMBXSHM_MailboxConfig_t *config, ICS_BOOL cached)
{
  ICS_ERROR err;

  ICS_UINT cpuNum;
  ICS_ULONG cpuMask;

  int i;
  
  /* check the validity of the generic parts of the template */
  if ( (NULL == config) ||
       (NULL == config->name) ||
       (0 == config->participants[0]) ||
       (0 == config->participants[config->cpuID]) ||
       (config->cpuID >= EMBXSHM_MAX_CPUS) )
  {
    return EMBX_INVALID_ARGUMENT;
  }

  /* Extract information from the config struct */
  embx_cpuNum   = config->cpuID;

  /* Generate the CPU participant bitmask */
  embx_cpuMask = 0;
  for (i=0; i<EMBXSHM_MAX_CPUS; i++) 
  {
    if (config->participants[i])
    {
      embx_cpuMask |= 1UL << i;
    }
  }

  /* Test whether ICS is already initialised */
  if (ICS_cpu_info(&cpuNum, &cpuMask) == ICS_NOT_INITIALIZED)
  {
    /* Have we been given enough info to call ics_init() ? */
    if (embx_cpuNum == -1 || embx_cpuMask == 0)
      return EMBX_DRIVER_NOT_INITIALIZED;
    
    /* Now we have enough info to call ics_cpu_init() */
    err = ics_cpu_init(embx_cpuNum, embx_cpuMask, ICS_INIT_CONNECT_ALL);
    if (err != ICS_SUCCESS)
    {
      return EMBX_ERROR_CODE(err);
    }
  }

  embx_heapSize  = config->sharedSize;
  embx_heapFlags = (cached ? ICS_CACHED : ICS_UNCACHED);	/* Set default memory allocation flags */

  /* Be careful not to double create the EMBX heap */
  if ((embx_heap == NULL) && embx_heapSize)
  {
    /* Create the EMBX SHM heap */
    err = ics_heap_create(NULL, embx_heapSize, 0, &embx_heap);
    if (err != ICS_SUCCESS)
      goto error_term;
#if defined(__arm__)
    /* Add the heap specifying a remote cached mapping */
    err = ICS_region_add(ics_heap_base(embx_heap,embx_heapFlags),
			 ics_heap_pbase(embx_heap),
			 ics_heap_size(embx_heap),
			 embx_heapFlags,
			 embx_cpuMask,(cached ? &embx_rgnCached : &embx_rgnUncached));
    if (err != ICS_SUCCESS)
      goto error_destroy;
#elif defined (__sh__) || defined(__st200__)
    /* Add the heap specifying a remote cached mapping */
    err = ICS_region_add(ics_heap_base(embx_heap, ICS_CACHED),
			 ics_heap_pbase(embx_heap),
			 ics_heap_size(embx_heap),
			 ICS_CACHED,
			 embx_cpuMask, &embx_rgnCached);
    if (err != ICS_SUCCESS)
      goto error_destroy;

    /* Add the heap specifying a remote uncached mapping */
    err = ICS_region_add(ics_heap_base(embx_heap, ICS_UNCACHED),
			 ics_heap_pbase(embx_heap),
			 ics_heap_size(embx_heap),
			 ICS_UNCACHED,
			 embx_cpuMask, &embx_rgnUncached);
    if (err != ICS_SUCCESS)
      goto error_remove;
#else
#error "Undeinfed CPU type"
#endif /* defined (__arm__) */
  }

  return EMBX_SUCCESS;

error_remove:
  ICS_region_remove(embx_rgnCached, 0);
  
error_destroy:
  ics_heap_destroy(embx_heap, 0);
  
error_term:
  ICS_cpu_term(0);

  return EMBX_ERROR_CODE(err);
}

EMBX_Transport_t *EMBXSHM_mailbox_factory (EMBX_VOID *param)
{
  EMBX_ERROR err;

  EMBXSHM_MailboxConfig_t *config = (EMBXSHM_MailboxConfig_t *) param;

  err = transportConfig(config, ICS_FALSE);

  return NULL;
}

EMBX_Transport_t *EMBXSHMC_mailbox_factory (EMBX_VOID *param)
{
  EMBX_ERROR err;

  EMBXSHM_MailboxConfig_t *config = (EMBXSHM_MailboxConfig_t *) param;

  err = transportConfig(config, ICS_TRUE);

  return NULL;
}

EMBX_ERROR EMBX_Alloc (EMBX_TRANSPORT tp, EMBX_UINT size, EMBX_VOID **bufferp)
{
  ICS_VOID *buf;
  
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  if (embx_heap == NULL)
    return EMBX_INVALID_TRANSPORT;

  buf = ics_heap_alloc(embx_heap, size, embx_heapFlags);
  if (buf != NULL)
  {
    *bufferp = buf;
    
    return EMBX_SUCCESS;
  }

  return EMBX_NOMEM;
}

EMBX_ERROR EMBX_Free (EMBX_VOID *buffer)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  if (embx_heap == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* XXXX Need to handle non local buffers */
  err = ics_heap_free(embx_heap, buffer);

  return EMBX_ERROR_CODE(err);
}

EMBX_ERROR EMBX_Release (EMBX_TRANSPORT tp, EMBX_VOID *buffer)
{
  ICS_ERROR err;

  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  if (embx_heap == NULL)
    return EMBX_INVALID_TRANSPORT;

  /* XXXX Need to handle non local buffers */
  err = ics_heap_free(embx_heap, buffer);

  return EMBX_SUCCESS;
}

EMBX_ERROR EMBX_GetBufferSize (EMBX_VOID *buffer, EMBX_UINT *size)
{
  if (!embx_initialised)
    return EMBX_DRIVER_NOT_INITIALIZED;

  /* XXXX Need to implement this ? */

  return EMBX_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
