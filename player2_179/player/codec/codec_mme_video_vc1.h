/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : codec_mme_video_vc1.h
Author :           Mark C

Definition of the stream specific codec implementation for VC-1 video in player 2


Date        Modification                                    Name
----        ------------                                    --------
06-Mar-07   Created                                         Mark C

************************************************************************/

#ifndef H_CODEC_MME_VIDEO_VC1
#define H_CODEC_MME_VIDEO_VC1

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video.h"

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

/// The VC1 video codec proxy.
class Codec_MmeVideoVc1_c : public Codec_MmeVideo_c
{
private:

    // Data

    VC9_TransformerCapability_t         Vc1TransformCapability;
    VC9_InitTransformerParam_fmw_t      Vc1InitializationParameters;

    BufferDataDescriptor_t*             Vc1MbStructDescriptor;
    BufferType_t                        Vc1MbStructType;
    BufferPool_t                        Vc1MbStructPool;

    // Functions

    void                SaveIntensityCompensationData          (unsigned int            RefBufferIndex,
                                                                VC9_IntensityComp_t*    Intensity);
    void                GetForwardIntensityCompensationData    (unsigned int            RefBufferIndex,
                                                                VC9_IntensityComp_t*    Intensity);
    void                GetBackwardIntensityCompensationData   (unsigned int            RefBufferIndex,
                                                                VC9_IntensityComp_t*    Intensity);

public:

    //
    // Constructor/Destructor methods
    //

    Codec_MmeVideoVc1_c(                void );
    ~Codec_MmeVideoVc1_c(               void );

    //
    // Stream specific functions
    //

protected:

    CodecStatus_t   Reset( void );
    CodecStatus_t   HandleCapabilities( void );
    CodecStatus_t   RegisterOutputBufferRing(   Ring_t                    Ring );
    CodecStatus_t   FillOutTransformerInitializationParameters( void );
    CodecStatus_t   FillOutSetStreamParametersCommand( void );
    CodecStatus_t   FillOutDecodeCommand(       void );
    CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    CodecStatus_t   DumpSetStreamParameters(                    void    *Parameters );
    CodecStatus_t   DumpDecodeParameters(                       void    *Parameters );
    CodecStatus_t   CheckCodecReturnParameters( CodecBaseDecodeContext_t *Context );
};
#endif
