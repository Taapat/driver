//{{{  Title
/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : codec_mme_video_mjpeg.cpp
Author :           Julian

Implementation of the Sorenson H263 video codec class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
28-May-08   Created                                         Julian

************************************************************************/
//}}}
// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_video_mjpeg.h"
#include "mjpeg.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

//{{{  Locally defined structures
typedef struct MjpegCodecStreamParameterContext_s
{
    CodecBaseStreamParameterContext_t   BaseContext;
} MjpegCodecStreamParameterContext_t;

typedef struct MjpegCodecDecodeContext_s
{
    CodecBaseDecodeContext_t            BaseContext;
    JPEGDECHW_VideoDecodeParams_t       DecodeParameters;
    JPEGDECHW_VideoDecodeReturnParams_t DecodeStatus;
} MjpegCodecDecodeContext_t;

#define BUFFER_MJPEG_CODEC_STREAM_PARAMETER_CONTEXT               "MjpegCodecStreamParameterContext"
#define BUFFER_MJPEG_CODEC_STREAM_PARAMETER_CONTEXT_TYPE {BUFFER_MJPEG_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(MjpegCodecStreamParameterContext_t)}
static BufferDataDescriptor_t                   MjpegCodecStreamParameterContextDescriptor = BUFFER_MJPEG_CODEC_STREAM_PARAMETER_CONTEXT_TYPE;

#define BUFFER_MJPEG_CODEC_DECODE_CONTEXT        "MjpegCodecDecodeContext"
#define BUFFER_MJPEG_CODEC_DECODE_CONTEXT_TYPE   {BUFFER_MJPEG_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(MjpegCodecDecodeContext_t)}
static BufferDataDescriptor_t                   MjpegCodecDecodeContextDescriptor        = BUFFER_MJPEG_CODEC_DECODE_CONTEXT_TYPE;


//}}}

//{{{  Constructor
// /////////////////////////////////////////////////////////////////////////
//
//      Cosntructor function, fills in the codec specific parameter values
//

Codec_MmeVideoMjpeg_c::Codec_MmeVideoMjpeg_c( void )
{
    Configuration.CodecName                             = "Mjpeg video";

    Configuration.DecodeOutputFormat                    = FormatVideo420_MacroBlock;

    Configuration.StreamParameterContextCount           = 1;
    Configuration.StreamParameterContextDescriptor      = &MjpegCodecStreamParameterContextDescriptor;

    Configuration.DecodeContextCount                    = 4;
    Configuration.DecodeContextDescriptor               = &MjpegCodecDecodeContextDescriptor;

    Configuration.MaxDecodeIndicesPerBuffer             = 2;
    Configuration.SliceDecodePermitted                  = false;
    Configuration.DecimatedDecodePermitted              = false;

    Configuration.TransformName[0]                      = JPEGHWDEC_MME_TRANSFORMER_NAME "0";
    Configuration.TransformName[1]                      = JPEGHWDEC_MME_TRANSFORMER_NAME "1";
    Configuration.AvailableTransformers                 = 2;

    Configuration.SizeOfTransformCapabilityStructure    = sizeof(JPEGDECHW_VideoDecodeCapabilityParams_t);
    Configuration.TransformCapabilityStructurePointer   = (void *)(&MjpegTransformCapability);

    Configuration.AddressingMode                        = PhysicalAddress;

    // We do not need the coded data after decode is complete
    Configuration.ShrinkCodedDataBuffersAfterDecode     = true;

    DecodingWidth                                       = MJPEG_DEFAULT_WIDTH;
    DecodingHeight                                      = MJPEG_DEFAULT_HEIGHT;

    Reset();
}
//}}}
//{{{  Destructor
// /////////////////////////////////////////////////////////////////////////
//
//      Destructor function, ensures a full halt and reset 
//      are executed for all levels of the class.
//

Codec_MmeVideoMjpeg_c::~Codec_MmeVideoMjpeg_c( void )
{
    Halt();
    Reset();
}
//}}}
//{{{  Reset
// /////////////////////////////////////////////////////////////////////////
//
//      Reset function for Mjpeg specific members.
//      
//

CodecStatus_t   Codec_MmeVideoMjpeg_c::Reset( void )
{
    return Codec_MmeVideo_c::Reset();
}
//}}}

//{{{  HandleCapabilities
// /////////////////////////////////////////////////////////////////////////
//
//      Function to deal with the returned capabilities
//      structure for an Mjpeg mme transformer.
//
CodecStatus_t   Codec_MmeVideoMjpeg_c::HandleCapabilities( void )
{
    CODEC_TRACE("MME Transformer '%s' capabilities are :-\n", JPEGHWDEC_MME_TRANSFORMER_NAME);
    CODEC_TRACE("    Api version %x\n", MjpegTransformCapability.api_version);

    return CodecNoError;
}
//}}}
//{{{  FillOutTransformerInitializationParameters
// /////////////////////////////////////////////////////////////////////////
//
//      Function to deal with the returned capabilities
//      structure for an Mjpeg mme transformer.
//
CodecStatus_t   Codec_MmeVideoMjpeg_c::FillOutTransformerInitializationParameters( void )
{
    // Fillout the command parameters
    MjpegInitializationParameters.CircularBufferBeginAddr_p     = (U32*)0x00000000;
    MjpegInitializationParameters.CircularBufferEndAddr_p       = (U32*)0xffffffff;

    // Fillout the actual command
    MMEInitializationParameters.TransformerInitParamsSize       = sizeof(JPEGDECHW_VideoDecodeInitParams_t);
    MMEInitializationParameters.TransformerInitParams_p         = (MME_GenericParams_t)(&MjpegInitializationParameters);

    return CodecNoError;
}
//}}}
//{{{  FillOutSetStreamParametersCommand
// /////////////////////////////////////////////////////////////////////////
//
//      Function to fill out the stream parameters
//      structure for an Mjpeg mme transformer.
//
CodecStatus_t   Codec_MmeVideoMjpeg_c::FillOutSetStreamParametersCommand( void )
{
    CODEC_ERROR ("%s - This should not be called as we have no stream parameters\n");
    return CodecError;
}
//}}}
//{{{  FillOutDecodeCommand
// /////////////////////////////////////////////////////////////////////////
//
//      Function to fill out the decode parameters 
//      structure for an avs mme transformer.
//

CodecStatus_t   Codec_MmeVideoMjpeg_c::FillOutDecodeCommand(       void )
{
    MjpegCodecDecodeContext_t*          Context         = (MjpegCodecDecodeContext_t *)DecodeContext;
    //MjpegFrameParameters_t*             Parsed          = (MjpegFrameParameters_t *)ParsedFrameParameters->FrameParameterStructure;
    //MjpegVideoPictureHeader_t*          PictureHeader   = &Parsed->PictureHeader;
    JPEGDECHW_VideoDecodeParams_t*      Param;
    JPEGDECHW_DecodedBufferAddress_t*   Decode;

    CODEC_DEBUG("%s\n", __FUNCTION__);

    // For Mjpeg we do not do slice decodes.
    KnownLastSliceInFieldFrame                  = true;

    Param                                       = &Context->DecodeParameters;
    Decode                                      = &Param->DecodedBufferAddr;

    Param->PictureStartAddr_p                   = (JPEGDECHW_CompressedData_t)CodedData;
    Param->PictureEndAddr_p                     = (JPEGDECHW_CompressedData_t)(CodedData + CodedDataLength + 128);

    Decode->Luma_p                              = (JPEGDECHW_LumaAddress_t)BufferState[CurrentDecodeBufferIndex].BufferLumaPointer;
    Decode->Chroma_p                            = (JPEGDECHW_ChromaAddress_t)BufferState[CurrentDecodeBufferIndex].BufferChromaPointer;
    Decode->LumaDecimated_p                     = NULL;
    Decode->ChromaDecimated_p                   = NULL;

    //{{{  Initialise decode buffers to bright pink
    #if 0
    unsigned int        LumaSize        = DecodingWidth*DecodingHeight;
    unsigned char*      LumaBuffer;
    unsigned char*      ChromaBuffer;
    CurrentDecodeBuffer->ObtainDataReference( NULL, NULL, (void**)&LumaBuffer, UnCachedAddress);
    ChromaBuffer                        = LumaBuffer+LumaSize;
    memset (LumaBuffer,   0xff, LumaSize);
    memset (ChromaBuffer, 0xff, LumaSize/2);
    #endif
    //}}}

    //{{{  Fill in remaining fields
    Param->MainAuxEnable                                                = JPEGDECHW_MAINOUT_EN;
    Param->HorizontalDecimationFactor                                   = JPEGDECHW_HDEC_1;
    Param->VerticalDecimationFactor                                     = JPEGDECHW_VDEC_1;

    Param->xvalue0                                                      = 0;
    Param->xvalue1                                                      = 0;
    Param->yvalue0                                                      = 0;
    Param->yvalue1                                                      = 0;
    Param->DecodingMode                                                 = JPEGDECHW_NORMAL_DECODE;
    Param->AdditionalFlags                                              = JPEGDECHW_ADDITIONAL_FLAG_NONE;
    //}}}

    // Fillout the actual command
    memset( &Context->BaseContext.MMECommand, 0x00, sizeof(MME_Command_t) );

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize        = sizeof(JPEGDECHW_VideoDecodeReturnParams_t);
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p          = (MME_GenericParams_t)(&Context->DecodeStatus);
    Context->BaseContext.MMECommand.ParamSize                           = sizeof(JPEGDECHW_VideoDecodeParams_t);
    Context->BaseContext.MMECommand.Param_p                             = (MME_GenericParams_t)(&Context->DecodeParameters);

    DecodeContext->MMECommand.NumberInputBuffers                        = 0;
    DecodeContext->MMECommand.NumberOutputBuffers                       = 0;

    DecodeContext->MMECommand.DataBuffers_p                             = (MME_DataBuffer_t**)DecodeContext->MMEBufferList;

    return CodecNoError;
}
//}}}
//{{{  ValidateDecodeContext
////////////////////////////////////////////////////////////////////////////
///
/// Unconditionally return success.
/// 
/// Success and failure codes are located entirely in the generic MME structures
/// allowing the super-class to determine whether the decode was successful. This
/// means that we have no work to do here.
///
/// \return CodecNoError
///
CodecStatus_t   Codec_MmeVideoMjpeg_c::ValidateDecodeContext( CodecBaseDecodeContext_t *Context )
{
    MjpegCodecDecodeContext_t*          MjpegContext    = (MjpegCodecDecodeContext_t *)Context;

    //CODEC_TRACE("%s\n", __FUNCTION__);
    //CODEC_TRACE("    pm_cycles            %d\n", MjpegContext->DecodeStatus.pm_cycles);
    //CODEC_TRACE("    pm_dmiss             %d\n", MjpegContext->DecodeStatus.pm_dmiss);
    //CODEC_TRACE("    pm_imiss             %d\n", MjpegContext->DecodeStatus.pm_imiss);
    //CODEC_TRACE("    pm_bundles           %d\n", MjpegContext->DecodeStatus.pm_bundles);
    //CODEC_TRACE("    pm_pft               %d\n", MjpegContext->DecodeStatus.pm_pft);
   // CODEC_DEBUG("%s: DecodeTimeInMicros   %d\n", __FUNCTION__, MjpegContext->DecodeStatus.DecodeTimeInMicros);



    return CodecNoError;
}
//}}}
//{{{  DumpSetStreamParameters
// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the set stream
//      parameters from an mme command.
//
CodecStatus_t   Codec_MmeVideoMjpeg_c::DumpSetStreamParameters(         void    *Parameters )
{
    return CodecNoError;
}
//}}}
//{{{  DumpDecodeParameters
// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the decode
//      parameters from an mme command.
//

unsigned int MjpegStaticPicture;
CodecStatus_t   Codec_MmeVideoMjpeg_c::DumpDecodeParameters(            void    *Parameters )
{
    JPEGDECHW_VideoDecodeParams_t*              FrameParams;

    FrameParams     = (JPEGDECHW_VideoDecodeParams_t *)Parameters;

    MjpegStaticPicture++;

    CODEC_DEBUG ("********** Picture %d *********\n", MjpegStaticPicture-1);

    return CodecNoError;
}
//}}}


