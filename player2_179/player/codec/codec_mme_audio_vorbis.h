/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_vorbis.h
Author :           Adam

Definition of the stream specific codec implementation for Real Media Audio in player 2


Date        Modification                                    Name
----        ------------                                    --------
28-Jan-09   Created                                         Julian

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_VORBIS
#define H_CODEC_MME_AUDIO_VORBIS

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_audio.h"
#include "codec_mme_audio_stream.h"
#include "vorbis_audio.h"

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


class Codec_MmeAudioVorbis_c : public Codec_MmeAudioStream_c
{
protected:

    // Data

    // Functions

public:

    // Constructor/Destructor methods

    Codec_MmeAudioVorbis_c(                void );
    ~Codec_MmeAudioVorbis_c(               void );

    // Stream specific functions

protected:

    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams );

#if 0
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );


    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   FillOutSendBuffersCommand(                  void );
    CodecStatus_t   FillOutDecodeContext(                       void );

    CodecStatus_t   SendMMEDecodeCommand(                       void );

    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void    *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void    *Parameters );
    void            CallbackFromMME(                            MME_Event_t     Event,
                                                                MME_Command_t  *CallbackData);
#endif

};
#endif
