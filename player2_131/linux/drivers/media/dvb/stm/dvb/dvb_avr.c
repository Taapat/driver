/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 dvp output device driver for ST SoC display subsystems.
 *
\***********************************************************************/

//#define NICK_TEST_HACK

/******************************
 * INCLUDES
 ******************************/
#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/videodev.h>
#include <linux/dvb/video.h>
#include <linux/interrupt.h>
#include <stmdisplay.h>
#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>
#include <linux/device.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include "dvb_module.h"
#include "stmdisplayoutput.h"
#include "stmvout_driver.h"
#include "stm_v4l2.h"
#include "backend.h"
#include "dvb_video.h"
#include "osdev_device.h"
#include "ksound.h"
#include "pseudo_mixer.h"

#include "dvb_avr.h"
#include "dvb_avr_audio.h"
#include "dvb_avr_video.h"

/******************************
 * Private CONSTANTS and MACROS
 ******************************/

#define DVP_ID 0
#define AVR_DEFAULT_TARGET_LATENCY  		390000   // usec

/*
 * ALSA mixer controls
 */
#define STM_CTRL_AUDIO_MUTE               0
#define STM_CTRL_AUDIO_SAMPLE_RATE        STM_CTRL_AUDIO_MUTE + 1
#define STM_CTRL_AUDIO_MASTER_LATENCY     STM_CTRL_AUDIO_SAMPLE_RATE + 1
#define STM_CTRL_AUDIO_HDMI_LATENCY       STM_CTRL_AUDIO_MASTER_LATENCY + 1
#define STM_CTRL_AUDIO_SPDIF_LATENCY      STM_CTRL_AUDIO_HDMI_LATENCY + 1
#define STM_CTRL_AUDIO_ANALOG_LATENCY     STM_CTRL_AUDIO_SPDIF_LATENCY + 1

/*
 * A/V receiver controls to control the "I2S0" audio input
 */
#define V4L2_CID_STM_AUDIO_MUTE              (V4L2_CID_PRIVATE_BASE+110)
#define V4L2_CID_STM_AUDIO_FORMAT_RECOGNIZER (V4L2_CID_PRIVATE_BASE+116)
#define V4L2_CID_STM_AUDIO_AAC_DECODE	     (V4L2_CID_PRIVATE_BASE+117)

#define AncillaryAssert( v, s )		if( !(v) ) { printk( "Assert fail - %s\n", s ); return -EINVAL; }

#define DVP_CAPTURE_OFF 0
#define DVP_CAPTURE_ON  1

/******************************
 * FUNCTION PROTOTYPES
 ******************************/

extern int stm_v4l2_register_driver(struct stm_v4l2_driver *driver);

/******************************
 * GLOBAL VARIABLES
 ******************************/

/*  AUDIO OUTPUT Devices

    * "MIXER0_PRIMARY" audio, primary audio mixer input
    * "MIXER0_SECONDARY" audio, secondary audio mixer input
    * "MIXER1_PRIMARY" audio, physically independent audio device used for second room audio output
*/
struct staout_description {
  char name[32];
  int audioId; // id associated with audio description
  int  deviceId;
  int  virtualId;
};

//< Describes the audio output surfaces to user
static struct staout_description g_aoutDevice[] = {
  { "MIXER0_PRIMARY",    0, 0, 0},  //<! "MIXER0_PRIMARY" audio, primary audio mixer input
  { "MIXER0_SECONDARY",  0, 0, 0},  //<! "MIXER0_SECONDARY" audio, secondary audio mixer input
  { "MIXER1_PRIMARY",    0, 0, 0},  //<! "MIXER1_PRIMARY", audio, physically independent audio device used for second room audio output
};

const int g_aoutCount = sizeof(g_aoutDevice)/sizeof(struct staout_description);


/*  Video Capture Devices
    * "D1-DVP0", video input taken from the DVP
    * "VIDEO0", decoded video produced by the primary video pipeline (/dev/dvb/adapter0/video0)
    * "VIDEO1", decoded video produced by the secondary video pipeline (/dev/dvb/adapter0/video1)
    * "VIDEO2", decoded video produced by the BD-J MPEG drip pipeline (/dev/dvb/adapter0/video2)
*/
struct stvin_description {
  char name[32];
  int  deviceId;
  int  virtualId;
};

//< Describes the input surfaces to user
static struct stvin_description g_vinDevice[] = {
  { "NULL", 0, 0 },
  { "D1-DVP0", 0, 0 }
//  { "VIDEO0",  0, 0 },
//  { "VIDEO1",  0, 0 },
//  { "VIDEO2",  0, 0 }
};

const int g_vinCount = sizeof(g_vinDevice)/sizeof(struct stvin_description);


/*  Audio Capture Devices
    * "I2S0", audio input taken from the PCM reader
    * "I2S1"
    * "AUDIO0", decoded audio produced by the primary audio pipeline (/dev/dvb/adapter0/audio0)
    * "AUDIO1", decoded audio produced by the secondary audio pipeline (/dev/dvb/adapter0/audio1)
*/
struct stain_description {
  char name[32];
  int audioId;  // id associated with audio description)
  int  deviceId;
  int  virtualId;
};

//< Describes the audio input surfaces to user
static struct stain_description g_ainDevice[] = {
  { "NULL", 0, 0 },
  { "I2S0", 0, 0 },
  { "I2S1", 0, 0 }
//  { "AUDIO0",  0, 0 },
//  { "AUDIO1",  0, 0 }
};

const int g_ainCount = sizeof(g_ainDevice)/sizeof(struct stain_description);

/******************************
 * EXTERNAL VISIBLE INTERFACES
 ******************************/

/* avr_set_external_time_mapping ()
 * This function enables the external time mapping required for AVsynchronization. In this way, the system time is used to
 * sync audio and video. This function needs to be called always (even for audion only and video only) but only once,
 * as the second value would overwrite the first one and create an indesired activity into the player.
 *
 * NOTE1: SystemTime, NativeTime and the Pts values must be max 33 bits long ->  &= 0x1ffffffffULL
 * NOTE2: all the timings values are expressed in usecs in these functions */
int avr_set_external_time_mapping (avr_v4l2_shared_handle_t *shared_context, struct StreamContext_s* stream,
				     unsigned long long nativetime, unsigned long long systemtime)
{
	int result = 0;
	struct PlaybackContext_s* playback = shared_context->avr_device_context.Playback;

	//
	// If already set then just return
	//

	if( !shared_context->update_player2_time_mapping )
		return 0;

	shared_context->update_player2_time_mapping	= false;

	//

	result  = StreamSetOption (stream, PLAY_OPTION_EXTERNAL_TIME_MAPPING, 1);
	if (result < 0) {
		DVB_ERROR("PLAY_OPTION_EXTERNAL_TIME_MAPPING set failed\n");
		return -EINVAL;
	}

	//
	// Modify the system time with the specified latency,
	// this is contained within a glbal variable, I anticipate
	// that a control will be implemented to set it before
	// starting (or switching the mode of) a capture.
	//

	systemtime		+= shared_context->target_latency;

	//

	/* compensate the mapping if audio_video_latency_offset is negative. -ve values mean that the player
	 * will make audio appear earlier which would cause the minimum system latency value to be exceeded.
	 * by adjusting the systemtime by the audio_video_latency_offset we are effectively keeping the
	 * audio latency the same and changing increasing the video latency instead.
	 *
	 * TODO: we should unconditionally apply the offset, at present it is harmless to shorten the video
	 *       latency. however we intend to tune the audio latency (mostly by being less rigorous in our
	 *       handling of DTS). at this point we run the risk of overshortening the video latency so for
	 *       now we handle audio-trails-video by extending the audio latency.
	 */
	if (shared_context->audio_video_latency_offset < 0) {
#if 0
		DVB_DEBUG("Munging nativetime (%lld -> %lld)\n",
				nativetime, nativetime + audio_video_latency_offset);
#endif
		nativetime += shared_context->audio_video_latency_offset;
	}

	result = PlaybackSetNativePlaybackTime(playback, nativetime, systemtime);
	if (result < 0) {
		DVB_ERROR("PlaybackSetNativePlaybackTime failed with %d\n", result);
		return -EINVAL;
	}

	return result;
}

/*
 * Invalidate the mapping we already have
 */
void avr_invalidate_external_time_mapping (avr_v4l2_shared_handle_t *shared_context)
{
	shared_context->update_player2_time_mapping	= true;
}

/*
 * Notify the capture code that the master audio/video offset has been changed.
 *
 * This function is registered with the pseudocard and is called whenever the mixer settings are changed.
 * In reaction we must update audio_video_latency_offset and ensure that the player's external time mapping
 * is updated to account for the new latency values.
 */
static void avr_update_external_time_mapping (void *ctx, const struct snd_pseudo_mixer_settings *mixer_settings)
{
	avr_v4l2_shared_handle_t *shared_context = ctx;
	long long old_offset = shared_context->audio_video_latency_offset;

	// record the current A/V mapping
	shared_context->audio_video_latency_offset = mixer_settings->master_latency * 90; // convert ms to PES time units

	// provoke an update if required
	if (old_offset != shared_context->audio_video_latency_offset)
		shared_context->update_player2_time_mapping = true;
}


/******************************
 * IOCTL MANAGEMENT
 ******************************/


static int avr_ioctl_overlay_start(avr_v4l2_shared_handle_t *shared_context,
				   struct stm_v4l2_handles *handle)
{
	int ret = 0;

	// Always setup a new mapping when we start a new overlay
	avr_invalidate_external_time_mapping(shared_context);

	if (shared_context->avr_device_context.Playback == NULL)
	{
		ret = PlaybackCreate (&shared_context->avr_device_context.Playback);
		if (ret < 0)
		{
			DVB_ERROR("PlaybackCreate failed\n" );
			return -EINVAL;
		}
	}

	// apply any 'sticky' policies (ones that can't be changes after startup) before going any further

	/* select the system clock as master. we are timestamping incoming data using the system clock
	 * so we must ensure that the output system uses an identical clock or the input and output will
	 * drift appart.
	 */
	ret = PlaybackSetOption (shared_context->avr_device_context.Playback,
			         PLAY_OPTION_MASTER_CLOCK, PLAY_OPTION_VALUE_SYSTEM_CLOCK_MASTER);
	if (ret < 0) {
		DVB_ERROR("PLAY_OPTION_VALUE_SYSTEM_CLOCK_MASTER coult not be set\n" );
		return -EINVAL;
	}

	// Audio Start - if and only if an audio input has been selected (handle is non-NULL)

	if (shared_context->avr_device_context.AudioCaptureStatus == DVP_CAPTURE_ON)
	{
		if (handle->v4l2type[STM_V4L2_AUDIO_INPUT].driver && handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle) {
			ret = AvrAudioIoctlOverlayStart(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
			if (0 != ret) {
				DVB_ERROR("AvrAudioIoctlOverlayStart failed\n" );
				return ret;
			}
		}
	}

	// Video Start - if and only if a video input has been selected (handle is non-NULL)

	if (shared_context->avr_device_context.VideoCaptureStatus == DVP_CAPTURE_ON)
	{
		if (handle->v4l2type[STM_V4L2_VIDEO_INPUT].driver && handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle) {
			ret = DvpVideoIoctlOverlayStart( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle );
			if( ret != 0 ) {
				DVB_ERROR("DvpVideoIoctlOverlayStart failed\n" );
				return ret;
			}
		}
	}

	return 0;
}


static int avr_ioctl_overlay_stop(avr_v4l2_shared_handle_t *shared_context,
				  struct stm_v4l2_handles *handle)
{
	int ret = 0;

	if (shared_context->avr_device_context.Playback == NULL) {
		//DVB_DEBUG("Playback already NULL\n");
		return 0;
	}

	if (shared_context->avr_device_context.AudioCaptureStatus == DVP_CAPTURE_OFF)
	{
		if(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle)
			AvrAudioIoctlOverlayStop(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
	}

	if (shared_context->avr_device_context.VideoCaptureStatus == DVP_CAPTURE_OFF)
	{
		if(handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle) {
		    ret = DvpVideoClose( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle );
			if (ret < 0) {
				DVB_ERROR("DvpVideoClose failed \n"); // no recovery possible
				return -EINVAL;
			}
		}
	}

	if ((shared_context->avr_device_context.VideoCaptureStatus == DVP_CAPTURE_OFF) &&
			(shared_context->avr_device_context.AudioCaptureStatus == DVP_CAPTURE_OFF))
	{
		ret = PlaybackDelete(shared_context->avr_device_context.Playback);
		if (ret < 0) {
	            DVB_ERROR("PlaybackDelete failed\n");
	            return -EINVAL;
		}
		shared_context->avr_device_context.Playback = NULL;
	}

	return 0;
}

/**
 * Select a the video input specified by id.
 *
 * Currently we assume we know what id was (because we only support one)
 *
 * \todo Most of this initialization ought to migrate into the video driver.
 */
static int avr_ioctl_video_set_input(avr_v4l2_shared_handle_t *shared_context,
				     struct stm_v4l2_handles *handle,
				     int id)
{
#ifdef NICK_TEST_HACK
static dvp_v4l2_video_handle_t *video_context = NULL;

    if( video_context != NULL )
    {
	handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle = video_context;
	return 0;
    }
#else
   	dvp_v4l2_video_handle_t *video_context;
#endif

   	if (handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle == NULL)
   	{
   		// allocate video handle for driver registration

   		video_context = kzalloc(sizeof(dvp_v4l2_video_handle_t),GFP_KERNEL);
   		if (NULL == video_context) {
   			DVB_ERROR("Handle allocation failed\n");
   			return -ENOMEM;
   		}

   		video_context->SharedContext = shared_context;
   		video_context->DeviceContext = &shared_context->avr_device_context;
   		video_context->DvpRegs = shared_context->mapped_dvp_registers;
   		video_context->DvpIrq = shared_context->dvp_irq;
   		video_context->DvpLatency = shared_context->target_latency;
   		DvpVideoIoctlSetControl( video_context, V4L2_CID_STM_DVPIF_RESTORE_DEFAULT, 0 );

   		handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle = video_context;
   	}
   	else
   		DVB_DEBUG ("Video Handle already allocated\n");

	return 0;
}

/**
 * Select a the audio input specified by id.
 *
 * \todo Honour id! We are going to be supporting two inputs soon...
 */
static int avr_ioctl_audio_set_input(avr_v4l2_shared_handle_t *shared_context,
				     struct stm_v4l2_handles *handle,
				     int id)
{
   	if (handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle == NULL)
   	{
   		// allocate audio handle for driver registration
   		handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle = AvrAudioNew(shared_context);
   		if (handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle == NULL) {
   			DVB_ERROR("Handle allocation failed\n");
   			return -ENOMEM;
   		}
   	}
   	else
   		DVB_DEBUG ("Audio Handle already allocated\n");

	return 0;
}

static int avr_ioctl(struct stm_v4l2_handles *handle, struct stm_v4l2_driver *driver,
		     int device, int type, struct file *file, unsigned int cmd, void *arg)
{
    avr_v4l2_shared_handle_t *shared_context = driver->priv;
    int res;

    switch(cmd)
    {

   	/**********************************************************/
   	/*  		VIDEO IOCTL CALLS				  			  */
	/**********************************************************/


   	case VIDIOC_S_FBUF:
	{
	    struct v4l2_framebuffer *argp = arg;

	    DvpVideoIoctlSetFramebuffer( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle,
					 argp->fmt.width, argp->fmt.height, argp->fmt.bytesperline, argp->fmt.priv );
	    break;
	}

	case VIDIOC_S_STD:
	{
	    v4l2_std_id id = *((v4l2_std_id *)arg);

	    res	= DvpVideoIoctlSetStandard( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, id );
	    if( res != 0 )
		return res;

	    break;
	}

	case VIDIOC_ENUMINPUT:
	{
		struct v4l2_input* input = arg;
		int input_id = 0;

		input_id = input->index - driver->start_index[device];
		if ( input_id < 0  || input_id >= g_vinCount ){
			//DVB_ERROR("VIDIOC_ENUMINPUT: Input number (%d) out of range\n", input_id );
			return -EINVAL;
		}

		strcpy(input->name, g_vinDevice[input_id].name);

		break;
	}

	case VIDIOC_S_INPUT:
	{
	    int input_id = *((unsigned int*)arg);

	    input_id -= driver->start_index[device];
		if (input_id < 0  || input_id >= g_vinCount ){
			//DVB_ERROR("VIDIOC_S_INPUT: Input number (%d) out of range\n", input_id );
			return -EINVAL;
		}

		// the only values I can get are 0 and 1 for now

		if (input_id == 0)
			shared_context->avr_device_context.VideoCaptureStatus = DVP_CAPTURE_OFF;
		else
			shared_context->avr_device_context.VideoCaptureStatus = DVP_CAPTURE_ON;

		res = avr_ioctl_video_set_input(shared_context, handle, input_id);
		if (res) return res;

    	break;
	}

	// this probably needs to call stream on when finished however for now
	// we make this to mean we will start capturing into our buffer
	case VIDIOC_OVERLAY:
	{
		int *start = arg;

		if (!start) return -EINVAL;
		if (!handle) return -EINVAL;

		if (*start == 1) // START
		{
			res = avr_ioctl_overlay_start(shared_context, handle);
			if (0 != res) return res;
		}
		else if (*start == 0) // STOP
		{
			res = avr_ioctl_overlay_stop(shared_context, handle);
			if (0 != res) return res;
		}
		else
		{
		    DVB_ERROR("Option not supported, I'm sorry.\n" );
		    return -EINVAL;
		}

	    break;
	}

	case VIDIOC_S_CROP:
	{
		struct v4l2_crop *crop = (struct v4l2_crop*)arg;

		res = DvpVideoIoctlCrop( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, crop );
		if( res != 0 )
		    return res;

		break;
	}


   	/**********************************************************/
   	/*  		Ancillary data capture IOCTL CALLS			  */
	/**********************************************************/


	case VIDIOC_REQBUFS:
	{
	struct v4l2_requestbuffers 	*request = (struct v4l2_requestbuffers *)arg;
	dvp_v4l2_video_handle_t		*VideoContext = (dvp_v4l2_video_handle_t *)handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle;
	unsigned int			 i;

	    AncillaryAssert( (request->type == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_REQBUFS: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );
	    AncillaryAssert( (request->memory == V4L2_MEMORY_MMAP), "VIDIOC_REQBUFS: Memory not V4L2_MEMORY_MMAP" );

	    res	= DvpVideoIoctlAncillaryRequestBuffers( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle,
							request->count, request->reserved[0], &request->count, &request->reserved[0] );
	    if( res != 0 )
		return res;

	    for( i=0; i<request->count; i++ )
		VideoContext->AncillaryBufferState[i].Mapped	= false;

	    break;
	}

	case VIDIOC_QUERYBUF:
	{
	struct  v4l2_buffer 	*buffer = (struct v4l2_buffer *)arg;
	dvp_v4l2_video_handle_t	*VideoContext = (dvp_v4l2_video_handle_t *)handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle;
	bool			 Queued;
	bool			 Done;
	unsigned char 		*PhysicalAddress;
	unsigned char 		*UnCachedAddress;
	unsigned long long	 CaptureTime;
	unsigned int		 Bytes;
	unsigned int		 Size;

	    AncillaryAssert( (buffer->type == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_QUERYBUF: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );

	    res	= DvpVideoIoctlAncillaryQueryBuffer( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle,
						     buffer->index, &Queued, &Done, &PhysicalAddress, &UnCachedAddress, &CaptureTime, &Bytes, &Size );
	    if( res != 0 )
		return res;

	    buffer->bytesused		= Bytes;
	    buffer->flags		= (Queued ? V4L2_BUF_FLAG_QUEUED : 0) |
					  (Done   ? V4L2_BUF_FLAG_DONE   : 0) |
					  (VideoContext->AncillaryBufferState[buffer->index].Mapped ? V4L2_BUF_FLAG_MAPPED : 0);
	    buffer->field		= V4L2_FIELD_NONE;
	    buffer->timestamp.tv_sec	= (unsigned int)(CaptureTime / 1000000);
	    buffer->timestamp.tv_usec	= (unsigned int)(CaptureTime - (1000000 * buffer->timestamp.tv_sec));
	    //buffer->timecode	= ;
	    buffer->sequence		= 0;
	    buffer->memory		= V4L2_MEMORY_MMAP;
	    buffer->m.offset		= PhysicalAddress - VideoContext->AncillaryBufferPhysicalAddress;
	    buffer->length		= Size;

	    break;
	}

	case VIDIOC_QBUF:
	{
	struct v4l2_buffer 	*buffer = (struct v4l2_buffer *)arg;
	dvp_v4l2_video_handle_t	*VideoContext = (dvp_v4l2_video_handle_t *)handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle;

	    AncillaryAssert( (buffer->type == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_QBUF: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );
	    AncillaryAssert( (buffer->memory == V4L2_MEMORY_MMAP), "VIDIOC_QBUF: Memory not V4L2_MEMORY_MMAP" );

	    res	= DvpVideoIoctlAncillaryQueueBuffer( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, buffer->index );
	    if( res != 0 )
		return res;

	    buffer->flags	= V4L2_BUF_FLAG_QUEUED |
				  (VideoContext->AncillaryBufferState[buffer->index].Mapped ? V4L2_BUF_FLAG_MAPPED : 0);

	    break;
	}

	case VIDIOC_DQBUF:
	{
	struct  v4l2_buffer 	*buffer = (struct v4l2_buffer *)arg;
	dvp_v4l2_video_handle_t	*VideoContext = (dvp_v4l2_video_handle_t *)handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle;
	bool			 Blocking;
	bool			 Queued;
	bool			 Done;
	unsigned char 		*PhysicalAddress;
	unsigned char 		*UnCachedAddress;
	unsigned long long	 CaptureTime;
	unsigned int		 Bytes;
	unsigned int		 Size;

	    AncillaryAssert( (buffer->type == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_DQBUF: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );
	    AncillaryAssert( (buffer->memory == V4L2_MEMORY_MMAP), "VIDIOC_DQBUF: Memory not V4L2_MEMORY_MMAP" );

	    Blocking		= (file->f_flags & O_NONBLOCK) == 0;

	    res	= DvpVideoIoctlAncillaryDeQueueBuffer( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, &buffer->index, Blocking );
	    if( res != 0 )
		return res;

	    res	= DvpVideoIoctlAncillaryQueryBuffer( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle,
						     buffer->index, &Queued, &Done, &PhysicalAddress, &UnCachedAddress, &CaptureTime, &Bytes, &Size );
	    if( res != 0 )
		return res;

	    buffer->bytesused		= Bytes;
	    buffer->flags		= V4L2_BUF_FLAG_TIMECODE 	      |
					  (Queued ? V4L2_BUF_FLAG_QUEUED : 0) |
					  (Done   ? V4L2_BUF_FLAG_DONE   : 0) |
					  (VideoContext->AncillaryBufferState[buffer->index].Mapped ? V4L2_BUF_FLAG_MAPPED : 0);
	    buffer->field		= V4L2_FIELD_NONE;
	    buffer->timestamp.tv_sec	= (unsigned int)(CaptureTime / 1000000);
	    buffer->timestamp.tv_usec	= (unsigned int)(CaptureTime - (1000000 * buffer->timestamp.tv_sec));
	    //buffer->timecode		= ;
	    buffer->sequence		= 0;
	    buffer->memory		= V4L2_MEMORY_MMAP;
	    buffer->m.offset		= PhysicalAddress - VideoContext->AncillaryBufferPhysicalAddress;
	    buffer->length		= Size;

	    break;
	}

	case VIDIOC_STREAMON:
	{
	int *type = (int *)arg;

	    AncillaryAssert( ((*type) == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_STREAMON: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );

	    res	= DvpVideoIoctlAncillaryStreamOn( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle );
	    if( res != 0 )
		return res;

	    break;
	}

	case VIDIOC_STREAMOFF:
	{
	int *type = (int *)arg;

	    AncillaryAssert( ((*type) == V4L2_BUF_TYPE_VBI_CAPTURE), "VIDIOC_STREAMOFF: Type not V4L2_BUF_TYPE_VBI_CAPTURE" );

	    res	= DvpVideoIoctlAncillaryStreamOff( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle );
	    if( res != 0 )
		return res;

	    break;
	}


   	/**********************************************************/
   	/*  		AUDIO IOCTL CALLS  							  */
	/**********************************************************/


	case VIDIOC_ENUMAUDIO:
	{
	    struct v4l2_audio *audio = arg;
	    int input_id = 0;

	    input_id = audio->index - driver->start_index[device];
	    if (input_id < 0 || input_id >= g_ainCount) {
			//DVB_ERROR("VIDIOC_ENUMAUDIO: Input number out of range\n" );
			return -EINVAL;
	    }

	    strcpy(audio->name, g_ainDevice[input_id].name);

	    break;
	}

	case VIDIOC_S_AUDIO:
	{
		struct v4l2_audio *audio = arg;
	    int input_id = 0;

	    input_id = audio->index - driver->start_index[device];
	    if (input_id < 0 || input_id >= g_ainCount) {
			//DVB_ERROR("VIDIOC_S_AUDIO: Input number out of range\n" );
			return -EINVAL;
		}

	    if (input_id == 0)
	    	shared_context->avr_device_context.AudioCaptureStatus = DVP_CAPTURE_OFF;
	    else
	    	shared_context->avr_device_context.AudioCaptureStatus = DVP_CAPTURE_ON;

		res = avr_ioctl_audio_set_input(shared_context, handle, input_id);
		if (res) return res;

		g_ainDevice[input_id].audioId = input_id;

		break;
	}

	case VIDIOC_ENUMAUDOUT:
	{
	    struct v4l2_audioout *audioout = arg;

	    audioout->index -= driver->start_index[device];
	    if (audioout->index < 0 || audioout->index >= g_aoutCount) {
		DVB_ERROR("VIDIOC_ENUMAUDOUT: Output number out of range %d\n", audioout->index);
		return -EINVAL;
	    }

	    strcpy(audioout->name, g_aoutDevice[audioout->index].name);

	    break;
	}

	case VIDIOC_S_AUDOUT:
	{
	    struct v4l2_audioout *audioout = arg;

	    audioout->index -= driver->start_index[device];
	    if (audioout->index < 0 || audioout->index >= g_aoutCount) {
		DVB_ERROR("VIDIOC_S_AUDOUT: Output number out of range %d\n", audioout->index);
		return -EINVAL;
	    }

	    // allocate handle for driver registration

	    handle->v4l2type[STM_V4L2_AUDIO_OUTPUT].handle = kmalloc(sizeof(struct v4l2_audioout),GFP_KERNEL);
	    if (handle->v4l2type[STM_V4L2_AUDIO_OUTPUT].handle == NULL) {
		DVB_ERROR("VIDIOC_S_AUDOUT: kmalloc failed\n" );
		return -EINVAL;
	    }

	    g_aoutDevice[audioout->index].audioId = audioout->index;

	    break;
	}


   	/**********************************************************/
   	/*  		VIDEO AND AUDIO CONTROLS					  */
	/**********************************************************/


	case VIDIOC_S_CTRL:
	{
	    ULONG  ctrlid 		= 0;
	    ULONG  ctrlvalue 		= 0;
	    struct v4l2_control* pctrl 	= arg;

	    ctrlid 		= pctrl->id;
	    ctrlvalue 		= pctrl->value;

	    switch (ctrlid)
	    {
	        case V4L2_CID_STM_AUDIO_FORMAT_RECOGNIZER:
		    if ((ctrlvalue != 0) && (ctrlvalue != 1))
			 return -ERANGE;
		    AvrAudioSetFormatRecogniserEnable(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle, ctrlvalue);
	            break;

	        case V4L2_CID_STM_AUDIO_AAC_DECODE:
		    if ((ctrlvalue != 0) && (ctrlvalue != 1))
			return -ERANGE;
		    AvrAudioSetAacDecodeEnable(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle, ctrlvalue);
	            break;

	        case V4L2_CID_STM_AUDIO_MUTE: // 1 = mute on; 0 = mute off
	        {
	            int ret = 0;

	            if ((ctrlvalue != 0) && (ctrlvalue != 1)) return -ERANGE;

	            ret = AvrAudioSetEmergencyMuteReason(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle, ctrlvalue);
	            if (ret < 0) {
	        	DVB_ERROR("dvp_deploy_emergency_mute failed\n" );
			return -EINVAL;
	            }
	            break;
	        }

	        default:
	        {
	            int ret = 0;

		    ret = DvpVideoIoctlSetControl( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, ctrlid, ctrlvalue );
		    if( ret < 0 )
		    {
		        DVB_ERROR("Set control invalid\n" );
	                return -EINVAL;
		    }
		}
	    }

	    break;
	}

	case VIDIOC_G_CTRL:
	{
	    int ret;
	    struct v4l2_control* pctrl = arg;

	    switch (pctrl->id)
	    {
	        case V4L2_CID_STM_AUDIO_FORMAT_RECOGNIZER:
	            pctrl->value = AvrAudioGetFormatRecogniserEnable(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
	            break;

	        case V4L2_CID_STM_AUDIO_AAC_DECODE:
	            pctrl->value = AvrAudioGetAacDecodeEnable(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
	            break;

	        case V4L2_CID_STM_AUDIO_MUTE:
	            pctrl->value = AvrAudioGetEmergencyMuteReason(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
	            break;

	        default:
		    ret = DvpVideoIoctlGetControl( handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle, pctrl->id, &pctrl->value );
		    if( ret < 0 )
		    {
		        DVB_ERROR("Get control invalid\n" );
	                return -EINVAL;
		    }
	    }

	    break;
	}

	default:
	{
	    return -ENODEV;
	}

    } // end switch (cmd)

    return 0;
}

static int avr_close(struct stm_v4l2_handles *handle, int type, struct file  *file)
{
	int ret = 0;
	avr_v4l2_shared_handle_t *shared_context = NULL;

	// all the v4l2 drivers with the same handle share the same private data
	// --> fetch just one

	if (!handle->v4l2type[STM_V4L2_VIDEO_INPUT].driver) {
		if (!handle->v4l2type[STM_V4L2_AUDIO_INPUT].driver) {
			DVB_DEBUG("Video and Audio handles NULL. Nothing to do.\n");
			return -EINVAL;
		}
		shared_context = handle->v4l2type[STM_V4L2_AUDIO_INPUT].driver->priv;
	}
	else {
		shared_context = handle->v4l2type[STM_V4L2_VIDEO_INPUT].driver->priv;
	}

	shared_context->avr_device_context.AudioCaptureStatus = DVP_CAPTURE_OFF;
	shared_context->avr_device_context.VideoCaptureStatus = DVP_CAPTURE_OFF;

	ret = avr_ioctl_overlay_stop(shared_context, handle);
	if (ret) {
		DVB_ERROR("avr_ioctl_overlay_stop failed\n");
		return -EINVAL;
	}

	// free the handles

	if (handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle)
		kfree(handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle);

	if (handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle) {
		ret = AvrAudioDelete(handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle);
		if (ret) return ret; // error already reported
	}

	if (handle->v4l2type[STM_V4L2_AUDIO_OUTPUT].handle)
		kfree(handle->v4l2type[STM_V4L2_AUDIO_OUTPUT].handle);

	handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle = NULL;
	handle->v4l2type[STM_V4L2_AUDIO_INPUT].handle = NULL;
	handle->v4l2type[STM_V4L2_AUDIO_OUTPUT].handle = NULL;

	return 0;
}

// ////////////////////////////////////////////////////////////////////////////////////
//
// MMap code for ancillary buffers
//
//    This is almost entirely stolen from Pete's code in dvb_v4l2.c
//    except for the range check. Since my buffers are likely to be
//    only 32/64 bytes long, I will only allow the mapping of all of them
//    in one lump.
//

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static struct page* avr_vm_nopage(struct vm_area_struct *vma, unsigned long vaddr, int *type)
{
    struct page	*page;
    void		*page_addr;
    unsigned long	 page_frame;

    if (vaddr > vma->vm_end)
	return NOPAGE_SIGBUS;

    /*
     * Note that this assumes an identity mapping between the page offset and
     * the pfn of the physical address to be mapped. This will get more complex
     * when the 32bit SH4 address space becomes available.
     */

    page_addr	= (void*)((vaddr - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT) +
			  ((dvp_v4l2_video_handle_t *)(vma->vm_private_data))->AncillaryBufferPhysicalAddress);
    page_frame 	= ((unsigned long)page_addr >> PAGE_SHIFT);

    if(!pfn_valid(page_frame))
	return NOPAGE_SIGBUS;

    page = virt_to_page(__va(page_addr));

    get_page(page);

    if (type)
	*type = VM_FAULT_MINOR;

    return page;
}
#else /* >= 2.6.24 */
static int avr_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page     *page;
	void            *page_addr;
	unsigned long    page_frame;

	if (vmf->virtual_address > vma->vm_end)
		return VM_FAULT_SIGBUS;

	/*
	 * Note that this assumes an identity mapping between the page offset and
	 * the pfn of the physical address to be mapped. This will get more complex
	 * when the 32bit SH4 address space becomes available.
	 */

	page_addr = (void*)(((unsigned long) vmf->virtual_address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT) +
			((dvp_v4l2_video_handle_t *)(vma->vm_private_data))->AncillaryBufferPhysicalAddress);
	page_frame  = ((unsigned long)page_addr >> PAGE_SHIFT);

	if(!pfn_valid(page_frame))
		return VM_FAULT_SIGBUS;

	page = virt_to_page(__va(page_addr));
	get_page(page);
	vmf->page = page;
	return 0;
}
#endif /* >= 2.6.24 */

static void avr_vm_open(struct vm_area_struct *vma)
{
	//DVB_DEBUG("/n");
}

static void avr_vm_close(struct vm_area_struct *vma)
{
	//DVB_DEBUG("/n");
}

static struct vm_operations_struct avr_vm_ops_memory =
{
	.open     = avr_vm_open,
	.close    = avr_vm_close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2.6.24)
	.nopage   = avr_vm_nopage,
#else
	.fault    = avr_vm_fault,
#endif
};

static int avr_mmap(struct stm_v4l2_handles *handle, int type, struct file *file, struct vm_area_struct*  vma)
{
unsigned int		 i;
unsigned int		 ValidMapSize;
dvp_v4l2_video_handle_t	*VideoContext = (dvp_v4l2_video_handle_t *)handle->v4l2type[STM_V4L2_VIDEO_INPUT].handle;

//

    if (!(vma->vm_flags & VM_WRITE))
	return -EINVAL;

    ValidMapSize	= (VideoContext->AncillaryBufferCount * VideoContext->AncillaryBufferSize) + (1 << PAGE_SHIFT) - 1;
    ValidMapSize	= ValidMapSize & (~((1 << PAGE_SHIFT) - 1));

    if( (vma->vm_pgoff != 0) || ((vma->vm_end - vma->vm_start) != ValidMapSize) )
    {
	printk( "avr_mmap - Only whole buffer memory can be mapped - Invalid address range should be Offset = 0, length = Count * Size(%d*%d)[%08lx %04lx]\n", VideoContext->AncillaryBufferCount, VideoContext->AncillaryBufferSize, vma->vm_pgoff, (vma->vm_end - vma->vm_start) );
	return -EINVAL;
    }

    vma->vm_flags       |= VM_DONTEXPAND | VM_LOCKED;
    vma->vm_page_prot    = pgprot_noncached(vma->vm_page_prot);
    vma->vm_private_data = VideoContext;
    vma->vm_ops 	 = &avr_vm_ops_memory;

//

    for( i=0; i<VideoContext->AncillaryBufferCount; i++ )
	VideoContext->AncillaryBufferState[i].Mapped	= true;

//

    return 0;
}

//
// End of MMap code for ancillary buffers
//
// ////////////////////////////////////////////////////////////////////////////////////

struct stm_v4l2_driver avr_video_overlay = {
	.type            	= STM_V4L2_VIDEO_INPUT,
	.number_controls 	= {1,1,1},
	.number_indexs   	= {2,2,2},
	.ioctl      		= avr_ioctl,
	.close      		= avr_close,
	.poll       		= NULL,
	.mmap       		= avr_mmap,
};

struct stm_v4l2_driver avr_audio_overlay = {
	.type            	= STM_V4L2_AUDIO_INPUT,
	.number_controls 	= {1,1,1},
	.number_indexs   	= {3,3,3},
	.ioctl      		= avr_ioctl,
	.close      		= avr_close,
	.poll       		= NULL,
	.mmap       		= NULL,
};

struct stm_v4l2_driver avr_audio_output = {
	.type            	= STM_V4L2_AUDIO_OUTPUT,
	.number_controls 	= {1,1,1},
	.number_indexs   	= {3,3,3},
	.ioctl      		= avr_ioctl,
	.close      		= avr_close,
	.poll       		= NULL,
	.mmap       		= NULL,
};

/* Platform procedures */

static avr_v4l2_shared_handle_t *avr_v4l2_shared_context_new(void)
{
	avr_v4l2_shared_handle_t *ctx = kzalloc(sizeof(avr_v4l2_shared_handle_t), GFP_KERNEL);

	if (!ctx)
		return NULL;

	// Initialize avr_device_context
	ctx->avr_device_context.Id                       = DVP_ID;
	ctx->avr_device_context.DemuxContext             = &ctx->avr_device_context;        /* wire directly to own demux by default */
	ctx->avr_device_context.Playback                 = NULL;
	ctx->avr_device_context.StreamType               = STREAM_TYPE_TRANSPORT;
	ctx->avr_device_context.DvbContext               = NULL; // DvbContext;
	ctx->avr_device_context.DemuxStream              = NULL;
	ctx->avr_device_context.VideoStream              = NULL;
	ctx->avr_device_context.AudioStream              = NULL;

	ctx->avr_device_context.VideoState.video_blank             = 0;
	ctx->avr_device_context.VideoState.play_state              = VIDEO_STOPPED;
	ctx->avr_device_context.VideoState.stream_source           = VIDEO_SOURCE_MEMORY;
	ctx->avr_device_context.VideoState.video_format            = VIDEO_FORMAT_16_9;
	ctx->avr_device_context.VideoState.display_format          = VIDEO_FULL_SCREEN;
	ctx->avr_device_context.VideoSize.w                        = 0;
	ctx->avr_device_context.VideoSize.h                        = 0;
	ctx->avr_device_context.VideoSize.aspect_ratio             = 0;
	ctx->avr_device_context.FrameRate                          = 0;
	ctx->avr_device_context.PlaySpeed                          = DVB_SPEED_NORMAL_PLAY;
	ctx->avr_device_context.VideoId                            = DEMUX_INVALID_ID;     /* CodeToInteger('v','i','d','s'); */
	ctx->avr_device_context.VideoEncoding                      = VIDEO_ENCODING_AUTO;
	ctx->avr_device_context.VideoEvents.Write                  = 0;
	ctx->avr_device_context.VideoEvents.Read                   = 0;
	ctx->avr_device_context.VideoEvents.Overflow               = 0;
	ctx->avr_device_context.VideoCaptureStatus                 = DVP_CAPTURE_OFF;

	ctx->avr_device_context.AudioState.AV_sync_state           = 1;
	ctx->avr_device_context.AudioState.mute_state              = 0;
	ctx->avr_device_context.AudioState.play_state              = AUDIO_STOPPED;
	ctx->avr_device_context.AudioState.stream_source           = AUDIO_SOURCE_DEMUX;
	ctx->avr_device_context.AudioState.channel_select          = AUDIO_STEREO;
	ctx->avr_device_context.AudioState.bypass_mode             = false;        /*the sense is inverted so this really means off*/
	ctx->avr_device_context.AudioId                            = DEMUX_INVALID_ID;     /* CodeToInteger('a','u','d','s'); */
	ctx->avr_device_context.AudioEncoding                      = AUDIO_ENCODING_LPCMB;
	ctx->avr_device_context.AudioCaptureStatus                 = DVP_CAPTURE_OFF;

	// Is this needed Julian W????
	init_waitqueue_head (&ctx->avr_device_context.VideoEvents.WaitQueue);
	// End is this needed

	mutex_init   (&(ctx->avr_device_context.VideoWriteLock));
	mutex_init   (&(ctx->avr_device_context.AudioWriteLock));

	ctx->target_latency		= AVR_DEFAULT_TARGET_LATENCY;
	ctx->audio_video_latency_offset	= 0;

	return ctx;
}

static int avr_probe(struct device *dev)
{
	unsigned int base, size;
	struct platform_device *dvp_device_data = to_platform_device(dev);
	avr_v4l2_shared_handle_t *shared_context;
	int res;

	if (!dvp_device_data || !dvp_device_data->name) {
		DVB_ERROR("Device probe failed.  Check your kernel SoC config!!\n" );
		return -ENODEV;
	}

	shared_context = avr_v4l2_shared_context_new();
	if (!shared_context) {
		DVB_ERROR("Cannot allocate memory for shared AVR context\n");
		return -ENOMEM;
	}

	// to enable the pads syscfg40|=(1<<16) fd7041a0

	// all the v4l2 drivers registered below share the same private data
	avr_video_overlay.priv = shared_context;
	avr_audio_overlay.priv = shared_context;
	avr_audio_output.priv = shared_context;

	stm_v4l2_register_driver(&avr_video_overlay);
	stm_v4l2_register_driver(&avr_audio_overlay);
	stm_v4l2_register_driver(&avr_audio_output);

	avr_audio_overlay.start_index[1] = (int)V4L2_CID_STM_AUDIO_FORMAT_RECOGNIZER;
	avr_audio_output.start_index[1]  = (int)V4L2_CID_STM_AUDIO_AAC_DECODE;
	avr_video_overlay.start_index[1] = (int)V4L2_CID_STM_BLANK;

	// discover the platform resources and record this in the shared context
	base = platform_get_resource(dvp_device_data, IORESOURCE_MEM, 0)->start;                                    // 0xFDA40000
	size = (((unsigned int)(platform_get_resource(dvp_device_data, IORESOURCE_MEM, 0)->end)) - base) + 1;       // 0x1000
	shared_context->dvp_irq = platform_get_resource(dvp_device_data, IORESOURCE_IRQ, 0)->start;                                  // 46 + MUXED_IRQ_BASE
	// 7200 mapped_dvp_registers here please;5A
	shared_context->mapped_dvp_registers = ioremap(base, size);

	// ensure the mixer sends us update whenever the latency values change
	res = snd_pseudo_register_mixer_observer(0, avr_update_external_time_mapping, shared_context);
	if (res < 0)
		DVB_ERROR("Cannot register mixer observer (%d), using default latency value\n", res);
		// no further recovery possible

	DVB_DEBUG("AVR DVP Settings initialised\n" );

	return 0;
}

static int avr_remove(struct device *dev)
{
	DVB_ERROR("Removal of AVR driver is not yet supported\n");

	//(void) snd_pseudo_deregister_mixer_observer(0, dvp_update_external_time_mapping, NULL);
	//kfree(shared_context);

	return -EINVAL;
}

static struct device_driver avr_driver = {
	.name = "dvp",
	.bus = &platform_bus_type,
	.probe = avr_probe,
	.remove = avr_remove,
};

__init int avr_init(void)
{
	return driver_register(&avr_driver);
}

#if 0
static void avr_cleanup(void)
{
	driver_unregister(&avr_driver);
}
#endif
