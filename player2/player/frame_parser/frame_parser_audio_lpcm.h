/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : frame_parser_audio_lpcm.h
Author :           Adam

Definition of the frame parser audio lpcm class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
10-Jul-07   Created (from frame_parser_audio_mpeg.h)        Adam

************************************************************************/

#ifndef H_FRAME_PARSER_AUDIO_LPCM
#define H_FRAME_PARSER_AUDIO_LPCM

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "lpcm_audio.h"
#include "frame_parser_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_AudioLpcm_c : public FrameParser_Audio_c
{
private:

    // Data
    
    LpcmAudioParsedFrameHeader_t ParsedFrameHeader;
    
    LpcmAudioStreamParameters_t	*StreamParameters;
    LpcmAudioStreamParameters_t CurrentStreamParameters;
    LpcmAudioFrameParameters_t *FrameParameters;

    // Functions

public:

    //
    // Constructor function
    //

    FrameParser_AudioLpcm_c( void );
    ~FrameParser_AudioLpcm_c( void );

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

    static FrameParserStatus_t ParseFrameHeader( unsigned char *FrameHeader, LpcmAudioParsedFrameHeader_t *ParsedFrameHeader );
};

#endif /* H_FRAME_PARSER_AUDIO_LPCM */

