/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : surface.h
Author :           Julian

Definition of the surface class for havana.


Date        Modification                                    Name
----        ------------                                    --------
19-Feb-07   Created                                         Julian

************************************************************************/

#ifndef MANIFESTOR_VIDEO_STMFB_H
#define MANIFESTOR_VIDEO_STMFB_H

#include "osinline.h"
#include <stmdisplay.h>
#include "allocinline.h"
#include "manifestor_video.h"

#undef  MANIFESTOR_TAG
#define MANIFESTOR_TAG          "ManifestorVideoStmfb_c::"

#define STMFB_BUFFER_HEADROOM   12              /* Number of buffers to wait after queue is full before we restart queueing buffers */

/// Video manifestor based on the stgfb core driver API.
class Manifestor_VideoStmfb_c : public Manifestor_Video_c
{
private:
    stm_display_device_t*       DisplayDevice;
    stm_display_plane_t*        Plane;
    stm_display_output_t*       Output;

    stm_plane_crop_t            srcRect;
    stm_plane_crop_t            dstRect;

    unsigned int                VideoBufferBase;

    stm_display_buffer_t        DisplayBuffer[MAXIMUM_NUMBER_OF_DECODE_BUFFERS];

#if defined (QUEUE_BUFFER_CAN_FAIL)
    OS_Semaphore_t              DisplayAvailable;
    bool                        DisplayAvailableValid;
    unsigned int                DisplayHeadroom;
    bool                        DisplayFlush;
    bool                        WaitingForHeadroom;
#endif

    int                         ClockRateAdjustment;

public:

    /* Constructor / Destructor */
    Manifestor_VideoStmfb_c                            (void);
    ~Manifestor_VideoStmfb_c                           (void);

    /* Overrides for component base class functions */
    ManifestorStatus_t   Halt                          (void);
    ManifestorStatus_t   Reset                         (void);

    /* Manifestor video class functions */
    ManifestorStatus_t  OpenOutputSurface              (DeviceHandle_t          DisplayDevice,
                                                        unsigned int            PlaneId,
                                                        unsigned int            OutputId);
    ManifestorStatus_t  CloseOutputSurface             (void);
    ManifestorStatus_t  CreateDecodeBuffers            (unsigned int            Count,
                                                        unsigned int            Width,
                                                        unsigned int            Height);
    ManifestorStatus_t  DestroyDecodeBuffers           (void);

    ManifestorStatus_t  QueueBuffer                    (unsigned int                    BufferIndex,
                                                        struct ParsedFrameParameters_s* FrameParameters,
                                                        struct ParsedVideoParameters_s* VideoParameters,
                                                        struct VideoOutputTiming_s*     VideoOutputTiming,
                                                        struct BufferStructure_s*       BufferStructure );
    ManifestorStatus_t  QueueInitialFrame              (unsigned int                    BufferIndex,
                                                        struct ParsedVideoParameters_s* VideoParameters,
                                                        struct BufferStructure_s*       BufferStructure);
    ManifestorStatus_t  QueueNullManifestation         (void);
    ManifestorStatus_t  FlushDisplayQueue              (void);

    ManifestorStatus_t  Enable                         (void);
    ManifestorStatus_t  Disable                        (void);
    ManifestorStatus_t  UpdateDisplayWindows           (void);
    ManifestorStatus_t  CheckInputDimensions           (unsigned int                    Width,
                                                        unsigned int                    Height);

    ManifestorStatus_t   SynchronizeOutput(             void );

    /* The following functions are public because they are accessed via C stub functions */
    void                DisplayCallback                (struct StreamBuffer_s*  Buffer,
                                                        TIME64                  VsyncTime);
    void                InitialFrameDisplayCallback    (struct StreamBuffer_s*  Buffer,
                                                        TIME64                  VsyncTime);
    void                DoneCallback                   (struct StreamBuffer_s*  Buffer,

                                                        TIME64                  VsyncTime);
#ifdef __TDT__
#ifdef UFS922
    int get_dei_fmd(char *page, char **start, off_t off, int count,int *eof);
    int set_dei_fmd(struct file *file, const char __user *buf, unsigned long count);

    int get_dei_mode(char *page, char **start, off_t off, int count,int *eof);
    int set_dei_mode(struct file *file, const char __user *buf, unsigned long count);

    int get_dei_ctrl(char *page, char **start, off_t off, int count,int *eof);
    int set_dei_ctrl(struct file *file, const char __user *buf, unsigned long count);
#endif
    int get_psi_brightness(char *page, char **start, off_t off, int count,int *eof);
    int set_psi_brightness(struct file *file, const char __user *buf, unsigned long count);

    int get_psi_saturation(char *page, char **start, off_t off, int count,int *eof);
    int set_psi_saturation(struct file *file, const char __user *buf, unsigned long count);

    int get_psi_contrast(char *page, char **start, off_t off, int count,int *eof);
    int set_psi_contrast(struct file *file, const char __user *buf, unsigned long count);

    int get_psi_tint(char *page, char **start, off_t off, int count,int *eof);
    int set_psi_tint(struct file *file, const char __user *buf, unsigned long count);
#endif
};

#endif
