/************************************************************************
COPYRIGHT (C) ST Microelectronics 2007

Source file name : codec_mme_audio_spdifin.h
Author :           Gael Lassure

Definition of the stream specific codec implementation for SPDIF Input


Date        Modification                                    Name
----        ------------                                    --------
24-May-07   Created (from codec_mme_audio_spdifin.h)       Gael Lassure

************************************************************************/

#ifndef H_CODEC_MME_AUDIO_SPDIFIN
#define H_CODEC_MME_AUDIO_SPDIFIN

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//
typedef struct Codec_SpdifinEof_s
{
    MME_Command_t             Command;
    MME_DataBuffer_t          DataBuffer;
    MME_DataBuffer_t        * DataBuffers[1];
    MME_ScatterPage_t         ScatterPage;
    MME_StreamingBufferParams_t Params;
    bool                      SentEOFCommand;
} Codec_SpdifinEOF_t;

typedef struct Codec_SpdifinStatus_s
{
    enum eMulticomSpdifinState State;
    enum eMulticomSpdifinPC    StreamType;
    U32                        PlayedSamples;
} Codec_SpdifinStatus_t;

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Codec_MmeAudioSpdifin_c : public Codec_MmeAudio_c
{
protected:

    // Data    
    eAccDecoderId              DecoderId;

    // AutoDetect SPDIFIN decoding Status
    Codec_SpdifinStatus_t      SpdifStatus;

    // Memory allocation for sending an EOF command.
    Codec_SpdifinEOF_t         EOF;

    // Externally useful information
    int                                 DecodeErrors;
    unsigned long long int              NumberOfSamplesProcessed;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeAudioSpdifin_c(            void );
    ~Codec_MmeAudioSpdifin_c(           void );

    //
    // Override Base Component class method
    //

    CodecStatus_t   Reset(      void );
    
    CodecStatus_t   CreateAttributeEvents(                      void );
    CodecStatus_t   GetAttribute(                               const char                     *Attribute,
								PlayerAttributeDescriptor_t    *Value );
    CodecStatus_t   SetAttribute(                               const char                     *Attribute,
								PlayerAttributeDescriptor_t    *Value );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   FillOutTransformerGlobalParameters        ( MME_LxAudioDecoderGlobalParams_t *GlobalParams );
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );
    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   FillOutSendBufferCommand(                   void );
    CodecStatus_t   ValidateDecodeContext(                      CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void                     *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void                     *Parameters );
    CodecStatus_t   TerminateMMETransformer(                    void );
    void            SetCommandIO(void);
    
    CodecStatus_t   SendEofCommand(                             void );
    CodecStatus_t   DiscardQueuedDecodes(                       void );
};
#endif //H_CODEC_MME_AUDIO_SPDIFIN
