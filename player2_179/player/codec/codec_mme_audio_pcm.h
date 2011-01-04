/************************************************************************
COPYRIGHT (C) STMicroelectronics 2009

Source file name : codec_mme_audio_pcm.h
Author :           Julian

Definition of the stream specific codec implementation for pcm audio in player 2


Date        Modification                                    Name
----        ------------                                    --------
13-Aug-09   Created                                         Julian

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_PCM
#define H_CODEC_MME_AUDIO_PCM

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "Pcm_TranscoderTypes.h"
#include "codec_mme_audio.h"
#include "pcm_audio.h"

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

class Codec_MmeAudioPcm_c : public Codec_MmeAudio_c
{
protected:

    // Data

    eAccBoolean         RestartTransformer;

    // Functions

public:

    // Constructor/Destructor methods
    Codec_MmeAudioPcm_c(            void );
    ~Codec_MmeAudioPcm_c(           void );

    // Extension to base functions
    CodecStatus_t   HandleCapabilities(         void );

protected:

    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams );
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );
    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void    *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void    *Parameters );

};
#endif /* H_CODEC_MME_AUDIO_PCM */
