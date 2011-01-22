/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : dvb_video.h - video device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-03   Created                                         Julian

************************************************************************/

#ifndef H_DVB_VIDEO
#define H_DVB_VIDEO

#include <dvbdev.h>

#define MAX_VIDEO_EVENT         8
struct VideoEvent_s
{
    struct video_event          Event[MAX_VIDEO_EVENT];         /*! Linux dvb event structure */
    unsigned int                Write;                          /*! Pointer to next event location to write by decoder*/
    unsigned int                Read;                           /*! Pointer to next event location to read by user */
    unsigned int                Overflow;                       /*! Flag to indicate events have been missed */
    wait_queue_head_t           WaitQueue;                      /*! Queue to wake up any waiters */
    struct mutex                Lock;                           /*! Protection for access to Read and Write pointers */
};

#define DVB_OPTION_VALUE_INVALID                0xffffffff

struct dvb_device*  VideoInit                  (struct DeviceContext_s*        Context);
int                 VideoIoctlStop             (struct DeviceContext_s*        Context,
                                                unsigned int                   Mode);
int                 VideoIoctlPlay             (struct DeviceContext_s*        Context);
int                 VideoIoctlSetId            (struct DeviceContext_s*        Context,
                                                int                            Id);
int                 VideoIoctlGetSize          (struct DeviceContext_s*        Context,
                                                video_size_t*                  Size);
int                 VideoIoctlSetSpeed         (struct DeviceContext_s*        Context,
                                                int                            Speed);
int                 VideoIoctlSetPlayInterval  (struct DeviceContext_s*        Context,
                                                video_play_interval_t*         PlayInterval);
int                 PlaybackInit               (struct DeviceContext_s*        Context);
int                 VideoSetOutputWindow       (struct DeviceContext_s*        Context,
                                                unsigned int                   Left,
                                                unsigned int                   Top,
                                                unsigned int                   Width,
                                                unsigned int                   Height);
int                 VideoSetInputWindow        (struct DeviceContext_s*        Context,
                                                unsigned int                   Left,
                                                unsigned int                   Top,
                                                unsigned int                   Width,
                                                unsigned int                   Height);
int                 VideoGetPixelAspectRatio   (struct DeviceContext_s*        Context,
                                                unsigned int*                  Numerator,
                                                unsigned int*                  Denominator);
#endif
