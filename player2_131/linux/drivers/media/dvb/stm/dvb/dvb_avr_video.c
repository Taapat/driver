/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr_video.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 dvp output device driver for ST SoC display subsystems.
 *
\***********************************************************************/

//#define OFFSET_THE_IMAGE

/******************************
 * INCLUDES
 ******************************/
#include <asm/page.h>
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
#include <linux/kthread.h>
#include <stmdisplay.h>
#include <Linux/video/stmfb.h>
#include <linux/stm/stmcoredisplay.h>
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
#include "stmvout.h"
#include "osdev_device.h"
#include "ksound.h"
#include "linux/dvb/stm_ioctls.h"
#include "pseudo_mixer.h"
#include "monitor_inline.h"

#include "dvb_avr.h"
#include "dvb_avr_video.h"

/******************************
 * FUNCTION PROTOTYPES
 ******************************/

extern struct class_device* player_sysfs_get_class_device(void *playback, void* stream);
extern int player_sysfs_get_stream_id(struct class_device* stream_dev);
extern void player_sysfs_new_attribute_notification(struct class_device *stream_dev);

/******************************
 * LOCAL VARIABLES
 ******************************/

#define inrange(v,l,u) 			(((v) >= (l)) && ((v) <= (u)))
#define Clamp(v,l,u)			{ if( (v) < (l) ) v = (l); else if ( (v) > (u) ) v = (u); }
#define min( a, b )			(((a)<(b)) ? (a) : (b))
#define Abs( a )			(((a)<0) ? (-a) : (a))

#define DvpTimeForNFrames(N)		(((N) * Context->StreamInfo.FrameRateDenominator * 1000000ULL) / Context->StreamInfo.FrameRateNumerator)
#define DvpCorrectedTimeForNFrames(N)	((DvpTimeForNFrames((N)) * Context->DvpFrameDurationCorrection) >> DVP_CORRECTION_FIXED_POINT_BITS);

#define DvpRange(T)			(((T * DVP_MAX_SUPPORTED_PPM_CLOCK_ERROR) / 1000000) + 1)
#define DvpValueMatchesFrameTime(V,T)	inrange( V, T - DvpRange(T), T + DvpRange(T) )

#define ControlValue(s)			((Context->DvpControl##s == DVP_USE_DEFAULT) ? Context->DvpControlDefault##s : Context->DvpControl##s)

/******************************
 * GLOBAL VARIABLES
 ******************************/


/**
 * Lookup table to help the sysfs callbacks locate the context structure.
 *
 * It really is rather difficult to get hold of a context variable from a sysfs callback. The sysfs module
 * does however provide a means for us to discover the stream id. Since the sysfs attribute is unique to
 * each stream we can use the id to index into a table and thus grab hold of a context variable.
 *
 * Thus we are pretty much forced to use a global variable but at least it doesn't stop us being
 * multi-instance.
 */
static dvp_v4l2_video_handle_t *VideoContextLookupTable[16];

/******************************
 * SUPPORTED VIDEO MODES
 ******************************/

#if 0
typedef struct
{
  ULONG           FrameRate;
  stm_scan_type_t ScanType;
  ULONG           ActiveAreaWidth;
  ULONG           ActiveAreaHeight;
  ULONG           ActiveAreaXStart;
  ULONG           FullVBIHeight;
  ULONG           OutputStandards;
  BOOL            SquarePixel;
  ULONG           HDMIVideoCodes[HDMI_CODE_COUNT];
} stm_mode_params_t;

//

typedef struct
{
  ULONG          PixelsPerLine;
  ULONG          LinesByFrame;
  ULONG          ulPixelClock;
  BOOL           HSyncPolarity;
  ULONG          HSyncPulseWidth;
  BOOL           VSyncPolarity;
  ULONG          VSyncPulseWidth;
} stm_timing_params_t;

//

typedef struct
{
  stm_display_mode_t  Mode;
  stm_mode_params_t   ModeParams;
  stm_timing_params_t TimingParams;
} stm_mode_line_t;
#endif


static const stm_mode_line_t ModeParamsTable[] =
{
  /* SD/ED modes */
  { STVTG_TIMING_MODE_480I60000_13514,
    { 60000, SCAN_I, 720, 480, 119, 36, STM_OUTPUT_STD_NTSC_M, false, {0,0} },
    { 858, 525, 13513500, false, 62, false, 3}
  },
  { STVTG_TIMING_MODE_480P60000_27027,
    { 60000, SCAN_P, 720, 480, 122, 36, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE293M), false, {2,3} },
    { 858, 525, 27027000, false, 62, false, 6}
  },
  { STVTG_TIMING_MODE_480I59940_13500,
    { 59940, SCAN_I, 720, 480, 119, 36, (STM_OUTPUT_STD_NTSC_M | STM_OUTPUT_STD_NTSC_443 | STM_OUTPUT_STD_NTSC_J | STM_OUTPUT_STD_PAL_M | STM_OUTPUT_STD_PAL_60), false, {0,0} },
    { 858, 525, 13500000, false, 62, false, 3}
  },
  { STVTG_TIMING_MODE_480P59940_27000,
    { 59940, SCAN_P, 720, 480, 122, 36, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE293M), false, {2,3} },
    { 858, 525, 27000000, false, 62, false, 6}
  },
  { STVTG_TIMING_MODE_480I59940_12273,
    { 59940, SCAN_I, 640, 480, 118, 38, (STM_OUTPUT_STD_NTSC_M | STM_OUTPUT_STD_NTSC_J | STM_OUTPUT_STD_PAL_M), true, {0,0} },
    { 780, 525, 12272727, false, 59, false, 3}
  },
  { STVTG_TIMING_MODE_576I50000_13500,
    { 50000, SCAN_I, 720, 576, 132, 44, (STM_OUTPUT_STD_PAL_BDGHI | STM_OUTPUT_STD_PAL_N | STM_OUTPUT_STD_PAL_Nc | STM_OUTPUT_STD_SECAM), false, {0,0} },
    { 864, 625, 13500000, false, 63, false, 3}
  },
  { STVTG_TIMING_MODE_576P50000_27000,
    { 50000, SCAN_P, 720, 576, 132, 44, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE293M), false, {17,18} },
    { 864, 625, 27000000, false, 64, false, 5}
  },
  { STVTG_TIMING_MODE_576I50000_14750,
    { 50000, SCAN_I, 768, 576, 155, 44, (STM_OUTPUT_STD_PAL_BDGHI | STM_OUTPUT_STD_PAL_N | STM_OUTPUT_STD_PAL_Nc | STM_OUTPUT_STD_SECAM), true, {0,0} },
    { 944, 625, 14750000, false, 71, false, 3}
  },

  /*
   * 1080p modes, SMPTE 274M (analogue) and CEA-861-C (HDMI)
   */
  { STVTG_TIMING_MODE_1080P60000_148500,
    { 60000, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,16} },
    { 2200, 1125, 148500000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P59940_148352,
    { 59940, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,16} },
    { 2200, 1125, 148351648, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P50000_148500,
    { 50000, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,31} },
    { 2640, 1125, 148500000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P30000_74250,
    { 30000, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,34} },
    { 2200, 1125,  74250000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P29970_74176,
    { 29970, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,34} },
    { 2200, 1125,  74175824, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P25000_74250,
    { 25000, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,33} },
    { 2640, 1125,  74250000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P24000_74250,
    { 24000, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,32} },
    { 2750, 1125,  74250000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080P23976_74176,
    { 23976, SCAN_P, 1920, 1080, 192, 41, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,32} },
    { 2750, 1125,  74175824, true, 44, true, 5}
  },

  /*
   * 1080i modes, SMPTE 274M (analogue) and CEA-861-C (HDMI)
   */
  { STVTG_TIMING_MODE_1080I60000_74250,
    { 60000, SCAN_I, 1920, 1080, 192, 40, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,5} },
    { 2200, 1125,  74250000, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080I59940_74176,
    { 59940, SCAN_I, 1920, 1080, 192, 40, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,5} },
    { 2200, 1125,  74175824, true, 44, true, 5}
  },
  { STVTG_TIMING_MODE_1080I50000_74250_274M,
    { 50000, SCAN_I, 1920, 1080, 192, 40, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE274M), true, {0,20} },
    { 2640, 1125,  74250000, true, 44, true, 5}
  },
  /* Australian 1080i. */
  { STVTG_TIMING_MODE_1080I50000_72000,
    { 50000, SCAN_I, 1920, 1080, 352, 124, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_AS4933), false, {0,39} },
    { 2304, 1250,  72000000, true, 168, false,  5}
  },

  /*
   * 720p modes, SMPTE 296M (analogue) and CEA-861-C (HDMI)
   */
  { STVTG_TIMING_MODE_720P60000_74250,
    { 60000, SCAN_P, 1280,  720, 260, 25, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE296M), true, {0,4} },
    { 1650,  750,  74250000, true, 40, true, 5}
  },
  { STVTG_TIMING_MODE_720P59940_74176,
    { 59940, SCAN_P, 1280,  720, 260, 25, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE296M), true, {0,4} },
    { 1650,  750,  74175824, true, 40, true, 5}
  },
  { STVTG_TIMING_MODE_720P50000_74250,
    { 50000, SCAN_P, 1280,  720, 260, 25, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_SMPTE296M), true, {0,19} },
    { 1980,  750,  74250000, true,  40, true, 5}
  },

  /*
   * A 1280x1152@50Hz Australian analogue HD mode.
   */
  { STVTG_TIMING_MODE_1152I50000_48000,
    { 50000, SCAN_I, 1280, 1152, 235, 178, STM_OUTPUT_STD_AS4933, false, {0,0} },
    { 1536, 1250,  48000000, true, 44, true, 5}
  },

  /*
   * good old VGA, the default fallback mode for HDMI displays, note that
   * the same video code is used for 4x3 and 16x9 displays and it is up to
   * the display to determine how to present it (see CEA-861-C). Also note
   * that CEA specifies the pixel aspect ratio is 1:1 .
   */
  { STVTG_TIMING_MODE_480P59940_25180,
    { 59940, SCAN_P, 640, 480, 144, 35, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_VESA), true, {1,0} },
    { 800, 525, 25174800, false, 96, false, 2}
  },
  { STVTG_TIMING_MODE_480P60000_25200,
    { 60000, SCAN_P, 640, 480, 144, 35, (STM_OUTPUT_STD_CEA861C | STM_OUTPUT_STD_VESA), true, {1,0} },
    { 800, 525, 25200000, false, 96, false, 2}
  },

  /*
   * CEA861 pixel repeated modes for SD and ED modes. These can only be set on the
   * HDMI output, to match a particular analogue mode on the master output.
   */

  /* these formats will never be handled by the capture interface */
};

/******************************
 * SYSFS TOOLS
 ******************************/

static dvp_v4l2_video_handle_t *DvpVideoSysfsLookupContext (struct class_device *class_dev)
{
    int streamid = player_sysfs_get_stream_id(class_dev);
    BUG_ON(streamid < 0 ||
	   streamid >= ARRAY_SIZE(VideoContextLookupTable) ||
	   NULL == VideoContextLookupTable[streamid]);
    return VideoContextLookupTable[streamid];
}

static ssize_t DvpVideoSysfsShowFrameCaptureNotification (struct class_device *class_dev, char *buf)
{
    int v = atomic_read(&DvpVideoSysfsLookupContext(class_dev)->DvpFrameCaptureNotification);
    return sprintf(buf, "%d\n", v);
}

static ssize_t DvpVideoSysfsStoreFrameCaptureNotification (struct class_device *class_dev, const char *buf, size_t count)
{
    int v;

    if (1 != sscanf(buf, "%i", &v))
	return -EINVAL;

    if (v < 0)
	return -EINVAL;

    atomic_set(&DvpVideoSysfsLookupContext(class_dev)->DvpFrameCaptureNotification, v);
    DVB_TRACE("DvpFrameCaptureNotification : %d\n", atomic_read(&DvpVideoSysfsLookupContext(class_dev)->DvpFrameCaptureNotification));
    return count;
}

CLASS_DEVICE_ATTR(frame_capture_notification, 0600,
	          DvpVideoSysfsShowFrameCaptureNotification, DvpVideoSysfsStoreFrameCaptureNotification);

static void DvpVideoSysfsUpdateFrameCaptureNotification (dvp_v4l2_video_handle_t *VideoContext)
{
    int old, new;

    // conditionally decrement the atomic such that we can reliably detect a one to zero transition.
    do {
	old = new = atomic_read(&VideoContext->DvpFrameCaptureNotification);

	if (new > 0)
	    new--;
    } while (old != new && old != atomic_cmpxchg(&VideoContext->DvpFrameCaptureNotification, old, new));

    if (old == 1)
	sysfs_notify (&((*(VideoContext->DvpSysfsClassDevice)).kobj), NULL, "frame_capture_notification");
}

/**
 * Create the sysfs attributes unique to A/VR video streams.
 *
 * There is no partnering destroy method because destruction is handled automatically by the class device
 * machinary.
 */
static int DvpVideoSysfsCreateAttributes (dvp_v4l2_video_handle_t *VideoContext)
{
    int Result 						= 0;
    playback_handle_t playerplayback 			= NULL;
    stream_handle_t playerstream 			= NULL;
    int streamid;

    Result = StreamGetPlayerEnvironment (VideoContext->DeviceContext->VideoStream, &playerplayback, &playerstream);
    if (Result < 0) {
	DVB_ERROR("StreamGetPlayerEnvironment failed\n");
	return -1;
    }

    VideoContext->DvpSysfsClassDevice = player_sysfs_get_class_device(playerplayback, playerstream);
    if (VideoContext->DvpSysfsClassDevice == NULL) {
    	DVB_ERROR("get_class_device failed -> cannot create attribute \n");
        return -1;
    }

    streamid = player_sysfs_get_stream_id(VideoContext->DvpSysfsClassDevice);
    if (streamid < 0 || streamid >= ARRAY_SIZE(VideoContextLookupTable)) {
	DVB_ERROR("streamid out of range -> cannot create attribute\n");
	return -1;
    }

    VideoContextLookupTable[streamid] = VideoContext;

    Result  = class_device_create_file(VideoContext->DvpSysfsClassDevice,
	    			       &class_device_attr_frame_capture_notification);
    if (Result) {
	DVB_ERROR("class_device_create_file failed (%d)\n", Result);
        return -1;
    }

    player_sysfs_new_attribute_notification(VideoContext->DvpSysfsClassDevice);

    return 0;
}

// ////////////////////////////////////////////////////////////////////////////

int DvpVideoClose( dvp_v4l2_video_handle_t	*Context )
{

int Result;

//

    if( Context->VideoRunning )
    {
        Context->VideoRunning 			= false;
        Context->Synchronize			= false;
        Context->SynchronizerRunning	= false;

        up( &Context->DvpVideoInterruptSem );
        up( &Context->DvpSynchronizerWakeSem );

		while( Context->DeviceContext->VideoStream != NULL )
		{
			unsigned int  Jiffies = ((HZ)/1000)+1;

		    set_current_state( TASK_INTERRUPTIBLE );
		    schedule_timeout( Jiffies );
		}

		DvpVideoIoctlAncillaryRequestBuffers( Context, 0, 0, NULL, NULL );

		OSDEV_IOUnMap( (unsigned int)Context->AncillaryInputBufferUnCachedAddress );
	        OSDEV_FreePartitioned( DVP_ANCILLARY_PARTITION, Context->AncillaryInputBufferPhysicalAddress );

		Result	= DisplayDelete (BACKEND_VIDEO_ID, Context->DeviceContext->Id);
		if( Result )
		{
		    printk("Error in %s: DisplayDelete failed\n",__FUNCTION__);
		    return -EINVAL;
		}

		free_irq( Context->DvpIrq, Context );
    }

    return 0;
}

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to get a buffer
//

static int DvpGetVideoBuffer( dvp_v4l2_video_handle_t	*Context )
{
int			 Result;
unsigned int		 Dimensions[2];
unsigned int		 BufferIndex;
DvpBufferStack_t	*Record;

//

    Dimensions[0]				= Context->StreamInfo.width;
    Dimensions[1]				= Context->StreamInfo.height;

    Record					= &Context->DvpBufferStack[Context->DvpNextBufferToGet % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];
    memset( Record, 0x00, sizeof(DvpBufferStack_t) );

//

    Result 	= StreamGetDecodeBuffer(	Context->DeviceContext->VideoStream,
						&Record->Buffer,
						&Record->Data,
						SURF_YCBCR422R,
						2, Dimensions,
						&BufferIndex,
						&Context->BytesPerLine );
    if (Result != 0)
    {
	printk( "Error in %s: StreamGetDecodeBuffer failed\n", __FUNCTION__ );
	return -1;
    }

    if( Context->BytesPerLine != (Dimensions[0] * 2) )
    {
	printk( "Error in %s: StreamGetDecodeBuffer returned an unexpected bytes per line value (%d instead of %d)\n", __FUNCTION__, Context->BytesPerLine, (Dimensions[0] * 2) );
    }

//

    Context->DvpNextBufferToGet++;

    return 0;
}

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to release currently held, but not injected buffers
//

static int DvpReleaseBuffers( dvp_v4l2_video_handle_t	*Context )
{
int			 Result;
DvpBufferStack_t	*Record;

//

    while( Context->DvpNextBufferToInject < Context->DvpNextBufferToGet )
    {
	Record	= &Context->DvpBufferStack[Context->DvpNextBufferToInject % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];

	Result	= StreamReturnDecodeBuffer( Context->DeviceContext->VideoStream, Record->Buffer );
	if( Result < 0 )
	{
	    printk("Error in %s: StreamReturnDecodeBuffer failed\n", __FUNCTION__);
	    // No point returning, we may as well try and release the rest
	}

	Context->DvpNextBufferToInject++;
    }

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to inject a buffer to the player
//

static int DvpInjectVideoBuffer( dvp_v4l2_video_handle_t	*Context )
{
int			 Result;
DvpBufferStack_t	*Record;
unsigned long long	 ElapsedFrameTime;
unsigned long long	 PresentationTime;
unsigned long long	 Pts;
StreamInfo_t		 Packet;

//

    if( Context->DvpNextBufferToInject >= Context->DvpNextBufferToGet )
    {
	printk( "DvpInjectVideoBuffer - No buffer yet to inject.\n" );
	return -1;
    }

    //
    // Make the call to set the time mapping.
    //

    Pts	= (((Context->DvpBaseTime * 27) + 150) / 300) & 0x00000001ffffffffull;

    Result = avr_set_external_time_mapping (Context->SharedContext, Context->DeviceContext->VideoStream, Pts, Context->DvpBaseTime );
    if( Result < 0 )
    {
	printk("DvpInjectVideoBuffer - dvp_enable_external_time_mapping failed\n" );
	return Result;
    }

//

    Record				= &Context->DvpBufferStack[Context->DvpNextBufferToInject % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];

    //
    // Calculate the expected fill time, Note the correction factor on the incoming values has 1 as 2^DVP_CORRECTION_FIXED_POINT_BITS.
    //

    Context->DvpCalculatingFrameTime	= true;

    ElapsedFrameTime			= DvpCorrectedTimeForNFrames(Context->DvpFrameCount + Context->DvpLeadInVideoFrames);

    Context->DvpDriftFrameCount++;
    Context->DvpLastDriftCorrection	= -(Context->DvpCurrentDriftError * Context->DvpDriftFrameCount) / (long long)(2 * DVP_MAXIMUM_TIME_INTEGRATION_FRAMES);

    Record->ExpectedFillTime		= Context->DvpBaseTime + ElapsedFrameTime + Context->DvpLastDriftCorrection;

    //
    // Rebase our calculation values
    //

    if( ElapsedFrameTime >= (1ULL << 31) )
    {
	Context->DvpDriftFrameCount	= 0;				// We zero the drift data, because it is encapsulated in the ExpectedFillTime
	Context->DvpLastDriftCorrection	= 0;
	Context->DvpBaseTime		= Record->ExpectedFillTime;
	Context->DvpFrameCount		= -Context->DvpLeadInVideoFrames;
    }

    Context->DvpCalculatingFrameTime	= false;

    //
    // Construct a packet to inject the information - NOTE we adjust the time to cope for a specific video latency
    //

    PresentationTime		= Record->ExpectedFillTime + ControlValue(VideoLatency) - Context->DvpLatency;
    Pts				 = (((PresentationTime * 27) + 150) / 300) & 0x00000001ffffffffull;

    memcpy( &Packet, &Context->StreamInfo, sizeof(StreamInfo_t) );

    Packet.buffer 		= phys_to_virt( (unsigned int)Record->Data );
    Packet.buffer_class		= Record->Buffer;
    Packet.pixel_aspect_ratio 	= 1;
    Packet.top_field_first	= ControlValue(TopFieldFirst);
    Packet.h_offset		= 0;

//printk( "ElapsedFrameTime = %12lld - %016llx - %12lld, %12lld - %016llx\n", ElapsedFrameTime, Pts, StreamInfo.FrameRateNumerator, StreamInfo.FrameRateDenominator, DvpFrameDurationCorrection );

    Result 	= StreamInjectPacket( Context->DeviceContext->VideoStream, (const unsigned char*)(&Packet), sizeof(StreamInfo_t), true, Pts );
    if (Result < 0)
    {
	printk("Error in %s: StreamInjectDataPacket failed\n", __FUNCTION__);
	return Result;
    }

    //
    // The ownership of the buffer has now been passed to the player, so we release our hold
    //

    Result	= StreamReturnDecodeBuffer( Context->DeviceContext->VideoStream, Record->Buffer );
    if( Result < 0 )
    {
	printk("Error in %s: StreamReturnDecodeBuffer failed\n", __FUNCTION__);
	// No point in returning
    }

    //
    // Move on to next buffer
    //

    Context->DvpFrameCount++;
    Context->DvpNextBufferToInject++;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to configure the capture buffer pointers
//
unsigned long capture_address;
static int DvpConfigureNextCaptureBuffer( dvp_v4l2_video_handle_t	*Context,
					  unsigned long long		 Now )
{
volatile int 		*DvpRegs	= Context->DvpRegs;
DvpBufferStack_t        *Record;
unsigned int		 BufferAdvance;

    //
    // select one to move onto
    //	    We ensure that the timing for capture is right,
    //      this may involve not filling buffers or refilling
    //	    the current buffer in an error condition
    //

    BufferAdvance		= 0;
    Record			= &Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];

    while( inrange((Now - Record->ExpectedFillTime), ((7 * DvpTimeForNFrames(1)) / 8), 0x8000000000000000ULL) )
    {
	Context->DvpNextBufferToFill++;
	if( Context->DvpNextBufferToFill >= Context->DvpNextBufferToGet )
	{
	    printk( "DVP Video - No buffer to move onto - We dropped a frame (%d, %d)\n", Context->DvpNextBufferToFill, Context->DvpNextBufferToGet );

	    if( Context->StandardFrameRate )
		Context->DvpInterruptFrameCount--;			// Pretend this interrupt never happened

	    Context->DvpNextBufferToFill--;
	    break;
	}

	Record			= &Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];
	BufferAdvance++;
    }

    if( (BufferAdvance == 0) && ((Context->DvpNextBufferToFill + 1) != Context->DvpNextBufferToGet) )
	printk( "DVP Video - Too early to fill buffer(%d), Discarding a frame (%lld)\n", Context->DvpNextBufferToFill, (Record->ExpectedFillTime - Now) );
    else if( BufferAdvance > 1 )
    {
	printk( "DVP Video - Too late to fill buffer(%d), skipped %d buffers (%lld)\n", Context->DvpNextBufferToFill, (BufferAdvance - 1), (Record->ExpectedFillTime - Now) );
	if( Context->StandardFrameRate )
	    Context->DvpInterruptFrameCount	+= (BufferAdvance - 1);		// Compensate for the missed interrupts
    }

    //
    // Move onto it
    //

#ifdef OFFSET_THE_IMAGE
    DvpRegs[GAM_DVP_VTP]	= (unsigned int)Record->Data + 128 + (64 * Context->RegisterVMP);
    DvpRegs[GAM_DVP_VBP]	= (unsigned int)Record->Data + 128 + (64 * Context->RegisterVMP) + Context->RegisterVBPminusVTP;
    capture_address = (unsigned int)Record->Data + 128 + (64 * Context->RegisterVMP);
#else
    DvpRegs[GAM_DVP_VTP]	= (unsigned int)Record->Data;
    DvpRegs[GAM_DVP_VBP]	= (unsigned int)Record->Data + Context->RegisterVBPminusVTP;
    capture_address = (unsigned int)Record->Data;
#endif
//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to halt the capture hardware
//

static int DvpHaltCapture( dvp_v4l2_video_handle_t	*Context )
{
unsigned int		 Tmp;
volatile int 		*DvpRegs	= Context->DvpRegs;

    //
    // Mark state
    //

    Context->DvpState		= DvpMovingToInactive;

    //
    // make sure nothing is going on
    //

    DvpRegs[GAM_DVP_CTL]	= 0x80080000;
    DvpRegs[GAM_DVP_ITM]	= 0x00;
    Tmp				= DvpRegs[GAM_DVP_ITS];

    //
    // Mark state
    //

    Context->DvpState		= DvpInactive;

    //
    // Make sure no one thinks an ancillary capture is in progress
    //

    Context->AncillaryCaptureInProgress	= false;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to configure the capture hardware
//

static int DvpParseModeValues( dvp_v4l2_video_handle_t	*Context )
{
const stm_mode_params_t		*ModeParams;
const stm_timing_params_t	*TimingParams;
bool				 Interlaced;
unsigned int			 SixteenBit;
unsigned int			 Pitch;
unsigned int			 HOffset;
unsigned int			 Width;
unsigned int			 VOffset;
unsigned int			 Height;
unsigned int			 TopVoffset;
unsigned int			 BottomVoffset;
unsigned int			 TopHeight;
unsigned int			 BottomHeight;

//

    ModeParams		= &Context->DvpCaptureMode->ModeParams;
    TimingParams	= &Context->DvpCaptureMode->TimingParams;

    //
    // Modify default control values based on current mode
    //     NOTE because some adjustments have an incestuous relationship,
    //		we need to calculate some defaults twice.
    //

    Context->DvpControlDefault16Bit				= ModeParams->ActiveAreaWidth > 732;
    Context->DvpControlDefaultIncompleteFirstPixel		= (ControlValue(ActiveAreaAdjustHorizontalOffset) & 1);
    Context->DvpControlDefaultOddPixelCount			= ((ModeParams->ActiveAreaWidth + ControlValue(ActiveAreaAdjustWidth)) & 1);
    Context->DvpControlDefaultExternalVRefPolarityPositive	= TimingParams->VSyncPolarity;
    Context->DvpControlDefaultHRefPolarityPositive		= TimingParams->HSyncPolarity;

    Context->DvpControlDefaultBigEndian				= (ModeParams->ActiveAreaXStart & 1);

    Context->DvpControlDefaultVideoLatency			= (int)Context->DvpLatency;

    //
    // Setup the stream info based on the current capture mode
    //

    Context->StandardFrameRate				= true;

    if( ModeParams->FrameRate == 59940 )
    {
	Context->StreamInfo.FrameRateNumerator		= 60000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else if( ModeParams->FrameRate == 29970 )
    {
	Context->StreamInfo.FrameRateNumerator		= 30000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else if( ModeParams->FrameRate == 23976 )
    {
	Context->StreamInfo.FrameRateNumerator		= 24000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else
    {
	Context->StreamInfo.FrameRateNumerator		= ModeParams->FrameRate;
	Context->StreamInfo.FrameRateDenominator	= 1000;
    }

    if (ModeParams->ScanType == SCAN_I)
	Context->StreamInfo.FrameRateNumerator		/= 2;

//

    if( DvpTimeForNFrames(1) > 20000 )				// If frame time is more than 20ms then use counts appropriate to 30 or less fps
    {
	Context->DvpWarmUpVideoFrames	= DVP_WARM_UP_VIDEO_FRAMES_30;
	Context->DvpLeadInVideoFrames	= DVP_LEAD_IN_VIDEO_FRAMES_30;
    }
    else
    {
	Context->DvpWarmUpVideoFrames	= DVP_WARM_UP_VIDEO_FRAMES_60;
	Context->DvpLeadInVideoFrames	= DVP_LEAD_IN_VIDEO_FRAMES_60;
    }

    //
    // Fill in the stream info fields
    //

    Context->StreamInfo.width		= ModeParams->ActiveAreaWidth  + ControlValue(ActiveAreaAdjustWidth);
    Context->StreamInfo.height		= ModeParams->ActiveAreaHeight + ControlValue(ActiveAreaAdjustHeight);
    Context->StreamInfo.interlaced	= ModeParams->ScanType == SCAN_I;
    Context->StreamInfo.h_offset	= 0;
    Context->StreamInfo.v_offset	= 0;
    Context->StreamInfo.VideoFullRange	= ControlValue(FullRange);
    Context->StreamInfo.ColourMode	= ControlValue(ColourMode);

    //
    // Precalculate the register values
    //

    Interlaced			= Context->StreamInfo.interlaced;
    Pitch			= 2 * Context->StreamInfo.width;
    SixteenBit			= ControlValue(16Bit);
printk( "SixteenBit %d, Interlaced %d, Pitch %d\n", SixteenBit, Interlaced, Pitch );

    HOffset			= ModeParams->ActiveAreaXStart + ControlValue(ActiveAreaAdjustHorizontalOffset);
    Width			= ModeParams->ActiveAreaWidth + ControlValue(ActiveAreaAdjustWidth);
    VOffset			= ModeParams->FullVBIHeight  + ControlValue(ActiveAreaAdjustVerticalOffset);
    Height			= ModeParams->ActiveAreaHeight + ControlValue(ActiveAreaAdjustHeight);

#ifdef OFFSET_THE_IMAGE
// NAUGHTY - Since we are offseting the image, we need to crop it, or it will write past the buffer end
Width /= 2;
Height /= 2;
#endif

    TopVoffset			= Context->StreamInfo.interlaced ? ((VOffset + 1) / 2) : VOffset;
    BottomVoffset		= Context->StreamInfo.interlaced ? (VOffset / 2) : VOffset;
    TopHeight			= Context->StreamInfo.interlaced ? ((Height + 1) / 2) : Height;
    BottomHeight		= Context->StreamInfo.interlaced ? (Height / 2) : Height;

    Context->RegisterTFO	= ((HOffset     * (SixteenBit ? 1 : 2)) | (TopVoffset          << 16));
    Context->RegisterTFS	= (((Width - 1) * (SixteenBit ? 1 : 2)) | ((TopHeight - 1)     << 16)) + Context->RegisterTFO;
    Context->RegisterBFO	= ((HOffset     * (SixteenBit ? 1 : 2)) | (BottomVoffset       << 16));
    Context->RegisterBFS	= (((Width -1)  * (SixteenBit ? 1 : 2)) | ((BottomHeight - 1)  << 16)) + Context->RegisterBFO;

    Context->RegisterCVS	= Width | ((Height / (Interlaced ? 2 : 1)) << 16);

    Context->RegisterVMP	= Pitch * (Interlaced ? 2 : 1);
    Context->RegisterVBPminusVTP= (Interlaced ? Pitch : 0);
    Context->RegisterHLL	= Width / (SixteenBit ? 2 : 1);

    //
    // Setup the control register values (No constants, so I commented each field)
    //

    Context->RegisterCTL	= (ControlValue(16Bit)				<< 30) |	// HD_EN		- 16 bit capture
				  (ControlValue(BigEndian) 			<< 23) |	// BIG_NOT_LITTLE	- Big endian format
				  (ControlValue(FullRange)			<< 16) |	// EXTENDED_1_254	- Clip input to 1..254 rather than 16..235 luma and 16..240 chroma
				  (ControlValue(ExternalSynchroOutOfPhase)	<< 15) |	// SYNCHRO_PHASE_NOTOK	- External H and V signals not in phase
				  (ControlValue(HorizontalResizeEnable)		<< 10) |	// HRESIZE_EN		- Enable horizontal resize
				  (ControlValue(ExternalVRefOddEven)		<<  9) |	// ODDEVEN_NOT_VSYNC	- External vertical reference is an odd/even signal
				  (ControlValue(OddPixelCount)			<<  8) |	// PHASE[1]		- Number of pixels to capture is odd
				  (ControlValue(IncompleteFirstPixel)		<<  7) |	// PHASE[0]		- First pixel is incomplete (Y1 not CB0_Y0_CR0)
				  (ControlValue(ExternalVRefPolarityPositive) 	<<  6) |	// V_REF_POL		- External Vertical counter reset on +ve edge of VREF
				  (ControlValue(HRefPolarityPositive)		<<  5) |	// H_REF_POL		- Horizontal counter reset on +ve edge of HREF
				  (ControlValue(ExternalSync)			<<  4) |	// EXT_SYNC		- Use external HSI/VSI
				  (ControlValue(VsyncBottomHalfLineEnable)	<<  3) |	// VSYNC_BOT_HALF_LINE_EN - VSOUT starts at the middle of the last top field line
				  (ControlValue(ExternalSyncPolarity)		<<  2) |	// EXT_SYNC_POL		- external sync polarity
				  (0                                            <<  1) |	// ANCILLARY_DATA_EN	- ANC/VBI data collection enable managed throughout
				  (1						<<  0);		// VID_EN		- Enable capture

    if( Context->StreamInfo.interlaced )
	Context->RegisterCTL 	|= (1 << 29);		// Reserved		- I am guessing this is an interlaced flag

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to configure the capture hardware
//

static int DvpConfigureCapture( dvp_v4l2_video_handle_t	*Context )
{
volatile int 		*DvpRegs	= Context->DvpRegs;
DvpBufferStack_t        *Record;
unsigned int		 Tmp;

//

    Record			= &Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];

    //
    // make sure nothing is going on
    //

    DvpRegs[GAM_DVP_CTL]	= 0x80080000;
    DvpRegs[GAM_DVP_ITM]	= 0x00;

    //
    // Clean up the ancillary data - match the consequences of the following writes
    // also capture the control variable that we do not allow to change during streams
    //

    Context->AncillaryInputBufferInputPointer		= Context->AncillaryInputBufferUnCachedAddress;
    memset( Context->AncillaryInputBufferUnCachedAddress, 0x00, Context->AncillaryInputBufferSize );

    Context->AncillaryPageSizeSpecified			= ControlValue(AncPageSizeSpecified);
    Context->AncillaryPageSize				= ControlValue(AncPageSize);

    //
    // Program the structure registers
    //

    DvpRegs[GAM_DVP_TFO]	= Context->RegisterTFO;
    DvpRegs[GAM_DVP_TFS]	= Context->RegisterTFS;
    DvpRegs[GAM_DVP_BFO]	= Context->RegisterBFO;
    DvpRegs[GAM_DVP_BFS]	= Context->RegisterBFS;
    DvpRegs[GAM_DVP_VTP]	= (unsigned int)Record->Data;
    DvpRegs[GAM_DVP_VBP]	= (unsigned int)Record->Data + Context->RegisterVBPminusVTP;
    DvpRegs[GAM_DVP_VMP]	= Context->RegisterVMP;
    DvpRegs[GAM_DVP_CVS]	= Context->RegisterCVS;
    DvpRegs[GAM_DVP_VSD]	= 0;					// Set synchronization delays to zero
    DvpRegs[GAM_DVP_HSD]	= 0;
    DvpRegs[GAM_DVP_HLL]	= Context->RegisterHLL;
    DvpRegs[GAM_DVP_HSRC]	= 0; 					// Disable sample rate converters
    DvpRegs[GAM_DVP_VSRC]	= 0;
    DvpRegs[GAM_DVP_PKZ]	= 0x2;					// Set packet size to 4 ST bus words

    DvpRegs[GAM_DVP_ABA]	= (unsigned int)Context->AncillaryInputBufferPhysicalAddress;
    DvpRegs[GAM_DVP_AEA]	= (unsigned int)Context->AncillaryInputBufferPhysicalAddress + Context->AncillaryInputBufferSize - DVP_ANCILLARY_BUFFER_CHUNK_SIZE;	// Note address of last 128bit word
    DvpRegs[GAM_DVP_APS]	= Context->AncillaryPageSize;

    Tmp				= DvpRegs[GAM_DVP_ITS];			// Clear interrupts before enabling
    DvpRegs[GAM_DVP_ITM]	= (1 << 4);				// Interested in Vsync top only

    DvpRegs[GAM_DVP_CTL]	= Context->RegisterCTL;			// Let er rip

//

printk( "%08x %08x %08x %08x - %08x %08x %08x %08x - %08x %08x %08x %08x - %08x %08x\n",
		DvpRegs[GAM_DVP_TFO], DvpRegs[GAM_DVP_TFS], DvpRegs[GAM_DVP_BFO], DvpRegs[GAM_DVP_BFS],
		DvpRegs[GAM_DVP_VTP], DvpRegs[GAM_DVP_VBP], DvpRegs[GAM_DVP_VMP], DvpRegs[GAM_DVP_CVS],
		DvpRegs[GAM_DVP_VSD], DvpRegs[GAM_DVP_HSD], DvpRegs[GAM_DVP_HLL], DvpRegs[GAM_DVP_HSRC],
		DvpRegs[GAM_DVP_VSRC], DvpRegs[GAM_DVP_CTL] );
//

    return 0;

}
// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to start capturing data
//

static int DvpStartup( dvp_v4l2_video_handle_t	*Context )
{
int			 Result;

    //
    // Configure the capture
    //

    Context->DvpCalculatingFrameTime	= false;
    Context->DvpBaseTime		= INVALID_TIME;
    Context->DvpFrameCount		= 0;
#if 0
    Context->DvpFrameDurationCorrection	= 1 << DVP_CORRECTION_FIXED_POINT_BITS;			// Fixed point 2^DVP_CORRECTION_FIXED_POINT_BITS is one.
#else
    printk( "$$$$$$ Nick friggs the initial DvpFrameDurationCorrection to match the CB101's duff clock $$$$$$\n" );
    Context->DvpFrameDurationCorrection	= 0xfff8551;						// Fixed point 0.999883
#endif
    Context->DvpCurrentDriftError	= 0;
    Context->DvpLastDriftCorrection	= 0;
    Context->DvpDriftFrameCount		= 0;
    Context->DvpState			= DvpStarting;

    avr_invalidate_external_time_mapping(Context->SharedContext);

    sema_init( &Context->DvpVideoInterruptSem, 0 );

    Result				= DvpConfigureCapture( Context );
    if( Result < 0 )
    {
	printk( "DvpStartup - Failed to configure capture hardware.\n" );
	return -EINVAL;
    }

//

    down_interruptible( &Context->DvpVideoInterruptSem );

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to enter the run state, where we capture and move onto the next buffer
//

static int DvpRun( dvp_v4l2_video_handle_t	*Context )
{
unsigned long long	ElapsedFrameTime;

    //
    // Set the start time, and switch to moving to run state
    //

    ElapsedFrameTime			= DvpTimeForNFrames(Context->DvpLeadInVideoFrames + 1);
    Context->DvpRunFromTime		= Context->DvpBaseTime + ElapsedFrameTime - 8000;	// Allow for jitter by subtracting 8ms

    Context->DvpState			= DvpMovingToRun;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to stop capturing data
//

static int DvpStop( dvp_v4l2_video_handle_t	*Context )
{
int			 Result;

    //
    // Halt the capture
    //

    Result			= DvpHaltCapture( Context );
    if( Result < 0 )
    {
	printk( "Error in %s: Failed to halt capture hardware.\n", __FUNCTION__ );
	return -EINVAL;
    }

    //
    // And adjust the state accordingly
    //

    Context->DvpBaseTime	= INVALID_TIME;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to handle a warm up failure
//	    What warmup failure usually means, is that the
//	    frame rate is either wrong, or the source is
//	    way way out of spec.
//

static int DvpFrameRate(	dvp_v4l2_video_handle_t	*Context,
				unsigned long long 	 MicroSeconds,
				unsigned int		 Frames )
{
unsigned long long 	MicroSecondsPerFrame;

    //
    // First convert the us per frame into an idealized
    // number (one where the clock is perfectish).
    //

    MicroSecondsPerFrame	= ((MicroSeconds << DVP_CORRECTION_FIXED_POINT_BITS) + (Context->DvpFrameDurationCorrection >> 1)) / Context->DvpFrameDurationCorrection;
    MicroSecondsPerFrame	= (MicroSecondsPerFrame + (Frames/2)) / Frames;

    printk( "DvpFrameRate - Idealized MicroSecondsPerFrame = %5lld (%6lld, %4d, %08llx)\n", MicroSecondsPerFrame, MicroSeconds, Frames, Context->DvpFrameDurationCorrection );

//

    Context->StandardFrameRate	= true;

    if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 16667) )				// 60fps = 16666.67 us
    {
	Context->StreamInfo.FrameRateNumerator		= 60;
	Context->StreamInfo.FrameRateDenominator	= 1;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 16683) )			// 59fps = 16683.33 us
    {
	Context->StreamInfo.FrameRateNumerator		= 60000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 20000) )			// 50fps = 20000.00 us
    {
	Context->StreamInfo.FrameRateNumerator		= 50;
	Context->StreamInfo.FrameRateDenominator	= 1;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 33333) )			// 30fps = 33333.33 us
    {
	Context->StreamInfo.FrameRateNumerator		= 30;
	Context->StreamInfo.FrameRateDenominator	= 1;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 33367) )			// 29fps = 33366.67 us
    {
	Context->StreamInfo.FrameRateNumerator		= 30000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 40000) )			// 25fps = 40000.00 us
    {
	Context->StreamInfo.FrameRateNumerator		= 25;
	Context->StreamInfo.FrameRateDenominator	= 1;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 41667) )			// 24fps = 41666.67 us
    {
	Context->StreamInfo.FrameRateNumerator		= 25;
	Context->StreamInfo.FrameRateDenominator	= 1;
    }
    else if( DvpValueMatchesFrameTime(MicroSecondsPerFrame, 41708) )			// 23fps = 41708.33 us
    {
	Context->StreamInfo.FrameRateNumerator		= 24000;
	Context->StreamInfo.FrameRateDenominator	= 1001;
    }
    else if( MicroSecondsPerFrame != Context->StreamInfo.FrameRateDenominator )	// If it has changed since the last time
    {
	Context->StandardFrameRate			= false;
	Context->StreamInfo.FrameRateNumerator		= 1000000;
	Context->StreamInfo.FrameRateDenominator	= MicroSecondsPerFrame;
    }
										// If it was non standard, but has not changed since
										// last integration, we let it become the new standard.

//

    if( MicroSecondsPerFrame < 32000 )						// Has the interlaced flag been set incorrectly
	Context->StreamInfo.interlaced 			= false;

//

    if( Context->StandardFrameRate )
	printk( "DvpFrameRate - Framerate = %lld/%lld (%s)\n", Context->StreamInfo.FrameRateNumerator, Context->StreamInfo.FrameRateDenominator,
								(Context->StreamInfo.interlaced ? "Interlaced":"Progressive") );

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks function to handle a warm up failure
//	    What warmup failure usually means, is that the
//	    frame rate is either wrong, or the source is
//	    way way out of spec.
//

static int DvpWarmUpFailure( dvp_v4l2_video_handle_t	*Context,
			     unsigned long long 	 MicroSeconds )
{
//    printk( "$$$ DvpWarmUpFailure %5d => %5lld %5lld $$$\n", DvpTimeForNFrames(1), DvpTimeForNFrames(Context->DvpInterruptFrameCount), MicroSeconds );

    //
    // Select an appropriate frame rate
    //

    DvpFrameRate( Context, MicroSeconds, Context->DvpInterruptFrameCount );

    //
    // Automatically switch to a slow startup
    //

    Context->DvpWarmUpVideoFrames	= DVP_WARM_UP_VIDEO_FRAMES_60;
    Context->DvpLeadInVideoFrames	= DVP_LEAD_IN_VIDEO_FRAMES_60;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks dvp ancillary data capture interrupt handler
//

static int DvpAncillaryCaptureInterrupt( dvp_v4l2_video_handle_t	*Context )
{
bool			 AncillaryCaptureWasInProgress;
unsigned int		 Index;
volatile int 		*DvpRegs;
unsigned int		 Size;
unsigned char		*Capturestart;
unsigned int		 Transfer0;
unsigned int		 Transfer1;

//

    DvpRegs				= Context->DvpRegs;
    AncillaryCaptureWasInProgress	= Context->AncillaryCaptureInProgress;

    Capturestart			= Context->AncillaryInputBufferInputPointer;
    Transfer0				= 0;
    Transfer1				= 0;

    //
    // Is there any ongoing captured data
    //

    if( Context->AncillaryCaptureInProgress )
    {
	//
	// Capture the message packets
	//

	while( Context->AncillaryInputBufferInputPointer[0] )
	{
	    if( Context->AncillaryPageSizeSpecified )
	    {
		Context->AncillaryInputBufferInputPointer	+= Context->AncillaryPageSize;
	    }
	    else
	    {
		// Length of packet is in byte 6 (but the hardware only captures from the third onwards)
		// The length is in 32 bit words, and is in the bottom 6 bits of the word.
		// There are 6 header bytes captured in total.
		// The hardware will write in 16 byte chunks (I see though doc says 8)

		Size						 = (Context->AncillaryInputBufferInputPointer[3] & 0x3f) * 4;
		Size						+= 6;
		Size						 = (Size + 15) & 0xfffffff0;
		Context->AncillaryInputBufferInputPointer	+= Size;
	    }

	    if( (Context->AncillaryInputBufferInputPointer - Context->AncillaryInputBufferUnCachedAddress) >= Context->AncillaryInputBufferSize )
		Context->AncillaryInputBufferInputPointer	-= Context->AncillaryInputBufferSize;
	}

	//
	// Calculate transfer sizes ( 0 before end of circular buffer, 1 after wrap point of circular buffer)
	//

	if( Context->AncillaryInputBufferInputPointer < Capturestart )
	{
	    Transfer0	= (Context->AncillaryInputBufferUnCachedAddress + Context->AncillaryInputBufferSize) - Capturestart;
	    Transfer1	= Context->AncillaryInputBufferInputPointer - Context->AncillaryInputBufferUnCachedAddress;
	}
	else
	{
	    Transfer0	= Context->AncillaryInputBufferInputPointer - Capturestart;
	    Transfer1	= 0;
	}
    }

    //
    // If any data was captured handle it
    //

    if( (Transfer0 + Transfer1) > 0 )
    {
	//
	// Copy into any user buffer
	//

	if( Context->AncillaryStreamOn )
	{
	    if( Context->AncillaryBufferNextFillIndex == Context->AncillaryBufferNextQueueIndex )
	    {
		// printk( "DvpAncillaryCaptureInterrupt - No buffer queued to capture into.\n" );
	    }
	    else if( (Transfer0 + Transfer1) > Context->AncillaryBufferSize )
	    {
		printk( "DvpAncillaryCaptureInterrupt - Captured data too large for buffer (%d bytes), discarded.\n", (Transfer0 + Transfer1) );
	    }
	    else
	    {
		Index		= Context->AncillaryBufferQueue[Context->AncillaryBufferNextFillIndex % DVP_MAX_ANCILLARY_BUFFERS];
		Context->AncillaryBufferState[Index].Queued	= false;
		Context->AncillaryBufferState[Index].Done	= true;
		Context->AncillaryBufferState[Index].Bytes	= (Transfer0 + Transfer1);
		Context->AncillaryBufferState[Index].FillTime	= Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE].ExpectedFillTime;

		memcpy( Context->AncillaryBufferState[Index].UnCachedAddress, Capturestart, Transfer0 );
		if( Transfer1 != 0 )
		    memcpy( Context->AncillaryBufferState[Index].UnCachedAddress + Transfer0, Context->AncillaryInputBufferUnCachedAddress, Transfer1 );

		Context->AncillaryBufferNextFillIndex++;

		up(  &Context->DvpAncillaryBufferDoneSem );
	    }
	}

	//
	// Clear out buffer (need zeroes to recognize data written, a pointer register would have been way too useful)
	//

	memset( Capturestart, 0x00, Transfer0 );
	if( Transfer1 != 0 )
	    memset( Context->AncillaryInputBufferUnCachedAddress, 0x00, Transfer1 );
    }

    //
    // Do we wish to continue capturing
    //

    Context->AncillaryCaptureInProgress	= Context->AncillaryStreamOn;

    if( Context->AncillaryCaptureInProgress != AncillaryCaptureWasInProgress )
	DvpRegs[GAM_DVP_CTL]			= Context->RegisterCTL					|
						  (Context->AncillaryPageSizeSpecified		<< 13)	|	// VALID_ANC_PAGE_SIZE_EXT - During ANC (VBI) collection size will be specified in APS register
						  (Context->AncillaryCaptureInProgress ? (1 << 1) : 0);		// ANCILLARY_DATA_EN       - Capture enable

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks dvp interrupt handler
//

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
int DvpInterrupt(int irq, void* data)
#else
int DvpInterrupt(int irq, void* data, struct pt_regs* pRegs)
#endif
{
dvp_v4l2_video_handle_t	*Context;
volatile int 		*DvpRegs;
unsigned int		 DvpIts;
ktime_t			 Ktime;
unsigned long long	 Now;
unsigned long long	 EstimatedBaseTime;
unsigned long long	 EstimatedBaseTimeRange;
unsigned long long 	 CorrectionFactor;
long long 		 CorrectionFactorChange;
long long		 AffectOfChangeOnPreviousFrameTimes;
long long		 ClampChange;
long long		 DriftError;
long long 		 DriftLimit;

//

    Context		= (dvp_v4l2_video_handle_t *)data;
    DvpRegs		= Context->DvpRegs;

    DvpIts		= DvpRegs[GAM_DVP_ITS];

    Ktime 		= ktime_get();
    Now			= ktime_to_us( Ktime );

    Context->DvpInterruptFrameCount++;

//printk( "DvpInterrupt - %d, %08x, %016llx - %lld, %d\n", Context->DvpState, DvpIts, Now, DvpTimeForNFrames(1), Context->DvpInterruptFrameCount );

    switch( Context->DvpState )
    {
	case DvpInactive:
			printk( "DvpInterrupt - Dvp inactive - possible implementation error.\n" );
			DvpHaltCapture( Context );				// Try and halt it
			break;

	case DvpStarting:
			Context->DvpBaseTime				= Now + DvpTimeForNFrames(1);	// Force trigger
			Context->DvpInterruptFrameCount			= 0;
			Context->DvpwarmUpSynchronizationAttempts	= 0;
			Context->DvpState				= DvpWarmingUp;

	case DvpWarmingUp:
			EstimatedBaseTime				= Now - DvpCorrectedTimeForNFrames(Context->DvpInterruptFrameCount);
			EstimatedBaseTimeRange				= 1 + (DvpTimeForNFrames(Context->DvpInterruptFrameCount) * DVP_MAX_SUPPORTED_PPM_CLOCK_ERROR)/1000000;

			if( !inrange( Context->DvpBaseTime, (EstimatedBaseTime - EstimatedBaseTimeRange) , (EstimatedBaseTime + EstimatedBaseTimeRange)) &&
			    (Context->DvpwarmUpSynchronizationAttempts < DVP_WARM_UP_TRIES) )
			{
//			    printk( "DvpInterrupt - Base adjustment %4lld(%5lld) (%016llx[%d] - %016llx)\n", EstimatedBaseTime - Context->DvpBaseTime, DvpTimeForNFrames(1), EstimatedBaseTime, Context->DvpInterruptFrameCount, Context->DvpBaseTime );
			    Context->DvpBaseTime			= Now;
			    Context->DvpInterruptFrameCount		= 0;
			    Context->DvpTimeAtZeroInterruptFrameCount	= Context->DvpBaseTime;
			    Context->Synchronize			= true;					// Trigger vsync lock
			    Context->DvpwarmUpSynchronizationAttempts++;
			    up( &Context->DvpSynchronizerWakeSem );
			}

			if( Context->DvpInterruptFrameCount < Context->DvpWarmUpVideoFrames )
			    break;

printk( "Warm up tries was %d\n", Context->DvpwarmUpSynchronizationAttempts );

			if( Context->DvpwarmUpSynchronizationAttempts >= DVP_WARM_UP_TRIES )
			    DvpWarmUpFailure( Context, (Now - Context->DvpBaseTime) );

			up( &Context->DvpVideoInterruptSem );

			Context->DvpState				= DvpStarted;
			Context->DvpIntegrateForAtLeastNFrames		= DVP_MINIMUM_TIME_INTEGRATION_FRAMES;

	case DvpStarted:
                        MonitorSignalEvent( MONITOR_EVENT_VIDEO_FIRST_FIELD_ACQUIRED, NULL, "DvpInterrupt: First field acquired" );
			break;


	case DvpMovingToRun:

//printk( "Moving %12lld  %12lld - %016llx %016llx\n", (Now - Context->DvpRunFromTime), (Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE].ExpectedFillTime - Now), Context->DvpRunFromTime, Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE].ExpectedFillTime );

			if( Now < Context->DvpRunFromTime )
			    break;

			Context->DvpState				= DvpRunning;

	case DvpRunning:
			//
			// Switch to next capture buffers.
			//

			DvpAncillaryCaptureInterrupt( Context );
			DvpConfigureNextCaptureBuffer( Context, Now );

			//
			// Keep the user up to date w.r.t. their frame timer/
			//

			DvpVideoSysfsUpdateFrameCaptureNotification(Context);

			//
			// Do we wish to re-calculate the correction value,
			// have we integrated for long enough, and is this value
			// Unjittered.
			//

			DriftError	= Context->DvpBufferStack[Context->DvpNextBufferToFill % DVP_VIDEO_DECODE_BUFFER_STACK_SIZE].ExpectedFillTime - Now;

			if( Context->DvpCalculatingFrameTime ||
			    (Context->DvpInterruptFrameCount < (2 * Context->DvpIntegrateForAtLeastNFrames)) ||
			    ((Context->DvpInterruptFrameCount < Context->DvpIntegrateForAtLeastNFrames) &&
			        ((Now - Context->DvpTimeOfLastFrameInterrupt) > (DvpTimeForNFrames(1) + DVP_MAXIMUM_FRAME_JITTER))) )
			{
			    if( !((Now - Context->DvpTimeOfLastFrameInterrupt) > (DvpTimeForNFrames(1) + DVP_MAXIMUM_FRAME_JITTER)) )
				Context->DvpLastFrameDriftError	= DriftError;
			    break;
			}

			//
			// If running at a non-standard framerate, then try for an
			// update, keep the integration period at the minimum number of frames
			//

			if( !Context->StandardFrameRate )
			{
			    DvpFrameRate( Context, (Now - Context->DvpTimeAtZeroInterruptFrameCount), Context->DvpInterruptFrameCount );

			    Context->DvpIntegrateForAtLeastNFrames	= DVP_MINIMUM_TIME_INTEGRATION_FRAMES/2;
			}

			//
			// Re-calculate applying a clamp to the change
			//

			CorrectionFactor			= 0;		// Initialize for print purposes
			AffectOfChangeOnPreviousFrameTimes	= 0;

			if( Context->StandardFrameRate )
			{
			    CorrectionFactor			= ((Now - Context->DvpTimeAtZeroInterruptFrameCount) << DVP_CORRECTION_FIXED_POINT_BITS) / DvpTimeForNFrames(Context->DvpInterruptFrameCount);
			    CorrectionFactorChange		= CorrectionFactor - Context->DvpFrameDurationCorrection;

			    ClampChange				= DVP_MAXIMUM_TIME_INTEGRATION_FRAMES / Context->DvpIntegrateForAtLeastNFrames;
			    ClampChange				= min( (128 * DVP_ONE_PPM), ((DVP_ONE_PPM * ClampChange * ClampChange) / 16) );
			    Clamp( CorrectionFactorChange, -ClampChange, ClampChange );

			    Context->DvpFrameDurationCorrection	+= CorrectionFactorChange;

			    //
			    // Adjust the base time so that the change only affects frames
			    // after those already calculated.
			    //

			    AffectOfChangeOnPreviousFrameTimes	= DvpTimeForNFrames(Context->DvpFrameCount + Context->DvpLeadInVideoFrames);
			    AffectOfChangeOnPreviousFrameTimes	= (AffectOfChangeOnPreviousFrameTimes * Abs(CorrectionFactorChange)) >> DVP_CORRECTION_FIXED_POINT_BITS;
			    Context->DvpBaseTime		+= (CorrectionFactorChange < 0) ? AffectOfChangeOnPreviousFrameTimes : -AffectOfChangeOnPreviousFrameTimes;
			}

			//
			// Now calculate the drift error - limitting to 2ppm
			//

			Context->DvpCurrentDriftError		= (Abs(DriftError) < Abs(Context->DvpLastFrameDriftError)) ? DriftError : Context->DvpLastFrameDriftError;

//printk( "oooh Last correction %4lld - Next correction %4lld (%4lld %4lld)\n", Context->DvpLastDriftCorrection, Context->DvpCurrentDriftError, DriftError, Context->DvpLastFrameDriftError );

			Context->DvpBaseTime			= Context->DvpBaseTime + Context->DvpLastDriftCorrection;
			Context->DvpDriftFrameCount		= 0;
			Context->DvpLastDriftCorrection		= 0;

			DriftLimit				= (1 * DvpTimeForNFrames(DVP_MAXIMUM_TIME_INTEGRATION_FRAMES)) / 1000000;
			Clamp( Context->DvpCurrentDriftError, -DriftLimit, DriftLimit );

                        printk( "New Correction factor %lld.%09lld [%3lld] (%lld.%09lld - %5d) - %5lld\n",
                                    (Context->DvpFrameDurationCorrection >> DVP_CORRECTION_FIXED_POINT_BITS),
                                    (((Context->DvpFrameDurationCorrection & 0xfffffff) * 1000000000) >> DVP_CORRECTION_FIXED_POINT_BITS),
				    AffectOfChangeOnPreviousFrameTimes,
				    (CorrectionFactor >> DVP_CORRECTION_FIXED_POINT_BITS),
                                    (((CorrectionFactor & 0xfffffff) * 1000000000) >> DVP_CORRECTION_FIXED_POINT_BITS),
				    Context->DvpInterruptFrameCount, Context->DvpCurrentDriftError );

			//
			// Initialize for next integration
			//

			Context->DvpInterruptFrameCount			= 0;
			Context->DvpTimeAtZeroInterruptFrameCount	= Now;
			Context->DvpIntegrateForAtLeastNFrames		= (Context->DvpIntegrateForAtLeastNFrames >= DVP_MAXIMUM_TIME_INTEGRATION_FRAMES) ?
										DVP_MAXIMUM_TIME_INTEGRATION_FRAMES :
										(Context->DvpIntegrateForAtLeastNFrames * 2);
			break;

	case DvpMovingToInactive:
			break;
    }

    //
    // Update our history
    //

    Context->DvpTimeOfLastFrameInterrupt		= Now;

//

    return IRQ_HANDLED;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks version of the video dvp thread, re-written to handle low latency
//

static int DvpVideoThread( void *data )
{
dvp_v4l2_video_handle_t	*Context;
int			 Result;

    //
    // Initialize context variables
    //

    Context			= (dvp_v4l2_video_handle_t *)data;

    //
    // Initialize then player with a stream
    //

    Result = PlaybackAddStream(	Context->DeviceContext->Playback,
				BACKEND_VIDEO_ID,
				BACKEND_PES_ID,
				BACKEND_DVP_ID,
				DEMUX_INVALID_ID,
				Context->DeviceContext->Id,			// SurfaceId .....
				&Context->DeviceContext->VideoStream );
    if( Result < 0 )
    {
	printk( "DvpVideoThread - PlaybackAddStream failed with %d\n", Result );
	return -EINVAL;
    }

    Result = DvpVideoSysfsCreateAttributes(Context);
    if (Result) {
	printk("DvpVideoThread - Failed to create sysfs attributes\n");
	// non-fatal
    }

    Context->DeviceContext->VideoState.play_state = VIDEO_PLAYING;

    //
    // Set the appropriate policies
    //

    StreamSetOption( Context->DeviceContext->VideoStream, PLAY_OPTION_VIDEO_ASPECT_RATIO,	VIDEO_FORMAT_16_9 );
    StreamSetOption( Context->DeviceContext->VideoStream, PLAY_OPTION_VIDEO_DISPLAY_FORMAT,	VIDEO_FULL_SCREEN );
    StreamSetOption( Context->DeviceContext->VideoStream, PLAY_OPTION_AV_SYNC, 			PLAY_OPTION_VALUE_ENABLE );

    //
    // Main loop
    //

    Context->VideoRunning		= true;
    while( Context->VideoRunning )
    {
	//
	// Handle setting up and executing a sequence
	//

	Context->DvpNextBufferToGet	= 0;
	Context->DvpNextBufferToInject	= 0;
	Context->DvpNextBufferToFill	= 0;

	Context->FastModeSwitch		= false;

	Result		= DvpParseModeValues( Context );
	if( (Result == 0) && !Context->FastModeSwitch )
	    Result	= DvpGetVideoBuffer( Context );
	if( (Result == 0) && !Context->FastModeSwitch )
	    Result	= DvpStartup( Context );
	if( (Result == 0) && !Context->FastModeSwitch )
	    Result	= DvpInjectVideoBuffer( Context );
	if( (Result == 0) && !Context->FastModeSwitch )
	    Result	= DvpRun( Context );

	//
	// Enter the main loop for a running sequence
	//

	while( (Result == 0) && Context->VideoRunning && !Context->FastModeSwitch )
	{
	    Result	= DvpGetVideoBuffer( Context );

	    if( (Result == 0) && Context->VideoRunning && !Context->FastModeSwitch )
		Result	= DvpInjectVideoBuffer( Context );
	}

	if( Result < 0 )
	    break;

	//
	// Shutdown the running sequence, either before exiting, or for a format switch
	//

	DvpStop( Context );
	StreamDrain( Context->DeviceContext->VideoStream, 1 );
	DvpReleaseBuffers( Context );
    }

    //
    // Nothing should be happening, remove the stream from the playback
    //

    Result 				= PlaybackRemoveStream( Context->DeviceContext->Playback, Context->DeviceContext->VideoStream );
    Context->DeviceContext->VideoStream = NULL;

    if( Result < 0 )
    {
	printk("Error in %s: PlaybackRemoveStream failed\n",__FUNCTION__);
	return -EINVAL;
    }
//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Nicks version of the dvp synchronizer thread, to start the output at a specified time
//

static int DvpSynchronizerThread( void *data )
{
dvp_v4l2_video_handle_t	*Context;
unsigned long long	 TriggerPoint;
long long		 FrameDuration;
long long		 SleepTime;
long long		 Frames;
ktime_t			 Ktime;
unsigned long long	 Now;
unsigned long long	 ExpectedWake;
unsigned int		 Jiffies;

    //
    // Initialize context variables
    //

    Context				= (dvp_v4l2_video_handle_t *)data;
    Context->SynchronizerRunning	= true;

    //
    // Enter the main loop waiting for something to happen
    //

    while( Context->SynchronizerRunning )
    {
	down_interruptible( &Context->DvpSynchronizerWakeSem );

	//
	// Do we want to do something
	//

	while( Context->SynchronizerRunning && Context->Synchronize )
	{
	    //
	    // When do we want to trigger (try for 32 us early to allow time
	    // for the procedure to execute, and to shake the sleep from my eyes).
	    //

	    TriggerPoint		= Context->DvpBaseTime + ControlValue(VideoLatency) - 32;
	    FrameDuration		= DvpTimeForNFrames(1);

	    if( Context->StreamInfo.interlaced &&
		!ControlValue(TopFieldFirst) )
		TriggerPoint		+= FrameDuration/2;

	    Context->Synchronize	= false;

	    Ktime 			= ktime_get();
	    Now				= ktime_to_us( Ktime );

	    SleepTime			= (long long)TriggerPoint - (long long)Now;
	    if( SleepTime < 0 )
		Frames			= ((-SleepTime) / FrameDuration) + 1;
	    else
		Frames			= -(SleepTime / FrameDuration);
	    SleepTime			= SleepTime + (Frames * FrameDuration);

	    //
	    // Now sleep
	    //

	    Jiffies		= (unsigned int)(((long long)HZ * SleepTime) / 1000000);
	    ExpectedWake	= Now + SleepTime;

	    set_current_state( TASK_INTERRUPTIBLE );
	    schedule_timeout( Jiffies );

	    //
	    // Now livelock until we can do the synchronize
	    //

	    do
	    {
		if( !Context->SynchronizerRunning || Context->Synchronize )
		    break;

		Ktime 		= ktime_get();
	    	Now		= ktime_to_us( Ktime );
	    } while( inrange( (ExpectedWake - Now), 0, 0x7fffffffffffffffull) &&
		     Context->SynchronizerRunning && !Context->Synchronize );

	    if( !Context->SynchronizerRunning || Context->Synchronize )
		break;

	    //
	    // Finally do the synchrnonize
	    //

	    DisplaySynchronize( BACKEND_VIDEO_ID, Context->DeviceContext->Id );			// Trigger vsync lock

	    Ktime 		= ktime_get();
	    Now			= ktime_to_us( Ktime );

	    //
	    // If we check to see if we were interrupted at any critical point, and retry the sync if so.
	    // We assume an interrupt occurred if we took more than 128us to perform the sync trigger.

	    if( (Now - ExpectedWake) > 128 )
		Context->Synchronize	= true;
	}
//printk( "Synchronize Vsync - Now-Expected = %lld\n", (Now - ExpectedWake) );
    }

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	The ioctl implementations for video
//

int DvpVideoIoctlSetFramebuffer(        dvp_v4l2_video_handle_t	*Context,
					unsigned int   		 Width,
                                        unsigned int   		 Height,
                                        unsigned int   		 BytesPerLine,
                                        unsigned int   		 Control )
{
printk( "VIDIOC_S_FBUF: W = %4d, H = %4d, BPL = %4d, Crtl = %08x\n", Width, Height, BytesPerLine, Control );

    Context->StreamInfo.width	= Width;
    Context->StreamInfo.height	= Height;
    Context->BytesPerLine	= BytesPerLine;
    Context->RegisterCTL	= Control;

    return 0;
}

// -----------------------------

int DvpVideoIoctlSetStandard(		dvp_v4l2_video_handle_t	*Context,
					v4l2_std_id		 Id )
{
int 			 i;
const stm_mode_line_t   *NewCaptureMode		= NULL;

//

    printk( "VIDIOC_S_STD: %08x\n", (unsigned int) Id );

    switch( Id )
    {
        case DVP_720_480_I60000:	Context->inputmode = STVTG_TIMING_MODE_480I60000_13514;		break;
        case DVP_720_480_P60000:	Context->inputmode = STVTG_TIMING_MODE_480P60000_27027;		break;
        case DVP_720_480_I59940:	Context->inputmode = STVTG_TIMING_MODE_480I59940_13500;		break;
        case DVP_720_480_P59940:	Context->inputmode = STVTG_TIMING_MODE_480P59940_27000;		break;
        case DVP_640_480_I59940:	Context->inputmode = STVTG_TIMING_MODE_480I59940_12273;		break;
        case DVP_720_576_I50000:	Context->inputmode = STVTG_TIMING_MODE_576I50000_13500;		break;
        case DVP_720_576_P50000:	Context->inputmode = STVTG_TIMING_MODE_576P50000_27000;		break;
        case DVP_768_576_I50000:	Context->inputmode = STVTG_TIMING_MODE_576I50000_14750;		break;
        case DVP_1920_1080_P60000:	Context->inputmode = STVTG_TIMING_MODE_1080P60000_148500;	break;
        case DVP_1920_1080_P59940:	Context->inputmode = STVTG_TIMING_MODE_1080P59940_148352;	break;
        case DVP_1920_1080_P50000:	Context->inputmode = STVTG_TIMING_MODE_1080P50000_148500;	break;
        case DVP_1920_1080_P30000:	Context->inputmode = STVTG_TIMING_MODE_1080P30000_74250;	break;
        case DVP_1920_1080_P29970:	Context->inputmode = STVTG_TIMING_MODE_1080P29970_74176;	break;
        case DVP_1920_1080_P25000:	Context->inputmode = STVTG_TIMING_MODE_1080P25000_74250;	break;
        case DVP_1920_1080_P24000:	Context->inputmode = STVTG_TIMING_MODE_1080P24000_74250;	break;
        case DVP_1920_1080_P23976:	Context->inputmode = STVTG_TIMING_MODE_1080P23976_74176;	break;
        case DVP_1920_1080_I60000:	Context->inputmode = STVTG_TIMING_MODE_1080I60000_74250;	break;
        case DVP_1920_1080_I59940:	Context->inputmode = STVTG_TIMING_MODE_1080I59940_74176;	break;
        case DVP_1920_1080_I50000_274M:	Context->inputmode = STVTG_TIMING_MODE_1080I50000_74250_274M;	break;
        case DVP_1920_1080_I50000_AS4933:Context->inputmode = STVTG_TIMING_MODE_1080I50000_72000;	break;
        case DVP_1280_720_P60000:	Context->inputmode = STVTG_TIMING_MODE_720P60000_74250;		break;
        case DVP_1280_720_P59940:	Context->inputmode = STVTG_TIMING_MODE_720P59940_74176;		break;
        case DVP_1280_720_P50000:	Context->inputmode = STVTG_TIMING_MODE_720P50000_74250;		break;
        case DVP_1280_1152_I50000:	Context->inputmode = STVTG_TIMING_MODE_1152I50000_48000;	break;
        case DVP_640_480_P59940:	Context->inputmode = STVTG_TIMING_MODE_480P59940_25180;		break;
        case DVP_640_480_P60000:	Context->inputmode = STVTG_TIMING_MODE_480P60000_25200;		break;
	default:
					printk( "DvpVideoIoctlSetStandard - UNKNOWN STANDARD %llu\n", Id);
	        			return -EINVAL;
    }

//

    for( i = 0; i < N_ELEMENTS (ModeParamsTable); i++ )
	if( ModeParamsTable[i].Mode == Context->inputmode )
	{
	    NewCaptureMode = &ModeParamsTable[i];
	    break;
	}

    if (!NewCaptureMode)
    {
	printk( "DvpVideoIoctlSetStandard - Mode not supported\n" );
	return -EINVAL;
    }

    //
    // we have a new capture mode, set the master copy, and force
    // a mode switch if any video sequence is already running.
    //

    DvpHaltCapture( Context );

    Context->DvpCaptureMode		= NewCaptureMode;
    Context->FastModeSwitch		= true;

    up( &Context->DvpVideoInterruptSem );

    return 0;
}

// -----------------------------

int DvpVideoIoctlOverlayStart(	dvp_v4l2_video_handle_t	*Context )
{
unsigned int		 i;
int			 Result;
char 			 TaskName[32];
struct task_struct	*Task;
struct sched_param 	 Param;

//

printk( "VIDIOC_OVERLAY: video start\n" );

    if( Context == NULL )
    {
	printk( "DvpVideoIoctlOverlayStart - Video context NULL\n" );
	return -EINVAL;
    }

//

    if( Context->VideoRunning )
    {
	DVB_DEBUG("Capture Thread already started\n" );
	return 0; //return -EBUSY;
    }

//

    Context->VideoRunning		= false;
    Context->DvpState			= DvpInactive;

    Context->SynchronizerRunning	= false;
    Context->Synchronize		= false;

//

    sema_init( &Context->DvpVideoInterruptSem, 0 );
    sema_init( &Context->DvpSynchronizerWakeSem, 0 );

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
    Result	= request_irq( Context->DvpIrq, DvpInterrupt, IRQF_DISABLED, "dvp", Context );
#else
    Result	= request_irq( Context->DvpIrq, DvpInterrupt, SA_INTERRUPT, "dvp", Context );
#endif

    if( Result != 0 )
    {
	printk("DvpVideoIoctlOverlayStart - Cannot get irq %d (result = %d)\n", Context->DvpIrq, Result );
	return -EBUSY;
    }

    //
    // Create the ancillary input data buffer - Note we capture the control value for the size just in case the user should try to change it after allocation.
    //

    Context->AncillaryInputBufferSize			= ControlValue(AncInputBufferSize);
    Context->AncillaryInputBufferPhysicalAddress	= OSDEV_MallocPartitioned( DVP_ANCILLARY_PARTITION, Context->AncillaryInputBufferSize );

printk( "VBI physical address = %08x\n", Context->AncillaryInputBufferPhysicalAddress );

    if( Context->AncillaryInputBufferPhysicalAddress == NULL )
    {
	printk( "DvpVideoIoctlOverlayStart - Unable to allocate ANC input buffer.\n" );
	return -ENOMEM;
    }

    Context->AncillaryInputBufferUnCachedAddress	= (unsigned char *)OSDEV_IOReMap( (unsigned int)Context->AncillaryInputBufferPhysicalAddress, Context->AncillaryInputBufferSize );
    Context->AncillaryInputBufferInputPointer		= Context->AncillaryInputBufferUnCachedAddress;

    memset( Context->AncillaryInputBufferUnCachedAddress, 0x00, Context->AncillaryInputBufferSize );

    //
    // Create the video tasks
    //

    strcpy( TaskName, "dvp video task" );
    Task 		= kthread_create( DvpVideoThread, Context, "%s", TaskName );
    if( IS_ERR(Task) )
    {
	printk( "DvpVideoIoctlOverlayStart - Unable to start video thread\n" );
	Context->VideoRunning		= false;
	Context->SynchronizerRunning	= false;
	free_irq( Context->DvpIrq, Context );
	return -EINVAL;
    }

    Param.sched_priority 	= DVP_VIDEO_THREAD_PRIORITY;
    if( sched_setscheduler( Task, SCHED_RR, &Param) )
    {
	printk("DvpVideoIoctlOverlayStart - FAILED to set scheduling parameters to priority %d (SCHED_RR)\n", Param.sched_priority);
	Context->VideoRunning		= false;
	Context->SynchronizerRunning	= false;
	free_irq( Context->DvpIrq, Context );
	return -EINVAL;
    }

    wake_up_process( Task );

//

    strcpy( TaskName, "dvp synchronizer task" );
    Task 		= kthread_create( DvpSynchronizerThread, Context, "%s", TaskName );
    if( IS_ERR(Task) )
    {
	printk( "DvpVideoIoctlOverlayStart - Unable to start synchronizer thread\n" );
	Context->VideoRunning		= false;
	Context->SynchronizerRunning	= false;
	free_irq( Context->DvpIrq, Context );
	return -EINVAL;
    }

    Param.sched_priority 	= DVP_SYNCHRONIZER_THREAD_PRIORITY;
    if( sched_setscheduler( Task, SCHED_RR, &Param) )
    {
	printk("DvpVideoIoctlOverlayStart - FAILED to set scheduling parameters to priority %d (SCHED_RR)\n", Param.sched_priority);
	Context->VideoRunning		= false;
	Context->SynchronizerRunning	= false;
	free_irq( Context->DvpIrq, Context );
	return -EINVAL;
    }

    wake_up_process( Task );

    //
    // Guarantee both tasks are started
    //

    for( i=0; (i<100) && (!Context->VideoRunning || !Context->SynchronizerRunning); i++ )
    {
	unsigned int  Jiffies = ((HZ)/1000)+1;

	set_current_state( TASK_INTERRUPTIBLE );
	schedule_timeout( Jiffies );
    }

    if( !Context->VideoRunning || !Context->SynchronizerRunning )
    {
	printk( "DvpVideoIoctlOverlayStart - FAILED to set start processes\n" );
	Context->VideoRunning		= false;
	Context->SynchronizerRunning	= false;
	free_irq( Context->DvpIrq, Context );
	return -EINVAL;
    }

//

    return 0;
}


// -----------------------------


int DvpVideoIoctlCrop(	dvp_v4l2_video_handle_t		*Context,
			struct v4l2_crop		*Crop )
{
int	ret;

DVB_DEBUG( "VIDIOC_S_CROP:\n" );

    if( (Context == NULL) || (Context->DeviceContext->VideoStream == NULL) )
	return -EINVAL;

    if (Crop->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
	ret = StreamSetOutputWindow( Context->DeviceContext->VideoStream, Crop->c.left, Crop->c.top, Crop->c.width, Crop->c.height );
	if (ret < 0) {
	    DVB_ERROR("StreamSetOutputWindow failed. \n ");
	    return -EINVAL;
	}
    }
    else if (Crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
	ret = StreamSetInputWindow( Context->DeviceContext->VideoStream, Crop->c.left, Crop->c.top, Crop->c.width, Crop->c.height );
	if (ret < 0) {
	    DVB_ERROR("StreamSetInputWindow failed. \n ");
	    return -EINVAL;
	}
    }
    else {
	DVB_ERROR("This type of crop does not exist. I'm sorry. \n ");
	return -EINVAL;
    }

    return 0;
}


// -----------------------------


#define Check( s, l, u )		printk( "%s\n", s ); if( (Value != DVP_USE_DEFAULT) && !inrange(Value,l,u) ) {printk( "Set Control - Value out of range (%d not in [%d..%d])\n", Value, l, u ); return -1; }
#define CheckAndSet( s, l, u, v )	Check( s, l, u ); v = Value;

int DvpVideoIoctlSetControl( 		dvp_v4l2_video_handle_t	*Context,
					unsigned int		 Control,
					unsigned int		 Value )
{
int	ret;

printk( "VIDIOC_S_CTRL->V4L2_CID_STM_DVPIF_" );

    switch( Control )
    {
	case V4L2_CID_STM_SRC_MODE_656:
		Check( "SRC_MODE_656", 0, 1 );
		if( Value )
		{
		    Context->DvpControl16Bit				= 0;
		    Context->DvpControlExternalSync			= 0;
		    Context->DvpControlHRefPolarityPositive		= 1;
		    Context->DvpControlExternalVRefPolarityPositive	= 1;

		    Context->DvpControlSrcMode656			= 1;
		}
		else
		{
		    Context->DvpControl16Bit				= 1;
		    Context->DvpControlExternalSync			= 1;
		    Context->DvpControlHRefPolarityPositive		= 0;
		    Context->DvpControlExternalVRefPolarityPositive	= 0;

		    Context->DvpControlSrcMode656			= 0;
		}
		break;


	case V4L2_CID_STM_DVPIF_RESTORE_DEFAULT:
		printk( "RESTORE_DEFAULT\n" );
		Context->DvpControlSrcMode656					= 0;

		Context->DvpControl16Bit					= DVP_USE_DEFAULT;
		Context->DvpControlBigEndian					= DVP_USE_DEFAULT;
		Context->DvpControlFullRange					= DVP_USE_DEFAULT;
		Context->DvpControlIncompleteFirstPixel				= DVP_USE_DEFAULT;
		Context->DvpControlOddPixelCount				= DVP_USE_DEFAULT;
		Context->DvpControlVsyncBottomHalfLineEnable			= DVP_USE_DEFAULT;
		Context->DvpControlExternalSync					= DVP_USE_DEFAULT;
		Context->DvpControlExternalSyncPolarity				= DVP_USE_DEFAULT;
		Context->DvpControlExternalSynchroOutOfPhase			= DVP_USE_DEFAULT;
		Context->DvpControlExternalVRefOddEven				= DVP_USE_DEFAULT;
		Context->DvpControlExternalVRefPolarityPositive			= DVP_USE_DEFAULT;
		Context->DvpControlHRefPolarityPositive				= DVP_USE_DEFAULT;
		Context->DvpControlActiveAreaAdjustHorizontalOffset		= DVP_USE_DEFAULT;
		Context->DvpControlActiveAreaAdjustVerticalOffset		= DVP_USE_DEFAULT;
		Context->DvpControlActiveAreaAdjustWidth			= DVP_USE_DEFAULT;
		Context->DvpControlActiveAreaAdjustHeight			= DVP_USE_DEFAULT;
		Context->DvpControlColourMode					= DVP_USE_DEFAULT;
		Context->DvpControlVideoLatency					= DVP_USE_DEFAULT;
		Context->DvpControlBlank					= DVP_USE_DEFAULT;
		Context->DvpControlAncPageSizeSpecified				= DVP_USE_DEFAULT;
		Context->DvpControlAncPageSize					= DVP_USE_DEFAULT;
		Context->DvpControlAncInputBufferSize				= DVP_USE_DEFAULT;
		Context->DvpControlHorizontalResizeEnable			= DVP_USE_DEFAULT;
		Context->DvpControlHorizontalResizeFactor			= DVP_USE_DEFAULT;
		Context->DvpControlTopFieldFirst				= DVP_USE_DEFAULT;

		Context->DvpControlDefault16Bit					= 1;
		Context->DvpControlDefaultBigEndian				= 1;
		Context->DvpControlDefaultFullRange				= 0;
		Context->DvpControlDefaultIncompleteFirstPixel			= 0;
		Context->DvpControlDefaultOddPixelCount				= 0;
		Context->DvpControlDefaultVsyncBottomHalfLineEnable		= 0;
		Context->DvpControlDefaultExternalSync				= 1;
		Context->DvpControlDefaultExternalSyncPolarity			= 1;
		Context->DvpControlDefaultExternalSynchroOutOfPhase		= 0;
		Context->DvpControlDefaultExternalVRefOddEven			= 0;
		Context->DvpControlDefaultExternalVRefPolarityPositive		= 1;
		Context->DvpControlDefaultHRefPolarityPositive			= 0;
		Context->DvpControlDefaultActiveAreaAdjustHorizontalOffset	= 0;
		Context->DvpControlDefaultActiveAreaAdjustVerticalOffset	= 0;
		Context->DvpControlDefaultActiveAreaAdjustWidth			= 0;
		Context->DvpControlDefaultActiveAreaAdjustHeight		= 0;
		Context->DvpControlDefaultColourMode				= 0;
		Context->DvpControlDefaultVideoLatency				= 0xffffffff;
		Context->DvpControlDefaultBlank					= 0;
		Context->DvpControlDefaultAncPageSizeSpecified			= 0;
		Context->DvpControlDefaultAncPageSize				= DVP_DEFAULT_ANCILLARY_PAGE_SIZE;
		Context->DvpControlDefaultAncInputBufferSize			= DVP_DEFAULT_ANCILLARY_INPUT_BUFFER_SIZE;
		Context->DvpControlDefaultHorizontalResizeEnable		= 0;
		Context->DvpControlDefaultHorizontalResizeFactor		= 0;
		Context->DvpControlDefaultTopFieldFirst				= 0;

		break;

	case  V4L2_CID_STM_BLANK:
	case  V4L2_CID_STM_DVPIF_BLANK:
		Check( "DVPIF_BLANK", 0, 1 );
		if( Context->DeviceContext->VideoStream == NULL )
		    return -EINVAL;

		ret = StreamEnable( Context->DeviceContext->VideoStream, Value );
		if( ret < 0 )
		{
		    printk( "DvpVideoIoctlSetControl - StreamEnable failed (%d)\n", Value );
		    return -EINVAL;
		}

		Context->DvpControlBlank	= Value;
		break;


	case  V4L2_CID_STM_DVPIF_16_BIT:
		CheckAndSet( "16_BIT", 0, 1, Context->DvpControl16Bit );
		break;
	case  V4L2_CID_STM_DVPIF_BIG_ENDIAN:
		CheckAndSet( "BIG_ENDIAN", 0, 1, Context->DvpControlBigEndian );
		break;
	case  V4L2_CID_STM_DVPIF_FULL_RANGE:
		CheckAndSet( "FULL_RANGE", 0, 1, Context->DvpControlFullRange );
		break;
	case  V4L2_CID_STM_DVPIF_INCOMPLETE_FIRST_PIXEL:
		CheckAndSet( "INCOMPLETE_FIRST_PIXEL", 0, 1, Context->DvpControlIncompleteFirstPixel );
		break;
	case  V4L2_CID_STM_DVPIF_ODD_PIXEL_COUNT:
		CheckAndSet( "ODD_PIXEL_COUNT", 0, 1, Context->DvpControlOddPixelCount );
		break;
	case  V4L2_CID_STM_DVPIF_VSYNC_BOTTOM_HALF_LINE_ENABLE:
		CheckAndSet( "VSYNC_BOTTOM_HALF_LINE_ENABLE", 0, 1, Context->DvpControlVsyncBottomHalfLineEnable );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNC:
		CheckAndSet( "EXTERNAL_SYNC", 0, 1, Context->DvpControlExternalSync );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNC_POLARITY:
		CheckAndSet( "EXTERNAL_SYNC_POLARITY", 0, 1, Context->DvpControlExternalSyncPolarity );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNCHRO_OUT_OF_PHASE:
		CheckAndSet( "EXTERNAL_SYNCHRO_OUT_OF_PHASE", 0, 1, Context->DvpControlExternalSynchroOutOfPhase );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_VREF_ODD_EVEN:
		CheckAndSet( "EXTERNAL_VREF_ODD_EVEN", 0, 1, Context->DvpControlExternalVRefOddEven );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_VREF_POLARITY_POSITIVE:
		CheckAndSet( "EXTERNAL_VREF_POLARITY_POSITIVE", 0, 1, Context->DvpControlExternalVRefPolarityPositive );
		break;
	case  V4L2_CID_STM_DVPIF_HREF_POLARITY_POSITIVE:
		CheckAndSet( "HREF_POLARITY_POSITIVE", 0, 1, Context->DvpControlHRefPolarityPositive );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HORIZONTAL_OFFSET:
		CheckAndSet( "ACTIVE_AREA_ADJUST_HORIZONTAL_OFFSET", -256, 256, Context->DvpControlActiveAreaAdjustHorizontalOffset );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_VERTICAL_OFFSET:
		CheckAndSet( "ACTIVE_AREA_ADJUST_VERTICAL_OFFSET", -256, 256, Context->DvpControlActiveAreaAdjustVerticalOffset );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_WIDTH:
		CheckAndSet( "ACTIVE_AREA_ADJUST_WIDTH", -256, 256, Context->DvpControlActiveAreaAdjustWidth );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HEIGHT:
		CheckAndSet( "ACTIVE_AREA_ADJUST_HEIGHT", -256, 256, Context->DvpControlActiveAreaAdjustHeight );
		break;
	case  V4L2_CID_STM_DVPIF_COLOUR_MODE:
		CheckAndSet( "COLOUR_MODE", 0, 2, Context->DvpControlColourMode );
		break;
	case  V4L2_CID_STM_DVPIF_VIDEO_LATENCY:
		CheckAndSet( "VIDEO_LATENCY", 0, 1000000, Context->DvpControlVideoLatency );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE_SPECIFIED:
		CheckAndSet( "ANC_PAGE_SIZE_SPECIFIED", 0, 1, Context->DvpControlAncPageSizeSpecified );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE:
		CheckAndSet( "ANC_PAGE_SIZE", 0, 4096, Context->DvpControlAncPageSize );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_INPUT_BUFFER_SIZE:
		Value		&= 0xfffffff0;
		CheckAndSet( "ANC_INPUT_BUFFER_SIZE", 0, 0x100000, Context->DvpControlAncInputBufferSize );
		break;
	case  V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_ENABLE:
		CheckAndSet( "HORIZONTAL_RESIZE_ENABLE", 0, 0, Context->DvpControlHorizontalResizeEnable );
		break;
	case  V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_FACTOR:
		CheckAndSet( "HORIZONTAL_RESIZE_FACTOR", 0, 2, Context->DvpControlHorizontalResizeFactor );
		break;
	case  V4L2_CID_STM_DVPIF_TOP_FIELD_FIRST:
		CheckAndSet( "TOP_FIELD_FIRST", 0, 1, Context->DvpControlTopFieldFirst );
		break;

	default:
		printk( "Unknown %08x\n", Control );
		return -1;
    }

    return 0;
}

// --------------------------------

#define GetValue( s, v )		printk( "%s\n", s ); *Value = v;

int DvpVideoIoctlGetControl( 		dvp_v4l2_video_handle_t	*Context,
					unsigned int		 Control,
					unsigned int		*Value )
{
printk( "VIDIOC_G_CTRL->V4L2_CID_STM_DVPIF_" );

    switch( Control )
    {
	case V4L2_CID_STM_DVPIF_RESTORE_DEFAULT:
		printk( "RESTORE_DEFAULT - Not a readable control\n" );
		*Value		= 0;
		break;


	case V4L2_CID_STM_SRC_MODE_656:
		GetValue( "SRC_MODE_656", Context->DvpControlSrcMode656 );
		break;
	case  V4L2_CID_STM_DVPIF_BLANK:
		GetValue( "DVPIF_BLANK", Context->DvpControlBlank );
		break;
	case  V4L2_CID_STM_DVPIF_16_BIT:
		GetValue( "16_BIT", Context->DvpControl16Bit );
		break;
	case  V4L2_CID_STM_DVPIF_BIG_ENDIAN:
		GetValue( "BIG_ENDIAN", Context->DvpControlBigEndian );
		break;
	case  V4L2_CID_STM_DVPIF_FULL_RANGE:
		GetValue( "FULL_RANGE", Context->DvpControlFullRange );
		break;
	case  V4L2_CID_STM_DVPIF_INCOMPLETE_FIRST_PIXEL:
		GetValue( "INCOMPLETE_FIRST_PIXEL", Context->DvpControlIncompleteFirstPixel );
		break;
	case  V4L2_CID_STM_DVPIF_ODD_PIXEL_COUNT:
		GetValue( "ODD_PIXEL_COUNT", Context->DvpControlOddPixelCount );
		break;
	case  V4L2_CID_STM_DVPIF_VSYNC_BOTTOM_HALF_LINE_ENABLE:
		GetValue( "VSYNC_BOTTOM_HALF_LINE_ENABLE", Context->DvpControlVsyncBottomHalfLineEnable );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNC:
		GetValue( "EXTERNAL_SYNC", Context->DvpControlExternalSync );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNC_POLARITY:
		GetValue( "EXTERNAL_SYNC_POLARITY", Context->DvpControlExternalSyncPolarity );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_SYNCHRO_OUT_OF_PHASE:
		GetValue( "EXTERNAL_SYNCHRO_OUT_OF_PHASE", Context->DvpControlExternalSynchroOutOfPhase );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_VREF_ODD_EVEN:
		GetValue( "EXTERNAL_VREF_ODD_EVEN", Context->DvpControlExternalVRefOddEven );
		break;
	case  V4L2_CID_STM_DVPIF_EXTERNAL_VREF_POLARITY_POSITIVE:
		GetValue( "EXTERNAL_VREF_POLARITY_POSITIVE", Context->DvpControlExternalVRefPolarityPositive );
		break;
	case  V4L2_CID_STM_DVPIF_HREF_POLARITY_POSITIVE:
		GetValue( "HREF_POLARITY_POSITIVE", Context->DvpControlHRefPolarityPositive );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HORIZONTAL_OFFSET:
		GetValue( "ACTIVE_AREA_ADJUST_HORIZONTAL_OFFSET", Context->DvpControlActiveAreaAdjustHorizontalOffset );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_VERTICAL_OFFSET:
		GetValue( "ACTIVE_AREA_ADJUST_VERTICAL_OFFSET", Context->DvpControlActiveAreaAdjustVerticalOffset );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_WIDTH:
		GetValue( "ACTIVE_AREA_ADJUST_WIDTH", Context->DvpControlActiveAreaAdjustWidth );
		break;
	case  V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HEIGHT:
		GetValue( "ACTIVE_AREA_ADJUST_HEIGHT", Context->DvpControlActiveAreaAdjustHeight );
		break;
	case  V4L2_CID_STM_DVPIF_COLOUR_MODE:
		GetValue( "COLOUR_MODE", Context->DvpControlColourMode );
		break;
	case  V4L2_CID_STM_DVPIF_VIDEO_LATENCY:
		GetValue( "VIDEO_LATENCY", Context->DvpControlVideoLatency );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE_SPECIFIED:
		GetValue( "ANC_PAGE_SIZE_SPECIFIED", Context->DvpControlAncPageSizeSpecified );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE:
		GetValue( "ANC_PAGE_SIZE", Context->DvpControlAncPageSize );
		break;
	case  V4L2_CID_STM_DVPIF_ANC_INPUT_BUFFER_SIZE:
		GetValue( "ANC_INPUT_BUFFER_SIZE", Context->DvpControlAncInputBufferSize );
		break;
	case  V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_ENABLE:
		GetValue( "HORIZONTAL_RESIZE_ENABLE", Context->DvpControlHorizontalResizeEnable );
		break;
	case  V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_FACTOR:
		GetValue( "HORIZONTAL_RESIZE_FACTOR", Context->DvpControlHorizontalResizeFactor );
		break;
	case  V4L2_CID_STM_DVPIF_TOP_FIELD_FIRST:
		GetValue( "TOP_FIELD_FIRST", Context->DvpControlTopFieldFirst );
		break;

	default:
		printk( "Unknown\n" );
		return -1;
    }

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	The management functions for the Ancillary data capture
//
//	Function to request buffers
//

int DvpVideoIoctlAncillaryRequestBuffers(	dvp_v4l2_video_handle_t	*Context,
						unsigned int		 DesiredCount,
						unsigned int		 DesiredSize,
						unsigned int		*ActualCount,
						unsigned int		*ActualSize )
{
unsigned int	i;

    //
    // Is this a closedown call
    //

    if( DesiredCount == 0 )
    {
	if( Context->AncillaryBufferCount == 0 )
	{
	    printk( "DvpVideoIoctlAncillaryRequestBuffers - Attempt to free when No buffers allocated.\n" );
	    return -EINVAL;
	}

	DvpVideoIoctlAncillaryStreamOff( Context );

	Context->AncillaryBufferCount	= 0;
	OSDEV_IOUnMap( (unsigned int)Context->AncillaryBufferUnCachedAddress );
	OSDEV_FreePartitioned( DVP_ANCILLARY_PARTITION, Context->AncillaryBufferPhysicalAddress );

	return 0;
    }

    //
    // Check that we do not already have buffers
    //

    if( Context->AncillaryBufferCount != 0 )
    {
	printk( "DvpVideoIoctlAncillaryRequestBuffers - Attempt to allocate buffers when buffers are already mapped.\n" );
	return -EINVAL;
    }

    //
    // Adjust the buffer counts and sizes to meet our restrictions
    //

    Clamp( DesiredCount, DVP_MIN_ANCILLARY_BUFFERS, DVP_MAX_ANCILLARY_BUFFERS );

    Clamp( DesiredSize,  DVP_MIN_ANCILLARY_BUFFER_SIZE, DVP_MAX_ANCILLARY_BUFFER_SIZE );

    if( (DesiredSize % DVP_ANCILLARY_BUFFER_CHUNK_SIZE) != 0 )
	DesiredSize	+= (DVP_ANCILLARY_BUFFER_CHUNK_SIZE - (DesiredSize % DVP_ANCILLARY_BUFFER_CHUNK_SIZE));

    //
    // Create buffers
    //

    Context->AncillaryBufferPhysicalAddress	= OSDEV_MallocPartitioned( DVP_ANCILLARY_PARTITION, DesiredSize * DesiredCount );
printk( "VBI physical address = %08x\n", Context->AncillaryBufferPhysicalAddress );
    if( Context->AncillaryBufferPhysicalAddress == NULL )
    {
	printk( "DvpVideoIoctlAncillaryRequestBuffers - Unable to allocate buffers.\n" );
	return -ENOMEM;
    }

    Context->AncillaryBufferUnCachedAddress	= (unsigned char *)OSDEV_IOReMap( (unsigned int)Context->AncillaryBufferPhysicalAddress, DesiredSize * DesiredCount );
    memset( Context->AncillaryBufferUnCachedAddress, 0x00, DesiredCount * DesiredSize );

    //
    // Initialize the state
    //

    Context->AncillaryStreamOn			= false;
    Context->AncillaryCaptureInProgress		= false;
    Context->AncillaryBufferCount		= DesiredCount;
    Context->AncillaryBufferSize		= DesiredSize;

    memset( Context->AncillaryBufferState, 0x00, DVP_MAX_ANCILLARY_BUFFERS * sizeof(AncillaryBufferState_t) );
    for( i=0; i<Context->AncillaryBufferCount; i++ )
    {
	Context->AncillaryBufferState[i].PhysicalAddress	= Context->AncillaryBufferPhysicalAddress + (i * DesiredSize);
	Context->AncillaryBufferState[i].UnCachedAddress	= Context->AncillaryBufferUnCachedAddress + (i * DesiredSize);
	Context->AncillaryBufferState[i].Queued			= false;
	Context->AncillaryBufferState[i].Done			= false;
	Context->AncillaryBufferState[i].Bytes			= 0;
	Context->AncillaryBufferState[i].FillTime		= 0;
    }

    //
    // Initialize the queued/dequeued/fill pointers
    //

    Context->AncillaryBufferNextQueueIndex	= 0;
    Context->AncillaryBufferNextFillIndex	= 0;
    Context->AncillaryBufferNextDeQueueIndex	= 0;

    sema_init( &Context->DvpAncillaryBufferDoneSem, 0 );

//

    *ActualCount				= Context->AncillaryBufferCount;
    *ActualSize					= Context->AncillaryBufferSize;

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to query the state of a buffer
//

int DvpVideoIoctlAncillaryQueryBuffer(		dvp_v4l2_video_handle_t	 *Context,
						unsigned int		  Index,
						bool			 *Queued,
						bool			 *Done,
						unsigned char		**PhysicalAddress,
						unsigned char		**UnCachedAddress,
						unsigned long long	 *CaptureTime,
						unsigned int		 *Bytes,
						unsigned int		 *Size )
{
    if( Index >= Context->AncillaryBufferCount )
    {
	printk( "DvpVideoIoctlAncillaryQueryBuffer - Index out of bounds (%d).\n", Index );
	return -EINVAL;
    }

//

    *Queued		= Context->AncillaryBufferState[Index].Queued;
    *Done		= Context->AncillaryBufferState[Index].Done;
    *PhysicalAddress	= Context->AncillaryBufferState[Index].PhysicalAddress;
    *UnCachedAddress	= Context->AncillaryBufferState[Index].UnCachedAddress;
    *CaptureTime	= Context->AncillaryBufferState[Index].FillTime + ControlValue(VideoLatency) - Context->DvpLatency;
    *Bytes		= Context->AncillaryBufferState[Index].Bytes;
    *Size		= Context->AncillaryBufferSize;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to queue a buffer
//

int DvpVideoIoctlAncillaryQueueBuffer(		dvp_v4l2_video_handle_t	*Context,
						unsigned int		 Index )
{
    if( Index >= Context->AncillaryBufferCount )
    {
	printk( "DvpVideoIoctlAncillaryQueryBuffer - Index out of bounds (%d).\n", Index );
	return -EINVAL;
    }

//

    if( Context->AncillaryBufferState[Index].Queued ||
	Context->AncillaryBufferState[Index].Done )
    {
	printk( "DvpVideoIoctlAncillaryQueryBuffer - Buffer already queued or awaiting dequeing.\n" );
	return -EINVAL;
    }

//

    memset( Context->AncillaryBufferState[Index].UnCachedAddress, 0x00, Context->AncillaryBufferSize );
    Context->AncillaryBufferState[Index].Queued			= true;

    Context->AncillaryBufferQueue[Context->AncillaryBufferNextQueueIndex % DVP_MAX_ANCILLARY_BUFFERS]	= Index;
    Context->AncillaryBufferNextQueueIndex++;

//

    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to de-queue a buffer
//

int DvpVideoIoctlAncillaryDeQueueBuffer(	dvp_v4l2_video_handle_t	*Context,
						unsigned int		*Index,
						bool			 Blocking )
{
    do
    {
	if( Blocking )
	    down_interruptible( &Context->DvpAncillaryBufferDoneSem );

//

	if( (Context->AncillaryBufferCount == 0) || !Context->AncillaryStreamOn )
	{
	    printk( "DvpVideoIoctlAncillaryDeQueueBuffer - Shutdown/StreamOff while waiting.\n" );
	    return -EINVAL;
	}

//

	if( Context->AncillaryBufferNextDeQueueIndex < Context->AncillaryBufferNextFillIndex )
	{
	    *Index	= Context->AncillaryBufferQueue[Context->AncillaryBufferNextDeQueueIndex % DVP_MAX_ANCILLARY_BUFFERS];
	    Context->AncillaryBufferNextDeQueueIndex++;

	    if( !Context->AncillaryBufferState[*Index].Done )
		printk( "DvpVideoIoctlAncillaryDeQueueBuffer - Internal state error, buffer is in done queue, but not marked as done - Implementation Error.\n" );

	    Context->AncillaryBufferState[*Index].Done	= false;

	    return 0;
	}
    } while( Blocking );

    return -EAGAIN;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to turn on stream aquisition
//

int DvpVideoIoctlAncillaryStreamOn(		dvp_v4l2_video_handle_t	*Context )
{
    Context->AncillaryStreamOn	= true;
    return 0;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to turn off stream aquisition
//

int DvpVideoIoctlAncillaryStreamOff(		dvp_v4l2_video_handle_t	*Context )
{
unsigned int		i;

    //
    // First mark us as off, and wait for any current capture activity to complete
    //

    Context->AncillaryStreamOn	= false;

    //
    // Clear the queued and done queues
    //

    Context->AncillaryBufferNextQueueIndex	= 0;
    Context->AncillaryBufferNextFillIndex	= 0;
    Context->AncillaryBufferNextDeQueueIndex	= 0;

    //
    // Tidy up the state records
    //

    for( i=0; i<Context->AncillaryBufferCount; i++ )
    {
	Context->AncillaryBufferState[i].Queued			= false;
	Context->AncillaryBufferState[i].Done			= false;
	Context->AncillaryBufferState[i].FillTime		= 0;
    }

    //
    // Make sure there is no-one waiting for a Dequeue
    //

    up(  &Context->DvpAncillaryBufferDoneSem );

//

    return 0;
}

