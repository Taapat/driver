/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_video_avs.h
Author :           Nick

Definition of the stream specific codec implementation for avs video in player 2


Date        Modification                                    Name
----        ------------                                    --------
25-Jan-07   Created                                         Nick

************************************************************************/

#ifndef H_CODEC_MME_VIDEO_AVS
#define H_CODEC_MME_VIDEO_AVS

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#define AVS_NUM_MME_INPUT_BUFFERS                       3
#define AVS_NUM_MME_OUTPUT_BUFFERS                      0
#define AVS_NUM_MME_BUFFERS                             (AVS_NUM_MME_INPUT_BUFFERS+AVS_NUM_MME_OUTPUT_BUFFERS)

#define AVS_MME_CURRENT_FRAME_BUFFER                    0
#define AVS_MME_FORWARD_REFERENCE_FRAME_BUFFER          1
#define AVS_MME_BACKWARD_REFERENCE_FRAME_BUFFER         2

#define AVS_MAXIMUM_PICTURE_WIDTH                       1920
#define AVS_MAXIMUM_PICTURE_HEIGHT                      1088

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//


/// The AVS video codec proxy.
class Codec_MmeVideoAvs_c : public Codec_MmeVideo_c
{
protected:

    // Data

    unsigned int                                DecodingWidth;
    unsigned int                                DecodingHeight;

    MME_AVSVideoDecodeCapabilityParams_t        AvsTransformCapability;
    MME_AVSVideoDecodeInitParams_t              AvsInitializationParameters;

    allocator_device_t                          IntraMbStructMemoryDevice;
    allocator_device_t                          MbStructMemoryDevice;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeVideoAvs_c(              void );
    ~Codec_MmeVideoAvs_c(             void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   Reset(                                      void );
    CodecStatus_t   HandleCapabilities(                         void );

    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );
    CodecStatus_t   FillOutDecodeCommand(                       void );
    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void    *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void    *Parameters );
};
#endif
