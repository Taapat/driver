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
#include "codec_mme_audio_stream.h"
#include "wma_audio.h"

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

class Codec_MmeAudioWma_c : public Codec_MmeAudioStream_c
{

private:


protected:

    // Data

    // Functions

public:

    // Constructor/Destructor methods
    Codec_MmeAudioWma_c(                void );
    ~Codec_MmeAudioWma_c(               void );

protected:

    // Stream specific functions
    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams );

};
#endif
