/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_wma.cpp
Author :           Adam

Implementation of the wma audio codec class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created (from codec_mme_audio_mpeg.cpp)         Adam

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Codec_MmeAudioWma_c
///
/// The WMA audio codec proxy.
///
/// It is difficult for the host processor to efficiently determine the WMA
/// frame boundaries (the stream must be huffman decoded before the frame
/// boundaries are apparent). For this reason the player makes no attempt
/// to discover the frame boundaries. Unfortunately this makes the WMA codec
/// proxy implementation particularly complex. For this class there is not a
/// one-to-one relationship between input buffers and output buffers requiring
/// us to operate the decoder with streaming input (MME_SEND_BUFFERS) extracting
/// frame based output whenever we believe the decoder to be capable of
/// providing data (MME_TRANSFORM).
///
/// To assist with this we introduce a thread whose job is to manage the
/// issue of MME_TRANSFORM commands. This thread is woken every time a
/// previous transform is complete and everytime an MME_SEND_BUFFERS command
/// is issued.
///
/// \todo We need to move more into the thread to eliminate the race conditions.
///

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_audio_wma.h"
#include "wma_properties.h"
#include "wma_audio.h"
#include "ksound.h"

//! AWTODO
#ifdef __KERNEL__
extern "C"{void flush_cache_all();};
#endif
//!

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
//#define NUM_SEND_BUFFERS_COMMANDS 4
#define MAX_ASF_PACKET_SIZE 16384

#define MAXIMUM_STALL_PERIOD	250000		// 1/4 second

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

typedef struct WmaAudioCodecStreamParameterContext_s
{
    CodecBaseStreamParameterContext_t   BaseContext;

    MME_LxAudioDecoderGlobalParams_t StreamParameters;
} WmaAudioCodecStreamParameterContext_t;

//#if __KERNEL__
#if 0
#define BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT         "WmaAudioCodecStreamParameterContext"
#define BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE    {BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromDeviceMemory, 32, 0, true, true, sizeof(WmaAudioCodecStreamParameterContext_t)}
#else
#define BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT         "WmaAudioCodecStreamParameterContext"
#define BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE    {BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(WmaAudioCodecStreamParameterContext_t)}
#endif

static BufferDataDescriptor_t            WmaAudioCodecStreamParameterContextDescriptor = BUFFER_WMA_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE;

// --------

typedef struct WmaAudioCodecDecodeContext_s
{
    CodecBaseDecodeContext_t            BaseContext;

    MME_LxAudioDecoderBufferParams_t    DecodeParameters;
    MME_LxAudioDecoderFrameStatus_t     DecodeStatus;
} WmaAudioCodecDecodeContext_t;

//#if __KERNEL__
#if 0
#define BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT   "WmaAudioCodecDecodeContext"
#define BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT_TYPE      {BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromDeviceMemory, 32, 0, true, true, sizeof(WmaAudioCodecDecodeContext_t)}
#else
#define BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT   "WmaAudioCodecDecodeContext"
#define BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT_TYPE      {BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(WmaAudioCodecDecodeContext_t)}
#endif

static BufferDataDescriptor_t            WmaAudioCodecDecodeContextDescriptor = BUFFER_WMA_AUDIO_CODEC_DECODE_CONTEXT_TYPE;

static OS_TaskEntry( WmaThreadStub )
{
    Codec_MmeAudioWma_c *Codec = ( Codec_MmeAudioWma_c * ) Parameter;

    Codec->WmaThread();
    OS_TerminateThread();
    return NULL;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// WMA decode thread.
///
/// \b WARNING: This method is public only to allow it to be called from a
///             C linkage callback. Do not call it directly.
///
/// This is basically an adaptation of Codec_MmeAudio_c::Input to dispatch
/// the TRANSFORM commands to match the SEND_BUFFERS done by the main code
/// that will be driven directly by the Codec_MmeAudio_c::Input.
/// Various items called by Codec_MmeAudio_c::Input are duplicated here to
/// be customised to the requirements or to detach them from member variables
/// that we have duplicated for this half of the process.
///
void Codec_MmeAudioWma_c::WmaThread()
{
    CodecStatus_t       Status;
    unsigned int        WmaDecodeContextSize;
    unsigned int        HighWatermarkOfDiscardDecodesUntil = 0;
    int                 BuffersAvailable        = 0;
    int                 TransformsActive        = 0;

    while( WmaThreadRunning )
    {
	OS_WaitForEvent( &IssueWmaTransformCommandEvent, 100);
	OS_ResetEvent( &IssueWmaTransformCommandEvent );

	OS_LockMutex( &WmaInputMutex );


	//
	// Check if we need to discard our current data.
	//

	if( DiscardDecodesUntil > HighWatermarkOfDiscardDecodesUntil )
	{
		CODEC_TRACE("Killing off pending MME commands (up to (approx.) %d)\n", DiscardDecodesUntil);

		HighWatermarkOfDiscardDecodesUntil = DiscardDecodesUntil;

		Status = AbortWmaTransformCommands();
		if( Status != CodecNoError )
		{
		     CODEC_ERROR("Could not abort all pending MME_TRANSFORM comamnds\n");
		     // no recovery possible
		}

		Status = AbortWmaSendBuffersCommands();
		if( Status != CodecNoError )
		{
		     CODEC_ERROR("Could not abort all pending MME_SEND_BUFFERS comamnds\n");
		     // no recovery possible
		}

		//
		// WMA transform gets confused over PTS generation when we abort send buffers
		// So we terminate and restart the mme transform, we also tidy up state to avoid
		// any negative repercusions of this act.
		//

		Status		= TerminateMMETransformer();
		if( Status == CodecNoError )
		    Status	= InitializeMMETransformer();

		if( Status != CodecNoError )
		{
		     CODEC_ERROR("Could not terminate and restart the WMA transformer\n");
		     // no recovery possible
		}

		ForceStreamParameterReload		= true;
		HighWatermarkOfDiscardDecodesUntil 	= 0;
		DiscardDecodesUntil			= 0;

		// We now expect a jump in the pts time, so we avoid any unsavoury errors, by invalidating our record
		LastNormalizedPlaybackTime	= UNSPECIFIED_TIME;
	}

	//
	// Conditionally issue a new MME_TRANSFORM command
	//

	BuffersAvailable        = SendBuffersCommandsIssued - SendBuffersCommandsCompleted;
	TransformsActive        = TransformCommandsIssued - TransformCommandsCompleted;
	while( ((BuffersAvailable - TransformsActive) >= SENDBUF_TRIGGER_TRANSFORM_COUNT) &&
	       (!TestComponentState(ComponentHalted)) &&
	       WmaCodedFramePool )
	{

	    if( WmaTransformContextBuffer != NULL )
	    {
		CODEC_ERROR("Already have a decode context.\n");
		break;
	    }

	    Status      = WmaTransformContextPool->GetBuffer( &WmaTransformContextBuffer );
	    if( Status != BufferNoError )
	    {
		CODEC_ERROR("Fail to get decode context.\n");
		break;
	    }

	    WmaTransformContextBuffer->ObtainDataReference( &WmaDecodeContextSize, NULL, (void **)&WmaTransformContext );
	    memset( WmaTransformContext, 0x00, WmaDecodeContextSize );

	    WmaTransformContext->DecodeContextBuffer  = WmaTransformContextBuffer;


	    Status = FillOutWmaTransformContext();
	    if( Status != CodecNoError )
	    {
		CODEC_ERROR("Cannot fill out WMA transform context\n");
		break;
	    }

	    Status = FillOutWmaTransformCommand();
	    if( Status != CodecNoError )
	    {
		CODEC_ERROR("Cannot fill out WMA transform command\n");
		break;
	    }

	    //
	    // Ensure that the coded frame will be available throughout the 
	    // life of the decode by attaching the coded frame to the decode
	    // context prior to launching the decode.
	    //

	    WmaTransformContextBuffer->AttachBuffer( CodedFrameBuffer );

	    Status = SendWmaTransformCommand();
	    if( Status != CodecNoError )
	    {
		CODEC_ERROR("Failed to send a decode command.\n" );
		ReleaseDecodeContext( WmaTransformContext );
		//! NULL them here, as ReleaseDecodeContext will only do for the MME_SEND_BUFFERS done with DecodeContext/etc
		WmaTransformContext       = NULL;
		WmaTransformContextBuffer = NULL;
		break;
	    }

	    FinishedWmaTransform();

	    BuffersAvailable        = SendBuffersCommandsIssued - SendBuffersCommandsCompleted;
	    TransformsActive        = TransformCommandsIssued - TransformCommandsCompleted;
	}

	//
	// Check for a marker frame stall
	//

	CheckForMarkerFrameStall();

//

	OS_UnLockMutex( &WmaInputMutex );

    } //while( WmaThreadRunning )

//

    //
    // About to terminate, make sure there are no pending commands.
    //

    Status = AbortWmaTransformCommands();
    if( Status != CodecNoError )
    {
	CODEC_ERROR("Could not abort all pending MME_TRANSFORM comamnds (resources will leak)\n");
	// no recovery possible
    }


    Status = AbortWmaSendBuffersCommands();
    if( Status != CodecNoError )
    {
	CODEC_ERROR("Could not abort all pending MME_SEND_BUFFERS comamnds (resources will leak)\n");
	// no recovery possible
    }

//

    CODEC_DEBUG( "Terminating playback thread\n" );
    OS_SetEvent( &WmaThreadTerminated );
}


// /////////////////////////////////////////////////////////////////////////
//
//      The get coded frame buffer pool fn
//

CodecStatus_t   Codec_MmeAudioWma_c::Input(     Buffer_t          CodedBuffer )
{
CodecStatus_t                   Status;
ParsedFrameParameters_t        *ParsedFrameParameters;
PlayerSequenceNumber_t         *SequenceNumberStructure;


    OS_LockMutex( &WmaInputMutex );

    //! get the coded frame params
    Status = CodedBuffer->ObtainMetaDataReference( Player->MetaDataParsedFrameParametersType, (void **)(&ParsedFrameParameters));
    if( Status != PlayerNoError )
    {
	CODEC_ERROR("No frame params on coded frame?!!\n");
	OS_UnLockMutex( &WmaInputMutex );
	return Status;
    }
    memcpy (&SavedParsedFrameParameters, ParsedFrameParameters, sizeof(ParsedFrameParameters_t));

    Status      = CodedBuffer->ObtainMetaDataReference( Player->MetaDataSequenceNumberType, (void **)(&SequenceNumberStructure) );
    if( Status != PlayerNoError )
    {
	CODEC_ERROR("Unable to obtain the meta data \"SequenceNumber\" - Implementation error\n" );
	OS_UnLockMutex( &WmaInputMutex );
	return Status;
    }
    memcpy(&SavedSequenceNumberStructure, SequenceNumberStructure, sizeof(PlayerSequenceNumber_t));


    //
    // Perform base operations
    //

    Status      = Codec_MmeAudio_c::Input( CodedBuffer );

    OS_UnLockMutex( &WmaInputMutex );

    if( Status != CodecNoError )
	return Status;

    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
//      When discarding queued decodes, poke the monitor task
//

CodecStatus_t   Codec_MmeAudioWma_c::DiscardQueuedDecodes( void )
{
CodecStatus_t	Status;

    OS_LockMutex( &WmaInputMutex );
    Status	= Codec_MmeBase_c::DiscardQueuedDecodes();
    OS_UnLockMutex( &WmaInputMutex );

    OS_SetEvent( &IssueWmaTransformCommandEvent );

    return Status;
}


////////////////////////////////////////////////////////////////////////////
///
/// Fill in the configuration parameters used by the super-class and reset everything.
///
Codec_MmeAudioWma_c::Codec_MmeAudioWma_c( void )
{
    Configuration.CodecName                             = "WMA audio";

    Configuration.CodedFrameCount                       = 256;
    Configuration.CodedMemorySize                       = 0x150000;
    Configuration.MaximumCodedFrameSize                 = 0x010000;

    Configuration.StreamParameterContextCount           = 1;
    Configuration.StreamParameterContextDescriptor      = &WmaAudioCodecStreamParameterContextDescriptor;

    Configuration.DecodeContextCount                    = SENDBUF_DECODE_CONTEXT_COUNT; //4;
    Configuration.DecodeContextDescriptor               = &WmaAudioCodecDecodeContextDescriptor;

//

    AudioDecoderTransformCapabilityMask.DecoderCapabilityFlags = (1 << ACC_WMAPROLSL);
//!
    WmaTransformContextPool                     = NULL;

    WmaCodedFrameMemory[CachedAddress]          = NULL;
    WmaCodedFrameMemory[UnCachedAddress]        = NULL;
    WmaCodedFrameMemory[PhysicalAddress]        = NULL;
    WmaCodedFramePool                           = NULL;

//!
    WmaThreadRunning                            = false;
    WmaThreadId                                 = OS_INVALID_THREAD;

//!
    DecoderId                                   = ACC_WMAPROLSL_ID;

    Reset();

//!

}


////////////////////////////////////////////////////////////////////////////
///
///     Destructor function, ensures a full halt and reset
///     are executed for all levels of the class.
///
Codec_MmeAudioWma_c::~Codec_MmeAudioWma_c( void )
{
    Halt();
    Reset();

}

////////////////////////////////////////////////////////////////////////////
///
/// The halt function release any resources, and reset all variables
///
///
CodecStatus_t   Codec_MmeAudioWma_c::Halt(     void )
{
    if( WmaThreadId != OS_INVALID_THREAD )
    {

	// notify the thread it should exit
	WmaThreadRunning        = false;

	// set any events the thread may be blocked waiting for
	OS_SetEvent( &IssueWmaTransformCommandEvent );

	// wait for the thread to come to rest
	OS_WaitForEvent( &WmaThreadTerminated, OS_INFINITE );

	// tidy up
	OS_TerminateEvent( &WmaThreadTerminated );
	OS_TerminateEvent( &IssueWmaTransformCommandEvent );
	OS_TerminateMutex( &DecodeContextPoolMutex );
	OS_TerminateMutex( &WmaInputMutex );
	WmaThreadId             = OS_INVALID_THREAD;
    }

    return Codec_MmeAudio_c::Halt ();

}

//!
////////////////////////////////////////////////////////////////////////////
///
/// The Reset function release any resources, and reset all variables
///
/// \todo This method is mismatched with the contructor; it frees events that it shouldn't
///
CodecStatus_t   Codec_MmeAudioWma_c::Reset(     void )
{
//!
    //
    // Release the decoded frame context buffer pool
    //

    if( WmaTransformContextPool != NULL )
    {
	BufferManager->DestroyPool( WmaTransformContextPool );
	WmaTransformContextPool  = NULL;
    }

    //
    // Release the coded frame buffer pool
    //

    if( WmaCodedFramePool != NULL )
    {
	BufferManager->DestroyPool( WmaCodedFramePool );
	WmaCodedFramePool  = NULL;
    }

    if( WmaCodedFrameMemory[CachedAddress] != NULL )
    {
#if __KERNEL__
	AllocatorClose( WmaCodedFrameMemoryDevice );
#endif

	WmaCodedFrameMemory[CachedAddress]      = NULL;
	WmaCodedFrameMemory[UnCachedAddress]    = NULL;
	WmaCodedFrameMemory[PhysicalAddress]    = NULL;
    }

    SendBuffersCommandsIssued                   = 0;
    SendBuffersCommandsCompleted                = 0;
    TransformCommandsIssued                     = 0;
    TransformCommandsCompleted                  = 0;

    LastNormalizedPlaybackTime = UNSPECIFIED_TIME;

    InPossibleMarkerStallState			= false;

//!
    return Codec_MmeAudio_c::Reset();
}

// /////////////////////////////////////////////////////////////////////////
//
//      The get coded frame buffer pool fn
//

CodecStatus_t   Codec_MmeAudioWma_c::GetCodedFrameBufferPool(
						BufferPool_t     *Pool,
						unsigned int     *MaximumCodedFrameSize )
{
    PlayerStatus_t          Status;

    Status = Codec_MmeBase_c::GetCodedFrameBufferPool(Pool,MaximumCodedFrameSize );
    if( Status != CodecNoError )
    {
	return Status;
    }

    // Create Wma thread and events and mutexes it uses
    // If WmaThreadId != OS_INVALID_THREAD all threads, events and mutexes are valid
    if (WmaThreadId == OS_INVALID_THREAD)
    {
	if( OS_InitializeMutex( &WmaInputMutex ) != OS_NO_ERROR )
	{
	    CODEC_ERROR( "Unable to create the WmaInputMutex\n");
	    return CodecError;
	}

	if( OS_InitializeMutex( &DecodeContextPoolMutex ) != OS_NO_ERROR )
	{
	    CODEC_ERROR( "Unable to create the DecodeContextPoolMutex\n");
	    OS_TerminateMutex ( &WmaInputMutex );
	    return CodecError;
	}

	if( OS_InitializeEvent( &IssueWmaTransformCommandEvent ) != OS_NO_ERROR )
	{
	    CODEC_ERROR( "Unable to create the SendBuffersCommandCompleted event\n" );
	    OS_TerminateMutex ( &DecodeContextPoolMutex );
	    OS_TerminateMutex ( &WmaInputMutex );
	    return CodecError;
	}

	if( OS_InitializeEvent( &WmaThreadTerminated ) != OS_NO_ERROR )
	{
	    CODEC_ERROR( "Unable to create the WMA thread terminated event\n" );
	    OS_TerminateEvent ( &IssueWmaTransformCommandEvent );
	    OS_TerminateMutex ( &DecodeContextPoolMutex );
	    OS_TerminateMutex ( &WmaInputMutex );
	    return CodecError;
	}

	WmaThreadRunning        = true;
	if( OS_CreateThread( &WmaThreadId, WmaThreadStub, this, "Player Wma Codec", OS_MID_PRIORITY+9 ) != OS_NO_ERROR )
	{
	    CODEC_ERROR( "Unable to create WMA playback thread\n" );
	    WmaThreadId         = OS_INVALID_THREAD;
	    WmaThreadRunning    = false;
	    OS_TerminateEvent( &WmaThreadTerminated );
	    OS_TerminateEvent( &IssueWmaTransformCommandEvent );
	    OS_TerminateMutex( &DecodeContextPoolMutex );
	    OS_TerminateMutex( &WmaInputMutex );
	    return CodecError;
	}
    }


    //
    // If we haven't already created the buffer pool, do it now.
    //

    if( WmaCodedFramePool == NULL )
    {
	//
	// Get the memory and Create the pool with it
	//

#if __KERNEL__
	Status = PartitionAllocatorOpen( &WmaCodedFrameMemoryDevice, Configuration.CodedMemoryPartitionName, //0x100000, true);
									Configuration.CodedMemorySize, true );
	if( Status != allocator_ok )
	{
	    report( severity_error, "Codec_MmeAudioWma_c::GetCodedFrameBufferPool(%s) - Failed to allocate memory\n", Configuration.CodecName );
	    return PlayerInsufficientMemory;
	}

	WmaCodedFrameMemory[CachedAddress]         = AllocatorUserAddress( WmaCodedFrameMemoryDevice );
	WmaCodedFrameMemory[UnCachedAddress]       = AllocatorUncachedUserAddress( WmaCodedFrameMemoryDevice );
	WmaCodedFrameMemory[PhysicalAddress]       = AllocatorPhysicalAddress( WmaCodedFrameMemoryDevice );
#else
	static unsigned char    WmaMemory[4*1024*1024];

	WmaCodedFrameMemory[CachedAddress]         = Memory;
	WmaCodedFrameMemory[UnCachedAddress]       = NULL;
	WmaCodedFrameMemory[PhysicalAddress]       = Memory;
	//Configuration.CodedMemorySize           = 4*1024*1024;
#endif

//

	Status  = BufferManager->CreatePool( &WmaCodedFramePool, CodedFrameBufferType,
				CODEC_MAX_DECODE_BUFFERS, Configuration.CodedMemorySize, WmaCodedFrameMemory );
				//Configuration.CodedFrameCount, Configuration.CodedMemorySize, WmaCodedFrameMemory );
	if( Status != BufferNoError )
	{
	    report( severity_error, "Codec_MmeAudioWma_c::GetCodedFrameBufferPool(%s) - Failed to create the pool.\n", Configuration.CodecName );
	    return PlayerInsufficientMemory;
	}

	Status      = WmaCodedFramePool->AttachMetaData( Player->MetaDataParsedFrameParametersType );
	if( Status != PlayerNoError )
	{
		report( severity_error, "Codec_MmeAudioWma_c::GetCodedFrameBufferPool(%s) - Failed to ParsedFrameParameters to the coded buffer pool.\n", Configuration.CodecName );
		return Status;
	}

	Status      = WmaCodedFramePool->AttachMetaData( Player->MetaDataSequenceNumberType );
	if( Status != PlayerNoError )
	{
		report( severity_error, "Codec_MmeAudioWma_c::GetCodedFrameBufferPool(%s) - Failed to attach sequence numbers to the coded buffer pool.\n", Configuration.CodecName );
		return Status;
	}
    }

//! AWTODO - create our own pool of decode contexts that are purely for MME_TRANSFORMS

    //
    // Now create the decode context buffers
    //

    Player->GetBufferManager( &BufferManager );

    if( WmaTransformContextPool == NULL ) {     //! I do this check because I noticed was getting called twice and want
						//! to be safe if not instantiated twice but is actually a cock-up

	Status      = BufferManager->CreatePool( &WmaTransformContextPool, DecodeContextType, 2 );
	if( Status != BufferNoError )
	{
		report( severity_error, "Codec_MmeAudioWma_c::Codec_MmeAudioWma_c(%s) - Failed to create a pool of decode context buffers.\n", Configuration.CodecName );
		return Status;
	}

	//Set this up here too - this will be stamped onto the ParsedFrameParameters of our WmaCodedFrames
	CurrentDecodeFrameIndex       = 0;
    }
//!

    WmaTransformContextBuffer         = NULL;

//!
    return CodecNoError;

}



////////////////////////////////////////////////////////////////////////////
///
/// Populate the supplied structure with parameters for WMA audio.
///
///
CodecStatus_t Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters( MME_LxAudioDecoderGlobalParams_t *GlobalParams_p,
								       WmaAudioStreamParameters_t *StreamParams )
{
    CODEC_TRACE("Initializing WMA layer audio decoder\n");

    MME_LxAudioDecoderGlobalParams_t &GlobalParams = *GlobalParams_p;
    GlobalParams.StructSize = sizeof(MME_LxAudioDecoderGlobalParams_t);
//    MME_LxWmaConfig_t &Config = *((MME_LxWmaConfig_t *) GlobalParams.DecConfig);
    MME_LxWmaProLslConfig_t &Config = *((MME_LxWmaProLslConfig_t *) GlobalParams.DecConfig);

//    MME_LxWmaProLslConfig_t &config = ((MME_LxWmaProLslConfig_t *) globalParams.DecConfig)[0];
    memset(&Config, 0, sizeof(Config));
    Config.DecoderId = ACC_WMAPROLSL_ID;
/*
Audio_DecoderTypes.h:   ACC_WMA9_ID,
Audio_DecoderTypes.h:   ACC_WMAPROLSL_ID,
Audio_DecoderTypes.h:   ACC_WMA_ST_FILE,
Audio_EncoderTypes.h:   ACC_WMAE_ID,
*/
    Config.StructSize = sizeof(Config);
#if 0
    config.MaxNbPages = NUM_SEND_BUFFERS_COMMANDS;
#else
    // In BL012_5 there are problems in the WMA decoder that can cause it to mismanage
    // the page arrays. Picking a 'large' number for MaxNbPages ensures that there are
    // a few unused page structures lying around which makes running of the end of the
    // page arrays slightly less likely
    Config.MaxNbPages = 16;
#endif
    Config.MaxPageSize = MAX_ASF_PACKET_SIZE; // default if no other is specified

  if(StreamParams != NULL) {
    Config.NbSamplesOut = StreamParams->SamplesPerFrame ? StreamParams->SamplesPerFrame : 2048;
	report( severity_info, "Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters - StreamParams->SamplesPerFrame %d\n",StreamParams->SamplesPerFrame );
	report( severity_info, "Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters - Config.NbSamplesOut %d\n",Config.NbSamplesOut );

    if (0 == StreamParams->StreamNumber) {
	// zero is an illegal stream number
	report( severity_error, "Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters - ILLEGAL STREAM NUMBER\n" );
	Config.NewAudioStreamInfo = ACC_MME_FALSE;
    } else {
	Config.NewAudioStreamInfo = ACC_MME_TRUE;

//      MME_WmaAudioStreamInfo_t &streamInfo = Config.AudioStreamInfo;
	MME_WmaProLslAudioStreamInfo_t &streamInfo = Config.AudioStreamInfo;
	streamInfo.nVersion            = (StreamParams->FormatTag == WMA_VERSION_2_9) ? 2 :     // WMA V2
					 (StreamParams->FormatTag == WMA_VERSION_9_PRO) ? 3 :   // WMA Pro
					 (StreamParams->FormatTag == WMA_LOSSLESS) ? 4 : 1;     // WMA lossless - Default to WMA version1?

	streamInfo.wFormatTag          = StreamParams->FormatTag;
	streamInfo.nSamplesPerSec      = StreamParams->SamplesPerSecond;
	streamInfo.nAvgBytesPerSec     = StreamParams->AverageNumberOfBytesPerSecond;
	streamInfo.nBlockAlign         = StreamParams->BlockAlignment;
	streamInfo.nChannels           = StreamParams->NumberOfChannels;
	streamInfo.nEncodeOpt          = StreamParams->EncodeOptions;
	streamInfo.nSamplesPerBlock    = StreamParams->SamplesPerBlock;
	report( severity_info, "Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters - streamInfo.nSamplesPerBlock %d\n",streamInfo.nSamplesPerBlock );
	streamInfo.dwChannelMask       = StreamParams->ChannelMask;
	streamInfo.nBitsPerSample      = StreamParams->BitsPerSample;
	streamInfo.wValidBitsPerSample = StreamParams->ValidBitsPerSample;
	streamInfo.wStreamId           = StreamParams->StreamNumber;
	report( severity_info, "INFO : streamInfo.nSamplesPerSec %d \n",streamInfo.nSamplesPerSec );

	// HACK: see ValidateCompletedCommand()
//      NumChannels = StreamParams.NumberOfChannels;
    }
  } else {
	report( severity_error, "Codec_MmeAudioWma_c::FillOutTransformerGlobalParameters - No Params\n" );
	//no params, no set-up
	Config.NewAudioStreamInfo = ACC_MME_FALSE;
	Config.StructSize = 2 * sizeof(U32) ; // only transmit the ID and StructSize (all other params are irrelevant)
  }
    // This is not what the ACC headers make it look like but this is what
    // the firmware actually expects (tightly packed against the config structure)
//    unsigned char *pcmParams = ((unsigned char *) &Config) + Config.StructSize;
//    FillPcmProcessingGlobalParams((void *) pcmParams);

	return Codec_MmeAudio_c::FillOutTransformerGlobalParameters( GlobalParams_p );

//  return CodecNoError;
}


////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's initialization parameters for Wma audio.
///
/// When this method completes Codec_MmeAudio_c::AudioDecoderInitializationParameters
/// will have been filled out with valid values sufficient to initialize an
/// WMA audio decoder 
///
CodecStatus_t   Codec_MmeAudioWma_c::FillOutTransformerInitializationParameters( void )
{
CodecStatus_t Status;
MME_LxAudioDecoderInitParams_t &Params = AudioDecoderInitializationParameters;

//

    MMEInitializationParameters.TransformerInitParamsSize = sizeof(Params);
    MMEInitializationParameters.TransformerInitParams_p = &Params;

//

    Status = Codec_MmeAudio_c::FillOutTransformerInitializationParameters();
    if (Status != CodecNoError)
	return Status;

//

    return FillOutTransformerGlobalParameters( &Params.GlobalParams, NULL );
}


////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's MME_SET_GLOBAL_TRANSFORMER_PARAMS parameters for Wma audio.
///
CodecStatus_t   Codec_MmeAudioWma_c::FillOutSetStreamParametersCommand( void )
{
CodecStatus_t Status;
WmaAudioStreamParameters_t *Parsed = (WmaAudioStreamParameters_t *)ParsedFrameParameters->StreamParameterStructure;
WmaAudioCodecStreamParameterContext_t   *Context = (WmaAudioCodecStreamParameterContext_t *)StreamParameterContext;



    //
    // Examine the parsed stream parameters and determine what type of codec to instanciate
    //

    DecoderId = ACC_WMA9_ID;

    //
    // Now fill out the actual structure
    //     

    memset( &(Context->StreamParameters), 0, sizeof(Context->StreamParameters) );
    Status = FillOutTransformerGlobalParameters( &(Context->StreamParameters), Parsed );
    if( Status != CodecNoError )
	return Status;

    //
    // Fillout the actual command
    //

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize        = 0;
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p          = NULL;
    Context->BaseContext.MMECommand.ParamSize                           = sizeof(Context->StreamParameters);
    Context->BaseContext.MMECommand.Param_p                             = (MME_GenericParams_t)(&Context->StreamParameters);

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's -=<< !! MME_SEND_BUFFERS !! >>=- parameters for WMA audio.
///1572864
CodecStatus_t   Codec_MmeAudioWma_c::FillOutDecodeCommand(       void )
{
WmaAudioCodecDecodeContext_t *Context = (WmaAudioCodecDecodeContext_t *)DecodeContext;

//! AWTODO - over-ride the MME Command type here
    DecodeContext->MMECommand.CmdCode                           = MME_SEND_BUFFERS;

    //
    // Initialize the frame parameters (we don't actually have much to say here)
    //

    memset( &Context->DecodeParameters, 0, sizeof(Context->DecodeParameters) );

    //
    // Zero the reply structure
    //

    memset( &Context->DecodeStatus, 0, sizeof(Context->DecodeStatus) );

    //
    // Fillout the actual command
    //

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize        = sizeof(Context->DecodeStatus);
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p          = (MME_GenericParams_t)(&Context->DecodeStatus);
    Context->BaseContext.MMECommand.ParamSize                           = sizeof(Context->DecodeParameters);
    Context->BaseContext.MMECommand.Param_p                             = (MME_GenericParams_t)(&Context->DecodeParameters);

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Validate the ACC status structure and squawk loudly if problems are found.
/// 
/// Dispite the squawking this method unconditionally returns success. This is
/// because the firmware will already have concealed the decode problems by
/// performing a soft mute.
///
/// \return CodecSuccess
///
CodecStatus_t   Codec_MmeAudioWma_c::ValidateDecodeContext( CodecBaseDecodeContext_t *Context )
{
WmaAudioCodecDecodeContext_t *DecodeContext = (WmaAudioCodecDecodeContext_t *) Context;
MME_LxAudioDecoderFrameStatus_t &Status = DecodeContext->DecodeStatus;
ParsedAudioParameters_t *AudioParameters;

    Buffer_t                              TheCurrentDecodeBuffer;
    ParsedFrameParameters_t *DecodedFrameParsedFrameParameters;
    PlayerStatus_t          Stat;


    CODEC_DEBUG(">><<\n");

    if (ENABLE_CODEC_DEBUG) {
	//DumpCommand(bufferIndex);
    }

    if (Status.DecStatus != 0) { //ACC_MPEG2_OK && Status.DecStatus != ACC_MPEG_MC_CRC_ERROR) {
	CODEC_ERROR("Codec_MmeAudioWma_c::ValidateDecodeContext - WMA audio decode error %d\n", Status.DecStatus);
	//DumpCommand(bufferIndex);
	// don't report an error to the higher levels (because the frame is muted)
    }

    // SYSFS
    AudioDecoderStatus = Status;

    //
    // Attach any codec derived metadata to the output buffer (or verify the
    // frame analysis if the frame analyser already filled everything in for
    // us).
    //

    AudioParameters = BufferState[DecodeContext->BaseContext.BufferIndex].ParsedAudioParameters;

    if(AudioParameters && AudioOutputSurface) {
	AudioParameters->Source.BitsPerSample = AudioOutputSurface->BitsPerSample;
	AudioParameters->Source.ChannelCount = AudioOutputSurface->ChannelCount;
	AudioParameters->Organisation = Status.AudioMode;
    } else {
	if (!AudioParameters) report (severity_info, "Codec_MmeAudioWma_c::ValidateDecodeContext - AudioParameters NULL\n");
	if (!AudioOutputSurface) report (severity_info, "Codec_MmeAudioWma_c::ValidateDecodeContext - AudioOutputSurface NULL\n");
	return CodecError;
    }

    // TODO: we should also validate the other audio parameters against the codec's reply

    //! This is probably the right time to synthesise a PTS

    TheCurrentDecodeBuffer = BufferState[DecodeContext->BaseContext.BufferIndex].Buffer; //= CurrentDecodeBuffer;
    Stat = TheCurrentDecodeBuffer->ObtainMetaDataReference( Player->MetaDataParsedFrameParametersReferenceType, (void **)(&DecodedFrameParsedFrameParameters) );

    if( Stat == PlayerNoError ) 
    {
        long long CalculatedDelta =
	    ((unsigned long long) Status.NbOutSamples * 1000000ull) /
	    ((unsigned long long) ConvertCodecSamplingFreq(Status.SamplingFreq));

	//
	// post-decode the DTS isn't interesting (especially given we had to fiddle with it in the
	// frame parser) so we discard it entirely rather then extracting it from the codec's reply.
	//

	DecodedFrameParsedFrameParameters->NativeDecodeTime = INVALID_TIME;
	DecodedFrameParsedFrameParameters->NormalizedDecodeTime = INVALID_TIME;

	//
	// suck out the PTS from the codec's reply
	//

        if (ACC_isPTS_PRESENT(Status.PTSflag.Bits.PTS_DTS_FLAG))
        {
            unsigned long long Temp = (unsigned long long)Status.PTS + (unsigned long long)(((unsigned long long)Status.PTSflag.Bits.PTS_Bit32) << 32);
            DecodedFrameParsedFrameParameters->NativePlaybackTime = Temp;
            DecodedFrameParsedFrameParameters->NormalizedPlaybackTime = ((Temp*1000000)/90000);
        }
        else if (LastNormalizedPlaybackTime != UNSPECIFIED_TIME )
        {
            // synthesise a PTS
            unsigned long long Temp = LastNormalizedPlaybackTime + CalculatedDelta;
            DecodedFrameParsedFrameParameters->NativePlaybackTime = (Temp * 90000)/100000;
            DecodedFrameParsedFrameParameters->NormalizedPlaybackTime = Temp;
        }

        // Squawk if time does not progress quite as expected.
        if ( LastNormalizedPlaybackTime != UNSPECIFIED_TIME )
        {
            long long RealDelta = DecodedFrameParsedFrameParameters->NormalizedPlaybackTime -
            LastNormalizedPlaybackTime;
            long long DeltaDelta = RealDelta - CalculatedDelta;
            long long PtsJitterTollerenceThreshold = 100;
            
            // Check that the predicted and actual times deviate by no more than the threshold
            if (DeltaDelta < -PtsJitterTollerenceThreshold || DeltaDelta > PtsJitterTollerenceThreshold) 
            {
                CODEC_ERROR( "Unexpected change in playback time. Expected %lldus, got %lldus (deltas: exp. %lld  got %lld )\n",
                             LastNormalizedPlaybackTime + CalculatedDelta,
                             DecodedFrameParsedFrameParameters->NormalizedPlaybackTime,
                             CalculatedDelta, RealDelta);
                
            }     
        }

        LastNormalizedPlaybackTime = DecodedFrameParsedFrameParameters->NormalizedPlaybackTime;
        
    } else {
        report( severity_info, "VALIDATE: ObtainMetaDataReference failed!\n");
    }

    return CodecNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the set stream 
//      parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioWma_c::DumpSetStreamParameters(           void    *Parameters )
{
    CODEC_ERROR("Not implemented\n");  
    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the decode
//      parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioWma_c::DumpDecodeParameters(              void    *Parameters )
{
    CODEC_ERROR("Not implemented\n");
    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Fill out class contextual variable required to issue an MME_TRANSFORM command.
///
/// This is the corollary of Codec_MmeAudio_c::FillOutDecodeContext although it has significantly more code
/// in order to deal with the WMA buffer pool.
/// 
CodecStatus_t   Codec_MmeAudioWma_c::FillOutWmaTransformContext( void )
{
CodecStatus_t Status;
CodecBufferState_t             *State;
PlayerSequenceNumber_t         *SequenceNumberStructure;

//

    //
    // Obtain a new buffer if needed
    //

    if( CurrentDecodeBufferIndex == INVALID_INDEX )
    {
	Status = WmaCodedFramePool->GetBuffer( &CodedFrameBuffer, UNSPECIFIED_OWNER, 4, false );
	if( Status != BufferNoError )
	{
		CODEC_ERROR("Failed to get a wma coded buffer.\n");
		//CurrentDecodeBuffer->DecrementReferenceCount();
		ReleaseDecodeContext( WmaTransformContext );
		return Status;
	}

	// Set up base parsed frame params pointer to point at our coded frame buffer
	Status = CodedFrameBuffer->ObtainMetaDataReference( Player->MetaDataParsedFrameParametersType, (void **)(&ParsedFrameParameters));
	if( Status != PlayerNoError )
	{
	    CODEC_ERROR("Failed to get ParsedFrameParameters on new coded frame\n");
	    ReleaseDecodeContext( WmaTransformContext );
	    return Status;
	}
	memcpy (ParsedFrameParameters, &SavedParsedFrameParameters, sizeof(ParsedFrameParameters_t));

	Status      = GetDecodeBuffer();
	CodedFrameBuffer->DecrementReferenceCount();            // Now attached to decode buffer or failed
	if( Status != CodecNoError )
	{
	    CODEC_ERROR("Failed to get decode buffer.\n");
	    ReleaseDecodeContext( WmaTransformContext );
	    return Status;
	}

	//! attach coded frame (with copied params) to our decode buffer
	Status = WmaTransformContextBuffer->AttachBuffer( CodedFrameBuffer );
	if( Status != PlayerNoError )
	{
	    CODEC_ERROR("Failed to attach new coded frame to decode buffer\n");
	    ReleaseDecodeContext( WmaTransformContext );
	    return Status;
	}

	//CODEC_TRACE("WmaThread - O/P frame %d\n",CurrentDecodeFrameIndex);
	ParsedFrameParameters->DisplayFrameIndex = CurrentDecodeFrameIndex++;

//        //!Now attach to CurrentDecodeBuffer (replacing one attached in GetDecodeBuffer() that was acquired from CodedFrameBuffer in Input()
//
//        Status = CurrentDecodeBuffer->DetachMetaData( Player->MetaDataParsedFrameParametersReferenceType );
//        if( Status != PlayerNoError )
//        {
//            CODEC_ERROR("FAILED DEttach ref to \"NewParsedFrameParameters\" from decode buffer.\n" );
//        }
//        Status = CurrentDecodeBuffer->AttachMetaData( Player->MetaDataParsedFrameParametersReferenceType, UNSPECIFIED_SIZE, (void *)NewParsedFrameParameters );
//        if( Status != PlayerNoError )
//        {
//            CODEC_ERROR("FAILED attach ref to \"NewParsedFrameParameters\" to decode buffer.\n" );
//        }

	//managed to fix the buffer pool attach problem, so will want to copy the sequence structure across here
	Status      = CodedFrameBuffer->ObtainMetaDataReference( Player->MetaDataSequenceNumberType, (void **)(&SequenceNumberStructure) );
	if( Status != PlayerNoError )
	{
	    CODEC_ERROR("Unable to obtain the meta data \"SequenceNumber\" - Implementation    error\n" );
	    ReleaseDecodeContext( WmaTransformContext );
	    return Status;
	}
	//! copy across
	memcpy(SequenceNumberStructure, &SavedSequenceNumberStructure, sizeof(PlayerSequenceNumber_t));

	//!DO WE NEED TO FABRICATE SOME VALUES HERE?

//        //not sure how, but we seem to have an extra reference here
//        //perhaps decrementing here may reveal what still has an attachment when it tries
//        //to decrement it from 0 refs!
//        NewCodedFrameBuffer->DecrementReferenceCount();
//
//
//        //! I guess we should do same for link to CodedFrameBuffer - replace with NewCodedFrameBuffer
//        CurrentDecodeBuffer->DetachBuffer( CodedFrameBuffer );
//        CurrentDecodeBuffer->AttachBuffer( NewCodedFrameBuffer );
    }
    else
    {
	CODEC_ERROR("already have valid CurrentDecodeBufferIndex\n");
	ReleaseDecodeContext( WmaTransformContext );
	return CodecError;
    }

//

    //
    // We're now back as the corollary of FillOutDecodeContext (the code above is the buffer pool management)
    //

    //
    // Record the buffer being used in the decode context
    //

    WmaTransformContext->BufferIndex   = CurrentDecodeBufferIndex;

    //
    // Provide default values for the input and output buffers (the sub-class can change this if it wants to).
    //
    // Happily the audio firmware is less mad than the video firmware on the subject of buffers.
    //

    memset( &WmaTransformContext->MMECommand, 0x00, sizeof(MME_Command_t) );

    WmaTransformContext->MMECommand.NumberInputBuffers = 0;
    WmaTransformContext->MMECommand.NumberOutputBuffers = 1;
    WmaTransformContext->MMECommand.DataBuffers_p = WmaTransformContext->MMEBufferList;

    // plumbing
    WmaTransformContext->MMEBufferList[0] = &WmaTransformContext->MMEBuffers[0];

    memset( &WmaTransformContext->MMEBuffers[0], 0, sizeof(MME_DataBuffer_t) );
    WmaTransformContext->MMEBuffers[0].StructSize = sizeof(MME_DataBuffer_t);
    WmaTransformContext->MMEBuffers[0].NumberOfScatterPages = 1;
    WmaTransformContext->MMEBuffers[0].ScatterPages_p = &WmaTransformContext->MMEPages[0];

    memset( &WmaTransformContext->MMEPages[0], 0, sizeof(MME_ScatterPage_t) );

     // output
    State   = &BufferState[CurrentDecodeBufferIndex];
    if( State->BufferStructure->ComponentCount != 1 )
	CODEC_ERROR("Decode buffer structure contains unsupported number of components (%d).\n", State->BufferStructure->ComponentCount );

     WmaTransformContext->MMEBuffers[0].TotalSize       = State->BufferLength - State->BufferStructure->ComponentOffset[0];
     WmaTransformContext->MMEPages[0].Page_p            = State->BufferPointer + State->BufferStructure->ComponentOffset[0];
     WmaTransformContext->MMEPages[0].Size              = State->BufferLength - State->BufferStructure->ComponentOffset[0];

//! AWTODO - this stuff gets picked up on in ReleaseDecodeContext
//! We do it for MME_TRANSFORM but not MME_SEND_BUFFERS, basically
    OS_LockMutex( &Lock );

    WmaTransformContext->DecodeInProgress               = true;
    BufferState[CurrentDecodeBufferIndex].DecodesInProgress++;
    BufferState[CurrentDecodeBufferIndex].OutputOnDecodesComplete   = true;

    OS_UnLockMutex( &Lock );

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Populate an MME_TRANSFORM command.
///
/// This is the corollary of Codec_MmeAudioWma_c::FillOutDecodeCommand .
/// 
CodecStatus_t   Codec_MmeAudioWma_c::FillOutWmaTransformCommand( void )
{
WmaAudioCodecDecodeContext_t        *Context = (WmaAudioCodecDecodeContext_t *)WmaTransformContext;

    //
    // Initialize the frame parameters (we don't actually have much to say here)
    //

    memset( &Context->DecodeParameters, 0, sizeof(Context->DecodeParameters) );

    //
    // Zero the reply structure
    //

    memset( &Context->DecodeStatus, 0, sizeof(Context->DecodeStatus) );

    //
    // Fillout the actual command
    //

    Context->BaseContext.MMECommand.StructSize = sizeof(MME_Command_t);
    Context->BaseContext.MMECommand.CmdCode    = MME_TRANSFORM;
    Context->BaseContext.MMECommand.CmdEnd     = MME_COMMAND_END_RETURN_NOTIFY;
    Context->BaseContext.MMECommand.DueTime    = 0;           // No Idea what to put here, I just want it done in sequence.

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize    = sizeof(Context->DecodeStatus);
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p      = (MME_GenericParams_t)(&Context->DecodeStatus);
    Context->BaseContext.MMECommand.ParamSize                       = sizeof(Context->DecodeParameters);
    Context->BaseContext.MMECommand.Param_p                         = (MME_GenericParams_t)(&Context->DecodeParameters);

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Issue an MME_TRANSFORM command.
///
/// This is the corollary of Codec_MmeBase_c::SendMMEDecodeCommand from
/// which a substantial quantity of code has been copied.
///
/// \todo Verify the correct action if the component is halted
/// 
CodecStatus_t   Codec_MmeAudioWma_c::SendWmaTransformCommand( void )
{
CodecStatus_t Status;

//

    //
    // Check that we have not commenced shutdown.
    //

    MMECommandPreparedCount++;

    if( TestComponentState(ComponentHalted) )
    {
	CODEC_ERROR("Attempting to send WMA transform command when compontent is halted\n");
	MMECommandAbortedCount++;
	// XXX: This code was refactored from Codec_MmeAudioWma_c::WmaThread(), it used to cause the
	//      imediate death of the thread (instant return), this felt wrong so instead report success and
	//      rely on other components to examine the component state. Once this path has been tested
	//      the CODEC_ERROR() can probably be removed.
	return CodecNoError;
    }

    //
    // Do we wish to dump the mme command
    //

#ifdef DUMP_COMMANDS
    DumpMMECommand( &WmaTransformContext->MMECommand );
#endif

    //
    // Perform the mme transaction - Note at this point we invalidate
    // our pointer to the context buffer, after the send we invalidate
    // out pointer to the context data.
    //

#ifdef __KERNEL__
    flush_cache_all();
#endif

    WmaTransformContextBuffer = NULL;
    WmaTransformContext->DecodeCommenceTime    = OS_GetTimeInMicroSeconds();

    Status = MME_SendCommand( MMEHandle, &WmaTransformContext->MMECommand );
    if( Status != MME_SUCCESS )
    {
	report( severity_error, "Codec_MmeAudioWma_c::WmaThread - Unable to send decode command (%08x).\n", Status );
	Status = CodecError; //return
    }
    else
    {
	WmaTransformContext       = NULL;
	Status = CodecNoError;
	TransformCommandsIssued++;
    }

    return Status;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Tidy up the contextual variables after issuing an MME_TRANSFORM command.
///
/// This is the corollary of Codec_MmeAudio_c::FinishedDecode .
/// 
void Codec_MmeAudioWma_c::FinishedWmaTransform( void )
{
    //
    // We have finished decoding into this buffer
    //

    CurrentDecodeBufferIndex        = INVALID_INDEX;
    CurrentDecodeIndex              = INVALID_INDEX;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Abort any pending MME_TRANSFORM commands.
/// 
CodecStatus_t Codec_MmeAudioWma_c::AbortWmaTransformCommands( void )
{
Buffer_t                         ArrayOfBuffers[2] = { 0, 0 };
unsigned int                     WmaDecodeContextSize;
CodecBaseDecodeContext_t        *HaltWmaDecodeContext1 = 0;
CodecBaseDecodeContext_t        *HaltWmaDecodeContext2 = 0;

//

    //
    // Scan the local decode context pool to cancel MME_TRANSFORM commands
    //

    if( WmaTransformContextPool == NULL )
    {
	CODEC_TRACE("No WMA decode context pool - no commands to abort\n");
	return CodecNoError;
    }

    WmaTransformContextPool->GetAllUsedBuffers(2,ArrayOfBuffers,0);

    if(ArrayOfBuffers[0]) {
	ArrayOfBuffers[0]->ObtainDataReference( &WmaDecodeContextSize, NULL, (void **)&HaltWmaDecodeContext1 );
    }

    if(ArrayOfBuffers[1]) {
	ArrayOfBuffers[1]->ObtainDataReference( &WmaDecodeContextSize, NULL, (void **)&HaltWmaDecodeContext2 );
    }

    //check if we should swap order of inspection
    if(HaltWmaDecodeContext1 && HaltWmaDecodeContext2) {
	if(HaltWmaDecodeContext1->MMECommand.CmdStatus.CmdId <
	   HaltWmaDecodeContext2->MMECommand.CmdStatus.CmdId) {
	    CodecBaseDecodeContext_t *Tmp = HaltWmaDecodeContext1;
	    HaltWmaDecodeContext1 = HaltWmaDecodeContext2;
	    HaltWmaDecodeContext2 = Tmp;
	}
    }

    //now complete/cancel them, newest first
    if(HaltWmaDecodeContext1) {
	int delay = 20;
	CODEC_DEBUG("waiting for MME_TRANSFORM to complete\n" );
	while(delay-- && (HaltWmaDecodeContext1->MMECommand.CmdStatus.State < MME_COMMAND_COMPLETED))
	{
	    OS_SleepMilliSeconds(10);
	}
	while(HaltWmaDecodeContext1->MMECommand.CmdStatus.State < MME_COMMAND_COMPLETED)
	{
	    CODEC_DEBUG("waiting for MME_TRANSFORM to abort\n" );
	    MME_AbortCommand(MMEHandle, HaltWmaDecodeContext1->MMECommand.CmdStatus.CmdId );
	    OS_SleepMilliSeconds(10);
	}
    }

    if(HaltWmaDecodeContext2) {
	int delay = 20;
	CODEC_DEBUG("waiting for MME_TRANSFORM to complete\n" );
	while(delay-- && (HaltWmaDecodeContext2->MMECommand.CmdStatus.State < MME_COMMAND_COMPLETED))
	{
	    OS_SleepMilliSeconds(10);
	}

	while(HaltWmaDecodeContext2->MMECommand.CmdStatus.State < MME_COMMAND_COMPLETED)
	{
	    CODEC_DEBUG("waiting for MME_TRANSFORM to abort\n");
	    MME_AbortCommand(MMEHandle, HaltWmaDecodeContext2->MMECommand.CmdStatus.CmdId );
	    OS_SleepMilliSeconds(10);
	}
    }


//

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
/// 
/// Abort any pending MME_SEND_BUFFERS commands.
/// 
CodecStatus_t Codec_MmeAudioWma_c::AbortWmaSendBuffersCommands( void )
{
BufferStatus_t            Status;
Buffer_t                  AllocatedDecodeBuffers[SENDBUF_DECODE_CONTEXT_COUNT] = { 0 };
CodecBaseDecodeContext_t *OutstandingDecodeCommandContexts[SENDBUF_DECODE_CONTEXT_COUNT] = { 0 };
unsigned int              NumOutstandingCommands = 0;
unsigned long long        TimeOut;

//

    OS_AutoLockMutex mutex( &DecodeContextPoolMutex );

    if( DecodeContextPool == NULL )
    {
	CODEC_TRACE("No decode context pool - no commands to abort\n");
	return CodecNoError;
    }

    //
    // Scan the decode context pool to cancel MME_SEND_BUFFERS commands
    //

    Status = DecodeContextPool->GetAllUsedBuffers( SENDBUF_DECODE_CONTEXT_COUNT, AllocatedDecodeBuffers, 0 );
    if( BufferNoError != Status )
    {
	CODEC_ERROR( "Could not get handles for in-use buffers\n" );
	return CodecError;
    }

    for( int i = 0; i < SENDBUF_DECODE_CONTEXT_COUNT; i++)
    {
	if( AllocatedDecodeBuffers[i] )
	{
	    Status = AllocatedDecodeBuffers[i]->ObtainDataReference(
			NULL, NULL, (void **)&OutstandingDecodeCommandContexts[i] );
	    if( BufferNoError != Status )
	    {
		CODEC_ERROR( "Could not get data reference for in-use buffer at %p\n",
			     AllocatedDecodeBuffers[i] );
		return CodecError;
	    }

	    CODEC_DEBUG( "Selected command %08x\n",
			 OutstandingDecodeCommandContexts[i]->MMECommand.CmdStatus.CmdId );
	    NumOutstandingCommands++;
	}
    }

    CODEC_DEBUG( "Waiting for %d MME_SEND_BUFFERS commands to abort\n", NumOutstandingCommands );

    TimeOut = OS_GetTimeInMicroSeconds() + 5000000;

    while( NumOutstandingCommands && OS_GetTimeInMicroSeconds() < TimeOut )
    {
	//
	// Scan for any completed commands
	//

	for( unsigned int i = 0; i < NumOutstandingCommands; /* no iterator */ )
	{
	    MME_Command_t &Command = OutstandingDecodeCommandContexts[i]->MMECommand;

	    CODEC_DEBUG( "Command %08x  State %d\n", Command.CmdStatus.CmdId, Command.CmdStatus.State );

	    // It might, perhaps, looks a little odd to check for a zero command identifier here. Basically
	    // the callback action calls ReleaseDecodeContext() which will zero the structures. We really
	    // ought to use a better techinque to track in-flight commands (and possible move the call to
	    // ReleaseDecodeContext() into the WMA playback thread.
	    if( 0 == Command.CmdStatus.CmdId ||
		MME_COMMAND_COMPLETED == Command.CmdStatus.State ||
		MME_COMMAND_FAILED == Command.CmdStatus.State )
	    {
		CODEC_DEBUG( "Retiring command %08x\n", Command.CmdStatus.CmdId );
		OutstandingDecodeCommandContexts[i] =
				OutstandingDecodeCommandContexts[NumOutstandingCommands - 1];
		NumOutstandingCommands--;
	    }
	    else
	    {
		i++;
	    }
	}

	//
	// Issue the aborts to the co-processor
	//

	for( unsigned int i = 0; i < NumOutstandingCommands; i++ )
	{
	    MME_Command_t &Command = OutstandingDecodeCommandContexts[i]->MMECommand;

	    CODEC_DEBUG( "Aborting command %08x\n", Command.CmdStatus.CmdId );

	    MME_ERROR Error = MME_AbortCommand(MMEHandle, Command.CmdStatus.CmdId );
	    if( MME_SUCCESS != Error )
	    {
		if( MME_INVALID_ARGUMENT &&
		    ( Command.CmdStatus.State == MME_COMMAND_COMPLETED ||
		      Command.CmdStatus.State == MME_COMMAND_FAILED ) )
		    CODEC_TRACE( "Ignored error during abort (command %08x already complete)\n",
				 Command.CmdStatus.CmdId );
		else
		    CODEC_ERROR( "Cannot issue abort on command %08x (%d)\n",
				 Command.CmdStatus.CmdId, Error );
	    }
	}

	//
	// Allow a little time for the co-processor to react
	//

	OS_SleepMilliSeconds(10);
    }

//

    if( NumOutstandingCommands )
    {
	CODEC_ERROR( "Timed out waiting for %d MME_SEND_BUFFERS commands to abort\n",
		     NumOutstandingCommands );
	return CodecError;
    }

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// OVER-RIDE THE SUPERCLASS VERSION TO SUIT MME_SEND_BUFFERS!
///
/// Populate the DecodeContext structure with parameters for MPEG audio.
/// This can be over-ridden by WMA to not have any output buffer, as it will
/// do an MME_SEND_BUFFERS instead of MME_TRANSFORM
///
CodecStatus_t Codec_MmeAudioWma_c::FillOutDecodeContext( void )
{
    //
    // Provide default values for the input and output buffers (the sub-class can change this if it wants to).
    //
    // Happily the audio firmware is less mad than the video firmware on the subject of buffers.
    //

    memset( &DecodeContext->MMECommand, 0x00, sizeof(MME_Command_t) );

    DecodeContext->MMECommand.NumberInputBuffers = 1;
    DecodeContext->MMECommand.NumberOutputBuffers = 0;
    DecodeContext->MMECommand.DataBuffers_p = DecodeContext->MMEBufferList;

    // plumbing
    DecodeContext->MMEBufferList[0] = &DecodeContext->MMEBuffers[0];

    memset( &DecodeContext->MMEBuffers[0], 0, sizeof(MME_DataBuffer_t) );
    DecodeContext->MMEBuffers[0].StructSize = sizeof(MME_DataBuffer_t);
    DecodeContext->MMEBuffers[0].NumberOfScatterPages = 1;
    DecodeContext->MMEBuffers[0].ScatterPages_p = &DecodeContext->MMEPages[0];

    memset( &DecodeContext->MMEPages[0], 0, sizeof(MME_ScatterPage_t) );

    // input
    DecodeContext->MMEBuffers[0].TotalSize = CodedDataLength;
    DecodeContext->MMEPages[0].Page_p = CodedData;
    DecodeContext->MMEPages[0].Size  = CodedDataLength;

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Clear up - to be over-ridden by WMA codec to do nothing, as actual decode done elsewhere
///
void Codec_MmeAudioWma_c::FinishedDecode( void )
{
    //
    // We have NOT finished decoding into this buffer
    // This is to replace the cleanup after an MME_TRANSFORM from superclass Input function
    // But as we over-ride this with an MME_SEND_BUFFERS, and do the transform from a seperate thread
    // we don't want to clean up anything here
    //

    return;
}

// /////////////////////////////////////////////////////////////////////////
//
//      Function to send a set stream parameters
//

CodecStatus_t   Codec_MmeAudioWma_c::SendMMEDecodeCommand(  void )
{
CodecStatus_t Status;

//

    OS_AutoLockMutex mutex( &DecodeContextPoolMutex );

//! This will only occur with WMA/OGG that over-ride FillOutDecodeContext/FinishedDecode to split the input/output 1-1
//! relationship and have their own thread to wait on event, check count and maybe fire off MME_TRANSFORM commands
    if(DecodeContext->MMECommand.CmdCode == MME_SEND_BUFFERS) 
    {
        // pass the PTS to the firmware...
        WmaAudioCodecDecodeContext_t *Context = (WmaAudioCodecDecodeContext_t *)DecodeContext;
        Buffer_t                    AttachedCodedDataBuffer;
        ParsedFrameParameters_t   * ParsedFrameParams;
        unsigned int                PTSFlag = 0;
        unsigned long long int      PTS = ACC_NO_PTS_DTS;
        MME_ADBufferParams_t *      BufferParams = (MME_ADBufferParams_t *) &(Context->DecodeParameters.BufferParams[0]);

        CodecStatus_t               Status;
        
        Status      = DecodeContext->DecodeContextBuffer->ObtainAttachedBufferReference( CodedFrameBufferType, &AttachedCodedDataBuffer );
        if( Status == BufferNoError )
        {
            Status = AttachedCodedDataBuffer->ObtainMetaDataReference( Player->MetaDataParsedFrameParametersType, 
                                                                       (void **)(&ParsedFrameParams));
            if( Status == BufferNoError )
            {
                if(ParsedFrameParams->NormalizedPlaybackTime != INVALID_TIME)
                {
                    PTS = ParsedFrameParams->NativePlaybackTime;
                    // inform the firmware a pts is present
                    PTSFlag = ACC_PTS_PRESENT;
                } 
                else 
                {
                    CODEC_ERROR("PTS = INVALID_TIME\n" );
                }
            } 
            else 
            {
                CODEC_ERROR("No ObtainMetaDataReference\n" );
            }
        } 
        else 
        {
            CODEC_ERROR("No ObtainAttachedBufferReference\n" );
        }
        
        BufferParams->PTSflags.Bits.PTS_DTS_FLAG = PTSFlag;
        BufferParams->PTSflags.Bits.PTS_Bit32    = PTS >> 32;
        BufferParams->PTS                        = PTS;

        SendBuffersCommandsIssued++;
        OS_SetEvent( &IssueWmaTransformCommandEvent );

    }

    Status = Codec_MmeBase_c::SendMMEDecodeCommand();

//

    return Status;
}



// /////////////////////////////////////////////////////////////////////////
//
//      This function checks for a marker frame stall, where we have 1 
//	or more coded frame buffers in the transformer, and 1 or more 
//	decode buffers, and a marker frame waiting, but nothing is going 
//	in or out of the transform.
//

CodecStatus_t   Codec_MmeAudioWma_c::CheckForMarkerFrameStall( void )
{
Buffer_t                  LocalMarkerBuffer;
unsigned long long 	  Now;

//

    if( MarkerBuffer != NULL )
    {
	Now	= OS_GetTimeInMicroSeconds();

	if( !InPossibleMarkerStallState || 
	    (SendBuffersCommandsIssued  != StallStateSendBuffersCommandsIssued) ||
	    (TransformCommandsCompleted != StallStateTransformCommandsCompleted) )
	{
	    TimeOfEntryToPossibleMarkerStallState	= Now;
	    StallStateSendBuffersCommandsIssued		= SendBuffersCommandsIssued;
	    StallStateTransformCommandsCompleted	= TransformCommandsCompleted;
	    InPossibleMarkerStallState			= true;
	}
	else if( (Now - TimeOfEntryToPossibleMarkerStallState) >= MAXIMUM_STALL_PERIOD )
	{
	    LocalMarkerBuffer				= TakeMarkerBuffer();
	    if( LocalMarkerBuffer )
		OutputRing->Insert( (unsigned int)LocalMarkerBuffer );

	    InPossibleMarkerStallState			= false;
	}
    }
    else
    {
	InPossibleMarkerStallState	= false;
    }

    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
//      Callback function from MME
//
//

void   Codec_MmeAudioWma_c::CallbackFromMME( MME_Event_t Event, MME_Command_t *CallbackData )
{
CodecBaseDecodeContext_t                *DecodeContext;

    CODEC_DEBUG("Callback!  CmdId %x  CmdCode %s  State %d  Error %d\n",
		CallbackData->CmdStatus.CmdId,
		CallbackData->CmdCode == MME_SET_GLOBAL_TRANSFORM_PARAMS ? "MME_SET_GLOBAL_TRANSFORM_PARAMS" :
		CallbackData->CmdCode == MME_SEND_BUFFERS ? "MME_SEND_BUFFERS" :
		CallbackData->CmdCode == MME_TRANSFORM ? "MME_TRANSFORM" : "UNKNOWN",
		CallbackData->CmdStatus.State,
		CallbackData->CmdStatus.Error);

//
    Codec_MmeBase_c::CallbackFromMME( Event, CallbackData );

    //
    // Switch to perform appropriate actions per command
    //

    switch( CallbackData->CmdCode )
    {
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
			break;

	case MME_TRANSFORM:
	    if (Event == MME_COMMAND_COMPLETED_EVT)
	    {
		TransformCommandsCompleted++;

		OS_SetEvent( &IssueWmaTransformCommandEvent );
	    }
	    else
	    {
		//report( severity_info, "Codec_MmeBase_c::CallbackFromMME(%s) - Transform Command returns INCOMPLETE \n", Configuration.CodecName );
		CODEC_DEBUG("Codec_MmeBase_c::CallbackFromMME(%s) - Transform Command returns INCOMPLETE \n", Configuration.CodecName );
	    }


			break;

	case MME_SEND_BUFFERS:
			DecodeContext           = (CodecBaseDecodeContext_t *)CallbackData;

			ReleaseDecodeContext( DecodeContext );
			SendBuffersCommandsCompleted++;

			break;

	default:
			break;
    }
}

