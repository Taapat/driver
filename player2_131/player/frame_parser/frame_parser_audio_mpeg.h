/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : frame_parser_audio_mpeg.h
Author :           Daniel

Definition of the frame parser audio mpeg class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
30-Mar-07   Created (from frame_parser_video_mpeg2.h)       Daniel

************************************************************************/

#ifndef H_FRAME_PARSER_AUDIO_MPEG
#define H_FRAME_PARSER_AUDIO_MPEG

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "mpeg_audio.h"
#include "frame_parser_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_AudioMpeg_c : public FrameParser_Audio_c
{
private:

    // Data
    
    MpegAudioParsedFrameHeader_t ParsedFrameHeader;
    
    MpegAudioStreamParameters_t	*StreamParameters;
    MpegAudioStreamParameters_t CurrentStreamParameters;
    MpegAudioFrameParameters_t *FrameParameters;

    // Functions

public:

    //
    // Constructor function
    //

    FrameParser_AudioMpeg_c( void );
    ~FrameParser_AudioMpeg_c( void );

    //
    // Overrides for component base class functions
    //

    FrameParserStatus_t   Reset(		void );

    //
    // FrameParser class functions
    //

    FrameParserStatus_t   RegisterOutputBufferRing(	Ring_t		Ring );

    //
    // Stream specific functions
    //

    FrameParserStatus_t   ReadHeaders( 					void );
    FrameParserStatus_t   ResetReferenceFrameList(			void );
    FrameParserStatus_t   PurgeQueuedPostDecodeParameterSettings(	void );
    FrameParserStatus_t   PrepareReferenceFrameList(			void );
    FrameParserStatus_t   ProcessQueuedPostDecodeParameterSettings(	void );
    FrameParserStatus_t   GeneratePostDecodeParameterSettings(		void );
    FrameParserStatus_t   UpdateReferenceFrameList(			void );

    FrameParserStatus_t   ProcessReverseDecodeUnsatisfiedReferenceStack(void );
    FrameParserStatus_t   ProcessReverseDecodeStack(			void );
    FrameParserStatus_t   PurgeReverseDecodeUnsatisfiedReferenceStack(	void );
    FrameParserStatus_t   PurgeReverseDecodeStack(			void );
    FrameParserStatus_t   TestForTrickModeFrameDrop(			void );

    static FrameParserStatus_t ParseFrameHeader( unsigned char *FrameHeader, MpegAudioParsedFrameHeader_t *ParsedFrameHeader );
    static FrameParserStatus_t ParseExtensionHeader( unsigned char *ExtensionHeader, unsigned int *ExtensionLength );
};

#endif /* H_FRAME_PARSER_AUDIO_MPEG */

