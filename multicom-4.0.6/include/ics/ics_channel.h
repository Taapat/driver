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

#ifndef _ICS_CHANNEL_H
#define _ICS_CHANNEL_H

typedef ICS_HANDLE   ICS_CHANNEL;
typedef ICS_VOID *   ICS_CHANNEL_SEND;

/* Callback function for Channels */
typedef ICS_ERROR (*ICS_CHANNEL_CALLBACK) (ICS_CHANNEL channel, ICS_VOID *param, void *buf);

ICS_EXPORT ICS_ERROR ICS_channel_alloc (ICS_CHANNEL_CALLBACK callback,
					ICS_VOID *param,
					ICS_VOID *base, ICS_UINT nslots, ICS_UINT ssize, ICS_UINT flags,
					ICS_CHANNEL *channelp);

ICS_EXPORT ICS_ERROR ICS_channel_free (ICS_CHANNEL channel, ICS_UINT flags);

ICS_EXPORT ICS_ERROR ICS_channel_open (ICS_CHANNEL channel, ICS_UINT flags, ICS_CHANNEL_SEND *schannelp);
ICS_EXPORT ICS_ERROR ICS_channel_close (ICS_CHANNEL_SEND schannel);

ICS_EXPORT ICS_ERROR ICS_channel_send (ICS_CHANNEL_SEND channel, ICS_VOID *buf, ICS_SIZE size, ICS_UINT flags);

ICS_EXPORT ICS_ERROR ICS_channel_recv (ICS_CHANNEL channel, ICS_VOID **bufp, ICS_LONG timeout);
ICS_EXPORT ICS_ERROR ICS_channel_release (ICS_CHANNEL channel, ICS_VOID *buf);

ICS_EXPORT ICS_ERROR ICS_channel_unblock (ICS_CHANNEL channel);

#endif /* _ICS_CHANNEL_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 */
