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

#ifndef _ICS_MQ_SYS_H
#define _ICS_MQ_SYS_H

/*
 * Simple fixed length message queues
 * This API provides a simple mechanism for queueing items into
 * a fixed length array located in CPU local memory
 *
 * It is based on fixed length FIFOs that are managed lock free
 * by ensuring the one-reader/one-writer semantic
 * The producer maintains the fptr and the consumer maintains the bptr
 *
 * An empty FIFO is represented when fptr == bptr and a full FIFO is
 * represented when (fptr + 1) == bptr
 * Wrapping of the FIFO indices is achieved using a mask operation so the
 * FIFO depth must be a power of 2.
 *
 */

typedef struct ics_mq 
{
  volatile ICS_UINT fptr;		/* Insert at this index - owned by producer */
  volatile ICS_UINT bptr;		/* Remove from this index - owned by consumer */

  ICS_UINT          sd;			/* Size of FIFO desc */
  ICS_UINT          nd;			/* Number of descs on the FIFO */

  ICS_CHAR          fifo[1];		/* Place holder for variable sized FIFO */

} ics_mq_t;

/* Returns the number of bytes required for an ICS mq desc
 * containing a FIFO with ND descriptors of DS size each
 */
#define _ICS_MQ_SIZE(ND, DS) 		(offsetof(ics_mq_t, fifo) + (ND)*(DS))

/* Calculate the number of free descs left in the FIFO based on the size & current fptr/bptr */
#define _ICS_MQ_FIFO_SPACE(ND, F, B)	((((ND)-1) - ((F) - (B))) & ((ND)-1))
#define _ICS_MQ_FIFO_FULL(ND, F, B)	((((F)+1) & (ND-1)) == (B))

/*
 * Copy a new message into the FIFO
 *
 * Returns ICS_QUEUE_FULL if the FIFO is full
 */
_ICS_OS_INLINE_FUNC_PREFIX 
ICS_ERROR ics_mq_insert (ics_mq_t *mq, void *buf, ICS_SIZE size)
{
  void     *msg;
  ICS_UINT  bptr, fptr;

  ICS_ASSERT(size <= mq->sd);

  fptr = mq->fptr;
  bptr = mq->bptr;

  ICS_ASSERT(fptr < mq->nd);
  ICS_ASSERT(bptr < mq->nd);

  /* Test for the FIFO being full */
  if (_ICS_MQ_FIFO_FULL(mq->nd, fptr, bptr))
  {
    return ICS_FULL;
  }

  ICS_PRINTF(ICS_DBG_MSG, "mq %p ndesc %d fptr %d bptr %d\n",
	     mq, mq->nd, fptr, bptr);

  /* Calculate the desc address of the current fptr */
  msg = (void *) (mq->fifo + (fptr * mq->sd));

  /* Copy message into the FIFO */
  _ICS_OS_MEMCPY(msg, buf, size);

  /* Move on fptr */
  mq->fptr = (fptr+1) & (mq->nd-1);

  return ICS_SUCCESS;
}

/*
 * Return a message from the FIFO queue
 *
 * The message queue descriptor is passed to the caller
 * and remains valid until ics_mq_release() is called
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void *ics_mq_recv (ics_mq_t *mq)
{
  void     *msg;
  ICS_UINT  fptr, bptr;
 
  fptr = mq->fptr;
  bptr = mq->bptr;

  ICS_ASSERT(fptr < mq->nd);
  ICS_ASSERT(bptr < mq->nd);

  if (fptr == bptr)
  {
    /* Empty FIFO */
    return NULL;
  }

  /* Return the desc address of the current bptr */
  msg = (void *) (mq->fifo + (bptr * mq->sd));

  ICS_PRINTF(ICS_DBG_MSG, "mq %p fptr %d bptr %d msg %p\n",
	     mq, fptr, bptr, msg);

  return msg;
}


/* 
 * Release back a message descriptor to the FIFO queue
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void ics_mq_release (ics_mq_t *mq)
{
  /* 
   * Releasing a message just needs us to move on the bptr
   */

  /* Move on bptr */
  mq->bptr = (mq->bptr+1) & (mq->nd-1);

  ICS_PRINTF(ICS_DBG_MSG, "mq %p new bptr %d\n", mq, mq->bptr);

  return;
}

_ICS_OS_INLINE_FUNC_PREFIX 
int ics_mq_full (ics_mq_t *mq)
{
  ICS_UINT bptr, fptr;
  
  ICS_ASSERT(mq);

  fptr = mq->fptr;
  bptr = mq->bptr;

  ICS_ASSERT(fptr < mq->nd);
  ICS_ASSERT(bptr < mq->nd);

  return _ICS_MQ_FIFO_FULL(mq->nd, fptr, bptr);
}

_ICS_OS_INLINE_FUNC_PREFIX 
int ics_mq_empty (ics_mq_t *mq)
{
  ICS_UINT bptr, fptr;
  
  ICS_ASSERT(mq);

  fptr = mq->fptr;
  bptr = mq->bptr;

  ICS_ASSERT(fptr < mq->nd);
  ICS_ASSERT(bptr < mq->nd);

  return (fptr == bptr);
}

/*
 * Initialise a message queue
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void ics_mq_init (ics_mq_t *mq, ICS_UINT ndesc, ICS_UINT sdesc)
{
  ICS_ASSERT(mq);

  ICS_ASSERT(ndesc >= 2);
  ICS_ASSERT(powerof2(ndesc));
  ICS_ASSERT(sdesc);
  
  ICS_PRINTF(ICS_DBG_MSG, "mq %p ndesc %d sdesc %d\n", mq, ndesc, sdesc);

  _ICS_OS_MEMSET(mq, 0, _ICS_MQ_SIZE(ndesc, sdesc));

  /* Initially the FIFO is empty */
  mq->fptr = mq->bptr = 0;

  mq->nd = ndesc;
  mq->sd = sdesc;

  ICS_PRINTF(ICS_DBG_MSG, "mq %p queue FIFO %p-%p\n",
	     mq, mq->fifo, mq->fifo + (ndesc*sdesc));

  return;
}

#endif /* _ICS_MQ_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
