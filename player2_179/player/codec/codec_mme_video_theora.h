/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : codec_mme_video_theora.h
Author :           Julian

Definition of the stream specific codec implementation for theora video in player 2


Date        Modification                                    Name
----        ------------                                    --------
10-Mar-09   Created                                         Julian

************************************************************************/

#ifndef H_CODEC_MME_VIDEO_THEORA
#define H_CODEC_MME_VIDEO_THEORA

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video.h"
#include "TheoraDecode_interface.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#define THEORA_NUM_MME_INPUT_BUFFERS            3
#define THEORA_NUM_MME_OUTPUT_BUFFERS           1
#define THEORA_NUM_MME_BUFFERS                  (THEORA_NUM_MME_INPUT_BUFFERS+THEORA_NUM_MME_OUTPUT_BUFFERS)

#define THEORA_MME_CODED_DATA_BUFFER            0
#define THEORA_MME_CURRENT_FRAME_BUFFER         1
#define THEORA_MME_REFERENCE_FRAME_BUFFER       2
#define THEORA_MME_GOLDEN_FRAME_BUFFER          3

#define THEORA_DEFAULT_PICTURE_WIDTH            320
#define THEORA_DEFAULT_PICTURE_HEIGHT           240

#define THEORA_BUFFER_SIZE                      0x200000

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/// The Theora video codec proxy.
class Codec_MmeVideoTheora_c : public Codec_MmeVideo_c
{
private:

    // Data

    unsigned int                        CodedWidth;
    unsigned int                        CodedHeight;

    THEORA_InitTransformerParam_t       TheoraInitializationParameters;
    THEORA_CapabilityParams_t           TheoraTransformCapability;

#if (THEORADEC_MME_VERSION >= 20)
    allocator_device_t                  InfoHeaderMemoryDevice;
    allocator_device_t                  CommentHeaderMemoryDevice;
    allocator_device_t                  SetupHeaderMemoryDevice;
    allocator_device_t                  BufferMemoryDevice;
#endif

    bool                                RestartTransformer;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeVideoTheora_c(             void );
    ~Codec_MmeVideoTheora_c(            void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   Reset(                                      void );
    CodecStatus_t   HandleCapabilities(                         void );
    CodecStatus_t   InitializeMMETransformer(                   void );

    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );
    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   FillOutDecodeBufferRequest(                 BufferStructure_t              *Request );

    CodecStatus_t   ValidateDecodeContext(                      CodecBaseDecodeContext_t       *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void                           *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void                           *Parameters );

    CodecStatus_t   SendMMEStreamParameters(                    void );


};
#endif
