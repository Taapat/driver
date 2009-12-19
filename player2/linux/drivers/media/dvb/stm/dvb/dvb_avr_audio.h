/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr_audio.h
 * Copyright (c) 2007-2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 dvp output device driver for ST SoC display subsystems header file
 *
\***********************************************************************/

#ifndef DVB_DVP_AUDIO_H_
#define DVB_DVP_AUDIO_H_

#include "dvb_avr.h"

typedef enum {
	EMERGENCY_MUTE_REASON_NONE,
	EMERGENCY_MUTE_REASON_USER,
	EMERGENCY_MUTE_REASON_ACCELERATED,
	EMERGENCY_MUTE_REASON_SAMPLE_RATE_CHANGE,
	EMERGENCY_MUTE_REASON_ERROR
} avr_audio_emergency_mute_reason_t;

typedef enum {
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_44100 = 0,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_48000 = 1,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_88200 = 2,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_176400 = 3,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_96000 = 4,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_192000 = 5,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_32000 = 6,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_16000 = 7,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_22050 = 8,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_24000 = 9,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_64000 = 10,
	AVR_AUDIO_DISCRETE_SAMPLE_RATE_128000 = 11
} avr_audio_discrete_sample_rate_t;

typedef struct dvp_v4l2_audio_handle_s 
{
	avr_v4l2_shared_handle_t *SharedContext;
	struct DeviceContext_s *DeviceContext;

	bool AacDecodeEnable;
	avr_audio_emergency_mute_reason_t EmergencyMuteReason;
	bool FormatRecogniserEnable;
	
	struct class_device *EmergencyMuteClassDevice;
	
	unsigned int SampleRate;
	unsigned int PreviousSampleRate;
	avr_audio_discrete_sample_rate_t DiscreteSampleRate;
	
	struct task_struct *ThreadHandle;
	bool ThreadRunning;
	volatile bool ThreadShouldStop;
	
} avr_v4l2_audio_handle_t;

avr_v4l2_audio_handle_t *AvrAudioNew(avr_v4l2_shared_handle_t *SharedContext);
int AvrAudioDelete(avr_v4l2_audio_handle_t *AudioContext);

bool AvrAudioGetAacDecodeEnable(avr_v4l2_audio_handle_t *AudioContext);
void AvrAudioSetAacDecodeEnable(avr_v4l2_audio_handle_t *AudioContext, bool Enabled);
avr_audio_emergency_mute_reason_t AvrAudioGetEmergencyMuteReason(avr_v4l2_audio_handle_t *AudioContext);
int AvrAudioSetEmergencyMuteReason(avr_v4l2_audio_handle_t *Context, avr_audio_emergency_mute_reason_t reason);
bool AvrAudioGetFormatRecogniserEnable(avr_v4l2_audio_handle_t *AudioContext);
void AvrAudioSetFormatRecogniserEnable(avr_v4l2_audio_handle_t *AudioContext, bool Enabled);

int AvrAudioIoctlOverlayStart(avr_v4l2_audio_handle_t *AudioContext);
int AvrAudioIoctlOverlayStop(avr_v4l2_audio_handle_t *AudioContext);

#endif /*DVB_DVP_AUDIO_H_*/
