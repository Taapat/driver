/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_video_mpeg2.h
Author :           Nick

Definition of the stream specific codec implementation for mpeg2 video in player 2


Date        Modification                                    Name
----        ------------                                    --------
25-Jan-07   Created                                         Nick

************************************************************************/

#ifndef H_CODEC_MME_VIDEO_DIVX
#define H_CODEC_MME_VIDEO_DIVX

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video.h"
#include "mpeg4.h"
#include "mme.h"

#include "VideoCompanion.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
#define NUM_MME_DIVX_BUFFERS 6
#define NUM_MME_INPUT_BUFFERS 3
#define NUM_MME_OUTPUT_BUFFERS 3

#define Divx_TransformerCapability_t MME_DivXVideoDecodeCapabilityParams_t
#define Divx_InitTransformerParam_t  MME_DivXVideoDecodeInitParams_t
// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//
// /////////////////////////////////////////////////////////////////////////

class Codec_MmeVideoDivx_c : public Codec_MmeVideo_c
{
protected:

	// Data
	MME_DivXVideoDecodeCapabilityParams_t DivxTransformCapability;
	Divx_InitTransformerParam_t           DivxInitializationParameters;

	MME_DivXVideoDecodeReturnParams_t ReturnParams;
	MME_DivXVideoDecodeParams_t       AdditionalParams;     

	BufferDataDescriptor_t  *DivxRasterStructDescriptor;
	BufferType_t             DivxRasterStructType;
	BufferPool_t             DivxRasterStructPool;  

	unsigned int CurrentWidth;
	unsigned int CurrentHeight;
	unsigned int CurrentVersion;

	unsigned int MaxBytesPerFrame;

	bool restartTransformer;
    
	bool DivxFirmware;
    
public:

	// Constructor/Destructor methods

	 Codec_MmeVideoDivx_c( void );
	~Codec_MmeVideoDivx_c( void );

	// Stream specific functions

protected:

	// Functions
	CodecStatus_t   HandleCapabilities( void );
	CodecStatus_t   Reset( void );

	CodecStatus_t   FillOutTransformerInitializationParameters( void );
	CodecStatus_t   FillOutSetStreamParametersCommand( void );
	CodecStatus_t   FillOutDecodeCommand( void );
    
	CodecStatus_t   ValidateDecodeContext( CodecBaseDecodeContext_t *Context );
    
	CodecStatus_t   DumpSetStreamParameters( void *Parameters );
	CodecStatus_t   DumpDecodeParameters( void *Parameters );

    CodecStatus_t   InitializeMMETransformer( void );
	CodecStatus_t   SendMMEStreamParameters( void );
    
};
#endif
