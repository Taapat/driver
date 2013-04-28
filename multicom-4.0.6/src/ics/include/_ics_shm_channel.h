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

#ifndef _ICS_SHM_CHANNEL_H
#define _ICS_SHM_CHANNEL_H

/* Local SHM (Receive) channel desc */
typedef struct ics_shm_channel
{
  ICS_CHANNEL			 handle;			/* Associated channel handle */

  ICS_CHANNEL_CALLBACK 		 callback;			/* User callback fn and parameter */
  void 				*param;

  _ICS_OS_MUTEX			 chanLock;			/* Protect this and the chn */

  _ICS_OS_EVENT	      	 	 event;				/* OS Event for blocking receives */

  ICS_UINT		 	 bptr;				/* Local mirror of the SHM chn bptr */

  void				*umem;				/* User supplied FIFO memory (may be NULL) */
  void				*mem;				/* FIFO memory address */
  void 				*fifo;				/* (Mapped) FIFO base address (PAGE aligned) */

  ICS_UINT			 full;				/* Count of number of times handler returned full */

} ics_shm_channel_t;

/* Local SHM (Send) channel desc */
typedef struct ics_shm_channel_send
{
  ICS_UINT			 cpuNum;			/* Target cpu # */
  ICS_UINT			 idx;				/* Target channel index */

  void				*fifo;				/* Mapping of remote FIFO memory */
  ICS_SIZE			 fifoSize;			/* Mapping size */

  ICS_HANDLE			 handle;			/* Target channel handle (debugging only) */

} ics_shm_channel_send_t;

/* SHM (Receive) Channel APIs */
ICS_EXPORT ICS_ERROR  ics_shm_channel_alloc (ICS_CHANNEL_CALLBACK callback, void *param,
					     ICS_VOID *mem, ICS_UINT nslots, ICS_UINT ssize, ICS_HANDLE *handlep);
ICS_EXPORT ICS_ERROR  ics_shm_channel_free (ICS_HANDLE handle);

ICS_EXPORT ICS_ERROR  ics_shm_channel_recv (ICS_HANDLE handle, ICS_ULONG timeout, void **bufp);
ICS_EXPORT ICS_UINT   ics_shm_channel_release (ICS_HANDLE handle, ICS_VOID *buf);

ICS_EXPORT ICS_ERROR  ics_shm_channel_unblock (ICS_HANDLE handle);
ICS_EXPORT ICS_ERROR  ics_shm_channel_unblock_all (void);

/* SHM (Send) Channel APIs */
ICS_EXPORT ICS_ERROR  ics_shm_channel_open (ICS_HANDLE handle, ics_shm_channel_send_t **channelp);
ICS_EXPORT ICS_ERROR  ics_shm_channel_close (ics_shm_channel_send_t *channel);

ICS_EXPORT ICS_ERROR  ics_shm_channel_send (ics_shm_channel_send_t *channel, ICS_VOID *buf, ICS_SIZE size);

/* Main mailbox ISR routine */
ICS_EXPORT void        ics_shm_channel_handler (void *param, ICS_UINT status);
ICS_EXPORT ICS_ERROR   ics_shm_service_channel (ICS_UINT idx);

#endif /* _ICS_SHM_CHANNEL_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */

