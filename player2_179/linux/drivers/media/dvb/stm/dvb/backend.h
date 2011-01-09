/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : backend.h - player access points
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
31-Jan-07   Created                                         Julian

************************************************************************/

#ifndef H_BACKEND
#define H_BACKEND

#include <linux/kthread.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include "linux/dvb/stm_ioctls.h"

#include "backend_ops.h"
#include "dvb_module.h"

#define MAX_STREAMS_PER_PLAYBACK        8
#define MAX_PLAYBACKS                   4

#define TRANSPORT_PACKET_SIZE           188
#define BLUERAY_PACKET_SIZE             (TRANSPORT_PACKET_SIZE+sizeof(unsigned int))
#define CONTINUITY_COUNT_MASK           0x0f000000
#define CONTINUITY_COUNT( H )           ((H & CONTINUITY_COUNT_MASK) >> 24)
#define PID_HIGH_MASK                   0x00001f00
#define PID_LOW_MASK                    0x00ff0000
#define PID( H )                        ((H & PID_HIGH_MASK) | ((H & PID_LOW_MASK) >> 16))

#define STREAM_HEADER_TYPE              0
#define STREAM_HEADER_FLAGS             1
#define STREAM_HEADER_SIZE              4
#define STREAM_LAST_PACKET_FLAG         0x01
#define AUDIO_STREAM_BUFFER_SIZE        (200*TRANSPORT_PACKET_SIZE)
#define VIDEO_STREAM_BUFFER_SIZE        (200*TRANSPORT_PACKET_SIZE)
#define DEMUX_BUFFER_SIZE               (512*TRANSPORT_PACKET_SIZE)

#define STREAM_INCOMPLETE               1
#define AUDIO_INCOMPLETE                (AUDIO_PAUSED+1)
#define VIDEO_INCOMPLETE                (VIDEO_FREEZED+1)

#define CodeToInteger(a,b,c,d)          ((a << 0) | (b << 8) | (c << 16) | (d << 24))

/*      Debug printing macros   */

#ifndef ENABLE_BACKEND_DEBUG
#define ENABLE_BACKEND_DEBUG            0
#endif

#define BACKEND_DEBUG(fmt, args...)  ((void) (ENABLE_BACKEND_DEBUG && \
                                            (printk("LinuxDvb:%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define BACKEND_TRACE(fmt, args...)  (printk("LinuxDvb:%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define BACKEND_ERROR(fmt, args...)  (printk("ERROR:LinuxDvb:%s: " fmt, __FUNCTION__, ##args))

/* The stream context provides details on a single audio or video stream */
struct StreamContext_s
{
    struct mutex                Lock;
    struct DeviceContext_s*     Device;                 /* Linux DVB device bundle giving control and data */

    stream_handle_t             Handle;                 /* Opaque handle to access backend actions */
    /* Stream functions */
    int                       (*Inject)        (stream_handle_t         Stream,
                                                const unsigned char*    Data,
                                                unsigned int            DataLength);
    int                       (*InjectPacket)  (stream_handle_t         Stream,
                                                const unsigned char*    Data,
                                                unsigned int            DataLength,
                                                bool                    PresentationTimeValid,
                                                unsigned long long      PresentationTime);
    int                       (*Delete)        (playback_handle_t       Playback,
                                                stream_handle_t         Stream);


    unsigned int                BufferLength;
    unsigned char*              Buffer;
    unsigned int                DataToWrite;

#if defined (USE_INJECTION_THREAD)
    struct task_struct*         InjectionThread;        /* Injection thread management */
    unsigned int                Injecting;
    unsigned int                ThreadTerminated;
    wait_queue_head_t           BufferEmpty;            /*! Queue to wake up any waiters */
    struct semaphore            DataReady;
#endif
};

/*
   The playback context keeps track of the bundle of streams plyaing synchronously
   Every stream or demux added to the bundle ups the usage count. Every one removed reduces it.
   The playback can be deleted when the count returns to zero
*/
struct PlaybackContext_s
{
    struct mutex                Lock;
    playback_handle_t           Handle;
    unsigned int                UsageCount;
};


static inline int StreamBufferFree   (struct StreamContext_s*         Stream)
{
    return (Stream->DataToWrite == 0);
}

/* Entry point list */

int BackendInit                            (void);
int BackendDelete                          (void);

int PlaybackCreate                         (struct PlaybackContext_s**      PlaybackContext);
int PlaybackDelete                         (struct PlaybackContext_s*       Playback);
int PlaybackAddStream                      (struct PlaybackContext_s*       Playback,
                                            char*                           Media,
                                            char*                           Format,
                                            char*                           Encoding,
                                            unsigned int                    DemuxId,
                                            unsigned int                    SurfaceId,
                                            struct StreamContext_s**        Stream);
int PlaybackRemoveStream                   (struct PlaybackContext_s*       Playback,
                                            struct StreamContext_s*         Stream);
int PlaybackSetSpeed                       (struct PlaybackContext_s*       Playback,
                                            unsigned int                    Speed);
int PlaybackGetSpeed                       (struct PlaybackContext_s*       Playback,
                                            unsigned int*                   Speed);
int PlaybackSetNativePlaybackTime          (struct PlaybackContext_s*       Playback,
                                            unsigned long long              NativeTime,
                                            unsigned long long              SystemTime);
int PlaybackSetOption                      (struct PlaybackContext_s*       Playback,
                                            play_option_t                   Option,
                                            unsigned int                    Value);
int PlaybackGetPlayerEnvironment           (struct PlaybackContext_s*     Playback,
                                            playback_handle_t*            playerplayback);

int StreamEnable                           (struct StreamContext_s*         Stream,
                                            unsigned int                    Enable);
int StreamSetId                            (struct StreamContext_s*         Stream,
                                            unsigned int                    DemuxId,
                                            unsigned int                    Id);
int StreamChannelSelect                    (struct StreamContext_s*         Stream,
                                            channel_select_t                Channel);
int StreamSetOption                        (struct StreamContext_s*         Stream,
                                            play_option_t                   Option,
                                            unsigned int                    Value);
int StreamGetOption                        (struct StreamContext_s*         Stream,
                                            play_option_t                   Option,
                                            unsigned int*                   Value);
int StreamGetPlayInfo                      (struct StreamContext_s*         Stream,
                                            struct play_info_s*             PlayInfo);
int StreamDrain                            (struct StreamContext_s*         Stream,
                                            unsigned int                    Discard);
int StreamDiscontinuity                    (struct StreamContext_s*         Stream,
                                            discontinuity_t                 Discontinuity);
int StreamStep                             (struct StreamContext_s*         Stream);
int StreamSwitch                           (struct StreamContext_s*         Stream,
                                            char*                           Format,
                                            char*                           Encoding);
int StreamGetDecodeBuffer                  (struct StreamContext_s*         Stream,
                                            buffer_handle_t*                Buffer,
                                            unsigned char**                 Data,
                                            unsigned int                    Format,
                                            unsigned int                    DimensionCount,
                                            unsigned int                    Dimensions[],
                                            unsigned int*                   Index,
                                            unsigned int*                   Stride);
int StreamReturnDecodeBuffer               (struct StreamContext_s*         Stream,
                                            buffer_handle_t*                Buffer);
int StreamGetDecodeBufferPoolStatus        (struct StreamContext_s*         Stream,
                                            unsigned int*                   BuffersInPool,
                                            unsigned int*                   BuffersWithNonZeroReferenceCount);
#ifdef __TDT__
int StreamGetOutputWindow                  (struct StreamContext_s*         Stream,
                                            unsigned int*                    X,
                                            unsigned int*                    Y,
                                            unsigned int*                    Width,
                                            unsigned int*                    Height);
#endif
int StreamSetOutputWindow                  (struct StreamContext_s*         Stream,
                                            unsigned int                    X,
                                            unsigned int                    Y,
                                            unsigned int                    Width,
                                            unsigned int                    Height);
int StreamSetInputWindow                   (struct StreamContext_s*         Stream,
                                            unsigned int                    X,
                                            unsigned int                    Y,
                                            unsigned int                    Width,
                                            unsigned int                    Height);
int StreamSetPlayInterval                  (struct StreamContext_s*         Stream,
                                            dvb_play_interval_t*            PlayInterval);

int StreamInject                           (struct StreamContext_s*         Stream,
                                            const unsigned char*            Buffer,
                                            unsigned int                    Length);

int StreamInjectPacket                     (struct StreamContext_s*         Stream,
                                            const unsigned char*            Buffer,
                                            unsigned int                    Length,
                                            bool                            PresentationTimeValid,
                                            unsigned long long              PresentationTime);

int StreamGetFirstBuffer                   (struct StreamContext_s*         Stream,
                                            const char __user*              Buffer,
                                            unsigned int                    Length);
int StreamIdentifyAudio                    (struct StreamContext_s*         Stream,
                                            unsigned int*                   Id);
int StreamIdentifyVideo                    (struct StreamContext_s*         Stream,
                                            unsigned int*                   Id);
int StreamGetPlayerEnvironment             (struct StreamContext_s*         Stream,
                                            playback_handle_t*              playerplayback,
                                            stream_handle_t*                playerstream);
static inline int PlaybackAddDemux         (struct PlaybackContext_s*       Playback,
                                            unsigned int                    DemuxId,
                                            struct StreamContext_s**        Demux)
{
    return PlaybackAddStream   (Playback, NULL, NULL, NULL, DemuxId, DemuxId, Demux);
}

static inline int PlaybackRemoveDemux      (struct PlaybackContext_s*       Playback,
                                            struct StreamContext_s*         Demux)
{
    return PlaybackRemoveStream (Playback, Demux);
}



stream_event_signal_callback   StreamRegisterEventSignalCallback       (struct StreamContext_s*         Stream,
                                                                        struct DeviceContext_s*         Context,
                                                                        stream_event_signal_callback    CallBack);
int DisplayDelete                          (char*                           Media,
                                            unsigned int                    SurfaceId);

int DisplaySynchronize                     (char*                           Media,
                                            unsigned int                    SurfaceId);

#endif
