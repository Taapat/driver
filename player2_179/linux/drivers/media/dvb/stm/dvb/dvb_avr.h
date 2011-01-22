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
#ifndef DVB_DVP_H_
#define DVB_DVP_H_

#include "dvb_avr_export.h"

typedef struct avr_v4l_shared_handle_s
{
	struct DeviceContext_s 	avr_device_context;

	unsigned long long		target_latency;
	long long 				audio_video_latency_offset;
	bool 					update_player2_time_mapping;
	unsigned int			dvp_irq;
	int*					mapped_dvp_registers;

	void                   *audio_context; //<! Cached for use by avr_set_vsync_offset()

	struct rw_semaphore mixer_settings_semaphore;
	struct snd_pseudo_mixer_settings mixer_settings; //<! Only valid if the magic number is correct
	const struct snd_pseudo_mixer_downmix_rom *downmix_firmware;
} avr_v4l2_shared_handle_t;

int avr_set_external_time_mapping (avr_v4l2_shared_handle_t *shared_context, struct StreamContext_s* stream,
                                     unsigned long long nativetime, unsigned long long systemtime);
void avr_invalidate_external_time_mapping (avr_v4l2_shared_handle_t *shared_context);

void avr_set_vsync_offset(avr_v4l2_shared_handle_t *shared_context, long long vsync_offset );
#endif /*DVB_DVP_H_*/

