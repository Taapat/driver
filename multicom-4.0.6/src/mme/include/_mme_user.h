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

#ifndef _MME_USER_SYS_H
#define _MME_USER_SYS_H

#define MME_MAJOR_NUM 		231
#define MME_DEV_NAME  		"mme"
#define MME_DEV_COUNT 		1

#include "_ics_mq.h"		/* Lock free FIFO implementation */

typedef struct mme_user_callback
{
	MME_Command_t		*command;	/* Completed command desc */
	MME_Event_t		 event;		/* Associated event status */
	
} mme_user_callback_t;

/* Per transformer instantation data structure */
typedef struct mme_user_trans
{
	MME_TransformerHandle_t  handle;	/* Transformer handle */
	
	_ICS_OS_EVENT		 event;		/* ioctl() blocks here */
	
	struct list_head	issuedCmds;	/* Linked list of issued commands */
	
	ics_mq_t		*mq;		/* Pending user callbacks */

} mme_user_trans_t;

/* Control desc hung off file pointer */
typedef struct mme_user
{
	_ICS_OS_MUTEX		ulock;			/* Lock to protect this structure */

	mme_user_trans_t       *insTrans[_MME_TRANSFORMER_INSTANCES];	/* Locally instantiated transformers */	

	struct list_head	allocatedBufs;		/* Linked list of all allocated buffers */

} mme_user_t;

/* Internal user command descriptor */
typedef struct mme_user_command 
{
	MME_Command_t		 command;		/* Kernel copy of MME Command desc */
	MME_Command_t		*userCommand;		/* Original address of user command desc */
        void			*userAdditionalInfo;
	
	atomic_t		 refCount;

	struct list_head	 list;			/* Doubly linked list */

} mme_user_command_t;

/* Internal user buffer descriptor */
typedef struct mme_user_buf
{
	unsigned long		 offset;	/* Physical offset of memory */
	unsigned long		 size;		/* Buffer size (mapped size) */
	ICS_MEM_FLAGS	 	 mflags;	/* Memory allocation flags */

	MME_DataBuffer_t	*mmeBuf;	/* MME DataBuffer */
	
	struct list_head	 list;		/* Doubly linked list */

} mme_user_buf_t;

extern int  mme_user_init_transformer (mme_user_t *instance, void *arg);
extern int  mme_user_term_transformer (mme_user_t *instance, void *arg);
extern void mme_user_transformer_release (mme_user_t *instance);

extern int  mme_user_get_capability (mme_user_t *instance, void *arg);

extern int  mme_user_abort_command (mme_user_t *instance, void *arg);
extern int  mme_user_send_command (mme_user_t *instance, void *arg);
extern int  mme_user_wait_command (mme_user_t *instance, void *arg);

extern int  mme_user_command_complete (mme_user_t *instance, mme_user_command_t *intCommand, MME_Event_t event);
extern void mme_user_command_free (mme_user_t *instance, mme_user_command_t *intCommand);

extern int  mme_user_help_task (mme_user_t *instance, void *arg);
extern void mme_user_help_callback (MME_Event_t event, MME_Command_t *command, void *param);

extern int  mme_user_alloc_buffer (mme_user_t *instance, void *arg);
extern int  mme_user_free_buffer (mme_user_t *instance, void *arg);
extern void mme_user_buffer_release (mme_user_t *instance);
extern int mme_user_mmeinit(mme_user_t *instance, void *arg);
extern int mme_user_mmeterm(mme_user_t *instance, void *arg);
extern int  mme_user_is_transformer_registered(mme_user_t *instance, void *arg);
extern int  mme_user_set_tuneable(mme_user_t *instance, void *arg);
extern int  mme_user_get_tuneable(mme_user_t *instance, void *arg);


#endif /* _MME_USER_SYS_H */

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
