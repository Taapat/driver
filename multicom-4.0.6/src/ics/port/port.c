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

/**************************************************************
 *
 * File: ics_port.c
 *
 * Description
 *    Routines to implement local ICS Port creation
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Free off a local Port data structure 
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
void destroyPort (ics_port_t *lp)
{
  ICS_assert(lp->state == _ICS_PORT_FREE);

  /* There should be no blocked receivers left now */
  ICS_assert(list_empty(&lp->postedRecvs));

  /* The MQ should have been released by releasePort */
  ICS_assert(lp->mq == NULL);

  /* Free off the port desc */
  _ICS_OS_FREE(lp);

  return;
}

/*
 * Return an allocated port desc back to the table
 *
 * In doing this we can increment a version number so
 * when this port slot is reused it has a different handle
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
void releasePort (ics_port_t *lp)
{
  lp->name[0] = '\0';
  lp->callback = lp->callbackParam = NULL;

  /* There should be no blocked receivers left now */
  ICS_assert(list_empty(&lp->postedRecvs));

  /* There should be no blocked channels */
  ICS_assert(lp->blockedChans == 0);

  /* The MQ should be empty */
  if (lp->mq)
  {
    /* There should be no pending messages */
    ICS_assert(ics_mq_empty(lp->mq));

    /* Free off MQ memory */
    _ICS_OS_FREE(lp->mq);
    lp->mq = NULL;
  }

  /* Increment (and wrap) the port incarnation version for future allocations */
  lp->version = (++lp->version & _ICS_HANDLE_VER_MASK);
  
  /* Generate the new port handle */
  lp->handle = _ICS_HANDLE(_ICS_TYPE_PORT, ics_state->cpuNum, lp->version, lp->handle);

  /* Finally mark as being free */
  lp->state = _ICS_PORT_FREE;

  return;
}

/* 
 * Attempt to grow the port table
 * Errors are not reported but next port allocation will fail
 * 
 * MULTITHREAD SAFE: Called holding the state lock
 */
void
growPortTable (void)
{
  ics_port_t **newTable;
  
  int tableSize = 2 * ics_state->portEntries;	/* Double size */

  ICS_PRINTF(ICS_DBG_PORT, "portEntries %d new %d\n", ics_state->portEntries, tableSize);

  if (tableSize > _ICS_MAX_PORTS)		/* Limit maximum size */
    return;

  _ICS_OS_ZALLOC(newTable, tableSize * sizeof(ics_port_t *));
  if (newTable == NULL)
    return;

  ICS_PRINTF(ICS_DBG_PORT, "Copy old table %p to %p size %d\n",
	     ics_state->port, newTable, ics_state->portEntries*sizeof(ics_port_t *));

  /* Copy old table into new one */
  _ICS_OS_MEMCPY(newTable, ics_state->port, ics_state->portEntries*sizeof(ics_port_t *));

  /* Free off old table */
  _ICS_OS_FREE(ics_state->port);

  /* Update state desc */
  ics_state->port = newTable;
  ics_state->portEntries = tableSize;

  return;
}

/*
 * Find a free local Port idx
 *
 * Returns -1 if non found
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
int findFreePort (void)
{
  int i;
  
  ICS_ASSERT(ics_state);

  for (i = 0; i < ics_state->portEntries; i++)
  {
    /* Look for either a free slot or a free desc */
    if (ics_state->port[i] == NULL || ics_state->port[i]->state == _ICS_PORT_FREE)
    {
      /* Found a free slot */

      if (i == (ics_state->portEntries-1))
      {
	/* Just used last slot, attempt to grow port table */
	growPortTable();
      }

      return i;
    }
  }

  /* No free slot found */
  return -1;
}

/*
 * Allocate and initialise the data structures for a new port
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
ics_port_t *allocPort (int idx)
{
  ics_port_t *lp;

  ICS_assert(idx >= 0 && idx < ics_state->portEntries);
  ICS_assert(ics_state->port[idx] == NULL);

  /* Allocate the port desc */
  _ICS_OS_ZALLOC(lp, sizeof(*lp));
  if (lp == NULL)
    return NULL;

  lp->state = _ICS_PORT_FREE;

  /* Initialise the posted receives list */
  INIT_LIST_HEAD(&lp->postedRecvs);

  /* Generate the initial port handle */
  lp->handle = _ICS_HANDLE(_ICS_TYPE_PORT, ics_state->cpuNum, lp->version, idx);

  /* Add port to the local table */
  ics_state->port[idx] = lp;

  return lp;
}

static
ics_port_t *createPortLocal (const ICS_CHAR *portName, ICS_PORT_CALLBACK callback, ICS_VOID *param,
			     ICS_UINT ndesc)
{
  int         idx;

  size_t      mqSize;
  size_t      ssize;

  ics_port_t *lp;

  /* Protect table against other threads, but not the ISR */
  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Find a free port index */
  idx = findFreePort();
  if (idx == -1)
  {
    /* No free local port slots */
    goto error_release;
  }

  lp = ics_state->port[idx];
  if (lp == NULL)
  {
    /* Empty slot - allocate a new port entry and handle */
    lp = allocPort(idx);
    if (lp == NULL)
    {
      /* Failed to allocate a new port */
      goto error_release;
    }
  }

  /* There should be no blocked receivers */
  ICS_assert(list_empty(&lp->postedRecvs));

  /* There should be no MQ */
  ICS_assert(lp->mq == NULL);

  /* Allocate an associated message queue */
  if (ndesc)
  {
    /* The message queue size parameter */
    ssize = _ICS_MSG_SSIZE;

    /* Allocate the per Port message queue */
    mqSize = _ICS_MQ_SIZE(ndesc, ssize);
    
    if ((lp->mq = _ICS_OS_MALLOC(mqSize)) == NULL)
    {
      /* Failed to allocate a message queue */
      _ICS_OS_FREE(lp);
      
      return NULL;
    }
    
    /* Initialise the Port message queue */
    ics_mq_init(lp->mq, ndesc, ssize);
  }

  /* Copy the port name if there is one */
  if (portName)
    strcpy(lp->name, portName);		/* Will also copy the '\0' */

  /* Record any user callback info */
  lp->callback      = callback;
  lp->callbackParam = param;
  lp->callbackBlock = ICS_FALSE;

  /* Mark port as being open - only now becomes visible to the ISR */
  lp->state = _ICS_PORT_OPEN;

  ICS_PRINTF(ICS_DBG_PORT, "created port %p [%d] '%s' ndesc %d callback %p\n",
	     lp, idx, (portName ? portName : "anonymous"), ndesc, callback);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return lp;

error_release:

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return NULL;
}

/*
 * Create a new local port, registering its name and handle with
 * the nameserver if supplied.
 * If portName is NULL then this is an anonymous port and its name 
 * is not registered with the global nameserver
 */
ICS_ERROR ICS_port_alloc (const ICS_CHAR *portName, 
			  ICS_PORT_CALLBACK callback, ICS_VOID *param,
			  ICS_UINT ndesc,
			  ICS_UINT flags,
			  ICS_PORT *portp)
{
  ICS_ERROR   err = ICS_SYSTEM_ERROR;

  ICS_UINT    validFlags = 0;

  ics_port_t *lp;

  if (ics_state == NULL)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  ICS_PRINTF(ICS_DBG_PORT, "port '%s' callback %p param %p ndesc %d flags 0x%x\n",
	     portName ? portName : "anonymous",
	     callback, param, ndesc, flags);

  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }
    
  if (portp == NULL)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  if (portName && (strlen(portName) > ICS_NSRV_MAX_NAME))
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Check ndesc is powerof2() but also allow 0 */
  if (!powerof2(ndesc) && !(ndesc == 0))
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Create a local port and assign a new handle */
  lp = createPortLocal(portName, callback, param, ndesc);
  if (lp == NULL)
  {
    /* Port creation error */
    err = ICS_ENOMEM;
    goto error;
  }

  ICS_assert(lp->handle != ICS_INVALID_HANDLE_VALUE);

  /* Now register the portName & handle with the nameserver */
  if (portName)
  {
    ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_PORT, "Register port '%s' handle 0x%x\n", portName, lp->handle);
    
    /* Register the new port with its handle in the nameserver */
    err = ICS_nsrv_add(portName, &lp->handle, sizeof(lp->handle), 0, &lp->nhandle);
    if (err != ICS_SUCCESS)
    {
      _ICS_OS_MUTEX_TAKE(&ics_state->lock);

      /* Failed to register port with nameserver
       * need to release the allocated Port
       */
      releasePort(lp);

      _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

      goto error;
    }
  }

  /* Return port handle to caller */
  *portp = lp->handle;
  
  return ICS_SUCCESS;

error:
  
  return err;
}

/* Called during close to wake up any blocked threads
 * Walks the postedRecvs list, invalidating and then
 * signalling any it finds
 *
 * MULTITHREAD SAFE: Called holding the state lock
 */
static
void wakeupBlockedRecvs (ics_port_t *lp)
{
  ics_event_t *event, *tmp;

  list_for_each_entry_safe(event, tmp, &lp->postedRecvs, list)
  {
    ICS_MSG_DESC *rdesc;

    rdesc = (ICS_MSG_DESC *) event->data;
    ICS_assert(rdesc);

    ICS_PRINTF(ICS_DBG_PORT, "Waking blocked event %p rdesc %p\n",
	       event, rdesc);

    /* Signal port closure via the event */
    event->state = _ICS_EVENT_ABORTED;

    /* Remove entry from the list */
    list_del_init(&event->list);

    /* Now signal completion to the posted receive event */
    _ICS_OS_EVENT_POST(&event->event);
  }

  return;
}

static
ICS_ERROR closePort (ics_port_t *lp)
{
  ICS_assert(lp);
  ICS_assert(lp->state == _ICS_PORT_OPEN);

  /* First deregister the port from the Nameserver */
  if (lp->name[0] != '\0')
  {
    ICS_ERROR err;
  
    ICS_PRINTF(ICS_DBG_PORT, "Deregistering '%s' from nameserver\n",
	       lp->name);

    /* NB: This will acquire/release the ics state lock */
    err = ICS_nsrv_remove(lp->nhandle, 0);
    if (err != ICS_SUCCESS)
      /* Probably due to the Master having already exited */
      ICS_EPRINTF(ICS_DBG_PORT, "Failed to deregister '%s' in nameserver : %s (%d)\n",
		  lp->name, ics_err_str(err), err);
  }

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  ICS_PRINTF(ICS_DBG_PORT, "Closing port lp %p name '%s' version %d\n",
	     lp, lp->name, lp->version);

  /* Change port state first, which will invalidate the 
   * entry wrt the ISR
   */
  lp->state = _ICS_PORT_CLOSING;

  if (lp->mq)
  {
    while (!ics_mq_empty(lp->mq))
    {
      ics_msg_t *msg = ics_mq_recv(lp->mq);
      
      /* XXXX Need to return any unprocessed (non-INLINE) mq messages */
      ICS_assert(msg->mflags & ICS_INLINE);

      ics_mq_release(lp->mq);
    }
  }

  if (!list_empty(&lp->postedRecvs))
  {
    /* Wakeup any blocked receive threads */
    wakeupBlockedRecvs(lp);
  }

  /* Release this port index */
  releasePort(lp);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);

  return ICS_SUCCESS;
}

ICS_ERROR ICS_port_free (ICS_PORT port, ICS_UINT flags)
{
  ICS_ERROR   err;
  
  ICS_UINT    validFlags = 0;

  ics_port_t *lp;
 
  int         type, cpuNum, ver, idx;
  
  if (ics_state == NULL)
  {
    return ICS_NOT_INITIALIZED;
  }

  if (port == ICS_INVALID_HANDLE_VALUE)
  {
    return ICS_HANDLE_INVALID;
  }

  if (flags & ~validFlags)
  {
    return ICS_INVALID_ARGUMENT;
  }

  /* Decode the target port handle */
  _ICS_DECODE_HDL(port, type, cpuNum, ver, idx);

  /* Check the target port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= ics_state->portEntries)
  {
    return ICS_HANDLE_INVALID;
  }

  if (cpuNum != ics_state->cpuNum)
  {
    /* Can only free local ports */
    return ICS_HANDLE_INVALID;
  }

  ICS_PRINTF(ICS_DBG_PORT, "port 0x%x -> type 0x%x cpuNum %d ver %d idx %d\n",
	     port, type, cpuNum, ver, idx);

  /* Get a handle onto the local port desc */
  lp = ics_state->port[idx];
  if (lp == NULL || lp->state != _ICS_PORT_OPEN || lp->version != ver)
  {
    return ICS_HANDLE_INVALID;
  }

  /* Now close the port */
  err = closePort(lp);

  return err;
}

/*
 * Initialise the ics_state port table
 * Called during ics_cpu_init()
 */
ICS_ERROR ics_port_init (void)
{
  /* Allocate an initial Port table array */
  _ICS_OS_ZALLOC(ics_state->port, _ICS_INIT_PORTS * sizeof(ics_port_t *));
  if (ics_state->port == NULL)
    return ICS_ENOMEM;

  ics_state->portEntries = _ICS_INIT_PORTS;

  return ICS_SUCCESS;
}

/* Close and free off all open ports
 * Call during ICS shutdown
 */
void ics_port_term (void)
{
  int i;
  ics_port_t *lp;

  for (i = 0; i < ics_state->portEntries; i++)
  {
    lp = ics_state->port[i];
    
    if (lp)
    {
      /* Close the Port */
      if (lp->state == _ICS_PORT_OPEN)
	closePort(lp);
     
      /* Free off all memory */
      destroyPort(lp);

      ics_state->port[i] = NULL;
    }
  }

  /* Free off port table */
  if (ics_state->port)
    _ICS_OS_FREE(ics_state->port);

  return;
}

/*
 * Client: Lookup a port in the nameserver 
 *
 * Block waiting for it to be created if BLOCK is set
 */
ICS_ERROR
ICS_port_lookup (const ICS_CHAR *name, ICS_UINT flags, ICS_LONG timeout, ICS_PORT *portp)
{
  ICS_ERROR err;
  ICS_UINT  validFlags = ICS_BLOCK;

  ICS_SIZE  size;

  if (ics_state == NULL)
  {
    err = ICS_NOT_INITIALIZED;
    goto error;
  }

  if (name == NULL || portp == NULL)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  if (flags & ~validFlags)
  {
    err = ICS_INVALID_ARGUMENT;
    goto error;
  }

  /* Simply call the Nameserver lookup function */
  err = ICS_nsrv_lookup(name, flags, timeout, portp, &size);

  if (err == ICS_SUCCESS)
    ICS_assert(size == sizeof(*portp));

  return err;

error:
  
  return err;
}

/*
 * Return the owning CPU number given a Port handle
 */
ICS_ERROR
ICS_port_cpu (ICS_PORT port, ICS_UINT *cpuNump)
{
  int type, cpuNum, ver, idx;

  if (ics_state == NULL)
  {
    return ICS_NOT_INITIALIZED;
  }

  if (cpuNump == NULL)
  {
    return ICS_INVALID_ARGUMENT;
  }

  if (port == ICS_INVALID_HANDLE_VALUE)
  {
    return ICS_HANDLE_INVALID;
  }

  /* Decode the port handle */
  _ICS_DECODE_HDL(port, type, cpuNum, ver, idx);

  /* Check the target port */
  if (type != _ICS_TYPE_PORT || cpuNum >= _ICS_MAX_CPUS || idx >= _ICS_MAX_PORTS)
  {
    return ICS_HANDLE_INVALID;
  }

  /* Return cpuNum to caller */
  *cpuNump = cpuNum;
  
  return ICS_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
