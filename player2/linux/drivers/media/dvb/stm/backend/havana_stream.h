/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : havana_stream.h derived from havana_player.h
Author :           Nick

Definition of the implementation of stream module for havana.


Date        Modification                                    Name
----        ------------                                    --------
14-Feb-07   Created                                         Julian

************************************************************************/

#ifndef H_HAVANA_STREAM
#define H_HAVANA_STREAM

#include "osdev_user.h"
#include "backend_ops.h"
#include "player.h"
#include "player_types.h"
#include "buffer_generic.h"
#include "player_generic.h"
#include "havana_playback.h"

/*      Debug printing macros   */
#ifndef ENABLE_STREAM_DEBUG
#define ENABLE_STREAM_DEBUG             1
#endif

#define STREAM_DEBUG(fmt, args...)      ((void) (ENABLE_STREAM_DEBUG && \
                                            (report(severity_note, "HavanaStream_c::%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define STREAM_TRACE(fmt, args...)      (report(severity_note, "HavanaStream_c::%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define STREAM_ERROR(fmt, args...)      (report(severity_error, "HavanaStream_c::%s: " fmt, __FUNCTION__, ##args))

#define STREAM_SPECIFIC_EVENTS          (EventSourceFrameRateChangeManifest | EventSourceSizeChangeManifest |   \
                                         EventFailedToDecodeInTime | EventFailedToDeliverDataInTime |           \
                                         EventTrickModeDomainChange |                                           \
                                         EventFirstFrameManifested |  EventStreamUnPlayable)

#if defined (CONFIG_CPU_SUBTYPE_STX7200)

#define FIRST_AUDIO_COPROCESSOR '3'
#define NUMBER_AUDIO_COPROCESSORS 2

#else

// on some systems the defaults burnt into the codec are sufficient
#define FIRST_AUDIO_COPROCESSOR '\0'
#define NUMBER_AUDIO_COPROCESSORS 0 /* UNKNOWN */

#endif

/// Player wrapper class responsible for managing a stream.
class HavanaStream_c
{
private:
    unsigned int                StreamId;
    unsigned int                DemuxId;
    OS_Mutex_t                  InputLock;
    bool                        LockInitialised;

    bool                        Manifest;

    PlayerStreamType_t          PlayerStreamType;
    PlayerStream_t              PlayerStream;
    DemultiplexorContext_t      DemuxContext;

    class HavanaPlayer_c*       HavanaPlayer;

    class Player_c*             Player;
    class Demultiplexor_c*      Demultiplexor;
    class Collator_c*           Collator;
    class FrameParser_c*        FrameParser;
    class Codec_c*              Codec;
    class OutputTimer_c*        OutputTimer;
    class Manifestor_c*         Manifestor;
    class BufferPool_c*         DecodeBufferPool;

    PlayerPlayback_t            PlayerPlayback;

    stream_event_signal_callback        EventSignalCallback;
    context_handle_t            CallbackContext;
    struct stream_event_s       StreamEvent;

public:

                                HavanaStream_c                 (void);
                               ~HavanaStream_c                 (void);

    HavanaStatus_t              Init                           (class HavanaPlayer_c*           HavanaPlayer,
                                                                class Player_c*                 Player,
                                                                PlayerPlayback_t                PlayerPlayback,
                                                                char*                           Media,
                                                                char*                           Format,
                                                                char*                           Encoding,
                                                                unsigned int                    SurfaceId);
    HavanaStatus_t              InjectData                     (const unsigned char*            Data,
                                                                unsigned int                    DataLength);
    HavanaStatus_t              InjectDataPacket               (const unsigned char*            Data,
                                                                unsigned int                    DataLength,
                                                                bool                            PlaybackTimeValid,
                                                                unsigned long long              PlaybackTime);
    HavanaStatus_t              Discontinuity                  (bool                            ContinuousReverse,
                                                                bool                            SurplusData);
    HavanaStatus_t              Drain                          (bool                            Discard);
    HavanaStatus_t              Enable                         (bool                            Manifest);
    HavanaStatus_t              SetId                          (unsigned int                    DemuxId,
                                                                unsigned int                    Id);
    HavanaStatus_t              ChannelSelect                  (channel_select_t                Channel);
    HavanaStatus_t              SetOption                      (play_option_t                   Option,
                                                                unsigned int                    Value);
    HavanaStatus_t              GetOption                      (play_option_t                   Option,
                                                                unsigned int*                   Value);
    HavanaStatus_t              Step                           (void);
    HavanaStatus_t              SetOutputWindow                (unsigned int                    X,
                                                                unsigned int                    Y,
                                                                unsigned int                    Width,
                                                                unsigned int                    Height);
    HavanaStatus_t              SetInputWindow                 (unsigned int                    X,
                                                                unsigned int                    Y,
                                                                unsigned int                    Width,
                                                                unsigned int                    Height);
    HavanaStatus_t              CheckEvent                     (struct PlayerEventRecord_s*     PlayerEvent);
    HavanaStatus_t              GetPlayInfo                    (struct play_info_s*             PlayInfo);
    HavanaStatus_t              GetDecodeBuffer                (buffer_handle_t*                DecodeBuffer,
                                                                unsigned char**                 Data,
                                                                unsigned int                    Format,
                                                                unsigned int                    DimensionCount,
                                                                unsigned int                    Dimensions[],
                                                                unsigned int*                   Index,
                                                                unsigned int*                   Stride);
    HavanaStatus_t              ReturnDecodeBuffer             (buffer_handle_t                 DecodeBuffer);
    HavanaStatus_t              GetDecodeBufferPoolStatus      (unsigned int* BuffersInPool, 
                                                                unsigned int* BuffersWithNonZeroReferenceCount);
    HavanaStatus_t              GetOutputWindow                (unsigned int*                   X,
                                                                unsigned int*                   Y,
                                                                unsigned int*                   Width,
                                                                unsigned int*                   Height);
    HavanaStatus_t              GetPlayerEnvironment           (PlayerPlayback_t*               PlayerPlayback,
                                                                PlayerStream_t*                 PlayerStream);
    stream_event_signal_callback        RegisterEventSignalCallback    (context_handle_t                Context,
                                                                        stream_event_signal_callback    EventSignalCallback);
};

#endif

