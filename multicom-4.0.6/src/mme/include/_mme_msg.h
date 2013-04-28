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

#ifndef _MME_MSG_SYS_H
#define _MME_MSG_SYS_H

#define _MME_MSG_CACHE_FLAGS	ICS_CACHED	/* Default cache flags for MME msg Meta data allocation */

#define _MME_MSG_SMALL_SIZE	1024		/* Maximum size of a cached MME msg buffer (MME meta data) */

typedef struct mme_msg
{
  struct list_head		 list;		/* Doubly linked list */

  MME_UINT			 size;		/* Allocated memory size */
  void				*buf;		/* Companion mapped buffer */
  
  void				*owner;		/* DEBUG: owning function */

} mme_msg_t;

/* Exported internal APIs */
MME_ERROR  mme_msg_init (void);
void       mme_msg_term (void);

mme_msg_t *mme_msg_alloc (MME_UINT size);
void       mme_msg_free (mme_msg_t *msg);

#endif /* _MME_MSG_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
