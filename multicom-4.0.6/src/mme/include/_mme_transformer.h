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

#ifndef _MME_TRANSFORMER_SYS_H
#define _MME_TRANSFORMER_SYS_H

#define _MME_TRANSFORMER_TASK_NAME		"tfm_%08x"	/* Transformer callback task name */

#define _MME_TRANSFORMER_TASK_PRIORITY		MME_GetTuneable(MME_TUNEABLE_TRANSFORMER_THREAD_PRIORITY)
#define _MME_TRANSFORMER_TIMEOUT		MME_GetTuneable(MME_TUNEABLE_TRANSFORMER_TIMEOUT)
#define _MME_TRANSFORMER_LOOKUP_TIMEOUT		(100) /* 100 ms */


/*
 * Control structure for per Transformer instantiations
 * on the Issuing side
 */
typedef struct mme_transformer
{
  MME_TransformerHandle_t	 handle;		/* Associated handle */

  _ICS_OS_TASK_INFO		 task;			/* Transformer OS Callback task */
  _ICS_OS_MUTEX			 tlock;			/* To protect this structure */

  ICS_PORT 			 mgrPort;		/* Transformer CPU owning manager port */
  ICS_PORT 			 receiverPort;		/* Receiver port */
  MME_TransformerHandle_t	 receiverHandle;	/* Transformer Receiver handle */

  ICS_PORT			 replyPort;		/* Port for Command reply messages */

  MME_GenericCallback_t 	 callback;		/* User Command completion callback */
  void 				*callbackData;

  MME_UINT			 numCmds;		/* Number of commands in flight */
  struct list_head		 issuedCmds;		/* Doubly linked list of issued command descs */
  struct list_head		 freeCmds;		/* Doubly linked list of free command descs */
  
  struct mme_command		*cmds;			/* Array of command descs (indexed from CmdId) */

} mme_transformer_t;

/* Exported internal APIs */
MME_ERROR          mme_transformer_create (MME_TransformerHandle_t receiverHandle,
					   ICS_PORT receiverPort, ICS_PORT mgrPort,
					   MME_GenericCallback_t callback, void *callbackData,
					   mme_transformer_t **transformerp, int createTask);

void               mme_transformer_term (mme_transformer_t *transformer);
mme_transformer_t *mme_transformer_instance (MME_TransformerHandle_t handle);

void               mme_transformer_cpu_down (ICS_UINT cpuNum);
void               mme_transformer_kill (mme_transformer_t *transformer);

#endif /* _MME_TRANSFORMER_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
