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

#include <bsp/_bsp.h>

typedef struct ics_interruptHandler
{
  ICS_MAILBOX_FN  handler;
  void           *param;
} ics_interruptHandler_t;

typedef struct ics_localMailboxSet
{
  struct ics_localMailboxSet	*next;
  ICS_UINT			 numValid;
  int		                 intLevel;
  int		                 intNumber;
#if defined __os21__
  interrupt_t		       	*intHandle;
#endif
  ics_interruptHandler_t	 intTable[_ICS_MAILBOX_NUM];
  ics_mailbox_t			*mailboxes[_ICS_MAILBOX_NUM];
  ICS_BOOL			 inUse[_ICS_MAILBOX_NUM];
} ics_localMailboxSet_t;

typedef struct ics_remoteMailboxSet
{
  struct ics_remoteMailboxSet   *next;
  ICS_UINT                       numValid;
  ics_mailbox_t			*mailboxes[_ICS_MAILBOX_NUM];
} ics_remoteMailboxSet_t;

typedef struct ics_mailboxLockSet
{
  struct ics_mailboxLockSet     *next;
  ICS_UINT                       inUseMask;
  ics_mailbox_lock_t             *spinlocks[_ICS_MAILBOX_LOCKS];
} ics_mailboxLockSet_t;

enum 
{
  /* absolute hw mbox offsets */
  _ICS_MBOX_OFFSET		= 0x004 / 4,	/* Offset of 1st mbox register */
  _ICS_MBOX_LOCKS		= 0x200 / 4,	/* Offset of 1st lock register */
  
  /* deltas between types of register */
  _ICS_MBOX_SIZEOF		= 0x004 / 4,
  _ICS_MBOX_LOCK_SIZEOF		= 0x004 / 4,

  /* offsets of various registers from an ics_mailbox_t * */
  _ICS_MBOX_STATUS		= 0x000 / 4,
  _ICS_MBOX_STATUS_SET		= 0x020 / 4,
  _ICS_MBOX_STATUS_CLEAR  	= 0x040 / 4,
  _ICS_MBOX_ENABLE        	= 0x060 / 4,
  _ICS_MBOX_ENABLE_SET		= 0x080 / 4,
  _ICS_MBOX_ENABLE_CLEAR	= 0x0a0 / 4
};

/*
 * static driver state
 */
static ICS_BOOL                  _alreadyInitialized;
static ics_localMailboxSet_t    *_mailboxLocalHead;
static _ICS_OS_MUTEX             _mailboxLocalLock;
static ics_remoteMailboxSet_t   *_mailboxRemoteHead;
static _ICS_OS_MUTEX             _mailboxRemoteLock;
static ics_mailboxLockSet_t     *_mailboxSpinlockHead;
static _ICS_OS_MUTEX             _mailboxSpinlockLock;

/* An array to store the mapped BSP mailbox addresses */
static void                     *_bspMailboxes[_ICS_MAX_CPUS*2];

static
ICS_ERROR bspInit (void)
{
  int i;

  /* 
   * Walk the BSP mailbox table, registering each mailbox
   */
  for (i = 0; i < bsp_mailbox_count; i++)
  {
    ICS_ERROR err;
    int intNum;

    /* Assert we don't exceed the mapping array */
    ICS_assert(bsp_mailbox_count <= ARRAY_SIZE(_bspMailboxes));

#if defined __os21__
    /* OS21 the supplied interrupt is a pointer to an interrupt name */
    intNum = (bsp_mailboxes[i].interrupt) ? (int) *(interrupt_name_t *)bsp_mailboxes[i].interrupt : 0;
#else
    intNum = bsp_mailboxes[i].interrupt;
#endif

    /* Must map the physical Mailbox register addresses into (uncached) kernel space */
    _bspMailboxes[i] = _ICS_OS_MMAP((unsigned long)bsp_mailboxes[i].base, _ICS_OS_PAGESIZE, ICS_FALSE);
      
    if (_bspMailboxes[i] == NULL)
      return ICS_SYSTEM_ERROR;

    ICS_PRINTF(ICS_DBG_MAILBOX, "register mbox[%d] %p/%p flags 0x%x\n", 
	       i, bsp_mailboxes[i].base, _bspMailboxes[i], bsp_mailboxes[i].flags);
     
    /* Register a platform specific hardware mailbox associating it with an OS interrrupt */
    err = ics_mailbox_register(_bspMailboxes[i], intNum, -1, bsp_mailboxes[i].flags);
    
    if (err != ICS_SUCCESS)
    {
      ICS_EPRINTF(ICS_DBG_MAILBOX, 
		  "BSP mailbox[%d] base %p/%p intNum %d flags 0x%x register failed : %s (%d)\n",
		  i, bsp_mailboxes[i].base, _bspMailboxes[i],
		  intNum, bsp_mailboxes[i].flags,
		  ics_err_str(err), err);

      return err;
    }
  }

  return ICS_SUCCESS;
}

static
void bspTerm (void)
{
  int i;
  
  /* 
   * Walk the BSP mailbox table, unmapping the mailbox mappings
   */
  for (i = 0; i < bsp_mailbox_count; i++)
  {
    /* Unmap the physical mailbox address mapping */
    _ICS_OS_MUNMAP(_bspMailboxes[i]);
  }
}

/*
 * interrupt handler to broker multiple interrupt handlers
 */
#if defined __KERNEL__
static irqreturn_t interruptDispatcher (int irq, void *param)
#elif defined __os21__
#define IRQ_HANDLED OS21_SUCCESS
#define IRQ_NONE OS21_FAILURE
  static int interruptDispatcher(void *param)
#else
  static int interruptDispatcher(void *param)
#endif
{
  int res = IRQ_NONE;

  ics_localMailboxSet_t *lms = (ics_localMailboxSet_t *) param;
  int i;

  for (i=0; i<_ICS_MAILBOX_NUM; i++) 
  {
    /* Examine all the active mailboxes */
    if (lms->inUse[i]) 
    {
      ICS_UINT enables = lms->mailboxes[i][_ICS_MBOX_ENABLE];
      ICS_UINT status  = lms->mailboxes[i][_ICS_MBOX_STATUS];
      
      /* Find which one triggered the interrupt
       * and call the associated handler with the associated
       * parameter and the masked mailbox status register
       */
      if (enables & status)
      {
	lms->intTable[i].handler(lms->intTable[i].param, (enables & status));
	res = IRQ_HANDLED;
      }
    }
  }

  return res;
}

/*
 * register a mailbox capable of generating interrupts on this CPU and
 * attach a primary handler to that interrupt
 */
static ICS_ERROR registerLocalMailbox (ICS_UINT *localBase, 
				       ICS_UINT numMailboxes, 
				       ICS_UINT intNumber, 
				       ICS_UINT intLevel)
{
  ics_localMailboxSet_t *lms;
  unsigned int i;
  int res;

  ICS_PRINTF(ICS_DBG_MAILBOX, "localBase %p numMailboxes %d intNumber 0x%x\n",
	     localBase, numMailboxes, intNumber);

  ICS_assert(localBase);
  ICS_assert(numMailboxes);

  lms = (ics_localMailboxSet_t *) _ICS_OS_MALLOC(sizeof(ics_localMailboxSet_t));
  if (!lms) {
    return ICS_ENOMEM;
  }

  lms->numValid = numMailboxes;
  lms->intLevel = intLevel;
  lms->intNumber = intNumber;

  for (i=0; i<_ICS_MAILBOX_NUM; i++)
  {
    lms->intTable[i].handler = NULL;
    lms->intTable[i].param   = (void *) lms;
    lms->mailboxes[i]        = (ics_mailbox_t *) (localBase + (i * _ICS_MBOX_SIZEOF));

    if (i<numMailboxes) {
      /* lms->mailboxes[i][_ICS_MBOX_ENABLE] = 0; */
      lms->mailboxes[i][_ICS_MBOX_STATUS] = 0;
    }

    lms->inUse[i]            = !(i < numMailboxes);
  }

#if defined __os21__
  lms->intHandle = interrupt_handle(intNumber);
  if(!lms->intHandle)
  {
    _ICS_OS_FREE((void *) lms);
    return ICS_INVALID_ARGUMENT;
  }

  res  = interrupt_install_shared(lms->intHandle, interruptDispatcher, (void *) lms);
  res += interrupt_enable(lms->intHandle);
  ICS_assert(0 == res);
#elif defined __sh__ && defined __KERNEL__
  res = request_irq(intNumber, interruptDispatcher, 0, "mailbox", (void *) lms);
  ICS_assert(0 == res);
#elif defined (__arm__) && defined __KERNEL__
  res = request_irq(intNumber, interruptDispatcher, 0, "mailbox", (void *) lms);
  ICS_assert(0 == res);
#else
#error Unsupported OS/CPU
#endif

  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);
  lms->next = _mailboxLocalHead;
  _mailboxLocalHead = lms;
  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);

  return ICS_SUCCESS;
}

static 
void deregisterLocalMailbox (ICS_UINT *localBase)
{
  ics_localMailboxSet_t *prev, *lms;

  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);

  /* hunt for the correct mailbox */
  for (prev=NULL, lms=_mailboxLocalHead;
       lms && (localBase != (ICS_UINT *) (lms->mailboxes[0]));
       prev=lms, lms=prev->next) {}
	
  /* if we found a matching element then unlink it from
   * the list, remove its interrupt handler and free its
   * storage
   */
  if (lms) {
    *(prev ? &prev->next : &_mailboxLocalHead) = lms->next;

#if defined __os21__
    interrupt_disable(lms->intHandle);
    interrupt_uninstall(lms->intHandle);
#elif defined __sh__ && defined __KERNEL__
    free_irq(lms->intNumber, (void *) lms);
#elif defined (__arm__) && defined __KERNEL__
    free_irq(lms->intNumber, (void *) lms);
#else
#error Unsupported CPU/OS
#endif

    _ICS_OS_FREE((ICS_VOID *) lms);
  }
  
  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);

}

static 
ICS_ERROR findLocalMailbox (ics_mailbox_t *mailbox, ics_localMailboxSet_t **pSet, int *pId)
{
  ics_localMailboxSet_t *lms;
  int i;

  /* This function must be called with the mailbox local lock taken */
  for (lms = _mailboxLocalHead; lms; lms = lms->next) 
  {
    for (i=0; i<_ICS_MAILBOX_NUM; i++)
    {
      if (lms->mailboxes[i] == mailbox) 
      {
	*pSet = lms;
	*pId  = i;
	return ICS_SUCCESS;
      }
    }
  }

  return ICS_INVALID_ARGUMENT;
}

static
ICS_ERROR registerRemoteMailbox (ICS_UINT *remoteBase, ICS_UINT numMailboxes)
{
  ics_remoteMailboxSet_t *rms;
  int i;

  ICS_assert(remoteBase);
  ICS_assert(numMailboxes);

  rms = (ics_remoteMailboxSet_t *) _ICS_OS_MALLOC(sizeof(ics_remoteMailboxSet_t));
  if (!rms)
    return ICS_ENOMEM;

  rms->numValid = numMailboxes;
  for (i=0; i<_ICS_MAILBOX_NUM; i++)
  {
    rms->mailboxes[i] = (ics_mailbox_t *) (remoteBase + (i * _ICS_MBOX_SIZEOF));
  }

  _ICS_OS_MUTEX_TAKE(&_mailboxRemoteLock);
  rms->next = _mailboxRemoteHead;
  _mailboxRemoteHead = rms;
  _ICS_OS_MUTEX_RELEASE(&_mailboxRemoteLock);

  return ICS_SUCCESS;
}

static
void deregisterRemoteMailbox (ICS_UINT *remoteBase)
{
  ics_remoteMailboxSet_t *prev, *curr;

  _ICS_OS_MUTEX_TAKE(&_mailboxRemoteLock);

  /* hunt for the correct mailbox */
  for (prev=NULL, curr=_mailboxRemoteHead;
       curr && (remoteBase != (ICS_UINT *) (curr->mailboxes[0]));
       prev=curr, curr=prev->next) {}
	
  /* if we found a matching element then unlink it from
   * the list and free its storage
   */
  if (curr)
  {
    *(prev ? &prev->next : &_mailboxRemoteHead) = curr->next;
    _ICS_OS_FREE((ICS_VOID *) curr);
  }

  _ICS_OS_MUTEX_RELEASE(&_mailboxRemoteLock);
}

/*
 * Register a set of hw lock addresses
 */
static 
ICS_ERROR registerSpinlocks (ICS_UINT *lockBase)
{
  ics_mailboxLockSet_t *mls;
  int i;
  
  ICS_ASSERT(lockBase);
  
  mls = (ics_mailboxLockSet_t *) _ICS_OS_MALLOC(sizeof(ics_mailboxLockSet_t));
  if (!mls)
    return ICS_ENOMEM;
  
  /* There are 32 hw lock registers per lock block */
  mls->inUseMask = 0;
  for (i=0; i<_ICS_MAILBOX_LOCKS; i++)
  {
    mls->spinlocks[i] = (ics_mailbox_lock_t *) (lockBase + (i * _ICS_MBOX_LOCK_SIZEOF));
  }
  
  _ICS_OS_MUTEX_TAKE(&_mailboxSpinlockLock);
  mls->next = _mailboxSpinlockHead;
  _mailboxSpinlockHead = mls;
  _ICS_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
  
  return ICS_SUCCESS;
}

/*
 * Deregister a set of hw lock addresses
 */
static
void deregisterSpinlocks (ICS_UINT *lockBase)
{
  ics_mailboxLockSet_t *prev, *curr;
  
  _ICS_OS_MUTEX_TAKE(&_mailboxSpinlockLock);
  
  /* hunt for the correct lockbase */
  for (prev=NULL, curr=_mailboxSpinlockHead; 
       curr && (lockBase != (ICS_UINT *) (curr->spinlocks[0]));
       prev=curr, curr=prev->next) 
    ;
  
  /* if we found a matching element then unlink it from
   * the list and free its storage
   */
  if (curr)
  {
    *(prev ? &prev->next : &_mailboxSpinlockHead) = curr->next;
    _ICS_OS_FREE((ICS_VOID *) curr);
  }
  
  _ICS_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
}

/*
 * Lookup the mailbox lock set and index of the given hw lock address
 *
 * Called holding the Spinlocks mutex
 */
static 
ICS_ERROR findSpinlock (ics_mailbox_lock_t *spinlock, ics_mailboxLockSet_t **pSet, int *pId)
{
  ics_mailboxLockSet_t *mls;
  
  for (mls = _mailboxSpinlockHead; mls; mls = mls->next) 
  {
    int i;

    for (i=0; i<_ICS_MAILBOX_LOCKS; i++) 
    {
      if (mls->spinlocks[i] == spinlock)
      {
	*pSet = mls;
	*pId  = i;
	return ICS_SUCCESS;
      }
    }
  }
  
  return ICS_INVALID_ARGUMENT;
}

/*
 * scan all remote mailboxes looking for the magic token (using a mask)
 * in the ENABLE register
 */
static
ics_mailbox_t *scanRemoteMailboxes (ICS_UINT token, ICS_UINT mask)
{
  ics_remoteMailboxSet_t *rms;

  /* This function must be called with the mailbox remote lock taken */

  for (rms = _mailboxRemoteHead; rms; rms = rms->next)
  {
    unsigned int i;

    for (i=0; i<rms->numValid; i++)
    {
      /* Ignore any mailboxes whose value is zero */
      if (rms->mailboxes[i][_ICS_MBOX_ENABLE] && 
	  (token == (rms->mailboxes[i][_ICS_MBOX_ENABLE] & mask)))
      {
	return rms->mailboxes[i];
      }
    }
  }
  
  return NULL;
}

static 
ICS_ERROR setInterruptState (ics_mailbox_t *mailbox, ICS_MAILBOX_FN handler, void *param)
{
  ICS_ERROR err;
  ics_localMailboxSet_t *lms;
  int id;

  /* This function must be called with the mailbox local lock taken */

  err = findLocalMailbox(mailbox, &lms, &id);

  if (ICS_SUCCESS == err)
  {
    lms->intTable[id].handler = handler;
    lms->intTable[id].param   = param;
  }

  return err;
}

/*
 * This function registers a set of mailbox registers (e.g. eight 
 * enable/status pairs). This set must the be the set of registers that
 * can generate an interrupt on the local processor. In order that the
 * mailbox manager can handle the interrupt it must be notified of the
 * interrupt number and (depending on the CPU) the interrupt level.
 *
 * THIS FUNCTION IS NOT THREAD-SAFE
 *
 * Errors:
 *   ICS_ENOMEM
 *   ICS_ALREADY_INITIALIZED
 */
ICS_ERROR ics_mailbox_init (void)
{
  ICS_ERROR res;

  if (_alreadyInitialized) {
    return ICS_ALREADY_INITIALIZED;
  }

  if (!_ICS_OS_MUTEX_INIT(&_mailboxLocalLock)) {
    return ICS_ENOMEM;
  }

  if (!_ICS_OS_MUTEX_INIT(&_mailboxRemoteLock)) {
    _ICS_OS_MUTEX_DESTROY(&_mailboxLocalLock);
    return ICS_ENOMEM;
  }

  if (!_ICS_OS_MUTEX_INIT(&_mailboxSpinlockLock)) {
    _ICS_OS_MUTEX_DESTROY(&_mailboxLocalLock);
    _ICS_OS_MUTEX_DESTROY(&_mailboxRemoteLock);
    return ICS_ENOMEM;
  }

  /* Read the BSP mbox table */
  res = bspInit();

  _alreadyInitialized = ICS_TRUE;

  return res;
}

void ics_mailbox_term (void)
{
  ics_localMailboxSet_t  *lms, *nlms;
  ics_remoteMailboxSet_t *rms, *nrms;
  ics_mailboxLockSet_t   *mls, *nmls;

  if (!_alreadyInitialized)
    return;

  /* Deregister all of the local mailboxes */
  for (lms = _mailboxLocalHead; lms != NULL; lms = nlms)
  {
    nlms = lms->next;
    deregisterLocalMailbox ((ICS_UINT *) lms->mailboxes[0]);
  }

  /* Deregister all of the remote mailboxes */
  for (rms = _mailboxRemoteHead; rms != NULL; rms = nrms)
  {
    nrms = rms->next;
    deregisterRemoteMailbox ((ICS_UINT *)rms->mailboxes[0]);
  }

  /* Deregister all of the spin locks */
  for (mls = _mailboxSpinlockHead; mls != NULL; mls = nmls)
  {
    nmls = mls->next;
    deregisterSpinlocks ((ICS_UINT *)mls->spinlocks[0]);
  }

  /* Destroy all of the locks created in ics_mailbox_init() */
  _ICS_OS_MUTEX_DESTROY(&_mailboxLocalLock);
  _ICS_OS_MUTEX_DESTROY(&_mailboxRemoteLock);
  _ICS_OS_MUTEX_DESTROY(&_mailboxSpinlockLock);

  /* Release the BSP mappings */
  bspTerm();
  
  /* Indicate that we are not initialised */
  _alreadyInitialized = ICS_FALSE;

  return;
}

ICS_ERROR ics_mailbox_register (void *pMailbox, int intNumber, int intLevel, unsigned long flags)
{
  ICS_UINT  *set, *locks;
  ICS_ERROR  err = ICS_SUCCESS;

  ICS_PRINTF(ICS_DBG_MAILBOX, "pMailbox %p intNumber 0x%x flags 0x%x\n",
	     pMailbox, intNumber, flags);

  ICS_assert(pMailbox);

  set = (ICS_UINT *) pMailbox + _ICS_MBOX_OFFSET;

  locks = (ICS_UINT *) pMailbox + _ICS_MBOX_LOCKS;

  /* A non zero intNumber signals a local mbox */
  if (intNumber)
  {
    err = registerLocalMailbox(set, _ICS_MAILBOX_NUM, intNumber, intLevel);
  
    /* Now register all the local locks */
    if (err == ICS_SUCCESS)
      err = registerSpinlocks(locks);
  }
  else
  {
    err = registerRemoteMailbox(set, _ICS_MAILBOX_NUM);
  }

  if (err != ICS_SUCCESS)
    /* Something went wrong */
    ics_mailbox_deregister(pMailbox);

  return err;
}

/*
 * This is a simple function to reclaim any memory allocated by the mailbox
 * manager.
 */
ICS_VOID ics_mailbox_deregister (void *pMailbox)
{
  ICS_UINT *set, *locks;

  set   = (ICS_UINT *) pMailbox + _ICS_MBOX_OFFSET;
  locks = (ICS_UINT *) pMailbox + _ICS_MBOX_LOCKS;

  /* Attempt to deregister as both a local and a remote mailbox
   * as we don't know which it is
   */
  deregisterLocalMailbox(set);
  deregisterRemoteMailbox(set);

  /* Deregister any lock locks too */
  deregisterSpinlocks(locks);
}

/*
 * Allocate a local mailbox register to a particular task. This function will
 * reserve a register and attach a user supplied handler to it. This handler
 * will be called whenever that register is the cause of an interupt.
 *
 * After allocation the mailbox registers will have its status set to zero
 * and have no bits enabled.
 *
 * Errors:
 *   ICS_INVALID_STATE
 *   ICS_ENOMEM
 */
ICS_ERROR ics_mailbox_alloc (void (*handler)(void *param, unsigned int), void *param, ics_mailbox_t **pMailbox)
{
  ics_localMailboxSet_t *lms;

  ICS_assert(handler);
  ICS_assert(pMailbox);

  ICS_PRINTF(ICS_DBG_MAILBOX, "handler %p param %p pmbox %p\n", handler, param, pMailbox);

  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);
  for (lms = _mailboxLocalHead; lms; lms = lms->next) 
  {
    int i;

    for (i=0; i<_ICS_MAILBOX_NUM; i++)
    {
      if (!(lms->inUse[i])) 
      {
	lms->inUse[i] = ICS_TRUE;
	_ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);
			
	lms->intTable[i].handler = handler;
	lms->intTable[i].param   = param;

	/* lms->mailboxes[i][_ICS_MBOX_ENABLE] = 0; */
	lms->mailboxes[i][_ICS_MBOX_STATUS] = 0;

	*pMailbox = lms->mailboxes[i];

	ICS_PRINTF(ICS_DBG_MAILBOX, "  *mbox=%p\n", *pMailbox);

	return ICS_SUCCESS;
      }
    }
  }
  
  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);
  return ICS_ENOMEM;
}

/*
 * Deallocate a local or remote mailbox register. Free any associated memory 
 * and for local mailboxes desensitize the mailbox to all interrupts.
 */
ICS_VOID ics_mailbox_free (ics_mailbox_t *mailbox)
{
  ics_localMailboxSet_t *lms;
  int id;

  /* if this is a local mailbox then mark it as no longer in use
   * and suppress any futher interrupts
   */
  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);
  if (findLocalMailbox(mailbox, &lms, &id) == ICS_SUCCESS)
  {
    ICS_assert(lms->inUse[id]);
    
    /* clear the in use flag and desensitize the mailbox to all
     * interrupts
     */
    lms->inUse[id] = ICS_FALSE;
    lms->mailboxes[id][_ICS_MBOX_ENABLE_CLEAR] = (ICS_UINT) -1;
    lms->intTable[id].handler = NULL;
    lms->intTable[id].param = NULL;
  }
  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);

  /* there are no resources to free for a remote mailbox */
}

/*
 * Scan all the remote mailboxes looking for a matching token taking into account a mask value
 */
ICS_ERROR ics_mailbox_find (ICS_UINT token, ICS_UINT mask, ics_mailbox_t **pMailbox)
{
  ics_mailbox_t *rmbox;

  if (!(rmbox = scanRemoteMailboxes(token, mask)))
      return ICS_NOT_INITIALIZED;
  
  *pMailbox = rmbox;

  return ICS_SUCCESS;
}

/*
 * Dump out all local and remote mailboxes to aid debugging
 */
void
ics_mailbox_dump (void)
{
  ics_localMailboxSet_t  *lms;
  ics_remoteMailboxSet_t *rms;

  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);

  for (lms = _mailboxLocalHead; lms; lms = lms->next) 
  {
    int i;

    for (i=0; i<_ICS_MAILBOX_NUM; i++)
    {
      ics_mailbox_t *mailbox = lms->mailboxes[i];
      
      ICS_EPRINTF(ICS_DBG_MAILBOX, "local mbox  %p : enable 0x%08x status 0x%08x\n",
		  mailbox, mailbox[_ICS_MBOX_ENABLE], mailbox[_ICS_MBOX_STATUS]);
    }
  }

  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);

  _ICS_OS_MUTEX_TAKE(&_mailboxRemoteLock);

  for (rms = _mailboxRemoteHead; rms; rms = rms->next)
  {
    unsigned int i;

    for (i=0; i<rms->numValid; i++)
    {
      ics_mailbox_t *mailbox = rms->mailboxes[i];
      
      ICS_EPRINTF(ICS_DBG_MAILBOX, "remote mbox %p : enable 0x%08x status 0x%08x\n",
		  mailbox, mailbox[_ICS_MBOX_ENABLE], mailbox[_ICS_MBOX_STATUS]);
    }
  }

  _ICS_OS_MUTEX_RELEASE(&_mailboxRemoteLock);
  
  return;
}

/*
 * Change the current interrupt handler (which must be installed when 
 * the mailbox is allocated) to the supplied replacement.
 */
ICS_ERROR ics_mailbox_update_interrupt_handler (ics_mailbox_t *mailbox, 
						ICS_MAILBOX_FN handler,
						void *param)
{
  ICS_ERROR err;

  _ICS_OS_MUTEX_TAKE(&_mailboxLocalLock);
  err = setInterruptState(mailbox, handler, param);
  _ICS_OS_MUTEX_RELEASE(&_mailboxLocalLock);

  return err;
}

/*
 * Sensitize the supplied mailbox (which is usually local) to the supplied
 * bit number.
 */
ICS_VOID ics_mailbox_interrupt_enable (ics_mailbox_t *mailbox, ICS_UINT bit)
{
  ICS_ASSERT(mailbox);

  mailbox[_ICS_MBOX_ENABLE_SET] = (1U << bit);

  return;
}

/*
 * Desensitize the supplied mailbox (which is usually local) to the supplied
 * bit number.
 */
ICS_VOID ics_mailbox_interrupt_disable (ics_mailbox_t *mailbox, ICS_UINT bit)
{
  ICS_ASSERT(mailbox);

  mailbox[_ICS_MBOX_ENABLE_CLEAR] = (1U << bit);

  /*
   * Make sure the call happens synchronously. We cannot read 
   * _ICS_MBOX_STATUS_CLEAR to confirm the write since that
   * could cause a bus error (because its write only) however the
   * bus guarantees ordering to the same peripheral so we can read
   * from a different address in the same peripheral.
   */
  (void) mailbox[_ICS_MBOX_ENABLE];

  return;
}

/*
 * Clear the supplied status bit of the supplied mailbox (which is usually
 * local).
 */
ICS_VOID ics_mailbox_interrupt_clear (ics_mailbox_t *mailbox, ICS_UINT bit)
{
  ICS_ASSERT(mailbox);

  mailbox[_ICS_MBOX_STATUS_CLEAR] = (1U << bit);

  /*
   * Make sure the call happens synchronously. We cannot read 
   * _ICS_MBOX_STATUS_CLEAR to confirm the write since that
   * could cause a bus error (because its write only) however the
   * bus guarantees ordering to the same peripheral so we can read
   * from a different address in the same peripheral.
   */
  (void) mailbox[_ICS_MBOX_STATUS];

  return;
}

/*
 * Set the supplied status bit of the supplied mailbox (which is usually remote).
 */
ICS_VOID ics_mailbox_interrupt_raise (ics_mailbox_t *mailbox, ICS_UINT bit)
{
  ICS_ASSERT(mailbox);

  mailbox[_ICS_MBOX_STATUS_SET] = (1U << bit);
}

/*
 * Read the status registers of the supplied mailbox.
 */
ICS_UINT ics_mailbox_status_get (ics_mailbox_t *mailbox)
{
  ICS_UINT status;
  ICS_ASSERT(mailbox);
  status = (ICS_UINT) mailbox[_ICS_MBOX_STATUS];
  return status;
}

/*
 * Assign value to the the status register of the supplied mailbox.
 */
ICS_VOID ics_mailbox_status_set (ics_mailbox_t *mailbox, ICS_UINT value)
{
  ICS_ASSERT(mailbox);
  mailbox[_ICS_MBOX_STATUS] = value;

  /*
   * Make sure the call happens synchronously
   */
  (void) mailbox[_ICS_MBOX_STATUS];
}

/*
 * Apply set/clear masks to the status register of the supplied mailbox.
 */
ICS_VOID ics_mailbox_status_mask (ics_mailbox_t *mailbox, ICS_UINT set, ICS_UINT clear)
{
  ICS_ASSERT(mailbox);
  if (set)
    mailbox[_ICS_MBOX_STATUS_SET]   = set;
  if (clear)
    mailbox[_ICS_MBOX_STATUS_CLEAR] = clear;

  /*
   * Make sure the call happens synchronously
   */
  (void) mailbox[_ICS_MBOX_STATUS];
}

/*
 * Assign value to the the enable register of the supplied mailbox.
 */
ICS_VOID ics_mailbox_enable_set (ics_mailbox_t *mailbox, ICS_UINT value)
{
  ICS_ASSERT(mailbox);
  mailbox[_ICS_MBOX_ENABLE] = value;
}

/*
 * Apply set/clear masks to the status register of the supplied mailbox.
 */
ICS_VOID ics_mailbox_enable_mask (ics_mailbox_t *mailbox, ICS_UINT set, ICS_UINT clear)
{
  ICS_ASSERT(mailbox);
  if (set)
    mailbox[_ICS_MBOX_ENABLE_SET]   = set;
  if (clear)
    mailbox[_ICS_MBOX_ENABLE_CLEAR] = clear;

  /*
   * Make sure the call happens synchronously
   */
  (void) mailbox[_ICS_MBOX_STATUS];
}

/*
 * Read the enable register of the supplied mailbox.
 */
ICS_UINT ics_mailbox_enable_get (ics_mailbox_t *mailbox)
{
  ICS_UINT enable;
  ICS_ASSERT(mailbox);
  enable = (ICS_UINT) mailbox[_ICS_MBOX_ENABLE];
  return enable;
}

/*
 * Allocate a local lock
 * Search all the mailbox lock sets until we find
 * an unused lock. Return address of hw lock to caller
 */
ICS_ERROR
ics_mailbox_lock_alloc (ics_mailbox_lock_t **pLock)
{
  ics_mailboxLockSet_t *mls;
  
  ICS_ASSERT(pLock);

  _ICS_OS_MUTEX_TAKE(&_mailboxSpinlockLock);
  
  for (mls = _mailboxSpinlockHead; mls; mls = mls->next)
  {
    int i;
    
    for (i=0; i<_ICS_MAILBOX_LOCKS; i++)
    {
      /* if this lock is in use move onto the next one */
      if (mls->inUseMask & (ICS_UINT) (1 << i))
	continue;
      
      /* allocate the lock */
      mls->inUseMask |= (ICS_UINT) (1 << i);
      *pLock = mls->spinlocks[i];

      _ICS_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
      
      return ICS_SUCCESS;
    }
  }
  
  _ICS_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);

  return ICS_ENOMEM;
}

/*
 * Deallocate a local lock
 */
ICS_VOID 
ics_mailbox_lock_free (ics_mailbox_lock_t *spinlock)
{
  ics_mailboxLockSet_t *mls;
  int id;
  
  ICS_ASSERT(spinlock);
  
  _ICS_OS_MUTEX_TAKE(&_mailboxSpinlockLock);

  if ((ICS_SUCCESS == findSpinlock(spinlock, &mls, &id)))
  {
    mls->inUseMask &= (ICS_UINT) ~(1 << id);
  }

  _ICS_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
