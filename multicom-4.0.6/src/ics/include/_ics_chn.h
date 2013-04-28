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

#ifndef _ICS_CHN_SYS_H
#define _ICS_CHN_SYS_H

/*
 * Inter-cpu message channels for STMicro SoC systems
 *
 * This interface is designed to be very light-weight and cache efficient with
 * the intention that these FIFOs are placed in inter-cpu shared memory
 *
 * It is based on fixed length FIFOs that are managed lock free
 * by ensuring the one-reader/one-writer semantic
 * The producer maintains the fptr and the consumer maintains the bptr
 * These pointers are seperated by a cacheline to ensure coherency isn't required
 *
 * An empty FIFO is represented when fptr == bptr and a full FIFO is
 * represented when (fptr + 1) == bptr
 *
 * Wrapping of the FIFO indices is achieved using a mask operation so the
 * FIFO depth must be a power of 2.
 *
 * MULTITHREAD SAFE: No locking is provided and so must be performed by the caller
 */

/* Calculate the number of free slots left in the FIFO based on the depth & current fptr/bptr */
#define _ICS_CHN_FIFO_SPACE(NS, F, B)	((((NS)-1) - ((F) - (B))) & ((NS)-1))
#define _ICS_CHN_FIFO_IS_FULL(NS, F, B)	((((F)+1) & (NS-1)) == (B))

/* Magic offset returned to indicate FIFO full */
#define _ICS_CHN_FIFO_FULL			((ICS_UINT) (-1))

/* Magic offset returned to indicate FIFO empty */
#define _ICS_CHN_FIFO_EMPTY			((ICS_UINT) (-2))

typedef struct chn_fifo_s
{
  volatile ICS_UINT fptr;		/* producer: Insert at this index */
  ICS_UINT          sends;		/* producer: STATISTICS: Number of insert calls */
  void             *owner;		/* producer: owning schannel handle */

  char _pad[_ICS_CACHELINE_PAD(2,1)];

  volatile ICS_UINT bptr;		/* consumer: Remove from this index */
  ICS_UINT          interrupts;		/* consumer: STATISTICS: Interrupt count */

  char _pad2[_ICS_CACHELINE_PAD(2,0)];

} chn_fifo_t;

typedef struct ics_chn_s 
{
  /* CACHELINE aligned */
  chn_fifo_t		fifo;		/* consumer/producer: FIFO desc */

  /* CACHELINE aligned */
  ICS_OFFSET		base;		/* consumer: Base paddr of fifo */

  ICS_UINT		ss;		/* consumer: Size of each FIFO slot */
  ICS_UINT		ns;		/* consumer: Number of slots in the FIFO */

  ICS_UINT		version;	/* consumer: Channel incarnation */

  ICS_OFFSET		paddr;		/* consumer: paddr of ics_chn_t : Needed for cache flush/purge */

  /* Pad to next cacheline */
  ICS_CHAR             _pad[_ICS_CACHELINE_PAD(3,2)];

  /* CACHELINE aligned */
} ics_chn_t;

/*
 * Grab a free channel FIFO slot if one is available
 * Returns _ICS_CHN_FIFO_FULL if it is full
 */
_ICS_OS_INLINE_FUNC_PREFIX 
ICS_UINT ics_chn_claim (ics_chn_t *chn, ICS_BOOL cached)
{
  ICS_OFFSET off;
  ICS_UINT   bptr, fptr;

  ICS_ASSERT(chn);
  
  /* About to read this, it was last updated by the peer */
  if (cached)
    /* Have to use PURGE as this could be a loopback send */
    _ICS_OS_CACHE_PURGE(&chn->fifo.bptr, chn->paddr + offsetof(ics_chn_t, fifo.bptr), sizeof(chn->fifo.bptr));
  
  fptr = chn->fifo.fptr;
  bptr = chn->fifo.bptr;
  
  ICS_PRINTF(ICS_DBG_CHN, "chn %p ns %d fptr %d bptr %d\n", chn, chn->ns, fptr, bptr);
  
  ICS_ASSERT(fptr < chn->ns);
  ICS_ASSERT(bptr < chn->ns);
  
  /* Test for the FIFO being full */
  if (_ICS_CHN_FIFO_IS_FULL(chn->ns, fptr, bptr))
  {
    return _ICS_CHN_FIFO_FULL;
  }

  /* Return the slot address of the current fptr */
  off = (fptr * chn->ss);
  return off;
}

/*
 * 'Send' a message to the peer by updating the FIFO
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void ics_chn_send (ics_chn_t *chn, ics_mailbox_t *remote, ICS_UINT bit, ICS_BOOL cached)
{
  ICS_ASSERT(chn);

  /*
   * Sending a message just needs us to move on the fptr
   *
   * NB: Caller is responsible for flushing FIFO slot to memory
   * prior to calling this function
   */

  ICS_PRINTF(ICS_DBG_CHN, "chn %p fptr %d\n", chn, chn->fifo.fptr);
  
  chn->fifo.fptr = (chn->fifo.fptr+1) & (chn->ns-1);

  if (cached)
    /* Flush out the updated fptr (leaving it in local cache) */
    _ICS_OS_CACHE_FLUSH(&chn->fifo.fptr, chn->paddr + offsetof(ics_chn_t, fifo.fptr), sizeof(chn->fifo.fptr));
   
  /* Signal the remote CPU */
  if (remote)
  {
    ICS_PRINTF(ICS_DBG_CHN, "chn %p signal rmbox %p bit %d\n", chn, remote, bit);
    ics_mailbox_interrupt_raise(remote, bit);
  }

  return;
}

/*
 * Return the FIFO offset of a new entry at the current
 * bptr, returns _ICS_CHN_FIFO_EMPTY when empty
 */
_ICS_OS_INLINE_FUNC_PREFIX 
ICS_UINT ics_chn_receive (ics_chn_t *chn, ICS_UINT *bptrp, ICS_BOOL cached)
{
  ICS_OFFSET off;
  ICS_UINT   fptr, bptr;

  ICS_ASSERT(chn);
  
  /* About to read this, it was last updated by the peer */
  /* Have to use PURGE as this could be a loopback send */
  if (cached)
    _ICS_OS_CACHE_PURGE(&chn->fifo,chn->paddr + offsetof(ics_chn_t, fifo), sizeof(chn->fifo));

  /* Use the local bptr if supplied, else use the SHM one */
  bptr = (bptrp ? *bptrp : chn->fifo.bptr);
  fptr = chn->fifo.fptr;

  /* Test for an empty FIFO */
  if (fptr == bptr)
     return _ICS_CHN_FIFO_EMPTY;
  
  ICS_ASSERT(fptr < chn->ns);
  ICS_ASSERT(bptr < chn->ns);

  /* Return the desc offset of the current bptr */
  off = (bptr * chn->ss);

  ICS_PRINTF(ICS_DBG_CHN, "chn %p fptr %d bptr %d off %x\n", chn, fptr, bptr, off);

  /* Move on the local bptr */
  if (bptrp)
    *bptrp = (bptr + 1) & (chn->ns-1);


  return off;
}

/* 
 * Release back a FIFO entry to the channel
 */
_ICS_OS_INLINE_FUNC_PREFIX 
ICS_UINT ics_chn_release (ics_chn_t *chn, ICS_BOOL cached)
{
  ICS_ASSERT(chn);
  ICS_ASSERT(powerof2(chn->ns));

  if (chn->fifo.bptr == chn->fifo.fptr)
    /* FIFO is empty! */
    return _ICS_CHN_FIFO_EMPTY;

  /* 
   * Releasing an entry just needs us to move on the bptr
   */

  ICS_PRINTF(ICS_DBG_CHN, "chn %p bptr %d\n", chn, chn->fifo.bptr);
  
  /* Move on bptr */
  chn->fifo.bptr = (chn->fifo.bptr+1) & (chn->ns-1);
  
  if (cached)
    /* Flush the new bptr through to memory */
    _ICS_OS_CACHE_FLUSH(&chn->fifo.bptr, chn->paddr + offsetof(ics_chn_t, fifo.bptr), sizeof(chn->fifo.bptr));
  
  /* Return the desc offset of the new bptr */
  return (chn->fifo.bptr * chn->ss);
}

/*
 * Initialise an ICS channel
 */
_ICS_OS_INLINE_FUNC_PREFIX 
void ics_chn_init (ics_chn_t *chn, ICS_OFFSET paddr, ICS_UINT nslots, ICS_UINT ssize)
{
  ICS_ERROR     err;

  ICS_OFFSET    paddr2;
  ICS_MEM_FLAGS mflags;
  
  ICS_ASSERT(chn);
  ICS_ASSERT(ICS_CACHELINE_ALIGNED(chn));

  ICS_ASSERT(nslots >= 2);
  ICS_ASSERT(powerof2(nslots));
  ICS_ASSERT(ssize);

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_CHN, "chn %p nslots %d ssize %d\n", chn, nslots, ssize);

  /* Initially the FIFO is empty */
  chn->fifo.fptr = chn->fifo.bptr = 0;

  /* Clear down owner */
  chn->fifo.owner  = NULL;
  
  /* Reset stats */
  chn->fifo.sends = chn->fifo.interrupts = 0;

  chn->ns     = nslots;	/* Number of slots */
  chn->ss     = ssize;	/* Slot size */
  chn->base   = paddr;	/* paddr of base of FIFO */

  chn->version++; 	/* Update version */

  /* Convert the channel desc address to a physical one */
  err = _ICS_OS_VIRT2PHYS(chn, &paddr2, &mflags);
  ICS_assert(err == ICS_SUCCESS);

  chn->paddr  = paddr2;	/* Stash this for faster cache flushes */

  /* Commit all updates to memory */
  _ICS_OS_CACHE_PURGE(chn, paddr2, sizeof(*chn));

  ICS_PRINTF(ICS_DBG_INIT|ICS_DBG_CHN, "chn %p fifo 0x%x-0x%x\n",
	     chn, 
	     chn->base, chn->base + (chn->ns*chn->ss));
}

#endif /* _ICS_CHN_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
