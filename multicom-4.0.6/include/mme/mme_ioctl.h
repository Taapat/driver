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

/*
 * Userspace <-> Kernel API for MME library
 */

#ifndef _MME_IOCTL_H
#define _MME_IOCTL_H

typedef struct mme_help_task {
	/* IN */
        MME_TransformerHandle_t		 handle;

        /* OUT */
        MME_Event_t			 event;
        MME_Command_t			*command;
} mme_help_task_t;

typedef struct mme_init_transformer {
        /* IN */
        const char			*name;
	unsigned int			 nameLength;
        MME_TransformerInitParams_t	 params;
	
        /* OUT */
	MME_ERROR			 status;
        MME_TransformerHandle_t		 handle;
} mme_init_transformer_t;

typedef struct mme_term_transformer {
	/* IN */
	MME_TransformerHandle_t		 handle;	

        /* OUT */
	MME_ERROR			 status;
} mme_term_transformer_t;

typedef struct mme_alloc_buffer {
	/* IN */
	MME_TransformerHandle_t		 handle;
	unsigned			 size;
	MME_AllocationFlags_t		 flags;

        /* OUT */
	unsigned long			 offset;
	unsigned			 mapSize;
	MME_ERROR			 status;
} mme_alloc_buffer_t;

typedef struct mme_free_buffer {
	/* IN */
	unsigned long			 offset;

        /* OUT */
	MME_ERROR			 status;
} mme_free_buffer_t;

typedef struct mme_abort_command {
	/* IN */
	MME_TransformerHandle_t		 handle;	
	MME_CommandId_t			 cmdId;

        /* OUT */
	MME_ERROR			 status;
} mme_abort_command_t;

typedef struct mme_send_command {
	/* INT */
	MME_TransformerHandle_t		 handle;
	MME_Command_t 			*command;

        /* OUT */
	MME_ERROR			 status;
} mme_send_command_t;

typedef struct mme_wait_command {
	/* IN */
	MME_TransformerHandle_t		 handle;
	MME_CommandId_t			 cmdId;
	MME_Time_t			 timeout;
	
        /* OUT */
	MME_ERROR			 status;
	MME_Event_t			 event;
} mme_wait_command_t;

typedef struct mme_get_capability {
	/* IN */
	char 			 	*name;
        int				 length;
	MME_TransformerCapability_t	*capability;

        /* OUT */
	MME_ERROR			status;
} mme_get_capability_t;

typedef struct mme_is_transformer_reg {
	/* IN */
	char 			*name;
 int				 length;
 /* OUT */
	MME_ERROR			 status;
} mme_is_transformer_reg_t;

typedef struct mme_tuneables {
	/* IN */
	int 			key;
	/* IN / OUT */
 int				value;
	MME_ERROR			 status;
} mme_tuneables_t;

typedef struct mme_userinit {
        /* OUT */
	MME_ERROR			 status;
} mme_userinit_t;

typedef struct mme_userterm {
        /* OUT */
	MME_ERROR			 status;
} mme_userterm_t;

#define MME_IOC_MAGIC 			'M'

#define MME_IOC_ABORTCOMMAND		_IOWR(MME_IOC_MAGIC, 0x1, mme_abort_command_t)
#define MME_IOC_SENDCOMMAND		_IOWR(MME_IOC_MAGIC, 0x2, mme_send_command_t)
#define MME_IOC_WAITCOMMAND		_IOWR(MME_IOC_MAGIC, 0x3, mme_wait_command_t)

#define MME_IOC_GETCAPABILITY 		_IOWR(MME_IOC_MAGIC, 0x10, mme_get_capability_t)
#define MME_IOC_HELPTASK		_IOWR(MME_IOC_MAGIC, 0x11, mme_help_task_t)
#define MME_IOC_INITTRANSFORMER		_IOWR(MME_IOC_MAGIC, 0x12, mme_init_transformer_t)
#define MME_IOC_TERMTRANSFORMER		_IOWR(MME_IOC_MAGIC, 0x13, mme_term_transformer_t)

#define MME_IOC_ALLOCBUFFER		_IOWR(MME_IOC_MAGIC, 0x20, mme_alloc_buffer_t)
#define MME_IOC_FREEBUFFER		 _IOWR(MME_IOC_MAGIC, 0x21, mme_free_buffer_t)
#define MME_IOC_ISTFRREG		   _IOWR(MME_IOC_MAGIC, 0x22, mme_is_transformer_reg_t)
#define MME_IOC_SETTUNEABLE		_IOWR(MME_IOC_MAGIC, 0x23, mme_tuneables_t)
#define MME_IOC_GETTUNEABLE		_IOWR(MME_IOC_MAGIC, 0x24, mme_tuneables_t)
#define MME_IOC_MMEINIT       		_IOWR(MME_IOC_MAGIC, 0x25, mme_userinit_t)
#define MME_IOC_MMETERM       		_IOWR(MME_IOC_MAGIC, 0x26, mme_userterm_t)

#endif /* _MME_IOCTL_H */


/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
