/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_wma.h
Author :           Adam

Definition of the stream specific codec implementation for wma audio in player 2


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created (from codec_mme_audio_mpeg.h)           Adam

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_WMA
#define H_CODEC_MME_AUDIO_WMA

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_audio.h"
#include "wma_audio.h"
#include "player_generic.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
#define SENDBUF_TRIGGER_TRANSFORM_COUNT         1
#define SENDBUF_DECODE_CONTEXT_COUNT            (SENDBUF_TRIGGER_TRANSFORM_COUNT+4)

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Codec_MmeAudioWma_c : public Codec_MmeAudio_c
{
//! Stuff for private thread to do MME_TRANFORM commands
private:
    /// The presence of the worker thread means we must be careful not to manipulate the pool
    /// while we are iterating over its members.
    OS_Mutex_t                  DecodeContextPoolMutex;

    bool                        WmaThreadRunning;
    OS_Thread_t                 WmaThreadId;
    OS_Event_t                  WmaThreadTerminated;
    BufferPool_t                WmaTransformContextPool;
    CodecBaseDecodeContext_t   *WmaTransformContext;
    Buffer_t                    WmaTransformContextBuffer;
    OS_Mutex_t                  WmaInputMutex;

    allocator_device_t          WmaCodedFrameMemoryDevice;
    void                       *WmaCodedFrameMemory[3];
    BufferPool_t                WmaCodedFramePool;
    BufferDataDescriptor_t     *WmaCodedFrameBufferDescriptor;
    BufferType_t                WmaCodedFrameBufferType;

    unsigned int               CurrentDecodeFrameIndex;

    ParsedFrameParameters_t    SavedParsedFrameParameters;
    PlayerSequenceNumber_t     SavedSequenceNumberStructure;

    bool			InPossibleMarkerStallState;
    unsigned long long		TimeOfEntryToPossibleMarkerStallState;
    int				StallStateSendBuffersCommandsIssued;
    int				StallStateTransformCommandsCompleted;

protected:

    // Data

    eAccDecoderId DecoderId;

    /// Event signalled whenever an MME_TRANSFORM command should, potentially, be issued.
    OS_Event_t IssueWmaTransformCommandEvent;
    int                         SendBuffersCommandsIssued;
    int                         SendBuffersCommandsCompleted;
    int                         TransformCommandsIssued;
    int                         TransformCommandsCompleted;

    unsigned long long          LastNormalizedPlaybackTime;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeAudioWma_c(                void );
    ~Codec_MmeAudioWma_c(               void );

    //
    // Overrides for component base class functions
    //

    CodecStatus_t   Halt(     void );
    CodecStatus_t   Reset(      void );

    void WmaThread();
    CodecStatus_t   FillOutDecodeContext( void );
    void            FinishedDecode( void );
    CodecStatus_t   GetCodedFrameBufferPool(    BufferPool_t             *Pool,
						unsigned int             *MaximumCodedFrameSize );

    void   CallbackFromMME(                     MME_Event_t               Event,
						MME_Command_t            *Command );
    CodecStatus_t   SendMMEDecodeCommand( void );

    CodecStatus_t   Input(                      Buffer_t          CodedBuffer );
    CodecStatus_t   CheckForMarkerFrameStall( 	void );

    CodecStatus_t   DiscardQueuedDecodes(       void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams,
								WmaAudioStreamParameters_t *StreamParams );
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );
    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void    *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void    *Parameters );

    //
    // Helper methods for the playback thread
    //

    CodecStatus_t   FillOutWmaTransformContext( void );
    CodecStatus_t   FillOutWmaTransformCommand( void );
    CodecStatus_t   SendWmaTransformCommand( void );
    void            FinishedWmaTransform( void );
    CodecStatus_t   AbortWmaTransformCommands( void );
    CodecStatus_t   AbortWmaSendBuffersCommands( void );
};
#endif
