/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : codec_mme_audio_mlp.h
Author :           Sylvain

Definition of the stream specific codec implementation for mlp audio in player 2


Date        Modification                                    Name
----        ------------                                    --------
11-Oct-07   Created                                         Sylvain

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_MLP
#define H_CODEC_MME_AUDIO_MLP

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "codec_mme_audio.h"

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

class Codec_MmeAudioMlp_c : public Codec_MmeAudio_c
{
protected:

    // Data
    
    eAccDecoderId DecoderId;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeAudioMlp_c(		void );
    ~Codec_MmeAudioMlp_c(		void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams );
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand( 		void );
    CodecStatus_t   FillOutDecodeCommand(       		void );
    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters( 			void	*Parameters );
    CodecStatus_t   DumpDecodeParameters( 			void	*Parameters );
};
#endif
