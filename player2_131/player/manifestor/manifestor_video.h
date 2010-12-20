/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : manifestor_video.h
Author :           Julian

Definition of the manifestor video class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
18-Jan-07   Created                                         Julian

************************************************************************/

#ifndef H_MANIFESTOR_VIDEO
#define H_MANIFESTOR_VIDEO

#include "osdev_user.h"
#include "allocinline.h"
#include "player.h"
#include "manifestor_base.h"

#undef  MANIFESTOR_TAG
#define MANIFESTOR_TAG          "ManifestorVideo_c::"

#define MAX_DEQUEUE_BUFFERS             (MAXIMUM_NUMBER_OF_DECODE_BUFFERS+8)
#define PAIRED_MACROBLOCK(v)            (((v) + 0x1f) & 0xffffffe0)
#define INPUT_WINDOW_SCALE_FACTOR       16

typedef enum
{
    BufferStateAvailable,
    BufferStateQueued,
    BufferStateNotQueued
} BufferState_t;

typedef enum
{
    InitialFramePossible,
    InitialFrameRequested,
    InitialFrameQueued,
    InitialFrameNotQueued,
    InitialFrameNotPossible
} InitialFrameState_t;

struct StreamBuffer_s
{
    unsigned int                BufferIndex;
    unsigned int                QueueCount;
    bool                        EventPending;

    BufferState_t               BufferState;

    class Manifestor_Video_c*   Manifestor;
    class Buffer_c*             BufferClass;
    struct VideoOutputTiming_s* OutputTiming;

    unsigned long long          NativePlaybackTime;
    unsigned long long          TimeSlot;
    unsigned long long          TimeOnDisplay;
    unsigned long               Data;
};

/* Generic window structure - in pixels */
struct Window_s
{
    unsigned int     X;         /* X coordinate of top left corner */
    unsigned int     Y;         /* Y coordinate of top left corner */
    unsigned int     Width;     /* Width of window in pixels */
    unsigned int     Height;    /* Height of window in pixels */
};

/// Framework for implementing video manifestors.
class Manifestor_Video_c : public Manifestor_Base_c
{
private:

    /* Generic stream information*/
    struct VideoDisplayParameters_s     StreamDisplayParameters;
    /*struct PanScan_s                    StreamPanScan;*/

    /*
    Information on window and surface positions
    We keep 5 sets of information.  SurfaceDescriptor describes the physical dimensions
    of the surface we are drawing on.  SurfaceWindow describes the window onto the surface
    we wish to use.  InputWindow is the portion of the incoming decoded picture we are
    going to copy onto the output and OutputWindow is the region we wish to copy the
    picture onto.  InputCrop is the region of interest specified by the user.
    */
    struct Window_s                     SurfaceWindow;          /* Position of window on the display surface */

    ManifestorStatus_t  SetDisplayWindows      (VideoDisplayParameters_t*       VideoParameters);

protected:

    bool                                Visible;
    struct Window_s                     InputCrop;              /* User specified portion of interest in decode buffer */
    struct Window_s                     InputWindow;            /* Input window onto decode buffer */
    struct Window_s                     OutputWindow;           /* Destination window on surface */
    bool                                DisplayUpdatePending;

    /* Display Information */
    unsigned int                                DisplayAspectRatioPolicyValue;
    unsigned int                                DisplayFormatPolicyValue;
    struct VideoOutputSurfaceDescriptor_s       SurfaceDescriptor;

    /* Buffer information */
    struct StreamBuffer_s               StreamBuffer[MAXIMUM_NUMBER_OF_DECODE_BUFFERS];
    unsigned int                        BufferOnDisplay;
    unsigned long long                  PtsOnDisplay;
    unsigned int                        LastQueuedBuffer;
    OS_Mutex_t                          BufferLock;
    unsigned int                        QueuedBufferCount;
    unsigned int                        NotQueuedBufferCount;

    unsigned long long                  NextTimeSlot;
    unsigned long long                  TimeSlotOnDisplay;
    unsigned long long                  FrameCount;

    InitialFrameState_t                 InitialFrameState;

#if 0
NICK
    unsigned int                        BufferWidth;
    unsigned int                        BufferHeight;
    unsigned int                        BufferCount;
    unsigned int                        BufferSize;

    unsigned int                        LumaBufferSize;
    unsigned int                        ChromaBufferSize;
    unsigned int                        TotalBufferSize;
#endif

    /* Data shared with buffer release process */
    OS_Event_t                          BufferReleaseThreadTerminated;
    OS_Thread_t                         BufferReleaseThreadId;
    bool                                BufferReleaseThreadRunning;

    /* Data shared with display signal process */
    OS_Event_t                          DisplaySignalThreadTerminated;
    OS_Thread_t                         DisplaySignalThreadId;
    bool                                DisplaySignalThreadRunning;
    OS_Semaphore_t                      BufferDisplayed;
    OS_Semaphore_t                      InitialFrameDisplayed;

    /* Dequeued Buffer information */
    struct StreamBuffer_s*              DequeuedStreamBuffers[MAX_DEQUEUE_BUFFERS];
    unsigned int                        DequeueIn;
    unsigned int                        DequeueOut;

    struct PlayerEventRecord_s          DisplayEvent;
    PlayerEventMask_t                   DisplayEventRequested;


    virtual ManifestorStatus_t  QueueBuffer            (unsigned int                    BufferIndex,
                                                        struct ParsedFrameParameters_s* FrameParameters,
                                                        struct ParsedVideoParameters_s* VideoParameters,
                                                        struct VideoOutputTiming_s*     VideoOutputTiming,
                                                        struct BufferStructure_s*       BufferStructure) = 0;

    virtual ManifestorStatus_t  QueueInitialFrame      (unsigned int                    BufferIndex,
                                                        struct ParsedVideoParameters_s* VideoParameters,
                                                        struct BufferStructure_s*       BufferStructure) = 0;

    virtual ManifestorStatus_t  CheckInputDimensions   (unsigned int                    Width,
                                                        unsigned int                    Height) = 0;

public:
    /* relayfs to differentiate the manifestor instance */
    unsigned int                        RelayfsIndex; 

    /* Constructor / Destructor */
    Manifestor_Video_c                                  (void);
    ~Manifestor_Video_c                                 (void);

    /* Overrides for component base class functions */
    ManifestorStatus_t   Halt                           (void);
    ManifestorStatus_t   Reset                          (void);

    /* Manifestor class functions */
    ManifestorStatus_t   GetDecodeBufferPool            (class BufferPool_c**   Pool);
    ManifestorStatus_t   GetSurfaceParameters           (void**                 SurfaceParameters);
    ManifestorStatus_t   GetNextQueuedManifestationTime (unsigned long long*    Time);
    ManifestorStatus_t   InitialFrame                   (class Buffer_c*        Buffer);
    ManifestorStatus_t   QueueDecodeBuffer              (class Buffer_c*        Buffer);
    ManifestorStatus_t   GetNativeTimeOfCurrentlyManifestedFrame       (unsigned long long*     Pts);
    ManifestorStatus_t   GetFrameCount                  (unsigned long long*    FrameCount);

    unsigned int         GetBufferId                    (void);
    ManifestorStatus_t   SetOutputWindow                (unsigned int           X,
                                                         unsigned int           Y,
                                                         unsigned int           Width,
                                                         unsigned int           Height);
    ManifestorStatus_t   GetOutputWindow                (unsigned int*          X,
                                                         unsigned int*          Y,
                                                         unsigned int*          Width,
                                                         unsigned int*          Height);
    ManifestorStatus_t   SetInputWindow                 (unsigned int           X,
                                                         unsigned int           Y,
                                                         unsigned int           Width,
                                                         unsigned int           Height);

    /* these virtual functions are implemented by the device specific part of the video manifestor */
    virtual ManifestorStatus_t  OpenOutputSurface      (DeviceHandle_t          DisplayDevice,
                                                        unsigned int            SurfaceId,
                                                        unsigned int            OutputId) = 0;
    virtual ManifestorStatus_t  CloseOutputSurface     (void) = 0;
    virtual ManifestorStatus_t  Enable                 (void) = 0;
    virtual ManifestorStatus_t  Disable                (void) = 0;
    virtual ManifestorStatus_t  UpdateDisplayWindows   (void) = 0;

    void                BufferReleaseThread            (void);
    void                DisplaySignalThread            (void);

    // Methods to be supplied by this derived classes

    ManifestorStatus_t   FillOutBufferStructure( BufferStructure_t       *RequestedStructure );

};
#endif
