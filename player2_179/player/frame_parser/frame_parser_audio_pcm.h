/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : frame_parser_audio_pcm.h
Author :

Definition of the frame parser PCM Audio class implementation for player 2.


Date        Modification                                Name
----        ------------                                --------
12-Aug-09   Created                                     Julian

************************************************************************/

#ifndef H_FRAME_PARSER_AUDIO_PCM
#define H_FRAME_PARSER_AUDIO_PCM

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "pcm_audio.h"
#include "frame_parser_audio.h"


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/// Frame parser for Ogg Pcm audio
class FrameParser_AudioPcm_c : public FrameParser_Audio_c
{
private:

    // Data

    PcmAudioParsedFrameHeader_t         ParsedFrameHeader;

    PcmAudioStreamParameters_t*         StreamParameters;
    PcmAudioStreamParameters_t          CurrentStreamParameters;
    PcmAudioFrameParameters_t*          FrameParameters;

    bool                                StreamDataValid;

    // Functions

    FrameParserStatus_t                 ReadStreamHeader(               void );

public:

    // Constructor function

    FrameParser_AudioPcm_c( void );
    ~FrameParser_AudioPcm_c( void );

    // Overrides for component base class functions

    FrameParserStatus_t   Reset(                                        void );

    // FrameParser class functions

    FrameParserStatus_t   RegisterOutputBufferRing(     Ring_t          Ring );

    // Stream specific functions

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

#endif

