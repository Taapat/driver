/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 5F, No. 36 Taiyuan St.
 * Jhubei City
 * Hsinchu County 302, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2008, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  * 
 * it under the terms of the GNU General Public License as published by  * 
 * the Free Software Foundation; either version 2 of the License, or     * 
 * (at your option) any later version.                                   * 
 *                                                                       * 
 * This program is distributed in the hope that it will be useful,       * 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * 
 * GNU General Public License for more details.                          * 
 *                                                                       * 
 * You should have received a copy of the GNU General Public License     * 
 * along with this program; if not, write to the                         * 
 * Free Software Foundation, Inc.,                                       * 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             * 
 *                                                                       * 
 *************************************************************************
 
    Module Name:
    rtmp_wext.h

    Abstract:
    This file was created for wpa_supplicant general wext driver support.

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Shiang      2007/04/03    Initial
    
*/

#ifndef __RTMP_WEXT_H__
#define __RTMP_WEXT_H__

#include <net/iw_handler.h>
#include "rt_config.h"


#define     WPARSNIE    0xdd
#define     WPA2RSNIE   0x30

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT

int wext_notify_event_assoc(
	RTMP_ADAPTER *pAd, 
	USHORT iweCmd, 
	int assoc);

int wext2Rtmp_Security_Wrapper(
	RTMP_ADAPTER *pAd);

#endif

#endif // __RTMP_WEXT_H__

