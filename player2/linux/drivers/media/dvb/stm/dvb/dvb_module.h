/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : dvb_module.h - streamer device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-03   Created                                         Julian

************************************************************************/

#ifndef H_DVB_MODULE
#define H_DVB_MODULE

#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include "linux/dvb/stm_ioctls.h"

#ifdef __TDT__
#include <pti_public.h>
#endif

#include "dvbdev.h"
#include "dmxdev.h"
#include "dvb_demux.h"

#ifdef __TDT__
/* forward declaration (because used in the header files below) */
struct DeviceContext_s;
#endif

#include "backend.h"
#include "dvb_video.h"
#include "dvb_audio.h"

#ifndef false
#define false   0
#define true    1
#endif

/*      Debug printing macros   */
#ifndef ENABLE_DVB_DEBUG
#define ENABLE_DVB_DEBUG                1
#endif

#define DVB_DEBUG(fmt, args...)         ((void) (ENABLE_DVB_DEBUG && \
                                            (printk("%s: " fmt, __FUNCTION__,##args), 0)))

/* Output trace information off the critical path */
#define DVB_TRACE(fmt, args...)         (printk("%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define DVB_ERROR(fmt, args...)         (printk("ERROR in %s: " fmt, __FUNCTION__,##args))


#define DVB_MAX_DEVICES_PER_ADAPTER     4

struct DemuxBuffer_s
{
    unsigned int                OutPtr;
    unsigned int                InPtr;
    struct task_struct*         InjectionThread;
    unsigned int                DemuxInjecting;
};

struct Rectangle_s
{
    unsigned int                X;
    unsigned int                Y;
    unsigned int                Width;
    unsigned int                Height;
};

struct Ratio_s
{
    unsigned int                Numerator;
    unsigned int                Denominator;
};

struct DeviceContext_s
{
    unsigned int                Id;


    struct dvb_device*          AudioDevice;
    struct audio_status         AudioState;
    unsigned int                AudioId;
    audio_encoding_t            AudioEncoding;
    struct StreamContext_s*     AudioStream;
    unsigned int                AudioOpenWrite;
    struct mutex                AudioWriteLock;
    struct mutex*               ActiveAudioWriteLock;
    unsigned int                AudioCaptureStatus;

    struct dvb_device*          VideoDevice;
    struct video_status         VideoState;
    unsigned int                VideoId;
    video_encoding_t            VideoEncoding;
    video_size_t                VideoSize;
    unsigned int                FrameRate;
    struct StreamContext_s*     VideoStream;
    unsigned int                VideoOpenWrite;
    struct VideoEvent_s         VideoEvents;
    struct mutex                VideoWriteLock;
    struct mutex*               ActiveVideoWriteLock;
    unsigned int                PlayOption[DVB_OPTION_MAX];
    unsigned int                PlayValue[DVB_OPTION_MAX];
    struct Rectangle_s          VideoOutputWindow;
    struct Rectangle_s          VideoInputWindow;
    struct Ratio_s              PixelAspectRatio;
    unsigned int                VideoCaptureStatus;


    struct DeviceContext_s*     DemuxContext;           /* av can be wired to different demux - default is self */
    struct DeviceContext_s*     SyncContext;            /* av can be synchronised to a different device - default is self */

    struct dvb_demux            DvbDemux;
    struct dmxdev               DmxDevice;
    struct dmx_frontend         MemoryFrontend;

    struct StreamContext_s*     DemuxStream;

    struct PlaybackContext_s*   Playback;
    stream_type_t               StreamType;
    int                         PlaySpeed;
    dvb_play_interval_t         PlayInterval;

    struct dvb_device*          CaDevice;
    unsigned char *dvr_in;
    unsigned char *dvr_out;
    unsigned int EncryptionOn;

#ifdef __TDT__
    struct PtiSession*          pPtiSession;
    int dvr_write;
    int                         VideoPlaySpeed;
    int provideToDecoder;
    int feedPesType;
    struct mutex injectMutex;
#endif
    struct DvbContext_s*        DvbContext;
};

struct DvbContext_s
{
    void*                       PtiPrivate;

    struct dvb_device           DvbDevice;
    struct dvb_adapter          DvbAdapter;

    struct mutex                Lock;

    struct DeviceContext_s      DeviceContext[DVB_MAX_DEVICES_PER_ADAPTER];

};

long DvbGenericUnlockedIoctl(struct file *, unsigned int, unsigned long);

#endif
