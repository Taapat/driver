/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_lpcm.cpp
Author :           Adam

Implementation of the lpcm audio codec class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
10-Jul-07   Created (from codec_mme_audio_mpeg.cpp)         Adam

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Codec_MmeAudioLpcm_c
///
/// The LPCM audio codec proxy.
///

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "codec_mme_audio_lpcm.h"
#include "lpcm_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

typedef struct LpcmAudioCodecStreamParameterContext_s
{
    CodecBaseStreamParameterContext_t	BaseContext;

    MME_LxAudioDecoderGlobalParams_t StreamParameters;
} LpcmAudioCodecStreamParameterContext_t;

//#if __KERNEL__
#if 0
#define BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT		"LpcmAudioCodecStreamParameterContext"
#define BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE	{BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromDeviceMemory, 32, 0, true, true, sizeof(LpcmAudioCodecStreamParameterContext_t)}
#else
#define BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT		"LpcmAudioCodecStreamParameterContext"
#define BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE	{BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(LpcmAudioCodecStreamParameterContext_t)}
#endif

static BufferDataDescriptor_t		 LpcmAudioCodecStreamParameterContextDescriptor = BUFFER_LPCM_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE;

// --------

typedef struct LpcmAudioCodecDecodeContext_s
{
    CodecBaseDecodeContext_t		BaseContext;

    MME_LxAudioDecoderFrameParams_t     DecodeParameters;
    MME_LxAudioDecoderFrameStatus_t     DecodeStatus;
} LpcmAudioCodecDecodeContext_t;

//#if __KERNEL__
#if 0
#define BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT	"LpcmAudioCodecDecodeContext"
#define BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT_TYPE	{BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromDeviceMemory, 32, 0, true, true, sizeof(LpcmAudioCodecDecodeContext_t)}
#else
#define BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT	"LpcmAudioCodecDecodeContext"
#define BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT_TYPE	{BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(LpcmAudioCodecDecodeContext_t)}
#endif

static BufferDataDescriptor_t		 LpcmAudioCodecDecodeContextDescriptor = BUFFER_LPCM_AUDIO_CODEC_DECODE_CONTEXT_TYPE;



////////////////////////////////////////////////////////////////////////////
///
/// Fill in the configuration parameters used by the super-class and reset everything.
///
Codec_MmeAudioLpcm_c::Codec_MmeAudioLpcm_c( void )
{
    Configuration.CodecName				= "LPCM audio";

    Configuration.CodedFrameCount			= 256;
    Configuration.CodedMemorySize			= 0x150000;	
    Configuration.MaximumCodedFrameSize			= 0x040000;

    Configuration.StreamParameterContextCount		= 1;
    Configuration.StreamParameterContextDescriptor	= &LpcmAudioCodecStreamParameterContextDescriptor;

    Configuration.DecodeContextCount			= 4;
    Configuration.DecodeContextDescriptor		= &LpcmAudioCodecDecodeContextDescriptor;

//

    AudioDecoderTransformCapabilityMask.DecoderCapabilityFlags = (1 << ACC_LPCM);

    DecoderId                                           = ACC_LPCM_ID;
    StreamParams                                        = NULL;

    Reset();
}


////////////////////////////////////////////////////////////////////////////
///
/// 	Destructor function, ensures a full halt and reset 
///	are executed for all levels of the class.
///
Codec_MmeAudioLpcm_c::~Codec_MmeAudioLpcm_c( void )
{
    Halt();
    Reset();
}

////////////////////////////////////////////////////////////////////////////
///
/// Populate the supplied structure with parameters for LPCM audio.
///
///
CodecStatus_t Codec_MmeAudioLpcm_c::FillOutTransformerGlobalParameters( MME_LxAudioDecoderGlobalParams_t *GlobalParams_p )
{

//

    CODEC_TRACE("Initializing LPCM layer audio decoder\n");

//

    MME_LxAudioDecoderGlobalParams_t &GlobalParams = *GlobalParams_p;
    GlobalParams.StructSize = sizeof(MME_LxAudioDecoderGlobalParams_t);

//

    MME_LxLpcmConfig_t &Config = *((MME_LxLpcmConfig_t *) GlobalParams.DecConfig);
    Config.DecoderId = DecoderId;
    Config.StructSize = sizeof(Config);

    Config.Config[LPCM_MODE] = ACC_LPCM_VIDEO;
    Config.Config[LPCM_WS_CH_GR2] = ACC_LPCM_WS_NO_GR2;
    Config.Config[LPCM_FS_CH_GR2] = 0;
    Config.Config[LPCM_BIT_SHIFT_CH_GR2] = 0; // ???
    Config.Config[LPCM_MIXING_PHASE] = 0; // ???
//    Config.Config[LPCM_OUT_RESAMPLING] = ACC_LPCM_AUTO_RSPL;
    Config.Config[LPCM_OUT_RESAMPLING] = ACC_LPCM_RSPL_48;

  if(StreamParams != NULL) {
    Config.Config[LPCM_DRC_CODE]      = StreamParams->DynamicRangeControl;
    Config.Config[LPCM_DRC_ENABLE]    = (0x80 == StreamParams->DynamicRangeControl ? ACC_MME_FALSE : ACC_MME_TRUE);
    Config.Config[LPCM_MUTE_FLAG]     = StreamParams->AudioMuteFlag;
    Config.Config[LPCM_EMPHASIS_FLAG] = StreamParams->AudioEmphasisFlag;
    Config.Config[LPCM_NB_CHANNELS]   = StreamParams->NumberOfAudioChannels + 1; // what about dual-mono?
    Config.Config[LPCM_WS_CH_GR1]     = StreamParams->QuantizationWordLength;
    Config.Config[LPCM_FS_CH_GR1]     = StreamParams->AudioSamplingFrequency;
    Config.Config[LPCM_CHANNEL_ASSIGNMENT] = StreamParams->NumberOfAudioChannels;
    Config.Config[LPCM_NB_ACCESS_UNITS] = StreamParams->NumberOfFrameHeaders;
    Config.Config[LPCM_NB_SAMPLES]      = StreamParams->NumberOfFrameHeaders * LPCM_SAMPLES_PER_FRAME(StreamParams->AudioSamplingFrequency);
  }

    return Codec_MmeAudio_c::FillOutTransformerGlobalParameters( GlobalParams_p );
}


////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's initialization parameters for lpcm audio.
///
/// When this method completes Codec_MmeAudio_c::AudioDecoderInitializationParameters
/// will have been filled out with valid values sufficient to initialize an
/// LPCM audio decoder 
///
CodecStatus_t   Codec_MmeAudioLpcm_c::FillOutTransformerInitializationParameters( void )
{
CodecStatus_t Status;
MME_LxAudioDecoderInitParams_t &Params = AudioDecoderInitializationParameters;

//

    MMEInitializationParameters.TransformerInitParamsSize = sizeof(Params);
    MMEInitializationParameters.TransformerInitParams_p = &Params;

//

    Status = Codec_MmeAudio_c::FillOutTransformerInitializationParameters();
    if (Status != CodecNoError)
        return Status;

//

    return FillOutTransformerGlobalParameters( &Params.GlobalParams );
}


////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's MME_SET_GLOBAL_TRANSFORMER_PARAMS parameters for lpcm audio.
///
CodecStatus_t   Codec_MmeAudioLpcm_c::FillOutSetStreamParametersCommand( void )
{
CodecStatus_t Status;
LpcmAudioStreamParameters_t *Parsed = (LpcmAudioStreamParameters_t *)ParsedFrameParameters->StreamParameterStructure;
LpcmAudioCodecStreamParameterContext_t	*Context = (LpcmAudioCodecStreamParameterContext_t *)StreamParameterContext;
    
    //
    // Examine the parsed stream parameters and determine what type of codec to instanciate
    //
    
    DecoderId = ACC_LPCM_ID;
    
    StreamParams = Parsed;

    //
    // Now fill out the actual structure
    //     
    
    memset( &(Context->StreamParameters), 0, sizeof(Context->StreamParameters) );
    Status = FillOutTransformerGlobalParameters( &(Context->StreamParameters) );
    if( Status != CodecNoError )
        return Status;

    //
    // Fillout the actual command
    //

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize	= 0;
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p		= NULL;
    Context->BaseContext.MMECommand.ParamSize				= sizeof(Context->StreamParameters);
    Context->BaseContext.MMECommand.Param_p				= (MME_GenericParams_t)(&Context->StreamParameters);

//

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's MME_TRANSFORM parameters for LPCM audio.
///
CodecStatus_t   Codec_MmeAudioLpcm_c::FillOutDecodeCommand(       void )
{
LpcmAudioCodecDecodeContext_t	*Context 	= (LpcmAudioCodecDecodeContext_t *)DecodeContext;
//LpcmAudioFrameParameters_t	*Parsed		= (LpcmAudioFrameParameters_t *)ParsedFrameParameters->FrameParameterStructure;
    unsigned int tmp_buf_num;
    unsigned int tmp_sct_num;
    unsigned int tmp_sct_sz;

    //
    // Initialize the frame parameters (we don't actually have much to say here)
    //
    
    memset( &Context->DecodeParameters, 0, sizeof(Context->DecodeParameters) );

    //
    // Zero the reply structure
    //
    
    memset( &Context->DecodeStatus, 0, sizeof(Context->DecodeStatus) );

    //
    //Nibble off the 8-byte LPCM PES private data area
    //
/*
    CODEC_ERROR("NIBBLE OFF %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+1)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+2)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+3)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+4)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+5)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+6)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+7))
               );  
    CODEC_ERROR("LEAVE IN   %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+8)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+9)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+10)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+11)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+12)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+13)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+14)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+15))
               );  

    CODEC_ERROR("LEAVE IN   %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+16)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+17)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+18)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+19)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+20)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+21)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+22)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+23))
               );  
    CODEC_ERROR("LEAVE IN   %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+24)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+25)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+26)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+27)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+28)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+29)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+30)),
                 *((unsigned char *)((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p+31))
               );  
*/
/*
	tmp_buf_num = Context->BaseContext.MMECommand.NumberInputBuffers-1;
	tmp_sct_num = Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->NumberOfScatterPages-1;
	tmp_sct_sz  = Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[0].Size;

    CODEC_ERROR("LASTBUFF   %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-8))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-7))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-6))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-5))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-4))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-3))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-2))),
                 *((unsigned char *)((unsigned int) Context->BaseContext.MMECommand.DataBuffers_p[tmp_buf_num]->ScatterPages_p[tmp_sct_num].Page_p+(tmp_sct_sz-1)))
               );  
*/
    Context->BaseContext.MMECommand.DataBuffers_p[0]->TotalSize -= 8;
    Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p = 
         (void *) ((unsigned int)Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Page_p + 8);
    Context->BaseContext.MMECommand.DataBuffers_p[0]->ScatterPages_p[0].Size -= 8;




    //
    // Fillout the actual command
    //

    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize	= sizeof(Context->DecodeStatus);
    Context->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p		= (MME_GenericParams_t)(&Context->DecodeStatus);
    Context->BaseContext.MMECommand.ParamSize				= sizeof(Context->DecodeParameters);
    Context->BaseContext.MMECommand.Param_p				= (MME_GenericParams_t)(&Context->DecodeParameters);

    return CodecNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// Validate the ACC status structure and squawk loudly if problems are found.
/// 
/// Dispite the squawking this method unconditionally returns success. This is
/// because the firmware will already have concealed the decode problems by
/// performing a soft mute.
///
/// \return CodecSuccess
///
CodecStatus_t   Codec_MmeAudioLpcm_c::ValidateDecodeContext( CodecBaseDecodeContext_t *Context )
{
LpcmAudioCodecDecodeContext_t *DecodeContext = (LpcmAudioCodecDecodeContext_t *) Context;
MME_LxAudioDecoderFrameStatus_t &Status = DecodeContext->DecodeStatus;
ParsedAudioParameters_t *AudioParameters;


    CODEC_DEBUG(">><<\n");
 
    if (ENABLE_CODEC_DEBUG) {
        //DumpCommand(bufferIndex);
    }
    
    if (Status.DecStatus != 0) { //ACC_MPEG2_OK && Status.DecStatus != ACC_MPEG_MC_CRC_ERROR) {
        CODEC_ERROR("LPCM audio decode error %d\n", Status.DecStatus);
        //DumpCommand(bufferIndex);
        // don't report an error to the higher levels (because the frame is muted)
    }

//    CODEC_ERROR("LPCM VALIDATE NbOutSamples %d\n", Status.NbOutSamples);

/*
    if (Status.NbOutSamples != 1152 && Status.NbOutSamples != 576) { // TODO: manifest constant
        CODEC_ERROR("Unexpected number of output samples (%d)\n", Status.NbOutSamples);
        //DumpCommand(bufferIndex);
    }
*/

    // SYSFS
    AudioDecoderStatus = Status;

    //
    // Attach any codec derived metadata to the output buffer (or verify the
    // frame analysis if the frame analyser already filled everything in for
    // us).
    //

    AudioParameters = BufferState[DecodeContext->BaseContext.BufferIndex].ParsedAudioParameters;
    
    AudioParameters->Source.BitsPerSample = AudioOutputSurface->BitsPerSample;
    AudioParameters->Source.ChannelCount = AudioOutputSurface->ChannelCount;
    AudioParameters->Organisation = Status.AudioMode;
    
    // TODO: we should also validate the other audio parameters against the codec's reply

    return CodecNoError;
}

// /////////////////////////////////////////////////////////////////////////
//
// 	Function to dump out the set stream 
//	parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioLpcm_c::DumpSetStreamParameters(		void	*Parameters )
{
    CODEC_ERROR("Not implemented\n");  
    return CodecNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// 	Function to dump out the decode
//	parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioLpcm_c::DumpDecodeParameters( 		void	*Parameters )
{
    CODEC_ERROR("Not implemented\n");  
    return CodecNoError;
}
