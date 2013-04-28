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

#ifndef _MME_COMMAND_SYS_H
#define _MME_COMMAND_SYS_H

#define _MME_COMMAND_TIMEOUT			MME_GetTuneable(MME_TUNEABLE_COMMAND_TIMEOUT)

#define _MME_HIGH_PRIORITY			0xaddebabe	/* Magic Command DueTime to jump to head of queue */

typedef enum
{
  _MME_COMMAND_IDLE      = 0,
  _MME_COMMAND_RUNNING   = 1,
  _MME_COMMAND_COMPLETE  = 2,
  _MME_COMMAND_KILLED    = 3,

} mme_command_state_t;


/*
 * Control structure for each outstanding transformer command
 * Used to hold the user's info for completing the operation
 */
typedef struct mme_command
{
  MME_CommandId_t		 cmdId;			/* Last CmdId issued (includes index/version) */

  MME_Command_t			*command;		/* Pointer to User's MME_Command struct */
  MME_Event_t		 	 event;			/* Command Event completion code */

  mme_command_state_t		 state;			/* State of command slot (e.g. RUNNING) */
  mme_msg_t			*msg;			/* Msg desc associated with this command issue */

  _ICS_OS_EVENT			 block;			/* MME_WaitCommand will block here for completion */

  struct list_head		 list;			/* Doubly linked list element */

} mme_command_t;


/* Exported internal APIs */
MME_ERROR mme_command_pack (mme_transformer_t *transformer, MME_Command_t *cmd, mme_msg_t **msgp);
MME_ERROR mme_command_complete (MME_Command_t *command, mme_transform_t *transform);
MME_ERROR mme_command_callback (mme_transformer_t *transformer, MME_Event_t event, mme_command_t *cmd);

MME_ERROR mme_command_unpack (mme_receiver_t *receiver, mme_transform_t *transform);
MME_ERROR mme_command_repack (mme_receiver_t *receiver, mme_transform_t *transform);

MME_ERROR mme_command_send (mme_transformer_t *transformer, MME_Command_t *command);

/* Management of local command descs */
void           mme_cmd_init (mme_transformer_t *transformer);
void           mme_cmd_term (mme_transformer_t *transformer);
mme_command_t *mme_cmd_alloc (mme_transformer_t *transformer);
void           mme_cmd_free (mme_transformer_t *transformer, mme_command_t *cmd);
void           mme_cmd_dump (mme_transformer_t *transformer);

#endif /* _MME_COMMAND_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
