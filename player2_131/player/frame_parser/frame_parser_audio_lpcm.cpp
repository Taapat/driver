/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : frame_parser_audio_mpeg.cpp
Author :           Daniel

Implementation of the mpeg audio frame parser class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
30-Mar-07   Created (from frame_parser_video_mpeg2.cpp)     Daniel

************************************************************************/

////////////////////////////////////////////////////////////////////////
///
/// \class FrameParser_AudioLpcm_c
/// \brief Frame parser for LPCM audio
///

#define ENABLE_FRAME_DEBUG 0

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "lpcm_audio.h"
#include "frame_parser_audio_lpcm.h"

//


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#undef FRAME_TAG
#define FRAME_TAG "LPCM audio frame parser"

static BufferDataDescriptor_t     LpcmAudioStreamParametersBuffer = BUFFER_LPCM_AUDIO_STREAM_PARAMETERS_TYPE;
static BufferDataDescriptor_t     LpcmAudioFrameParametersBuffer = BUFFER_LPCM_AUDIO_FRAME_PARAMETERS_TYPE;




// /////////////////////////////////////////////////////////////////////////
//
// Locally defined macros
//


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

////////////////////////////////////////////////////////////////////////////
///
/// Examine the supplied frame header and extract the information contained within.
///
/// This is a utility function shared by the frame parser and the equivalent
/// collator. Due to its shared nature this is a static method and does not access
/// any information not provided via the function arguments.
///
/// <b>LPCM format</b>
///
/// <pre>
/// AAAAAAAA BBBBBBBB CCCCCCCC CCCCCCCC
/// DEFGGGGG HHIIJKKK LLLLLLLL
///
/// Sign            Length          Position         Description
/// 
/// A                 8             (31-24)          sub_stream_id
/// B                 8             (23-16)          number_of_frame_headers
/// C                16             (15-0)           first_access_unit_pointer
/// D                 1             (31)             audio_emphasis_flag
/// E                 1             (30)             audio_mute_flag
/// F                 1             (29)             reserved
/// G                 5             (28-24)          audio_frame_number
/// H                 2             (23-22)          quantization_word_length
/// I                 2             (21-20)          audio_sampling_frequency
/// J                 1             (19)             reserved
/// K                 3             (18-16)          number_of_audio_channels
/// L                 8             (15-8)           dynamic_range_control
///
/// </pre>
///
/// Note: Be careful with the terminology. The actual stream will be comprised of a number
///       of 'subframes', with the header above used in the collator to analyse the pes
///       private data area to determine the collection of 10 or 20 subframes.
///       The same header (though padded to 8 bytes for alignment) will be rewritten
///       and prefixed to this bundle of subframes, to form a 'frame' for the parser.
///
///       i.e. stream |P|P|  0  |  0  | 0 |P|P| 0|  0  |  0  |0|P|P| 0 |  0  |  0  |P| etc...
///                   |E|D|  0  |  0  | 0 |E|D| 0|  0  |  0  |0|E|D| 0 |  0  |  0  |E|
///                   |S|A|  0  |  1  | 2 |S|A| 2|  3  |  4  |5|S|A| 5 |  6  |  7  |S|
///                        <------------->
///                               \subframes in each pes, may be incomplete and will be
///                                aggregated before passing from collator into...
///
///                      <--- 20 stream subframes now comprise one parser frame --->
///            frame  |P|  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  | ... |  2  |
///                   |D|  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |     |  0  |
///                   |A|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |     |     |
/// 
/// \return Frame parser status code, FrameParserNoError indicates success.
///
FrameParserStatus_t FrameParser_AudioLpcm_c::ParseFrameHeader( unsigned char *FrameHeaderBytes, LpcmAudioParsedFrameHeader_t *ParsedFrameHeader )
{
    unsigned int    FrameHeader1;
    unsigned int    FrameHeader2;

    FrameHeader1 = FrameHeaderBytes[0] << 24 | FrameHeaderBytes[1] << 16 |
                   FrameHeaderBytes[2] <<  8 | FrameHeaderBytes[3];
    FrameHeader2 = FrameHeaderBytes[4] << 24 | FrameHeaderBytes[5] << 16 |
                   FrameHeaderBytes[6] <<  8;

    FRAME_DEBUG( "LPCM PARSER NEW PDA %08x-%08x\n", FrameHeader1, FrameHeader2 );

    if ( (FrameHeaderBytes[0] & LPCM_AUDIO_START_CODE_MASK) != LPCM_AUDIO_START_CODE )
    {
    	FRAME_ERROR( "Invalid start code %x - %08x-%08x\n", FrameHeaderBytes[0] & LPCM_AUDIO_START_CODE_MASK, FrameHeader1, FrameHeader2 );
    	return FrameParserError;
    }

    ParsedFrameHeader->NumberOfFrameHeaders   = ((FrameHeader1 & 0x00FF0000) >> 16);
    ParsedFrameHeader->FirstAccessUnitPointer =  (FrameHeader1 & 0x0000FFFF);
    ParsedFrameHeader->AudioEmphasisFlag      = ((FrameHeader2 & 0x80000000) >> 31);
    ParsedFrameHeader->AudioMuteFlag          = ((FrameHeader2 & 0x40000000) >> 30);
    ParsedFrameHeader->AudioFrameNumber       = ((FrameHeader2 & 0x1F000000) >> 24);
    ParsedFrameHeader->QuantizationWordLength = ((FrameHeader2 & 0x00C00000) >> 22);
    ParsedFrameHeader->AudioSamplingFrequency = ((FrameHeader2 & 0x00300000) >> 20);
    ParsedFrameHeader->NumberOfAudioChannels  = ((FrameHeader2 & 0x00070000) >> 16);
    ParsedFrameHeader->DynamicRangeControl    = ((FrameHeader2 & 0x0000FF00) >>  8);


    // FirstAccessUnitPointer is an offset from its own last byte (inclusive)
    if (ParsedFrameHeader->FirstAccessUnitPointer < 4 && ParsedFrameHeader->FirstAccessUnitPointer != 0) {
        FRAME_ERROR("First access unit pointer points into the LPCM header (%d)\n", ParsedFrameHeader->FirstAccessUnitPointer);
        return FrameParserError;
    }
/*
    if (ParsedFrameHeader->FirstAccessUnitPointer >= *DataSize) {
        FRAME_ERROR("First access unit pointer pointers outside data block (%d)\n", ParsedFrameHeader->FirstAccessUnitPointer);
    }
*/
    if (ParsedFrameHeader->AudioFrameNumber >= 20 && ParsedFrameHeader->AudioFrameNumber != 31) {
        FRAME_ERROR("Invalid audio frame number (%d) PDA %08x-%08x\n",
                     ParsedFrameHeader->AudioFrameNumber, FrameHeader1, FrameHeader2);
        return FrameParserError;
    }

    if (ParsedFrameHeader->QuantizationWordLength >= 3) {
        FRAME_ERROR("Invalid quantization word length (%d) PDA %08x-%08x\n",
                     ParsedFrameHeader->QuantizationWordLength, FrameHeader1, FrameHeader2);
        return FrameParserError;
    }

    if (ParsedFrameHeader->NumberOfAudioChannels >= 7 &&
        ParsedFrameHeader->NumberOfAudioChannels != 9) {
        FRAME_ERROR("Invalid number of audio channels (%d) PDA %08x-%08x\n",
                     ParsedFrameHeader->NumberOfAudioChannels, FrameHeader1, FrameHeader2);
        return FrameParserError;
    }
    // SUCCESS!

    // Derived values
    ParsedFrameHeader->NumberOfSamples        = LPCM_SAMPLES_PER_FRAME(ParsedFrameHeader->AudioSamplingFrequency);
    ParsedFrameHeader->SamplingFrequency      = LPCM_SAMPLING_FREQ(ParsedFrameHeader->AudioSamplingFrequency)*1000;

    ParsedFrameHeader->Length = ParsedFrameHeader->NumberOfSamples * (ParsedFrameHeader->NumberOfAudioChannels+1);
    switch(ParsedFrameHeader->QuantizationWordLength) {
        case 0  : ParsedFrameHeader->Length *= 2; //*(16bits/8) -> 2
                  break;
        case 1  : ParsedFrameHeader->Length *= 5; //*(20bits/8) -> 2.5
                  ParsedFrameHeader->Length >> 1;
                  break;
        case 2  : ParsedFrameHeader->Length *= 3; //*(24bits/8) -> 3
                  break;
        default : FRAME_ERROR( "Invalid LPCM quantization\n");
                  return FrameParserError;
    }

    return FrameParserNoError;
}


////////////////////////////////////////////////////////////////////////////
///
///     Constructor
///
FrameParser_AudioLpcm_c::FrameParser_AudioLpcm_c( void )
{
    Configuration.FrameParserName		= "AudioLpcm";

    Configuration.StreamParametersCount		= 32;
    Configuration.StreamParametersDescriptor	= &LpcmAudioStreamParametersBuffer;

    Configuration.FrameParametersCount		= 32;
    Configuration.FrameParametersDescriptor	= &LpcmAudioFrameParametersBuffer;

//

    Reset();
}

////////////////////////////////////////////////////////////////////////////
///
///     Destructor
///
FrameParser_AudioLpcm_c::~FrameParser_AudioLpcm_c( void )
{
    Halt();
    Reset();
}

////////////////////////////////////////////////////////////////////////////
///
///     The Reset function release any resources, and reset all variable
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::Reset(  void )
{
    // CurrentStreamParameters is initialized in RegisterOutputBufferRing()

    return FrameParser_Audio_c::Reset();
}


////////////////////////////////////////////////////////////////////////////
///
///     The register output ring function
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::RegisterOutputBufferRing(       Ring_t          Ring )
{
PlayerStatus_t  Status;

    //
    // Clear our parameter pointers
    //

    StreamParameters                    = NULL;
    FrameParameters                     = NULL;
    
    //
    // Set illegal state forcing a parameter update on the first frame
    //
    
    memset( &CurrentStreamParameters, 0, sizeof(CurrentStreamParameters) );

    //
    // Pass the call down the line
    //

    return FrameParser_Audio_c::RegisterOutputBufferRing( Ring );
}


////////////////////////////////////////////////////////////////////////////
///
/// Parse the frame header and store the results for when we emit the frame.
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::ReadHeaders( void )
{
FrameParserStatus_t Status;

    //
    // Perform the common portion of the read headers function
    //

    FrameParser_Audio_c::ReadHeaders();

//
    FRAME_DEBUG("READHEADERS: Len-%d \n", BufferLength);
//    FRAME_ERROR("READHEADERS: Len-%d \n", BufferLength);

    Status = ParseFrameHeader( BufferData, &ParsedFrameHeader );
    if( Status != FrameParserNoError )
    {
    	FRAME_ERROR("Failed to parse frame header, bad collator selected?\n");
    	return Status;
    }
    
/*
    if ( ((ParsedFrameHeader.Length*20)+7) != BufferLength)
    {
    	FRAME_ERROR("Buffer length is inconsistant with frame header, bad collator selected?\n");
    	return FrameParserError;
    }
*/
    FrameToDecode = true;


    if((CurrentStreamParameters.NumberOfFrameHeaders   != ParsedFrameHeader.NumberOfFrameHeaders  ) ||
       (CurrentStreamParameters.AudioEmphasisFlag      != ParsedFrameHeader.AudioEmphasisFlag     ) ||
       (CurrentStreamParameters.AudioMuteFlag          != ParsedFrameHeader.AudioMuteFlag         ) ||
       (CurrentStreamParameters.QuantizationWordLength != ParsedFrameHeader.QuantizationWordLength) ||
       (CurrentStreamParameters.AudioSamplingFrequency != ParsedFrameHeader.AudioSamplingFrequency) ||
       (CurrentStreamParameters.NumberOfAudioChannels  != ParsedFrameHeader.NumberOfAudioChannels ) ||
       (CurrentStreamParameters.DynamicRangeControl    != ParsedFrameHeader.DynamicRangeControl   ))
    {
    	Status = GetNewStreamParameters( (void **) &StreamParameters );
    	if( Status != FrameParserNoError )
    	{
    	    FRAME_ERROR( "Cannot get new stream parameters\n" );
    	    return Status;
    	}

        StreamParameters->NumberOfFrameHeaders   = CurrentStreamParameters.NumberOfFrameHeaders   = ParsedFrameHeader.NumberOfFrameHeaders;
        StreamParameters->AudioEmphasisFlag      = CurrentStreamParameters.AudioEmphasisFlag      = ParsedFrameHeader.AudioEmphasisFlag;
        StreamParameters->AudioMuteFlag          = CurrentStreamParameters.AudioMuteFlag          = ParsedFrameHeader.AudioMuteFlag;
        StreamParameters->QuantizationWordLength = CurrentStreamParameters.QuantizationWordLength = ParsedFrameHeader.QuantizationWordLength;
        StreamParameters->AudioSamplingFrequency = CurrentStreamParameters.AudioSamplingFrequency = ParsedFrameHeader.AudioSamplingFrequency;
        StreamParameters->NumberOfAudioChannels  = CurrentStreamParameters.NumberOfAudioChannels  = ParsedFrameHeader.NumberOfAudioChannels;
        StreamParameters->DynamicRangeControl    = CurrentStreamParameters.DynamicRangeControl    = ParsedFrameHeader.DynamicRangeControl;
        
        UpdateStreamParameters = true;
    }

    Status = GetNewFrameParameters( (void **) &FrameParameters );
    if( Status != FrameParserNoError )
    {
    	FRAME_ERROR( "Cannot get new frame parameters\n" );
    	return Status;
    }

    // Nick inserted some default values here
    ParsedFrameParameters->FirstParsedParametersForOutputFrame          = true;
    ParsedFrameParameters->FirstParsedParametersAfterInputJump          = FirstDecodeAfterInputJump;
    ParsedFrameParameters->SurplusDataInjected                          = SurplusDataInjected;
    ParsedFrameParameters->ContinuousReverseJump                        = ContinuousReverseJump;
    ParsedFrameParameters->KeyFrame                                     = true;
    ParsedFrameParameters->ReferenceFrame                               = false;

    ParsedFrameParameters->NewFrameParameters		 = true;
    ParsedFrameParameters->SizeofFrameParameterStructure = sizeof(LpcmAudioFrameParameters_t);
    ParsedFrameParameters->FrameParameterStructure       = FrameParameters;
    
    // this does look a little pointless but I don't want to trash it until we
    // have got a few more audio formats supported (see \todo in mpeg_audio.h).    
    FrameParameters->BitRate = LPCM_BITS_PER_SAMPLE(ParsedFrameHeader.QuantizationWordLength) * ParsedFrameHeader.NumberOfSamples;
    FrameParameters->FrameSize = ParsedFrameHeader.Length;

    ParsedAudioParameters->Source.BitsPerSample = 0; // filled in by codec
    ParsedAudioParameters->Source.ChannelCount  = 0; // filled in by codec

    //will always be 48K, as 96K will be downsampled
    ParsedAudioParameters->Source.SampleRateHz = 48000; //ParsedFrameHeader.SamplingFrequency;

    //derive the sample count from the actual buffer size received
    ParsedAudioParameters->SampleCount = (BufferLength-8) / (StreamParameters->NumberOfAudioChannels+1);
    switch(StreamParameters->QuantizationWordLength) {
        case 0:  ParsedAudioParameters->SampleCount =  (ParsedAudioParameters->SampleCount >> 1);
                 break;
        case 1:  ParsedAudioParameters->SampleCount =  ((ParsedAudioParameters->SampleCount << 1)/5);
                 break;
        case 2:  ParsedAudioParameters->SampleCount =  (ParsedAudioParameters->SampleCount / 3);
                 break;
    }
    //take into account that 96K will be downsampled to 48K
    if(ParsedFrameHeader.SamplingFrequency==96000) {
        ParsedAudioParameters->SampleCount = (ParsedAudioParameters->SampleCount >> 1);
    }

    ParsedAudioParameters->Organisation = 0; // filled in by codec
    
    return FrameParserNoError;
}


////////////////////////////////////////////////////////////////////////////
///
///     The reset reference frame list function
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::ResetReferenceFrameList( void )
{
    FRAME_DEBUG(">><<");
    Player->CallInSequence( Stream, SequenceTypeImmediate, TIME_NOT_APPLICABLE, CodecFnReleaseReferenceFrame, CODEC_RELEASE_ALL );

    return FrameParserNoError;
}


///////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::PurgeQueuedPostDecodeParameterSettings()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::PurgeQueuedPostDecodeParameterSettings( void )
{
    return FrameParserNoError;
}


///////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::ProcessQueuedPostDecodeParameterSettings()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::ProcessQueuedPostDecodeParameterSettings( void )
{
    return FrameParserNoError;
}


////////////////////////////////////////////////////////////////////////////
///
/// Determine the display frame index and presentation time of the decoded frame.
///
/// For LPCM audio these can be determined immediately (although it the first
/// frame for decode does not contain a PTS we must synthesize one).
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::GeneratePostDecodeParameterSettings( void )
{
FrameParserStatus_t Status;

//
    
    //
    // Default setting
    //

    ParsedFrameParameters->DisplayFrameIndex            = INVALID_INDEX;
    ParsedFrameParameters->NativePlaybackTime           = INVALID_TIME;
    ParsedFrameParameters->NormalizedPlaybackTime       = INVALID_TIME;
    ParsedFrameParameters->NativeDecodeTime             = INVALID_TIME;
    ParsedFrameParameters->NormalizedDecodeTime         = INVALID_TIME;

    //
    // Record in the structure the decode and presentation times if specified
    //
//    FRAME_ERROR("POSTDECODE: PlaybackTimeValid-%d DecodeTimeValid-%d \n", (long int) CodedFrameParameters->PlaybackTimeValid, (long int) CodedFrameParameters->DecodeTimeValid);

    if( CodedFrameParameters->PlaybackTimeValid )
    {
	ParsedFrameParameters->NativePlaybackTime       = CodedFrameParameters->PlaybackTime;
	TranslatePlaybackTimeNativeToNormalized( CodedFrameParameters->PlaybackTime, &ParsedFrameParameters->NormalizedPlaybackTime );

//        FRAME_ERROR("READHEADERS: NativePlaybackTime-%ld NormalizedPlaybackTime-%ld\n", (long int) ParsedFrameParameters->NativePlaybackTime, (long int) ParsedFrameParameters->NormalizedPlaybackTime);
    }

    if( CodedFrameParameters->DecodeTimeValid )
    {
	ParsedFrameParameters->NativeDecodeTime         = CodedFrameParameters->DecodeTime;
	TranslatePlaybackTimeNativeToNormalized( CodedFrameParameters->DecodeTime, &ParsedFrameParameters->NormalizedDecodeTime );

//        FRAME_ERROR("READHEADERS: NativeDecodeTime-%ld NormalizedDecodeTime-%dl\n", (long int) ParsedFrameParameters->NativeDecodeTime, (long int) ParsedFrameParameters->NormalizedDecodeTime);
    }

    //
    // Sythesize the presentation time if required
    //

    
    Status = HandleCurrentFrameNormalizedPlaybackTime();
    if( Status != FrameParserNoError )
    {
        FRAME_ERROR("POSTDECODE: HandleCurrentFrameNormalizedPlaybackTime failed \n");
    	return Status;
    }

    //
    // We can't fail after this point so this is a good time to provide a display frame index
    //
    
    ParsedFrameParameters->DisplayFrameIndex		 = NextDisplayFrameIndex++;

    //
    // Use the super-class utilities to complete our housekeeping chores
    //
    
    HandleUpdateStreamParameters();

    if(ParsedFrameHeader.SamplingFrequency==96000) {
        //we'll send only 10 subframes in 96K to reduce latency
        GenerateNextFrameNormalizedPlaybackTime((ParsedFrameHeader.NumberOfSamples*10),
                                                    ParsedFrameHeader.SamplingFrequency);
    } else {
        //in 48K all 20 subframes will be fine
        GenerateNextFrameNormalizedPlaybackTime((ParsedFrameHeader.NumberOfSamples*20),
                                                    ParsedFrameHeader.SamplingFrequency);
    }

    //DumpParsedFrameParameters( ParsedFrameParameters, __PRETTY_FUNCTION__ );
    return FrameParserNoError;
}


////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::PrepareReferenceFrameList()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::PrepareReferenceFrameList( void )
{
    return FrameParserNoError;
}


////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::PrepareReferenceFrameList()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::UpdateReferenceFrameList( void )
{
    return FrameParserNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::ProcessReverseDecodeUnsatisfiedReferenceStack()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::ProcessReverseDecodeUnsatisfiedReferenceStack( void )
{
    return FrameParserNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::ProcessReverseDecodeStack()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::ProcessReverseDecodeStack(			void )
{
    return FrameParserNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::PurgeReverseDecodeUnsatisfiedReferenceStack()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::PurgeReverseDecodeUnsatisfiedReferenceStack(	void )
{
	
    return FrameParserNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::PurgeReverseDecodeStack()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::PurgeReverseDecodeStack(			void )
{
    return FrameParserNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Not required or implemented for LPCM audio.
///
/// \copydoc FrameParser_Audio_c::TestForTrickModeFrameDrop()
///
FrameParserStatus_t   FrameParser_AudioLpcm_c::TestForTrickModeFrameDrop(			void )
{
    return FrameParserNoError;
}


