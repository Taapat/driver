/************************************************************************
 * COPYRIGHT (C) SGS-THOMSON Microelectronics 2007
 *
 * Source file name : codec_dvp_video.cpp
 * Author :           Chris
 *
 * Implementation of the dvp null codec class for player 2.
 *
 *
 * Date        Modification                                    Name
 * ----        ------------                                    --------
 * 07-Aug-07   Created                                         Chris
 *
 * ************************************************************************/

#include "codec_dvp_video.h"
#include "dvp.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

#define BUFFER_CODED_FRAME_BUFFER        "CodedFrameBuffer"
#define BUFFER_CODED_FRAME_BUFFER_TYPE   {BUFFER_CODED_FRAME_BUFFER, BufferDataTypeBase, AllocateFromSuppliedBlock, 4, 64, false, false, 0}

static BufferDataDescriptor_t            InitialCodedFrameBufferDescriptor = BUFFER_CODED_FRAME_BUFFER_TYPE;


// /////////////////////////////////////////////////////////////////////////
//
// Class Constructor
//

Codec_DvpVideo_c::Codec_DvpVideo_c()
{
    //
    // Initialize class variables
    //

    CodedFramePool			= NULL;
    CodedFrameMemory[CachedAddress]	= ActualCodedFrameMemory;
    CodedFrameMemory[UnCachedAddress]	= NULL;				// If anyone uses these they will crash, and rightly so
    CodedFrameMemory[PhysicalAddress]	= NULL;

    DataTypesInitialized		= false;

    //
    // Setup the trick mode parameters
    //

    DvpTrickModeParameters.SubstandardDecodeSupported        = false;
    DvpTrickModeParameters.MaximumNormalDecodeRate           = 1;
    DvpTrickModeParameters.MaximumSubstandardDecodeRate      = 0;

    DvpTrickModeParameters.DefaultGroupSize                  = 1;
    DvpTrickModeParameters.DefaultGroupReferenceFrameCount   = 1;

//

    InitializationStatus	= CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// Class Destructor
//

Codec_DvpVideo_c::~Codec_DvpVideo_c()
{
    if( CodedFramePool != NULL )
    {
	BufferManager->DestroyPool( CodedFramePool );
	CodedFramePool  = NULL;
    }

    BaseComponentClass_c::Reset();
    BaseComponentClass_c::Halt();
}


// /////////////////////////////////////////////////////////////////////////
//
// Register the output buffer ring
//

CodecStatus_t   Codec_DvpVideo_c::RegisterOutputBufferRing( Ring_t Ring )
{
PlayerStatus_t          Status;
BufferPool_t            DummyPool;
unsigned int            DummyUnsignedInt;

//

    OutputRing  = Ring;

    //
    // Force the allocation of the coded frame buffers
    //

    Status      = GetCodedFrameBufferPool( &DummyPool, &DummyUnsignedInt );
    if( Status != CodecNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::RegisterOutputBufferRing(DVP) - Failed to get coded frame buffer pool.\n");
	return Status;
    }

    //
    // Obtain the decode buffer pool
    //

    Player->GetDecodeBufferPool( Stream, &DecodeBufferPool );
    if( DecodeBufferPool == NULL )
    {
	report( severity_error, "Codec_DvpVideo_c::RegisterOutputBufferRing(DVP) - This implementation does not support no-output decoding.\n");
	return PlayerNotSupported;
    }

    //
    // Attach the stream specific (audio|video|data)
    // parsed frame parameters to the decode buffer pool.
    //

    Status      = DecodeBufferPool->AttachMetaData(Player->MetaDataParsedVideoParametersType );
    if( Status != BufferNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::RegisterOutputBufferRing(DVP) - Failed to attach stream specific parsed parameters to all decode buffers.\n");
	return Status;
    }

    //
    // Go live
    //

    SetComponentState( ComponentRunning );
    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// Release a decode buffer - just do a decrement reference count
//

CodecStatus_t   Codec_DvpVideo_c::ReleaseDecodeBuffer( Buffer_t Buffer )
{
#if 0
unsigned int Length;
unsigned char     *Pointer;

    Buffer->ObtainDataReference( &Length, NULL, (void **)(&Pointer), CachedAddress );
    memset( Pointer, 0x10, 0xa8c00 );
#endif

    Buffer->DecrementReferenceCount();
    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// Input a buffer - just pass it on
//

CodecStatus_t   Codec_DvpVideo_c::Input( Buffer_t CodedBuffer )
{
CodecStatus_t			 Status;
unsigned int			 CodedDataLength;
StreamInfo_t			*StreamInfo;
Buffer_t			 MarkerBuffer;
BufferStructure_t		 BufferStructure;
ParsedFrameParameters_t		*ParsedFrameParameters;
ParsedVideoParameters_t		*ParsedVideoParameters;
Buffer_t			 CapturedBuffer;
ParsedVideoParameters_t		*CapturedParsedVideoParameters;

    //
    // Extract the useful coded data information
    //

    Status      = CodedBuffer->ObtainDataReference( NULL, &CodedDataLength, (void **)(&StreamInfo), CachedAddress );
    if( Status != PlayerNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Unable to obtain data reference.\n" );
	return Status;
    }

    Status      = CodedBuffer->ObtainMetaDataReference( Player->MetaDataParsedFrameParametersType, (void **)(&ParsedFrameParameters) );
    if( Status != PlayerNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Unable to obtain the meta data \"ParsedFrameParameters\".\n" );
	return Status;
    }

    Status      = CodedBuffer->ObtainMetaDataReference( Player->MetaDataParsedVideoParametersType, (void**)&ParsedVideoParameters );
    if( Status != PlayerNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Unable to obtain the meta data \"ParsedVideoParameters\".\n" );
	return Status;
    }

    //
    // Handle the special case of a marker frame
    //

    if( (CodedDataLength == 0) && !ParsedFrameParameters->NewStreamParameters && !ParsedFrameParameters->NewFrameParameters )
    {
	//
	// Get a marker buffer
	//

	memset( &BufferStructure, 0x00, sizeof(BufferStructure_t) );
	BufferStructure.Format  = FormatMarkerFrame;

	Status      = Manifestor->GetDecodeBuffer( &BufferStructure, &MarkerBuffer );
	if( Status != ManifestorNoError )
	{
	    report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Failed to get marker decode buffer from manifestor.\n");
	    return Status;
	}

	MarkerBuffer->TransferOwnership( IdentifierCodec );

	Status      = MarkerBuffer->AttachMetaData( Player->MetaDataParsedFrameParametersReferenceType, UNSPECIFIED_SIZE, (void *)ParsedFrameParameters );
	if( Status != PlayerNoError )
	{
	    report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Unable to attach a reference to \"ParsedFrameParameters\" to the marker buffer.\n");
	    return Status;
	}

	MarkerBuffer->AttachBuffer( CodedBuffer );

	//
	// Queue/pass on the buffer
	//

	OutputRing->Insert( (unsigned int)MarkerBuffer );
	return CodecNoError;
    }

    //
    // Attach the coded data fields to the decode/captured buffer
    //

    CapturedBuffer	= (Buffer_t)StreamInfo->buffer_class;
    if( CapturedBuffer == NULL )
    {
	report( severity_fatal,"Codec_DvpVideo_c::Input(DVP) - NULL Buffer\n" );
	return CodecNoError;
    }

//

    Status      = CapturedBuffer->ObtainMetaDataReference( Player->MetaDataParsedVideoParametersType, (void**)&CapturedParsedVideoParameters );
    if( Status != PlayerNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Unable to obtain the meta data \"ParsedVideoParameters\" from the captured buffer.\n" );
	return Status;
    }

    memcpy( CapturedParsedVideoParameters, ParsedVideoParameters, sizeof(ParsedVideoParameters_t) );

//

    Status      = CapturedBuffer->AttachMetaData( Player->MetaDataParsedFrameParametersReferenceType, UNSPECIFIED_SIZE, (void *)ParsedFrameParameters );
    if( Status != BufferNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Failed to attach Frame Parameters\n");
	return Status;
    }

    //
    // Switch the ownership hierarchy, and allow the captured buffer to exist on it's own.
    //

    CapturedBuffer->IncrementReferenceCount();

    Status	= CodedBuffer->DetachBuffer( CapturedBuffer );
    if( Status != BufferNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Failed to detach captured buffer from coded frame buffer\n");
	return Status;
    }

    Status      = CapturedBuffer->AttachBuffer( CodedBuffer );
    if( Status != BufferNoError )
    {
	report( severity_error, "Codec_DvpVideo_c::Input(DVP) - Failed to attach captured buffer to Coded Frame Buffer\n");
	return Status;
    }

    //
    // Pass the captured buffer on
    //

    OutputRing->Insert( (unsigned int)CapturedBuffer );

    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// Return the trick mode parameters
//

CodecStatus_t   Codec_DvpVideo_c::GetTrickModeParameters( CodecTrickModeParameters_t    **TrickModeParameters )
{
    *TrickModeParameters    = &DvpTrickModeParameters;
    return CodecNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
// Get the coded frame buffer pool
//

CodecStatus_t   Codec_DvpVideo_c::GetCodedFrameBufferPool(    BufferPool_t             *Pool,
			                                      unsigned int             *MaximumCodedFrameSize )
{
PlayerStatus_t          Status;

    //
    // If we haven't already created the buffer pool, do it now.
    //

    if( CodedFramePool == NULL )
    {
	//
	// Obtain the class list
	//

	Player->GetBufferManager( &BufferManager );
	Player->GetClassList( Stream, &Collator, &FrameParser, &Codec, &OutputTimer, &Manifestor );
	if( Manifestor == NULL )
	{
	    report( severity_error, "Codec_DvpVideo_c::GetCodedFrameBufferPool(DVP) - This implementation does not support no-output decoding.\n");
	    return PlayerNotSupported;
	}

	//
	// Initialize the data types we use.
	//

	Status  = InitializeDataTypes();
	if( Status != CodecNoError )
	    return Status;

	//
	// Create the pool using our class based memory
	//

	Status  = BufferManager->CreatePool( &CodedFramePool, CodedFrameBufferType, DVP_CODED_FRAME_COUNT, DVP_FRAME_MEMORY_SIZE, CodedFrameMemory );
	if( Status != BufferNoError )
	{
	    report( severity_error, "Codec_DvpVideo_c::GetCodedFrameBufferPool(DVP) - Failed to create the pool.\n");
	    return PlayerInsufficientMemory;
	}
    }

    //
    // Setup the parameters and return
    //

    *Pool                       = CodedFramePool;
    *MaximumCodedFrameSize      = DVP_MAXIMUM_FRAME_SIZE;

    return CodecNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
//      Function to initialize a type with the buffer manager
//

CodecStatus_t   Codec_DvpVideo_c::InitializeDataType(	BufferDataDescriptor_t   *InitialDescriptor,
							BufferType_t             *Type,
							BufferDataDescriptor_t  **ManagedDescriptor )
{
PlayerStatus_t  Status;

    Status      = BufferManager->FindBufferDataType( InitialDescriptor->TypeName, Type );
    if( Status != BufferNoError )
    {
	Status  = BufferManager->CreateBufferDataType( InitialDescriptor, Type );
	if( Status != BufferNoError )
	{
	    report( severity_error, "Codec_DvpVideo_c::InitializeDataType - Failed to create the '%s' buffer type.\n", InitialDescriptor->TypeName );
	    return Status;
	}
    }

    BufferManager->GetDescriptor( *Type, (BufferPredefinedType_t)InitialDescriptor->Type, ManagedDescriptor );
    return CodecNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
//      Function to initialize all the types with the buffer manager
//

CodecStatus_t   Codec_DvpVideo_c::InitializeDataTypes(   void )
{
CodecStatus_t   Status;

    if( DataTypesInitialized )
	return CodecNoError;

    //
    // Coded frame buffer type
    //

    Status      = InitializeDataType( &InitialCodedFrameBufferDescriptor, &CodedFrameBufferType, &CodedFrameBufferDescriptor );
    if( Status != PlayerNoError )
	return Status;

    DataTypesInitialized        = true;
    return CodecNoError;
}

