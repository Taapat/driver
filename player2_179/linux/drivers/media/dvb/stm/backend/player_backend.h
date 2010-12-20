/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : player_backend.h - player access points
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
31-Jan-07   Created                                         Julian

************************************************************************/

#ifndef H_PLAYER_BACKEND
#define H_PLAYER_BACKEND

#ifdef __cplusplus
extern "C" {
#endif
int BackendInit                (void);
int BackendDelete              (void);

int PlaybackCreate             (playback_handle_t      *playback);
int PlaybackTerminate          (playback_handle_t       playback);
int PlaybackAddDemux           (playback_handle_t       playback,
                                int                     demux_id,
                                demux_handle_t         *demux);
int PlaybackRemoveDemux        (playback_handle_t       playback,
                                demux_handle_t          demux);
int PlaybackAddStream          (playback_handle_t       playback,
                                char                   *media,
                                char                   *format,
                                char                   *encoding,
                                unsigned int            surface_id,
                                stream_handle_t        *stream);
int PlaybackRemoveStream       (playback_handle_t       playback,
                                stream_handle_t         stream);
int PlaybackSetSpeed           (playback_handle_t       playback,
                                int                     speed);
int PlaybackGetSpeed           (playback_handle_t       playback,
                                int                    *speed);
int PlaybackSetNativePlaybackTime      (playback_handle_t       playback,
                                        unsigned long long      nativeTime,
                                        unsigned long long      systemTime);
int PlaybackSetOption          (playback_handle_t       playback,
                                play_option_t           option,
                                unsigned int            value);
int PlaybackGetPlayerEnvironment (playback_handle_t     Playback,
                                  playback_handle_t*    playerplayback);

int DemuxInjectData            (demux_handle_t          demux,
                                const unsigned char    *data,
                                unsigned int            data_length);

int StreamInjectData           (stream_handle_t         stream,
                                const unsigned char*    data,
                                unsigned int            data_length);
int StreamInjectDataPacket     (stream_handle_t         stream,
                                const unsigned char*    data,
                                unsigned int            data_length,
#ifdef __TDT__
//FIXME
                                unsigned char           PresentationTimeValid,
#else
                                bool                    PresentationTimeValid,
#endif
                                unsigned long long      PresentationTime);
int StreamDiscontinuity        (stream_handle_t         stream,
                                discontinuity_t         discontinuity);
int StreamDrain                (stream_handle_t         stream,
                                unsigned int            discard);
int StreamEnable               (stream_handle_t         stream,
                                unsigned int            enable);
int StreamSetId                (stream_handle_t         stream,
                                unsigned int            demux_id,
                                unsigned int            id);
int StreamChannelSelect        (stream_handle_t         Stream,
                                channel_select_t        Channel);
int StreamSetOption            (stream_handle_t         stream,
                                play_option_t           option,
                                unsigned int            value);
int StreamGetOption            (stream_handle_t         stream,
                                play_option_t           option,
                                unsigned int*           value);
int StreamStep                 (stream_handle_t         stream);
int StreamSwitch               (stream_handle_t         stream,
                                char                   *format,
                                char                   *encoding);
int StreamSetAlarm             (stream_handle_t         stream,
                                unsigned long long      pts);
int StreamGetPlayInfo          (stream_handle_t         Stream,
                                struct play_info_s*     PlayInfo);
int StreamGetDecodeBuffer      (stream_handle_t         stream,
                                buffer_handle_t        *buffer,
                                unsigned char         **data,
                                unsigned int            Format,
                                unsigned int            DimensionCount,
                                unsigned int            Dimensions[],
                                unsigned int*           Index,
                                unsigned int*           Stride);
int StreamReturnDecodeBuffer   (stream_handle_t         Stream,
                                buffer_handle_t*        Buffer);
int StreamSetOutputWindow      (stream_handle_t         Stream,
                                unsigned int            X,
                                unsigned int            Y,
                                unsigned int            Width,
                                unsigned int            Height);
int StreamGetOutputWindow      (stream_handle_t         Stream,
                                unsigned int*           X,
                                unsigned int*           Y,
                                unsigned int*           Width,
                                unsigned int*           Height);
int StreamSetInputWindow       (stream_handle_t         Stream,
                                unsigned int            X,
                                unsigned int            Y,
                                unsigned int            Width,
                                unsigned int            Height);
int StreamSetPlayInterval      (stream_handle_t         Stream,
                                play_interval_t*        PlayInterval);
int StreamGetDecodeBufferPoolStatus      (stream_handle_t         stream,
                                          unsigned int*           BuffersInPool,
                                          unsigned int*           BuffersWithNonZeroReferenceCount);


stream_event_signal_callback    StreamRegisterEventSignalCallback      (stream_handle_t                 stream,
                                                                        context_handle_t                context,
                                                                        stream_event_signal_callback    callback);
int StreamGetPlayerEnvironment (stream_handle_t                 Stream,
                                playback_handle_t*              playerplayback,
                                stream_handle_t*                playerstream);
int DisplayCreate              (char*                           Media,
                                unsigned int                    SurfaceId);
#ifdef __TDT__
int isDisplayCreated           (char*           Media,
                                unsigned int    SurfaceId);
#endif
int DisplayDelete              (char*                           Media,
                                unsigned int                    SurfaceId);
int DisplaySynchronize         (char*                           Media,
                                unsigned int                    SurfaceId);
int ComponentGetAttribute      (component_handle_t              Component,
                                const char*                     Attribute,
                                union attribute_descriptor_u*   Value);
int ComponentSetAttribute      (component_handle_t              Component,
                                const char*                     Attribute,
                                union attribute_descriptor_u*   Value);
/*
int ComponentSetModuleParameters (component_handle_t            Component,
                                void*                           Data,
                                unsigned int                    Size);
*/
player_event_signal_callback    PlayerRegisterEventSignalCallback      (player_event_signal_callback    Callback);


int MixerGetInstance (int StreamId, component_handle_t* Classoid);
int ComponentSetModuleParameters (component_handle_t component, void *data, unsigned int size);

int MixerAllocSubStream   (component_handle_t Component, int *SubStreamId);
int MixerFreeSubStream    (component_handle_t Component, int SubStreamId);
int MixerSetupSubStream   (component_handle_t Component, int SubStreamId,
                           struct alsa_substream_descriptor *Descriptor);
int MixerPrepareSubStream (component_handle_t Component, int SubStreamId);
int MixerStartSubStream   (component_handle_t Component, int SubStreamId);
int MixerStopSubStream    (component_handle_t Component, int SubStreamId);


#ifdef __cplusplus
}
#endif

#endif
