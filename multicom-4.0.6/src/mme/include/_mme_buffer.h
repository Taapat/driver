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

#ifndef _MME_BUFFER_SYS_H
#define _MME_BUFFER_SYS_H

#define _MME_BUF_CACHE_FLAGS	ICS_CACHED	/* Default cache flags for MME_DataBuffer_t allocation */

#define _MME_BUF_POOL_SIZE	MME_GetTuneable(MME_TUNEABLE_BUFFER_POOL_SIZE)

/*
 * Internal MME_DataBuffer_t container
 */
typedef struct mme_buffer
{
  MME_DataBuffer_t		buffer;		/* MME_DataBuffer_t presented to user */

  MME_AllocationFlags_t		flags;
  MME_ScatterPage_t		pages[1];

  void                         *owner;		/* DEBUG buffer owner */

} mme_buffer_t;

/* Exported internal APIs */

#endif /* _MME_BUFFER_SYS_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
