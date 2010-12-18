/***********************************************************************
 *
 * File: dvb_v4l2_export.h
 * Copyright (c) 2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Header file for the V4L2 interface driver containing the defines
 * to be exported to the user level.
 *
\***********************************************************************/


#ifndef DVB_V4L2_EXPORT_H_
#define DVB_V4L2_EXPORT_H_

/*
 * V4L2 Implemented Controls 
 *
 */

enum 
{
    V4L2_CID_STM_DVBV4L2_FIRST		= (V4L2_CID_PRIVATE_BASE+300),
    
    V4L2_CID_STM_BLANK,	

    V4L2_CID_STM_AUDIO_O_FIRST,
    V4L2_CID_STM_AUDIO_O_LAST,
    
    V4L2_CID_STM_DVBV4L2_LAST
};


#endif /*DVB_V4L2_EXPORT_H_*/
