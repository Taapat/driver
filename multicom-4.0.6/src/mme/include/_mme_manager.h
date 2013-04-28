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

#ifndef _MME_MANAGER_SYS_H
#define _MME_MANAGER_SYS_H

#define _MME_MANAGER_PORT_NAME			"mme_manager_%02d"	/* Name of per CPU Manager port */
#define _MME_MANAGER_PORT_NSLOTS		32			/* Number of slots in Port mq */

#define _MME_MANAGER_TASK_NAME			"mme_manager"		/* Task name of Manager thread */

#define _MME_MANAGER_TASK_PRIORITY		MME_GetTuneable(MME_TUNEABLE_MANAGER_THREAD_PRIORITY)

/* map task priorities 5000 -> 4, 4000 -> 3, ..., 1000 -> 0 without using decimal division */
#define MME_PRIORITY_TO_ID(pri) (pri >> 10)

/*
 * Manager message types 
 */
typedef enum mme_manager_msg_type
{
  _MME_MANAGER_TRANS_INIT     = (0xbb << 24) | 0x01,	/* Instantiate a transformer */
  _MME_MANAGER_TRANS_TERM     = (0xbb << 24) | 0x02,	/* Terminate a transformer */
  _MME_MANAGER_GET_CAPABILITY = (0xbb << 24) | 0x03,	/* Get Transformer capability */
  _MME_MANAGER_IS_ALIVE       = (0xbb << 24) | 0x04,	/* Query status of a cpu */

} mme_manager_msg_type_t;

/* Instantiate transformer info */
typedef struct mme_init_trans
{
  /* Request */
  char				name[MME_MAX_TRANSFORMER_NAME+1];
  MME_Priority_t		priority;

  /* Result - filled in on companion */
  MME_TransformerHandle_t	handle;		/* Allocated Receiver transformer handle */
  ICS_PORT			port;		/* Transformer receiver port */

  /* Followed by any Transformer Init params */ 
} mme_init_trans_t;

/* Terminate transformer info */
typedef struct mme_term_trans
{
  /* Request */
  MME_TransformerHandle_t       handle;		/* Receiver Transformer handle */

} mme_term_trans_t;

typedef struct mme_capability
{
  char				name[MME_MAX_TRANSFORMER_NAME+1];
  MME_TransformerCapability_t   capability;	/* Filled in on Companion */

} mme_capability_t;

typedef struct mme_manager_msg
{
  mme_manager_msg_type_t	type;		/* Msg type */
  ICS_PORT			replyPort;	/* Port to send result to */
  MME_ERROR			res;		/* Operation result code */

  union
  {
    mme_init_trans_t		init;		/* _MME_MANAGER_TRANS_INIT */
    mme_term_trans_t		term;		/* _MME_MANAGER_TRANS_TERM */
    mme_capability_t		get;		/* _MME_MANAGER_GET_CAPABILITY */
  } u;

} mme_manager_msg_t;

/* Exported internal APIs */
extern void      mme_manager_task (void *param);
extern MME_ERROR mme_manager_send (ICS_PORT mgrPort, mme_manager_msg_type_t type,
				   mme_manager_msg_t *msg, MME_UINT msgSize);

#endif /* _MME_MANAGER_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
