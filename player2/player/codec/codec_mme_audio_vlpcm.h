/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_lpcm.h
Author :           Sylvain Barge

Definition of the stream specific codec implementation for mpeg audio in player 2


Date        Modification                                    Name
----        ------------                                    --------
18-Jul-07   Created                                         Sylvain Barge

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_LPCM
#define H_CODEC_MME_AUDIO_LPCM

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

class Codec_MmeAudioLpcm_c : public Codec_MmeAudio_c
{
protected:

    // Data
    
    eAccDecoderId DecoderId;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeAudioLpcm_c(		void );
    ~Codec_MmeAudioLpcm_c(		void );

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
#endif //H_CODEC_MME_AUDIO_LPCM
