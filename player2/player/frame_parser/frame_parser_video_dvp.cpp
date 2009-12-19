/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : frame_parser_video_dvp.cpp
Author :           Chris

Implementation of the dvp video frame parser class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
08-Aug-06   Created                                         Chris.

************************************************************************/

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "frame_parser_video_dvp.h"
#include "mpeg4.h"
#include "dvp.h"

//#define DUMP_HEADERS 1

static BufferDataDescriptor_t     DvpStreamParametersBuffer = BUFFER_MPEG4_STREAM_PARAMETERS_TYPE;
static BufferDataDescriptor_t     DvpFrameParametersBuffer = BUFFER_MPEG4_FRAME_PARAMETERS_TYPE;

// /////////////////////////////////////////////////////////////////////////
//
// 	The Constructor function
//
FrameParser_VideoDvp_c::FrameParser_VideoDvp_c( void )
{
    Configuration.FrameParserName		= "VideoDvp";

    Configuration.StreamParametersCount		= 32;
    Configuration.StreamParametersDescriptor	= &DvpStreamParametersBuffer;

    Configuration.FrameParametersCount		= 32;
    Configuration.FrameParametersDescriptor	= &DvpFrameParametersBuffer;

    Configuration.SupportSmoothReversePlay	= false;		// Cannot reverse captured data
    Configuration.InitializeStartCodeList	= false;		// We take a simple structure - no start codes

//
    FrameIndex = 0;
    
	FrameParameters  = NULL;
	StreamParameters = NULL;

	ReverseQueuedPostDecodeSettingsRing = NULL;	
	Reset();
}


// /////////////////////////////////////////////////////////////////////////
//
// 	The Destructor function
//

FrameParser_VideoDvp_c::~FrameParser_VideoDvp_c( 	void )
{
	Halt();
	Reset();
}


// /////////////////////////////////////////////////////////////////////////
//
// 	The Reset function release any resources, and reset all variable
//

FrameParserStatus_t   FrameParser_VideoDvp_c::Reset(	void )
{
	FrameParameters = NULL;	
	FirstDecodeOfFrame = true;
	memset( &ReferenceFrameList, 0x00, sizeof(ReferenceFrameList_t) );
	
	if (StreamParameters != NULL)
	{	
		delete StreamParameters;
		StreamParameters = NULL;
	}
	
	if( ReverseQueuedPostDecodeSettingsRing != NULL )
	{
		delete ReverseQueuedPostDecodeSettingsRing;
		ReverseQueuedPostDecodeSettingsRing     = NULL;
	}
			
	return FrameParser_Video_c::Reset();
}


// /////////////////////////////////////////////////////////////////////////
//
//      The register output ring function
//

FrameParserStatus_t   FrameParser_VideoDvp_c::RegisterOutputBufferRing(
					Ring_t		Ring )
{
FrameParserStatus_t	Status = FrameParserNoError;
	
	// Clear our parameter pointers
	     
	StreamParameters                    = NULL;
	FrameParameters                     = NULL;
	DeferredParsedFrameParameters       = NULL;
	DeferredParsedVideoParameters       = NULL;
	
	Status = FrameParser_Video_c::RegisterOutputBufferRing( Ring );
	
	return Status;
}


// /////////////////////////////////////////////////////////////////////////
//
// 	Deal with decode of a single frame in forward play
//

FrameParserStatus_t   FrameParser_VideoDvp_c::ForPlayProcessFrame(      void )
{	
	return FrameParser_Video_c::ForPlayProcessFrame();
}

// /////////////////////////////////////////////////////////////////////////
//
// 	Deal with a single frame in reverse play
//

FrameParserStatus_t   FrameParser_VideoDvp_c::RevPlayProcessFrame(      void )
{
	return FrameParser_Video_c::RevPlayProcessFrame();
}


// /////////////////////////////////////////////////////////////////////////
//
// 	Addition to the base queue a buffer for decode increments 
//	the field index.
//

FrameParserStatus_t   FrameParser_VideoDvp_c::ForPlayQueueFrameForDecode( void )
{
	return  FrameParser_Video_c::ForPlayQueueFrameForDecode();
}
 

// /////////////////////////////////////////////////////////////////////////
//
// 	Specific reverse play implementation of queue for decode
//	the field index.
//

FrameParserStatus_t   FrameParser_VideoDvp_c::RevPlayQueueFrameForDecode( void )
{
	return FrameParser_Video_c::RevPlayQueueFrameForDecode();
}


// /////////////////////////////////////////////////////////////////////////
//
// 	This function is responsible for walking the stacks to handle 
//	reverse decode.
//

FrameParserStatus_t   FrameParser_VideoDvp_c::RevPlayProcessDecodeStacks(		void )
{
	return FrameParser_Video_c::RevPlayProcessDecodeStacks();
}


// /////////////////////////////////////////////////////////////////////////
//
// 	This function is responsible for walking the stacks to discard
//	everything on them when we abandon reverse decode.
//

FrameParserStatus_t   FrameParser_VideoDvp_c::RevPlayPurgeDecodeStacks(		void )
{
	return FrameParser_Video_c::RevPlayPurgeDecodeStacks();
}

FrameParserStatus_t   FrameParser_VideoDvp_c::ReadHeaders( void )
{
Buffer_t		 Buffer;
BufferStructure_t	*BufferStructure;
BufferStatus_t		 BufferStatus;
StreamInfo_t		*StreamInfo;

    //
    // Find the stream info structure, and extract the buffer
    //

    StreamInfo	= (StreamInfo_t*)BufferData;
    Buffer	= (Buffer_t)StreamInfo->buffer_class;

    //
    // Switch the ownership of the buffer
    //

    Buffer->TransferOwnership( IdentifierFrameParser );

    //
    // Modify the buffer structure to match the actual capture
    //

    BufferStatus = Buffer->ObtainMetaDataReference( Player->MetaDataBufferStructureType, (void**)&BufferStructure );
    if (BufferStatus != BufferNoError)
    {
	report( severity_error, "FrameParser_VideoDvp_c::RevPlayPurgeDecodeStacks - Unable to access buffer structure parameters %x.\n", BufferStatus);
	return FrameParserError;
    }

    BufferStructure->ComponentOffset[0] = StreamInfo->h_offset * 2;

    //
    // Fill out appropriate frame and video parameters
    //

    ParsedFrameParameters->NewStreamParameters 			= false;
    ParsedFrameParameters->SizeofStreamParameterStructure 	= sizeof(StreamInfo_t);
    ParsedFrameParameters->StreamParameterStructure		= &StreamInfo;

    ParsedFrameParameters->FirstParsedParametersForOutputFrame 	= true;
    ParsedFrameParameters->FirstParsedParametersAfterInputJump	= false;
    ParsedFrameParameters->SurplusDataInjected			= false;
    ParsedFrameParameters->ContinuousReverseJump		= false;
    ParsedFrameParameters->KeyFrame				= true;
    ParsedFrameParameters->ReferenceFrame			= true;		// Turn off autogeneration of DTS
    ParsedFrameParameters->IndependentFrame			= true;
    ParsedFrameParameters->NumberOfReferenceFrameLists		= 0;
    
    ParsedFrameParameters->NewFrameParameters			= true;
    ParsedVideoParameters->Content.PixelAspectRatio		= StreamInfo->pixel_aspect_ratio;
    ParsedVideoParameters->Content.Width			= StreamInfo->width;
    ParsedVideoParameters->Content.Height			= StreamInfo->height;
    ParsedVideoParameters->Content.DisplayWidth			= StreamInfo->width;
    ParsedVideoParameters->Content.DisplayHeight		= StreamInfo->height;
    ParsedVideoParameters->Content.Progressive			= !StreamInfo->interlaced;
    ParsedVideoParameters->Content.FrameRate			= Rational_t(StreamInfo->FrameRateNumerator, StreamInfo->FrameRateDenominator);
    ParsedVideoParameters->Content.OverscanAppropriate 		= 0;
    ParsedVideoParameters->Content.VideoFullRange		= (StreamInfo->VideoFullRange != 0);
    ParsedVideoParameters->Content.ColourMatrixCoefficients	= ((StreamInfo->ColourMode == DVP_COLOUR_MODE_601) ? MatrixCoefficients_ITU_R_BT601 :
								  ((StreamInfo->ColourMode == DVP_COLOUR_MODE_709) ? MatrixCoefficients_ITU_R_BT709 :
														     MatrixCoefficients_Undefined));

    ParsedVideoParameters->InterlacedFrame			= StreamInfo->interlaced;    
    ParsedVideoParameters->DisplayCount[0]			= 1;
    ParsedVideoParameters->DisplayCount[1]			= ParsedVideoParameters->InterlacedFrame ? 1 : 0;
    ParsedVideoParameters->SliceType				= SliceTypeI;
    ParsedVideoParameters->TopFieldFirst			= StreamInfo->top_field_first;
    ParsedVideoParameters->PictureStructure			= StructureFrame;
 
    
    ParsedVideoParameters->PanScan.Count			= 0;

    FirstDecodeOfFrame						= true;
    FrameToDecode						= true;
    
    return FrameParserNoError;
}

FrameParserStatus_t   FrameParser_VideoDvp_c::CommitFrameForDecode(void)
{
	return FrameParserNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
//      Stream specific function to prepare a reference frame list
//

FrameParserStatus_t   FrameParser_VideoDvp_c::PrepareReferenceFrameList( void )
{
    ParsedFrameParameters->NumberOfReferenceFrameLists                  = 0;

    return FrameParserNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
//      Stream specific function to manage a reference frame list in forward play
//      we only record a reference frame as such on the last field, in order to 
//      ensure the correct management of reference frames in the codec, we immediately 
//      inform the codec of a release on the first field of a field picture.
//

FrameParserStatus_t   FrameParser_VideoDvp_c::ForPlayUpdateReferenceFrameList( void )
{	
    return FrameParserNoError;
}
