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
 * File: ics_nsrv.c
 *
 * Description
 *    Routines to implement the global ICS Handle nameserver
 *
 **************************************************************/

#include <ics.h>	/* External defines and prototypes */

#include "_ics_sys.h"	/* Internal defines and prototypes */

/*
 * Implementation of a very simple nameserver
 *
 * Nameserver entries basically consist of a string and object data
 * Users can register/deregister and lookup objects based on the name strings
 *
 * When looking up a name, a 'block' flag can be supplied which will cause the reply
 * to be delayed until an entry with a matching name is registered. This allows for
 * synchronisation between the CPUs
 */

/* Nameserver entry descriptor */
typedef struct ics_nsrv_entry
{
  ICS_CHAR		name[ICS_NSRV_MAX_NAME+1]; 	/* Registered name */

  ICS_NSRV_HANDLE	handle;				/* Associated handle */

  ICS_UINT		srcCpu;				/* Owning cpu */

  ICS_SIZE		size;				/* Size of object */

  ICS_UINT		hvalue;				/* Associated hash value */
  struct list_head	hlist;				/* Doubly linked list (hash chain) */

  ICS_CHAR		data[1];			/* Object data (variable size) */

} ics_nsrv_entry_t;

/* Pending nameserver lookup request */
typedef struct ics_lookup_req
{
  ICS_CHAR		name[ICS_NSRV_MAX_NAME+1]; 	/* Requested name */

  ICS_PORT		replyPort; 			/* Requester reply port */

  ICS_ULONG		timeout;			/* Timeout period for this req */
  
  _ICS_OS_TIME		reqTime;			/* Time of arrival of req */

  ICS_UINT		srcCpu;				/* Owning cpu */

  struct list_head	list;				/* Doubly linked list */

} ics_lookup_req_t;

/* Global Array of name entries */
static struct 
{
  ICS_UINT		 version;			/* Used to maintain handle version */
  ics_nsrv_entry_t	*entry;				/* Allocated entry */
} * ics_nsrvTable;
static ICS_UINT		 ics_numEntries;

/* Hash table for efficient lookup and round-robin behaviour */
#define _ICS_NSRV_HASH_ENTRIES	64
static struct list_head *ics_hashTable;
static ICS_UINT		 ics_hashEntries;

/* Doubly Linked list of lookup requests */
LIST_HEAD(ics_lookup_reqs);

/* Based on Dan Bernstein's string hash function posted eons ago on comp.lang.c */
static
ICS_UINT nameHash (const ICS_CHAR *s)
{
  ICS_UINT h = 5381;
  unsigned char c;

  for (c = *s; c != '\0'; c = *++s)
    h = h * 33 + c;
  
  return (h & 0xffffffff);
}

/* 
 * Initialise the Nameserver table
 *
 * Called by the nameserver task during initialisation
 */
static
ICS_ERROR initTable (int numEntries, int hashEntries)
{
  int idx;
  
  /* Allocate an initial array of entry pointers */
  _ICS_OS_ZALLOC(ics_nsrvTable, numEntries * sizeof(*ics_nsrvTable));
  if (ics_nsrvTable == NULL)
    return ICS_ENOMEM;

  ics_numEntries = numEntries;

  /* Allocate the name hash table */
  _ICS_OS_ZALLOC(ics_hashTable, hashEntries * sizeof(struct list_head));
  if (ics_hashTable == NULL)
    return ICS_ENOMEM;

  /* Initialise hash linked list heads */
  for (idx = 0; idx < hashEntries; idx++)
    INIT_LIST_HEAD(&ics_hashTable[idx]);

  ics_hashEntries = hashEntries;

  return ICS_SUCCESS;
}

/* Remove a nameserver entry */
static
void removeEntry (int idx)
{
  ics_nsrv_entry_t *nse = ics_nsrvTable[idx].entry;
  
  ICS_ASSERT(nse);

  /* Remove from hash list */
  list_del(&nse->hlist);
  
  /* Free off an clear entry slot */
  _ICS_OS_FREE(nse);
  ics_nsrvTable[idx].entry = NULL;

  /* Increment (and wrap) the handle incarnation version for future allocations */
  ics_nsrvTable[idx].version = (++ics_nsrvTable[idx].version & _ICS_HANDLE_VER_MASK);
}

static
void termTable (void)
{
  int idx;

  ICS_ASSERT(ics_nsrvTable);
  ICS_ASSERT(ics_hashTable);

  /* Free off all table entries */
  for (idx = 0; idx < ics_numEntries; idx++)
  {
    if (ics_nsrvTable[idx].entry != NULL)
      removeEntry(idx);
  }

  /* Free off the table itself */
  _ICS_OS_FREE(ics_nsrvTable);
  ics_nsrvTable = NULL;

  /* Hash table should now be empty too */
  for (idx = 0; idx < ics_hashEntries; idx++)
    ICS_assert(list_empty(&ics_hashTable[idx]));
  
  /* Free off the table itself */
  _ICS_OS_FREE(ics_hashTable);
  ics_hashTable = NULL;

  return;
}

/* 
 * Find a free idx in the nameserver table
 *
 * Returns -1 f none found
 *
 * MULTITHREAD SAFE: Single threaded
 */
static
int findFreeIndex (void)
{
  int i;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(ics_nsrvTable);

  for (i = 0; i < ics_numEntries; i++)
  {
    if (ics_nsrvTable[i].entry == NULL)
      return i;
  }

  /* XXXX Should we grow and copy the table ? */
  return -1;
}

/* 
 * Lookup a given name in the nameserver table
 *
 * Returns the entry if found, NULL otherwise
 *
 * XXXX We now allow multiple registrations of the same name,
 * so we should perhaps handle this by starting the search at
 * the last match each time ?
 *
 * MULTITHREAD SAFE: Single threaded
 */
static
ics_nsrv_entry_t *lookupName (ICS_CHAR *name)
{
  int hidx;
  ics_nsrv_entry_t *nse;

  ICS_UINT hash;
  
  ICS_ASSERT(ics_state);
  ICS_ASSERT(ics_nsrvTable);
  ICS_ASSERT(ics_hashTable);

  /* Find the correct hash list head */
  hash = nameHash(name);
  hidx = (hash % ics_hashEntries);
  
  list_for_each_entry(nse, &ics_hashTable[hidx], hlist)
  {
    if (hash == nse->hvalue && !strcmp (name, nse->name))
      return nse;
  }
  
  /* Not found */
  return NULL;
}

static
ICS_ERROR sendReply (ICS_UINT cpuNum, ICS_PORT replyPort, ICS_ERROR res, ICS_VOID *data, ICS_SIZE size,
		     ICS_NSRV_HANDLE handle)
{
  ICS_ERROR        err;
  ics_nsrv_reply_t reply;

  ICS_PRINTF(ICS_DBG_NSRV, "send reply handle 0x%x size %d to port 0x%x\n", handle, size, replyPort);
  
  reply.type   = _ICS_NSRV_REPLY;
  reply.err    = res;
  reply.handle = handle;

  ICS_assert(size <= ICS_NSRV_MAX_DATA);

  /* Copy data into reply msg */
  if (size)
    _ICS_OS_MEMCPY(&reply.data[0], data, size);

  _ICS_OS_MUTEX_TAKE(&ics_state->lock);

  /* Send an INLINE reply */
  /* Call the internal send message API whilst holding the state lock 
   * This API doesn't require the target CPU to be in the connected state
   */
  err = _ics_msg_send(replyPort, (void *) &reply, ICS_INLINE, offsetof(ics_nsrv_reply_t, data[size]), 
		      ICS_MSG_CONNECT, cpuNum);

  _ICS_OS_MUTEX_RELEASE(&ics_state->lock);
  
  return err;
}

/*
 * Called whenever a new name is registered so that
 * we can scan the pending lookup list and reply
 * to any matching requests
 */
static
void newNameNotify (ICS_CHAR *name, ICS_NSRV_HANDLE handle)
{
  ics_lookup_req_t *tmp, *lr;
  
  int              type, cpuNum, ver, idx;

  ics_nsrv_entry_t *nse;

  ICS_ASSERT(ics_state);

  /* Decode the nsrv entry handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);
  
  ICS_assert(idx >= 0);
  ICS_assert(ics_nsrvTable[idx].entry);
  
  nse = ics_nsrvTable[idx].entry;

  ICS_PRINTF(ICS_DBG_NSRV, "nse %p name '%s' idx %d\n", nse, name, idx);

  /* Walk the lookup request list looking for a match */
  list_for_each_entry_safe(lr, tmp, &ics_lookup_reqs, list)
  {
    if (!strcmp(lr->name, name))
    {
      /* Found a pending request 
       * Need to send the response message back to the original port
       */
      (void) sendReply(lr->srcCpu, lr->replyPort, ICS_SUCCESS, nse->data, nse->size, nse->handle);
       
      /* Remove request from list */
      list_del(&lr->list);

      /* Release lookup desc */
      _ICS_OS_FREE(lr);

      /* lr is released, skip to next entry */
      continue;
    }

    /* Housekeeping.
     * Remove any stale requests which will have now timed out on the client
     */
    if (_ICS_OS_TIME_EXPIRED(lr->reqTime, lr->timeout))
    {
      ICS_PRINTF(ICS_DBG_NSRV, "remove stale req %p '%s' srcCpu %d arrival %ld timeout %ld\n",
		 lr, lr->name, lr->srcCpu, lr->reqTime, lr->timeout);

      /* Remove request from list */
      list_del(&lr->list);

      /* Release lookup desc */
      _ICS_OS_FREE(lr);

      /* lr is released, skip to next entry */
      continue;
    }
  }

  return;
}

/*
 * Register a new named object in the nameserver table
 *
 * Returns the newly allocated entry handle in handlep on success
 */
static
ICS_ERROR registerName (ICS_CHAR *name, ICS_VOID *object, ICS_SIZE size, ICS_UINT srcCpu, ICS_NSRV_HANDLE *handlep)
{
  ics_nsrv_entry_t *nse;
  ICS_UINT          hash;
  int               idx, hidx;

  ICS_ASSERT(ics_state);

  ICS_assert(name);
  ICS_assert(strlen(name) <= ICS_NSRV_MAX_NAME);

  ICS_assert(size <= ICS_NSRV_MAX_DATA);

  /*
   * Create and insert a new object entry
   */

  /* Find a free table entry */
  idx = findFreeIndex();
  if (idx == -1)
  {
    /* XXXX Need to grow & copy table */
    return ICS_ENOMEM;
  }
  
  /* Allocate and zero a new table entry (including object storage space) */
  _ICS_OS_ZALLOC(nse, offsetof(ics_nsrv_entry_t, data[size]));

  /* Initialise the data structure */
  strncpy(nse->name, name, ICS_NSRV_MAX_NAME);
  nse->srcCpu = srcCpu;

  /* Copy the object and save its size */
  nse->size   = size;
  _ICS_OS_MEMCPY(&nse->data[0], object, size);

  INIT_LIST_HEAD(&nse->hlist);

  /* Generate a new nameserver entry handle
   */
  nse->handle = _ICS_HANDLE(_ICS_TYPE_NSRV, ics_state->cpuNum, ics_nsrvTable[idx].version, idx);

  /* Add new entry to table */
  ics_nsrvTable[idx].entry = nse;

  /* Insert into hash table */
  hash = nameHash(name);

  /* Stash hash value to aid lookup */
  nse->hvalue = hash;

  hidx = (hash % ics_hashEntries);
  list_add_tail(&nse->hlist, &ics_hashTable[hidx]);
  
  ICS_PRINTF(ICS_DBG_NSRV, "registered name '%s' size %d srcCpu %d handle 0x%x hidx %d\n",
	     name, size, srcCpu, nse->handle, hidx);

  /* Return handle to caller if requested */
  if (handlep)
    *handlep = nse->handle;

  return ICS_SUCCESS;
}

/*
 * Process an incoming REG request
 */
static
ICS_ERROR registerReq (ICS_UINT srcCpu, ics_nsrv_msg_t *msg, ICS_SIZE rsize)
{
  ICS_ERROR     err, res;

  ICS_CHAR     *name;
  ICS_PORT      replyPort;
  ICS_SIZE      size;

  ICS_NSRV_HANDLE handle = ICS_HANDLE_INVALID;

  name      = msg->name;
  replyPort = msg->replyPort;

  ICS_assert(replyPort != ICS_INVALID_HANDLE_VALUE);

  /* Determine its size by subtracting the nsrv_msg header */
  size  = rsize - offsetof(ics_nsrv_msg_t, data);
    
  ICS_assert(size && size <= ICS_NSRV_MAX_DATA);

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' size %d replyPort 0x%x\n", name, size, replyPort);

  /* Register the new <name,object> 
   */
  res = registerName(name, msg->data, size, srcCpu, &handle);
  if (res != ICS_SUCCESS)
    ICS_EPRINTF(ICS_DBG_NSRV, "Failed to register name '%s' size %d : res %d\n",
		name, size, res);
    
  /* Send a reply back to the caller */
  err = sendReply(srcCpu, replyPort, res, NULL, 0, handle);
  if (err != ICS_SUCCESS)
    ICS_EPRINTF(ICS_DBG_NSRV, "name '%s' : Failed to send reply to port 0x%x srcCpu %d : %d\n",
		name, replyPort, srcCpu, err);
  
  if (res == ICS_SUCCESS)
  {
    /* Now process any pending connect requests */
    newNameNotify(name, handle);
  }

  return err;
}

/*
 * Process an incoming UNREG request
 */
static
ICS_ERROR unregisterReq (ICS_UINT srcCpu, ics_nsrv_msg_t *msg)
{
  ICS_ERROR     err = ICS_SUCCESS;
  ICS_PORT      replyPort;
  
  ICS_NSRV_HANDLE  handle;
  int              type, cpuNum, ver, idx;

  replyPort = msg->replyPort;
  handle    = msg->u.reg.handle;

  ICS_PRINTF(ICS_DBG_NSRV, "handle 0x%x replyPort 0x%x\n", handle, replyPort);

  ICS_assert(replyPort != ICS_INVALID_HANDLE_VALUE);

  /* Decode the nsrv entry handle */
  _ICS_DECODE_HDL(handle, type, cpuNum, ver, idx);

  /* Check the supplied handle */
  if (type != _ICS_TYPE_NSRV || cpuNum >= _ICS_MAX_CPUS || cpuNum != ics_state->cpuNum || idx >= ics_numEntries ||
      ics_nsrvTable[idx].version != ver)
  {
    err = ICS_HANDLE_INVALID;
    goto sendReply;
  }

  if (ics_nsrvTable[idx].entry != NULL)
  {
    removeEntry(idx);
  }
  else
  {
    /* Name not found */
    err = ICS_NAME_NOT_FOUND;
  }

sendReply:
  
  /* Send a reply back to the caller */
  err = sendReply(srcCpu, replyPort, err, NULL, 0, handle);

  if (err != ICS_SUCCESS)
    ICS_EPRINTF(ICS_DBG_NSRV, "Failed to send reply to port 0x%x srcCpu %d : %d\n",
		replyPort, srcCpu, err);

  return err;
}


/*
 * Process an incoming name LOOKUP request
 */
static
ICS_ERROR lookupReq (ICS_UINT srcCpu, ics_nsrv_msg_t *msg)
{
  ICS_ERROR	    err = ICS_SUCCESS;
  int               hidx;
  ICS_CHAR         *name;
  ICS_PORT          replyPort;
  ICS_UINT          flags;
  ICS_ULONG         timeout;

  ics_nsrv_entry_t *nse;
  ics_lookup_req_t *lr;

  name      = msg->name;
  replyPort = msg->replyPort;
  flags     = msg->u.lookup.flags;
  timeout   = msg->u.lookup.timeout;

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' flags 0x%x replyPort 0x%x\n", name, flags, replyPort);

  /* Lookup the name index in the table */
  nse = lookupName(name);

  if (nse != NULL)
  {
    ICS_PRINTF(ICS_DBG_NSRV, "Found nse %p handle 0x%x data %p size %d\n", 
	       nse, nse->handle, nse->data, nse->size);
    
    /* Success, send back the data & handle to requesting port */
    err = sendReply(srcCpu, replyPort, ICS_SUCCESS, nse->data, nse->size, nse->handle);
    if (err != ICS_SUCCESS)
      goto error_reply;
    
    /* 
     * Relink at end of hash chain to ensure round-robin order
     */

    /* Find head of hash list */
    hidx = (nse->hvalue % ics_hashEntries);
    
    /* Unlink & re-link at tail */
    list_del(&nse->hlist);
    list_add_tail(&nse->hlist, &ics_hashTable[hidx]);
    
    return ICS_SUCCESS;
  }

  ICS_PRINTF(ICS_DBG_NSRV, "name '%s' replyPort 0x%x: Not found\n", name, replyPort);

  if ((flags & ICS_BLOCK))
  {
    /* Failed to find the registered name. We now need to remember this
     * lookup request so that it can be completed once the registration occurs
     */
    
    /* Allocate a new lookup req data structure */
    _ICS_OS_ZALLOC(lr, sizeof(*lr));
    if (lr == NULL)
    {
      return ICS_ENOMEM;
    }
    
    /* Initialise the lookup req data structure */
    strncpy(lr->name, name, ICS_NSRV_MAX_NAME);
    lr->replyPort = replyPort;
    lr->srcCpu    = srcCpu;
    lr->timeout   = timeout;
    lr->reqTime   = _ICS_OS_TIME_NOW();		/* Timestamp arrival */

    INIT_LIST_HEAD(&lr->list);
    
    /* Insert onto the request list */
    list_add(&lr->list, &ics_lookup_reqs);
  }
  else
  {
    /* 
     * Failed a non blocking request. Send error reply
     */
    err = sendReply(srcCpu, replyPort, ICS_NAME_NOT_FOUND, NULL, 0, ICS_INVALID_HANDLE_VALUE);
    if (err != ICS_SUCCESS)
      goto error_reply;
  }

  return ICS_SUCCESS;

error_reply:
  ICS_EPRINTF(ICS_DBG_NSRV, "name '%s' : Failed to send reply to port 0x%x srcCpu %d : %d\n",
	      name, replyPort, srcCpu, err);
  
  return err;
}


/* 
 * Called during local cpu shutdown to free off any
 * pending lookup requests
 */
static
void removeAllRequests (void)
{
  ics_lookup_req_t *tmp, *lr;
  
  ICS_ASSERT(ics_state);

  /* Walk the lookup request list freeing all requests */
  list_for_each_entry_safe(lr, tmp, &ics_lookup_reqs, list)
  {
    /* Send NACK to caller */
    (void) sendReply(lr->srcCpu, lr->replyPort, ICS_NAME_NOT_FOUND, NULL, 0, ICS_INVALID_HANDLE_VALUE);
    
    /* Remove request from list */
    list_del_init(&lr->list);

    /* Free list entry memory */
    _ICS_OS_FREE(lr);
  }

  ICS_assert(list_empty(&ics_lookup_reqs));

  return;
}

/* 
 * Called from cpuDown() to also remove any
 * pending lookup requests from the given cpu
 */
static
void removeRequests (ICS_UINT cpuNum, ICS_UINT flags)
{
  ics_lookup_req_t *tmp, *lr;
  
  ICS_ASSERT(ics_state);

  /* Walk the lookup request list freeing any request from the given cpu */
  list_for_each_entry_safe(lr, tmp, &ics_lookup_reqs, list)
  {
    if (lr->srcCpu != cpuNum)
      continue;

    /* Send NACK to caller (in orderly shutdown case) */
    if (!(flags & ICS_CPU_DEAD))
      (void) sendReply(lr->srcCpu, lr->replyPort, ICS_NAME_NOT_FOUND, NULL, 0, ICS_INVALID_HANDLE_VALUE);

    /* Remove request from list */
    list_del_init(&lr->list);

    /* Free list entry memory */
    _ICS_OS_FREE(lr);
  }

  return;
}

/*
 * Called to inform the nameserver that a CPU
 * is going down or is dead, and hence any objects owned by it should
 * now be removed
 */
static
ICS_ERROR cpuDown (ICS_UINT srcCpu, ics_nsrv_msg_t *msg)
{
  ICS_ERROR err;
  int       idx;
  ICS_PORT  replyPort;
  ICS_UINT  cpuNum;
  ICS_UINT  flags;

  ICS_ASSERT(ics_state);
  ICS_ASSERT(ics_nsrvTable);

  replyPort = msg->replyPort;
  cpuNum    = msg->u.cpu_down.cpuNum;
  flags     = msg->u.cpu_down.flags;

  ICS_assert(cpuNum < _ICS_MAX_CPUS);
  ICS_assert(replyPort != ICS_INVALID_HANDLE_VALUE);
  
  for (idx = 0; idx < ics_numEntries; idx++)
  {
    ics_nsrv_entry_t *nse;

    nse = ics_nsrvTable[idx].entry;
    
    /* Find and free off any related entries */
    if (nse && nse->srcCpu == cpuNum)
    {
      removeEntry(idx);
    }
  }

  /* Now remove all pending lookup requests */
  removeRequests(cpuNum, flags);

  /* Send acknowledge to caller */
  err = sendReply(srcCpu, replyPort, ICS_SUCCESS, NULL, 0, ICS_INVALID_HANDLE_VALUE);

  if (err != ICS_SUCCESS)
    ICS_EPRINTF(ICS_DBG_NSRV, "Failed to send reply to replyPort 0x%x srcCpu %d : %s (%d)\n",
		srcCpu, replyPort,
		ics_err_str(err), err);

  return err;
}

/*
 * Main ICS Nameserver task which has to handle
 * all incoming creation and administration requests
 */
void
ics_nsrv_task (void *param)
{
  ICS_ERROR     err = ICS_SUCCESS;
  ICS_MSG_DESC  rdesc;
  ICS_PORT      port;

  ICS_ASSERT(ics_state);

  ICS_assert(ics_state->cpuNum == _ICS_NSRV_CPU);

  ICS_assert(sizeof(ics_nsrv_msg_t) <= ICS_MSG_INLINE_DATA);	/* XXXX INLINE messages only */

  /* The local port was created in ics_nsrv_init() */
  port = ics_state->nsrvPort;

  ICS_assert(port != ICS_INVALID_HANDLE_VALUE);
  
  ICS_PRINTF(ICS_DBG_NSRV, "Starting: state %p port 0x%x\n", ics_state, port);

  /* Initialise the nameserver tables */
  initTable(_ICS_NSRV_NUM_ENTRIES, _ICS_NSRV_HASH_ENTRIES);

  /* Bootstrap by registering the Nameserver port */
  err = registerName(_ICS_NSRV_PORT_NAME, &port, sizeof(port), ics_state->cpuNum, NULL);
  ICS_assert(err == ICS_SUCCESS);

  while (1)
  {
    ics_nsrv_msg_t *msg;
    
    /* Post a blocking receive */
    err = ICS_msg_recv(port, &rdesc, ICS_TIMEOUT_INFINITE);

    if (err == ICS_PORT_CLOSED)
      /* Port closed - Exit */
      break;
    
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_NSRV, "ICS_msg_receiver returned error : %s %d\n",
		  ics_err_str(err), err);
      
      /* XXXX What should we do here */
      ICS_assert(0);
    }

    ICS_assert(rdesc.data);
    ICS_assert(rdesc.size);

    msg = (ics_nsrv_msg_t *) rdesc.data;
    switch (msg->type)
    {
    case _ICS_NSRV_LOOKUP:
      err = lookupReq(rdesc.srcCpu, msg);
      break;

    case _ICS_NSRV_REG:
      err = registerReq(rdesc.srcCpu, msg, rdesc.size);
      break;

    case _ICS_NSRV_UNREG:
      err = unregisterReq(rdesc.srcCpu, msg);
      break;

    case _ICS_NSRV_CPU_DOWN:
      err = cpuDown(rdesc.srcCpu, msg);
      break;

    default:
      ICS_EPRINTF(ICS_DBG_NSRV, "port 0x%x msg %p size %d unknown req 0x%x\n",
		  port, rdesc.data, rdesc.size, msg->type);
      /* XXXX What should we do here ? */
      ICS_assert(0);
    }

    if (err != ICS_SUCCESS)
      ICS_EPRINTF(ICS_DBG_NSRV, "Failed : req 0x%x : %s (%d)\n", msg->type, 
		  ics_err_str(err), err);
  }

  /* Exiting so release all resources */
  termTable();
 
  /* Delete any pending lookup requests */
  removeAllRequests();
  
  ICS_PRINTF(ICS_DBG_NSRV, "exit : %s (%d)\n",
	     ics_err_str(err), err);

  return;
}

/*
 * Create and start the nameserver task 
 */
static
ICS_ERROR createNameserverTask (void)
{
  ICS_ERROR err;
  
  char taskName[_ICS_OS_MAX_TASK_NAME];

  ICS_ASSERT(ics_state);

  snprintf(taskName, _ICS_OS_MAX_TASK_NAME, "ics_nsrv");

  /* Create a high priority thread for servicing these messages */
  err = _ICS_OS_TASK_CREATE(ics_nsrv_task, NULL, _ICS_OS_TASK_DEFAULT_PRIO, taskName, &ics_state->nsrvTask);
  if (err != ICS_SUCCESS)
  {
    ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "Failed to create Nameserver task %s : %d\n", taskName, err);
    
    return ICS_NOT_INITIALIZED;
  }
  
  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "created Nameserver task %p\n", ics_state->nsrvTask.task);

  return ICS_SUCCESS;
}

/*
 * Stop the nameserver task and free off resources
 */
void ics_nsrv_term (void)
{
  ICS_ASSERT(ics_state);

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "Shutting down cpu %d\n", ics_state->cpuNum);

  /* Unregister any names we may have registered in the Nameserver */
  (void) ics_nsrv_cpu_down(ics_state->cpuNum, 0);

  if (ics_state->cpuNum == _ICS_NSRV_CPU)
  {
    /* Closing this port will wake the nameserver thread */
    if (ics_state->nsrvPort != ICS_INVALID_HANDLE_VALUE)
    {
      ICS_port_free(ics_state->nsrvPort, 0);
      ics_state->nsrvPort = ICS_INVALID_HANDLE_VALUE;

      /* Delete the nameserver task */
      _ICS_OS_TASK_DESTROY(&ics_state->nsrvTask);
    }
  }

  return;
}


ICS_ERROR ics_nsrv_init (ICS_UINT cpuNum)
{
  ICS_ERROR err = ICS_SUCCESS;

  if (cpuNum == _ICS_NSRV_CPU)
  {
    /* Create the local Nameserver port 
     * We create it as a local port as the Nameserver task is not
     * up and running yet. Once it gets started it will register
     * the port itself as part of its bootstrap
     */
    err = ICS_port_alloc(NULL, NULL, NULL, _ICS_NSRV_PORT_NDESC, 0, &ics_state->nsrvPort);
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "Failed to create Nameserver port '%s'\n",
		  _ICS_NSRV_PORT_NAME);
      
      goto error;
    }

    /* Nameserver must be Port idx #1 (adminPort is #0) */
    ICS_assert(_ICS_HDL2IDX(ics_state->nsrvPort) == 1);

    /* Create the Nameserver task */
    err = createNameserverTask();
    if (err != ICS_SUCCESS)
    {
      /* Failed to create the nameserver task */
      ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "Failed to create Nameserver task : %d'\n", 
		  err);
      
      goto error;
    }
  }
  else
  {
    /* Chicken&Egg: We can't lookup the Nsrv Port as that would involve sending a
     * message to it. So we generate it directly assuming a Port index of 1
     */
    ics_state->nsrvPort = _ICS_HANDLE(_ICS_TYPE_PORT, _ICS_NSRV_CPU, 0 /* Version */, 1);
  }

  ICS_ASSERT(ics_state->nsrvPort != ICS_INVALID_HANDLE_VALUE);

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "nsrvPort 0x%x\n",
	     ics_state->nsrvPort);

  return ICS_SUCCESS;

error:
  /* Tidy up */
  ics_nsrv_term();

  ICS_EPRINTF(ICS_DBG_INIT|ICS_DBG_NSRV, "Failed : %s (%d)\n",
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
