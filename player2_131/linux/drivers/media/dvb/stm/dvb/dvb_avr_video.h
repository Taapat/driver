/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr_video.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 dvp output device driver for ST SoC display subsystems header file
 *
\***********************************************************************/
#ifndef STMDVPVIDEO_H_
#define STMDVPVIDEO_H_

// Include header from the standards directory that
// defines information encoded in an injected packet

#include "dvb_avr.h"
#include "dvp.h"
#include <linux/version.h>

/******************************
 * A/V receiver controls to control the "D1-DVP0" input
 *     Nick has started his new controls at an offset of 256
 *     while leaving the two controls that are actually used
 *     at their current values. Someone needs to correct this
 *     stuff, and rationalize the list to be contiguous 
 *     (according to Pete). This is an exercise for the reader.
 ******************************/

#define DVP_USE_DEFAULT						0x80000000

#define V4L2_CID_STM_BLANK        				(V4L2_CID_PRIVATE_BASE+100)
#define V4L2_CID_STM_SRC_MODE_656            			(V4L2_CID_PRIVATE_BASE+104)

#define V4L2_CID_STM_DVPIF_RESTORE_DEFAULT			(V4L2_CID_PRIVATE_BASE+0x100)
#define V4L2_CID_STM_DVPIF_16_BIT				(V4L2_CID_PRIVATE_BASE+0x101)
#define V4L2_CID_STM_DVPIF_BIG_ENDIAN				(V4L2_CID_PRIVATE_BASE+0x102)
#define V4L2_CID_STM_DVPIF_FULL_RANGE				(V4L2_CID_PRIVATE_BASE+0x103)
#define V4L2_CID_STM_DVPIF_INCOMPLETE_FIRST_PIXEL		(V4L2_CID_PRIVATE_BASE+0x104)
#define V4L2_CID_STM_DVPIF_ODD_PIXEL_COUNT			(V4L2_CID_PRIVATE_BASE+0x105)
#define V4L2_CID_STM_DVPIF_VSYNC_BOTTOM_HALF_LINE_ENABLE	(V4L2_CID_PRIVATE_BASE+0x106)
#define V4L2_CID_STM_DVPIF_EXTERNAL_SYNC			(V4L2_CID_PRIVATE_BASE+0x107)
#define V4L2_CID_STM_DVPIF_EXTERNAL_SYNC_POLARITY		(V4L2_CID_PRIVATE_BASE+0x108)
#define V4L2_CID_STM_DVPIF_EXTERNAL_SYNCHRO_OUT_OF_PHASE	(V4L2_CID_PRIVATE_BASE+0x109)
#define V4L2_CID_STM_DVPIF_EXTERNAL_VREF_ODD_EVEN		(V4L2_CID_PRIVATE_BASE+0x10a)
#define V4L2_CID_STM_DVPIF_EXTERNAL_VREF_POLARITY_POSITIVE	(V4L2_CID_PRIVATE_BASE+0x10b)
#define V4L2_CID_STM_DVPIF_HREF_POLARITY_POSITIVE		(V4L2_CID_PRIVATE_BASE+0x10c)
#define V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HORIZONTAL_OFFSET	(V4L2_CID_PRIVATE_BASE+0x10d)
#define V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_VERTICAL_OFFSET	(V4L2_CID_PRIVATE_BASE+0x10e)
#define V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_WIDTH		(V4L2_CID_PRIVATE_BASE+0x10f)
#define V4L2_CID_STM_DVPIF_ACTIVE_AREA_ADJUST_HEIGHT		(V4L2_CID_PRIVATE_BASE+0x110)
#define V4L2_CID_STM_DVPIF_COLOUR_MODE				(V4L2_CID_PRIVATE_BASE+0x111)
#define V4L2_CID_STM_DVPIF_VIDEO_LATENCY			(V4L2_CID_PRIVATE_BASE+0x112)
#define V4L2_CID_STM_DVPIF_BLANK				(V4L2_CID_PRIVATE_BASE+0x113)
#define V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE_SPECIFIED		(V4L2_CID_PRIVATE_BASE+0x114)
#define V4L2_CID_STM_DVPIF_ANC_PAGE_SIZE			(V4L2_CID_PRIVATE_BASE+0x115)
#define V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_ENABLE		(V4L2_CID_PRIVATE_BASE+0x116)
#define V4L2_CID_STM_DVPIF_HORIZONTAL_RESIZE_FACTOR		(V4L2_CID_PRIVATE_BASE+0x117)
#define V4L2_CID_STM_DVPIF_TOP_FIELD_FIRST			(V4L2_CID_PRIVATE_BASE+0x118)

#define V4L2_CID_STM_DVPIF_ANC_INPUT_BUFFER_SIZE		(V4L2_CID_PRIVATE_BASE+0x119)

/******************************
 * DEFINES
 ******************************/
#define GAM_DVP_CTL    (0x00/4)
#define GAM_DVP_TFO    (0x04/4)
#define GAM_DVP_TFS    (0x08/4)
#define GAM_DVP_BFO    (0x0C/4)
#define GAM_DVP_BFS    (0x10/4)
#define GAM_DVP_VTP    (0x14/4)
#define GAM_DVP_VBP    (0x18/4)
#define GAM_DVP_VMP    (0x1C/4)
#define GAM_DVP_CVS    (0x20/4)
#define GAM_DVP_VSD    (0x24/4)
#define GAM_DVP_HSD    (0x28/4)
#define GAM_DVP_HLL    (0x2C/4)
#define GAM_DVP_HSRC   (0x30/4)
#define GAM_DVP_HFC(x) ((0x34/4)+x)
#define GAM_DVP_VSRC   (0x60/4)
#define GAM_DVP_VFC(x) ((0x64/4)+x)
#define GAM_DVP_ITM    (0x98/4)
#define GAM_DVP_ITS    (0x9C/4)
#define GAM_DVP_STA    (0xA0/4)
#define GAM_DVP_LNSTA  (0xA4/4)
#define GAM_DVP_HLFLN  (0xA8/4)
#define GAM_DVP_ABA    (0xAC/4)
#define GAM_DVP_AEA    (0xB0/4)
#define GAM_DVP_APS    (0xB4/4)
#define GAM_DVP_PKZ    (0xFC/4)

/******************************
 * DVP video modes
 ******************************/

#define DVP_720_480_I60000		0x80000001		/* NTSC I60 						*/
#define DVP_720_480_P60000		0x80000002		/* NTSC P60 						*/
#define DVP_720_480_I59940		0x80000003		/* NTSC 						*/
#define DVP_720_480_P59940		0x80000004		/* NTSC P59.94 (525p ?) 				*/
#define DVP_640_480_I59940		0x80000005		/* NTSC square, PAL M square 				*/
#define DVP_720_576_I50000		0x80000006		/* PAL B,D,G,H,I,N, SECAM 				*/
#define DVP_720_576_P50000		0x80000007		/* PAL P50 (625p ?)					*/
#define DVP_768_576_I50000		0x80000008		/* PAL B,D,G,H,I,N, SECAM square 			*/
#define DVP_1920_1080_P60000		0x80000009		/* SMPTE 274M #1  P60 					*/
#define DVP_1920_1080_P59940		0x80000010		/* SMPTE 274M #2  P60 /1.001 				*/
#define DVP_1920_1080_P50000		0x80000011		/* SMPTE 274M #3  P50 					*/
#define DVP_1920_1080_P30000		0x80000012		/* SMPTE 274M #7  P30 					*/
#define DVP_1920_1080_P29970		0x80000013		/* SMPTE 274M #8  P30 /1.001 				*/
#define DVP_1920_1080_P25000		0x80000014		/* SMPTE 274M #9  P25 					*/
#define DVP_1920_1080_P24000		0x80000015		/* SMPTE 274M #10 P24 					*/
#define DVP_1920_1080_P23976		0x80000016		/* SMPTE 274M #11 P24 /1.001 				*/
#define DVP_1920_1080_I60000		0x80000017		/* EIA770.3 #3 I60 = SMPTE274M #4 I60 			*/
#define DVP_1920_1080_I59940		0x80000018		/* EIA770.3 #4 I60 /1.001 = SMPTE274M #5 I60 /1.001 	*/
#define DVP_1920_1080_I50000_274M	0x80000019		/* SMPTE 274M Styled 1920x1080 I50 CEA/HDMI Code 20 	*/
#define DVP_1920_1080_I50000_AS4933	0x80000020		/* AS 4933.1-2005 1920x1080 I50 CEA/HDMI Code 39 	*/
#define DVP_1280_720_P60000		0x80000021		/* EIA770.3 #1 P60 = SMPTE 296M #1 P60 			*/
#define DVP_1280_720_P59940		0x80000022		/* EIA770.3 #2 P60 /1.001= SMPTE 296M #2 P60 /1.001 	*/
#define DVP_1280_720_P50000		0x80000023		/* SMPTE 296M Styled 1280x720 50P CEA/HDMI Code 19 	*/
#define DVP_1280_1152_I50000		0x80000024		/* AS 4933.1 1280x1152 I50 				*/
#define DVP_640_480_P59940		0x80000025		/* 640x480p @ 59.94Hz                    		*/
#define DVP_640_480_P60000		0x80000026		/* 640x480p @ 60Hz                       		*/

/******************************
 * Useful defines
 ******************************/

#define INVALID_TIME				0xfeedfacedeadbeefULL
#define DVP_MAXIMUM_FRAME_JITTER		16				// us

#define DVP_MAX_VIDEO_DECODE_BUFFER_COUNT	64
#define	DVP_VIDEO_DECODE_BUFFER_STACK_SIZE	(DVP_MAX_VIDEO_DECODE_BUFFER_COUNT+1)
#define DVP_MAX_SUPPORTED_PPM_CLOCK_ERROR	200
#define DVP_WARM_UP_TRIES			32
#define DVP_WARM_UP_VIDEO_FRAMES_30		2
#define DVP_LEAD_IN_VIDEO_FRAMES_30		(2 + DVP_WARM_UP_VIDEO_FRAMES_30)
#define DVP_WARM_UP_VIDEO_FRAMES_60		2
#define DVP_LEAD_IN_VIDEO_FRAMES_60		(4 + DVP_WARM_UP_VIDEO_FRAMES_60)

#define DVP_MINIMUM_TIME_INTEGRATION_FRAMES	32
#define DVP_MAXIMUM_TIME_INTEGRATION_FRAMES	32768			// at 60fps must be at least 8192 to use the whole 28 bits of corrective muiltiplier
#define DVP_CORRECTION_FIXED_POINT_BITS		28
#define DVP_ONE_PPM				(1LL << (DVP_CORRECTION_FIXED_POINT_BITS - 20))
#define DVP_VIDEO_THREAD_PRIORITY		40
#define DVP_SYNCHRONIZER_THREAD_PRIORITY	99				// Runs at highest possible priority

#define DVP_DEFAULT_ANCILLARY_INPUT_BUFFER_SIZE	(16 * 1024)			// Defaults for the ancillary input buffers, modifyable by controls
#define DVP_DEFAULT_ANCILLARY_PAGE_SIZE		256

#define DVP_ANCILLARY_PARTITION			"BPA2_Region0"
#define DVP_MIN_ANCILLARY_BUFFERS		4
#define DVP_MAX_ANCILLARY_BUFFERS		32
#define DVP_ANCILLARY_BUFFER_CHUNK_SIZE		16				// it deals with 128 bit words
#define DVP_MIN_ANCILLARY_BUFFER_SIZE		(2 * DVP_ANCILLARY_BUFFER_CHUNK_SIZE)
#define DVP_MAX_ANCILLARY_BUFFER_SIZE		4096				// Hardware restriction

/******************************
 * Local types
 ******************************/

typedef enum
{
    DvpInactive		= 0,
    DvpStarting,				// Enterred by user level, awaiting first interrupt
    DvpWarmingUp,				// Enterred by Interrupt, Got first interrupt, collecting data to establish timing baseline
    DvpStarted,					// Enterred by Interrupt, baseline established
    DvpMovingToRun,				// Enterred by user level, awaiting first collection
    DvpRunning,					// Enterred by Interrupt, running freely
    DvpMovingToInactive				// Enterred by user level, shutting down
} DvpState_t;

//

typedef struct DvpBufferStack_s
{
    buffer_handle_t		 Buffer;
    unsigned char		*Data;
    unsigned long long		 ExpectedFillTime;
} DvpBufferStack_t;

//

typedef struct AncillaryBufferState_s
{
    unsigned char	*PhysicalAddress;
    unsigned char	*UnCachedAddress;
    bool		 Queued;
    bool		 Done;
    unsigned int	 Bytes;
    unsigned long long	 FillTime;

    bool		 Mapped;		// External V4L2 flag
} AncillaryBufferState_t;

//

typedef struct dvp_v4l2_video_handle_s
{
    avr_v4l2_shared_handle_t	*SharedContext;
    struct DeviceContext_s	*DeviceContext;
    volatile int 		*DvpRegs;
    unsigned int 		 DvpIrq;
    unsigned long long		 DvpLatency;

    stm_display_mode_t		 inputmode;
    unsigned int		 BytesPerLine;				// Obtained when we get a buffer
    StreamInfo_t		 StreamInfo;				// Derived values supplied to player

    unsigned int		 RegisterTFO;				// Calculated top field offset register
    unsigned int		 RegisterTFS;				// Calculated top field stop register
    unsigned int		 RegisterBFO;
    unsigned int		 RegisterBFS;
    unsigned int		 RegisterCVS;				// Calculated captured window size register
    unsigned int		 RegisterVMP;				// Pitch (in a field)
    unsigned int		 RegisterVBPminusVTP;			// Offset borrom field from top field in memory
    unsigned int		 RegisterHLL;				// Half line length
    unsigned int		 RegisterCTL;				// Calculated control register value

    unsigned int		 DvpWarmUpVideoFrames;			// These vary with input mode to allow similar times
    unsigned int		 DvpLeadInVideoFrames;

    struct semaphore 		 DvpVideoInterruptSem;
    struct semaphore 		 DvpSynchronizerWakeSem;
    struct semaphore 		 DvpAncillaryBufferDoneSem;

    bool			 VideoRunning;				// Indicates video process is running
    bool			 FastModeSwitch;			// Indicates that a fast mode switch is in progress

    bool			 SynchronizerRunning;			// Indicates synchronizer process is running
    bool			 Synchronize;				// Indicates to synchronizer to synchronize

    DvpState_t			 DvpState;

    const stm_mode_line_t	*DvpCaptureMode;

    unsigned int		 DvpNextBufferToGet;
    unsigned int		 DvpNextBufferToInject;
    unsigned int		 DvpNextBufferToFill;
    int				 DvpFrameCount;
    atomic_t                     DvpFrameCaptureNotification; //!< Count down timer (for sysfs) based on the number of frame processed.
    struct class_device		*DvpSysfsClassDevice;
    DvpBufferStack_t		 DvpBufferStack[DVP_VIDEO_DECODE_BUFFER_STACK_SIZE];

    unsigned int		 DvpIntegrateForAtLeastNFrames;
    unsigned int		 DvpInterruptFrameCount;
    unsigned int		 DvpwarmUpSynchronizationAttempts;
    unsigned long long		 DvpTimeAtZeroInterruptFrameCount;
    unsigned long long		 DvpTimeOfLastFrameInterrupt;
    bool			 StandardFrameRate;

    bool			 DvpCalculatingFrameTime;			// Boolean set by process, to tell interrupt to avoid manipulating base times
    long long 			 DvpLastDriftCorrection;
    long long			 DvpLastFrameDriftError;
    long long			 DvpCurrentDriftError;
    unsigned int		 DvpDriftFrameCount;

    unsigned long long		 DvpBaseTime;
    unsigned long long		 DvpRunFromTime;
    unsigned long long		 DvpFrameDurationCorrection;			// Fixed point 2^DVP_CORRECTION_FIXED_POINT_BITS is one.

    bool			 AncillaryStreamOn;
    bool			 AncillaryCaptureInProgress;
    unsigned char		*AncillaryInputBufferUnCachedAddress;		// The actual hardware buffer
    unsigned char		*AncillaryInputBufferPhysicalAddress;
    unsigned char		*AncillaryInputBufferInputPointer;
    unsigned int		 AncillaryInputBufferSize;
    unsigned int		 AncillaryPageSizeSpecified;
    unsigned int		 AncillaryPageSize;

    unsigned int		 AncillaryBufferCount;				// Ancillary buffer information
    unsigned int		 AncillaryBufferSize;
    unsigned char		*AncillaryBufferUnCachedAddress;		// Base pointers rather than individual buffers
    unsigned char		*AncillaryBufferPhysicalAddress;
    unsigned int		 AncillaryBufferNextQueueIndex;
    unsigned int		 AncillaryBufferNextFillIndex;
    unsigned int		 AncillaryBufferNextDeQueueIndex;
    unsigned int		 AncillaryBufferQueue[DVP_MAX_ANCILLARY_BUFFERS];
    AncillaryBufferState_t	 AncillaryBufferState[DVP_MAX_ANCILLARY_BUFFERS];

    // The current values of all the controls 

    int			 DvpControlSrcMode656;

    int			 DvpControl16Bit;
    int			 DvpControlBigEndian;
    int			 DvpControlFullRange;
    int			 DvpControlIncompleteFirstPixel;
    int			 DvpControlOddPixelCount;
    int			 DvpControlVsyncBottomHalfLineEnable;
    int			 DvpControlExternalSync;
    int			 DvpControlExternalSyncPolarity;
    int			 DvpControlExternalSynchroOutOfPhase;
    int			 DvpControlExternalVRefOddEven;
    int			 DvpControlExternalVRefPolarityPositive;
    int			 DvpControlHRefPolarityPositive;
    int			 DvpControlActiveAreaAdjustHorizontalOffset;
    int			 DvpControlActiveAreaAdjustVerticalOffset;
    int			 DvpControlActiveAreaAdjustWidth;
    int			 DvpControlActiveAreaAdjustHeight;
    int			 DvpControlColourMode;				// 0 => Default, 1 => 601, 2 => 709
    int			 DvpControlVideoLatency;
    int			 DvpControlBlank;
    int			 DvpControlAncPageSizeSpecified;
    int			 DvpControlAncPageSize;
    int			 DvpControlAncInputBufferSize;
    int			 DvpControlHorizontalResizeEnable;
    int			 DvpControlHorizontalResizeFactor;
    int			 DvpControlTopFieldFirst;

    int			 DvpControlDefault16Bit;
    int			 DvpControlDefaultBigEndian;
    int			 DvpControlDefaultFullRange;
    int			 DvpControlDefaultIncompleteFirstPixel;
    int			 DvpControlDefaultOddPixelCount;
    int			 DvpControlDefaultVsyncBottomHalfLineEnable;
    int			 DvpControlDefaultExternalSync;
    int			 DvpControlDefaultExternalSyncPolarity;
    int			 DvpControlDefaultExternalSynchroOutOfPhase;
    int			 DvpControlDefaultExternalVRefOddEven;
    int			 DvpControlDefaultExternalVRefPolarityPositive;
    int			 DvpControlDefaultHRefPolarityPositive;
    int			 DvpControlDefaultActiveAreaAdjustHorizontalOffset;
    int			 DvpControlDefaultActiveAreaAdjustVerticalOffset;
    int			 DvpControlDefaultActiveAreaAdjustWidth;
    int			 DvpControlDefaultActiveAreaAdjustHeight;
    int			 DvpControlDefaultColourMode;
    int			 DvpControlDefaultVideoLatency;
    int			 DvpControlDefaultBlank;
    int			 DvpControlDefaultAncPageSizeSpecified;
    int			 DvpControlDefaultAncPageSize;
    int			 DvpControlDefaultAncInputBufferSize;
    int			 DvpControlDefaultHorizontalResizeEnable;
    int			 DvpControlDefaultHorizontalResizeFactor;
    int			 DvpControlDefaultTopFieldFirst;

} dvp_v4l2_video_handle_t;


/******************************
 * Function prototypes of
 * functions exported by the
 * video code.
 ******************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
int DvpInterrupt(int irq, void* data);
#else
int DvpInterrupt(int irq, void* data, struct pt_regs* pRegs);
#endif

int DvpVideoClose( 			dvp_v4l2_video_handle_t	*Context );

int DvpVideoThreadHandle( 		void 			*Data );

int DvpVideoIoctlSetFramebuffer(	dvp_v4l2_video_handle_t	*Context,
					unsigned int		 Width,
					unsigned int		 Height,
					unsigned int		 BytesPerLine,
					unsigned int		 Control );

int DvpVideoIoctlSetStandard(           dvp_v4l2_video_handle_t	*Context,
					v4l2_std_id		 Id );

int DvpVideoIoctlOverlayStart(  	dvp_v4l2_video_handle_t	*Context );

int DvpVideoIoctlCrop(			dvp_v4l2_video_handle_t	*Context,
					struct v4l2_crop	*Crop );

int DvpVideoIoctlSetControl( 		dvp_v4l2_video_handle_t	*Context,
					unsigned int		 Control,
					unsigned int		 Value );

int DvpVideoIoctlGetControl( 		dvp_v4l2_video_handle_t	*Context,
					unsigned int		 Control,
					unsigned int		*Value );

// ////////////////////////////

int DvpVideoIoctlAncillaryRequestBuffers(	dvp_v4l2_video_handle_t	*Context,
						unsigned int		 DesiredCount,
						unsigned int		 DesiredSize,
						unsigned int		*ActualCount,
						unsigned int		*ActualSize );

int DvpVideoIoctlAncillaryQueryBuffer(		dvp_v4l2_video_handle_t	 *Context,
						unsigned int		  Index,
						bool			 *Queued,
						bool			 *Done,
						unsigned char		**PhysicalAddress,
						unsigned char		**UnCachedAddress,
						unsigned long long	 *CaptureTime,
						unsigned int		 *Bytes,
						unsigned int		 *Size );

int DvpVideoIoctlAncillaryQueueBuffer(		dvp_v4l2_video_handle_t	*Context,
						unsigned int		 Index );

int DvpVideoIoctlAncillaryDeQueueBuffer(	dvp_v4l2_video_handle_t	*Context,
						unsigned int		*Index,
						bool			 Blocking );

int DvpVideoIoctlAncillaryStreamOn(		dvp_v4l2_video_handle_t	*Context );
int DvpVideoIoctlAncillaryStreamOff(		dvp_v4l2_video_handle_t	*Context );


#endif /*STMDVPVIDEO_H_*/

