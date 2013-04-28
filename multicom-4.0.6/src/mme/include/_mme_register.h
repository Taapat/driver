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

#ifndef _MME_REGISTER_SYS_H
#define _MME_REGISTER_SYS_H

typedef struct mme_transformer_func
{
  MME_AbortCommand_t			abort;
  MME_GetTransformerCapability_t	getTransformerCapability;
  MME_InitTransformer_t			initTransformer;
  MME_ProcessCommand_t			processCommand;
  MME_TermTransformer_t			termTransformer;

} mme_transformer_func_t;

typedef struct mme_transformer_reg
{
  struct list_head		 	list;		/* Doubly linked list element */
  
  mme_transformer_func_t		funcs;		/* Transformer function pointers */

  ICS_NSRV_HANDLE			nhandle;	/* Nameserver registration handle */
  
  char 					name[1];	/* Variable length string */
  
} mme_transformer_reg_t;


/* Exported internal APIs */
mme_transformer_reg_t * mme_transformer_lookup (const char *name);

#endif /* _MME_REGISTER_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
