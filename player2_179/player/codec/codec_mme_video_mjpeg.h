/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : codec_mme_video_mjpeg.h
Author :           Julian

Definition of the stream specific codec implementation for MJPEG video in player 2


Date        Modification                                    Name
----        ------------                                    --------
28-Jul-09   Created                                         Julian

************************************************************************/

#ifndef H_CODEC_MME_VIDEO_MJPEG
#define H_CODEC_MME_VIDEO_MJPEG

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video.h"
#include "JPEGDECHW_VideoTransformerTypes.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#define MJPEG_DEFAULT_WIDTH                     720
#define MJPEG_DEFAULT_HEIGHT                    576

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/// The MJPEG video codec proxy.
class Codec_MmeVideoMjpeg_c : public Codec_MmeVideo_c
{
private:

    // Data

    JPEGDECHW_VideoDecodeCapabilityParams_t     MjpegTransformCapability;
    JPEGDECHW_VideoDecodeInitParams_t           MjpegInitializationParameters;

    unsigned int                                DecodingWidth;
    unsigned int                                DecodingHeight;
    unsigned int                                MaxBytesPerFrame;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeVideoMjpeg_c(                void );
    ~Codec_MmeVideoMjpeg_c(               void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   Reset(                                      void );
    CodecStatus_t   HandleCapabilities(                         void );

    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand(          void );

    CodecStatus_t   FillOutDecodeCommand(                       void );

    CodecStatus_t   ValidateDecodeContext(                      CodecBaseDecodeContext_t       *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void                           *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void                           *Parameters ); 
};
#endif
