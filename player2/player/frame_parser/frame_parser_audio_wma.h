/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : frame_parser_audio_wma.h
Author :           Adam

Definition of the frame parser audio wma class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created (from frame_parser_audio_mpeg.h)        Adam

************************************************************************/

#ifndef H_FRAME_PARSER_AUDIO_WMA
#define H_FRAME_PARSER_AUDIO_WMA

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "wma_audio.h"
#include "frame_parser_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_AudioWma_c : public FrameParser_Audio_c
{
private:

    // Data

    /*WmaAudioParsedFrameHeader_t ParsedFrameHeader;*/
    WmaAudioStreamParameters_t*         StreamParameters;
    WmaAudioFrameParameters_t*          FrameParameters;

    WmaAudioStreamParameters_t          CurrentStreamParameters;
    // Functions

    FrameParserStatus_t         ReadStreamParameters   (unsigned char*                  FrameHeaderBytes,
							WmaAudioStreamParameters_t*     StreamParameters);
public:

    //
    // Constructor function
    //

    FrameParser_AudioWma_c( void );
    ~FrameParser_AudioWma_c( void );

    //
    // Overrides for component base class functions
    //

    FrameParserStatus_t   Reset(                void );

    //
    // FrameParser class functions
    //

    FrameParserStatus_t   RegisterOutputBufferRing(     Ring_t          Ring );

    //
    // Stream specific functions
    //

    FrameParserStatus_t   ReadHeaders(                                  void );
    FrameParserStatus_t   ResetReferenceFrameList(                      void );
    FrameParserStatus_t   PurgeQueuedPostDecodeParameterSettings(       void );
    FrameParserStatus_t   PrepareReferenceFrameList(                    void );
    FrameParserStatus_t   ProcessQueuedPostDecodeParameterSettings(     void );
    FrameParserStatus_t   GeneratePostDecodeParameterSettings(          void );
    FrameParserStatus_t   UpdateReferenceFrameList(                     void );

    FrameParserStatus_t   ProcessReverseDecodeUnsatisfiedReferenceStack(void );
    FrameParserStatus_t   ProcessReverseDecodeStack(                    void );
    FrameParserStatus_t   PurgeReverseDecodeUnsatisfiedReferenceStack(  void );
    FrameParserStatus_t   PurgeReverseDecodeStack(                      void );
    FrameParserStatus_t   TestForTrickModeFrameDrop(                    void );

};

#endif /* H_FRAME_PARSER_AUDIO_WMA */

