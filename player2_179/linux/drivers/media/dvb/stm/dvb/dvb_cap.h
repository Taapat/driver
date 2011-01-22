/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Header file for the V4L2 dvp capture device driver for ST SoC
 *
\***********************************************************************/
#ifndef DVB_CAP_H_
#define DVB_CAP_H_

#include "dvb_cap_export.h"

typedef struct cap_v4l_shared_handle_s
{
	struct DeviceContext_s 	cap_device_context;

	unsigned long long		target_latency;
	long long 				audio_video_latency_offset;
	bool 					update_player2_time_mapping;
	unsigned int			cap_irq;
	unsigned int			cap_irq2;
	int*					mapped_cap_registers;
	int*					mapped_vtg_registers;
} cap_v4l2_shared_handle_t;

int cap_set_external_time_mapping (cap_v4l2_shared_handle_t *shared_context, struct StreamContext_s* stream,
                                     unsigned long long nativetime, unsigned long long systemtime);
void cap_invalidate_external_time_mapping (cap_v4l2_shared_handle_t *shared_context);

void cap_set_vsync_offset(cap_v4l2_shared_handle_t *shared_context, long long vsync_offset );
#endif /*DVB_CAP_H_*/

