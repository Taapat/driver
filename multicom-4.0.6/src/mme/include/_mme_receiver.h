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

#ifndef _MME_RECEIVER_SYS_H
#define _MME_RECEIVER_SYS_H

#define _MME_RECEIVER_TASK_NAME			"mmercv_%08x"	/* Name of Receiver task */

#define _MME_RECEIVER_TASK_PRIORITY		MME_GetTuneable(MME_TUNEABLE_TRANSFORMER_THREAD_PRIORITY)

/*
 * Reeiver Transform message types 
 */
typedef enum mme_receiver_msg_type
{
  _MME_RECEIVER_TRANSFORM    = (0xaa << 24) | 0x01,	/* Transformation Command */
  _MME_RECEIVER_ABORT        = (0xaa << 24) | 0x02,	/* ABORT command */
  _MME_RECEIVER_STARVATION   = (0xaa << 24) | 0x03,	/* DATA_NDERFLOW/NOT_ENOUGH_MEMORY */
  _MME_RECEIVER_KILL         = (0xaa << 24) | 0x04,	/* Kill command (issued on host) */

} mme_receiver_msg_type_t;

/* The MME_Command structure is marshalled into
 * this data structure for transmission to the Companion
 *
 * Due to it's large size this cannot be sent inline
 */
typedef struct mme_transform
{
  mme_receiver_msg_type_t	type;			/* Msg type */

  ICS_PORT			replyPort;		/* Port to send result to */
  MME_UINT                      size;			/* Size of this message */ 

  MME_TransformerHandle_t	receiverHandle;		/* Handle of Companion receiver */
  MME_Command_t			command;		/* Copy of orginal MME_Command_t */

  /* A variable sized structure follows;

     - Array of the MME_DataBuffer_t pointers (NumInput+NumOutput),
     
     - Array of the MME_DataBuffer_t structures for (NumInput+NumOutput)
     
     - Array of n MME_ScatterPage_t structures.
       (There are MME_DataBuffer_t*NumberOfScatterPages of them)
     
     - The number of generic parameters from CmdStatus struct
       encoded in the Command structure
     
     - An array of 64 bit-wide Command generic parameters
  */

} mme_transform_t;

typedef struct mme_starvation
{
  mme_receiver_msg_type_t	type;			/* Msg type */

  MME_Event_t			event;			/* Event code */
  MME_CommandStatus_t		status;			/* MME CommandStatus */

} mme_starvation_t;

typedef struct mme_abort
{
  mme_receiver_msg_type_t	type;			/* Msg type */

  MME_CommandId_t		cmdId;			/* CmdId to be cancelled */

} mme_abort_t;

typedef struct mme_kill
{
  mme_receiver_msg_type_t	type;			/* Msg type */

  MME_Event_t			event;			/* Event code */
  MME_CommandStatus_t		status;			/* MME CommandStatus */

} mme_kill_t;

/*
 * Union of all possible types of message
 */
typedef union mme_receiver_msg
{
  mme_receiver_msg_type_t	type;			/* Msg type */

  mme_transform_t		transform;
  mme_abort_t			abort;
  mme_starvation_t              starvation;
  mme_kill_t                    kill;

} mme_receiver_msg_t;

/*
 * Instantiated Transformer structure 
 */
typedef struct mme_receiver
{
  _ICS_OS_TASK_INFO		 task;		/* Receiver task */

  ICS_PORT			 port;		/* ICS port for this receiver */

  /* Transformer functions, copied on instantiation from the Registered transformer */
  mme_transformer_func_t	 funcs;

  void				*context;	/* User defined transformer context handle */

  mme_messageq_t		 msgQ;		/* Outstanding messages */

  mme_transform_t		**transforms;	/* transform msg addresses (used in Abort) */

  struct mme_execution_task	*exec;		/* Execution task associated with this receiver */

  struct list_head		 list;		/* Doubly linked list element (execution task list) */

  ICS_ULONG			 cmds;		/* STATISTICS: cmd execution count */
  ICS_STATS_HANDLE		 stats;		/* STATISTICS: cmd execution count handle */

} mme_receiver_t;

/* Exported internal APIs */
extern MME_ERROR mme_receiver_create (mme_transformer_reg_t *treg, MME_UINT paramsSize, MME_GenericParams_t params,
				      mme_receiver_t **receiverp);
extern MME_ERROR mme_receiver_term (mme_receiver_t *receiver);
extern void      mme_receiver_free (mme_receiver_t *receiver);

extern MME_ERROR mme_receiver_due_time (mme_receiver_t *receiver, MME_Time_t *timep);
extern MME_ERROR mme_receiver_process (mme_receiver_t *receiver);

#endif /* _MME_RECEIVER_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
