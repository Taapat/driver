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
 * Private header file for Multicom4 implementation
 *
 */ 

#ifndef _MME_SYS_H
#define _MME_SYS_H

#include <ics.h>		/* ICS exported APIs */

#include "_ics_os.h"		/* ICS OS wrapper functions */
#include "_ics_util.h"		/* ICS misc macros */
#include "_ics_debug.h"		/* ICS DEBUG memory allocation */

#include "_ics_list.h"		/* ICS linked list support */

#include "_mme_limits.h"
#include "_mme_handle.h"
#include "_mme_msg.h"
#include "_mme_register.h"
#include "_mme_manager.h"
#include "_mme_messageq.h"
#include "_mme_receiver.h"
#include "_mme_execution.h"
#include "_mme_buffer.h"
#include "_mme_transformer.h"
#include "_mme_command.h"

#include "_mme_debug.h"

/*
 * MME CmdId
 *
 * [31-24] Handle      (8-bit)
 * [23-16] Index       (8-bit)
 * [15-00] Version     (16-bit)
 *
 */

#define _MME_CMD_HANDLE_MASK		(0xff)
#define _MME_CMD_HANDLE_SHIFT		(24)

#define _MME_CMD_INDEX_MASK		(0xff)
#define _MME_CMD_INDEX_SHIFT		(16)

#define _MME_CMD_VERSION_MASK		(0xffff)
#define _MME_CMD_VERSION_SHIFT		(0)

#define _MME_CMD_HANDLE_VAL		(0xcc)

/* Generate the MME CmdId */
#define _MME_CMD_ID(IDX, VER)	(_MME_CMD_HANDLE_VAL << _MME_CMD_HANDLE_SHIFT |			\
				 ((IDX) & _MME_CMD_INDEX_MASK) << _MME_CMD_INDEX_SHIFT |	\
				 ((VER) & _MME_CMD_VERSION_MASK) << _MME_CMD_VERSION_SHIFT)

/* Extract the Command Handle from a CmdId */
#define _MME_CMD_HANDLE(CMDID)		(((CMDID) >> _MME_CMD_HANDLE_SHIFT) & _MME_CMD_HANDLE_MASK)

/* Extract the Command Index from a CmdId */
#define _MME_CMD_INDEX(CMDID)		(((CMDID) >> _MME_CMD_INDEX_SHIFT) & _MME_CMD_INDEX_MASK)

/* Extract the Command Version from a CmdId */
#define _MME_CMD_VERSION(CMDID)		(((CMDID) >> _MME_CMD_VERSION_SHIFT) & _MME_CMD_VERSION_MASK)

/* Primary MME state */
typedef struct mme_state 
{
  ICS_UINT				cpuNum;			/* ICS local cpu info */
  ICS_ULONG				cpuMask;

  _ICS_OS_MUTEX                  	lock;			/* Lock to protect this structure */

  ICS_HEAP				heap;			/* MME_DataBuffer_t heap */
  ICS_SIZE				heapSize;
  ICS_REGION				heapCached;		/* Cached mapping of heap */
  ICS_REGION				heapUncached;		/* Uncached mapping of heap */

  _ICS_OS_TASK_INFO			managerTask;		/* MME Manager task */
  ICS_PORT				managerPort;		/* MME Manager port handle */
  _ICS_OS_EVENT				managerExit;		/* MME_Run() blocks here */

  mme_transformer_t		       *insTrans[_MME_TRANSFORMER_INSTANCES];/* Array of locally Instantiated transformers */
  struct list_head		 	regTrans;		/* Locally Registered transformers (mme_transformer_reg_t) */
  
  mme_execution_task_t		       *executionTask[_MME_EXECUTION_TASKS];	/* Array of execution tasks */

  struct list_head			freeMsgs;		/* Doubly linked list of free msg descs */
  
  ICS_WATCHDOG				watchdog;		/* ICS watchdog handle */
  _ICS_OS_SPINLOCK	mmespinLock;		/* IRQ/Spinlock to protect from the msg handler IRQ */

} mme_state_t;

/* Global MME state structure */
extern mme_state_t *mme_state;

/* Exported internal APIs */
extern void mme_register_term (void);

#endif /* _MME_SYS_H */
 
/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

