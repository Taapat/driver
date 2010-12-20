/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_mme_audio_vorbis.cpp
Author :           Julian

Implementation of the Ogg Vorbis Audio codec class for player 2.


Date            Modification            Name
----            ------------            --------
10-Mar-09       Created                 Julian

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Codec_MmeAudioVorbis_c
///
/// The VORBIS audio codec proxy.
///

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "codec_mme_audio_vorbis.h"
#include "vorbis_audio.h"

#ifdef __KERNEL__
extern "C"{void flush_cache_all();};
#endif

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//


// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

#define BUFFER_VORBIS_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT         "VorbisAudioCodecStreamParameterContext"
#define BUFFER_VORBIS_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE    {BUFFER_VORBIS_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(StreamAudioCodecStreamParameterContext_t)}

static BufferDataDescriptor_t           VorbisAudioCodecStreamParameterContextDescriptor   = BUFFER_VORBIS_AUDIO_CODEC_STREAM_PARAMETER_CONTEXT_TYPE;

#define BUFFER_VORBIS_AUDIO_CODEC_DECODE_CONTEXT                   "VorbisAudioCodecDecodeContext"
#define BUFFER_VORBIS_AUDIO_CODEC_DECODE_CONTEXT_TYPE              {BUFFER_VORBIS_AUDIO_CODEC_DECODE_CONTEXT, BufferDataTypeBase, AllocateFromOSMemory, 32, 0, true, true, sizeof(StreamAudioCodecDecodeContext_t)}

static BufferDataDescriptor_t           VorbisAudioCodecDecodeContextDescriptor    = BUFFER_VORBIS_AUDIO_CODEC_DECODE_CONTEXT_TYPE;


//{{{  Constructor
////////////////////////////////////////////////////////////////////////////
///
/// Fill in the configuration parameters used by the super-class and reset everything.
///
Codec_MmeAudioVorbis_c::Codec_MmeAudioVorbis_c( void )
{
    CODEC_TRACE ("Codec_MmeAudioVorbis_c::%s\n", __FUNCTION__);
    Configuration.CodecName                             = "Vorbis audio";

    Configuration.StreamParameterContextCount           = 1;
    Configuration.StreamParameterContextDescriptor      = &VorbisAudioCodecStreamParameterContextDescriptor;

    Configuration.DecodeContextCount                    = 9;   // Vorbis demands 8 SEND_BUFFERS before it starts thinking about decoding
    Configuration.DecodeContextDescriptor               = &VorbisAudioCodecDecodeContextDescriptor;

    Configuration.MaximumSampleCount                    = 4096;

    AudioDecoderTransformCapabilityMask.DecoderCapabilityFlags = (1 << ACC_OGG);

    DecoderId                                           = ACC_OGG_ID;

    SendbufTriggerTransformCount                        = 8;    // Vorbis demands 8 SEND_BUFFERS before it starts thinking about decoding

    Reset();
}
//}}}
//{{{  Destructor
////////////////////////////////////////////////////////////////////////////
///
///     Destructor function, ensures a full halt and reset 
///     are executed for all levels of the class.
///
Codec_MmeAudioVorbis_c::~Codec_MmeAudioVorbis_c( void )
{
    CODEC_TRACE ("Codec_MmeAudioVorbis_c::%s\n", __FUNCTION__);
    Halt();
    Reset();
}
//}}}

//{{{  FillOutTransformerGlobalParameters
////////////////////////////////////////////////////////////////////////////
///
/// Populate the supplied structure with parameters for VORBIS audio.
///
///
CodecStatus_t Codec_MmeAudioVorbis_c::FillOutTransformerGlobalParameters (MME_LxAudioDecoderGlobalParams_t* GlobalParams_p)
{
    MME_LxAudioDecoderGlobalParams_t   &GlobalParams    = *GlobalParams_p;

    CODEC_TRACE ("Codec_MmeAudioVorbis_c::%s\n", __FUNCTION__);

    GlobalParams.StructSize             = sizeof(MME_LxAudioDecoderGlobalParams_t);

    MME_LxOvConfig_t &Config            = *((MME_LxOvConfig_t*)GlobalParams.DecConfig);

    Config.DecoderId                    = DecoderId;
    Config.StructSize                   = sizeof(Config);
    Config.MaxNbPages                   = 8;
    Config.MaxPageSize                  = 8192;
    Config.NbSamplesOut                 = 4096;

#if 0
    if (ParsedFrameParameters != NULL)
    {
        VorbisAudioStreamParameters_s*     StreamParams    = (VorbisAudioStreamParameters_s*)ParsedFrameParameters->StreamParameterStructure;
    }
    else
    {
        CODEC_TRACE("%s - No Params\n", __FUNCTION__);
        Config.StructSize               = 2 * sizeof(U32);      // only transmit the ID and StructSize (all other params are irrelevant)
    }
#endif

    CODEC_TRACE("DecoderId                  %d\n", Config.DecoderId);
    CODEC_TRACE("StructSize                 %d\n", Config.StructSize);
    CODEC_TRACE("MaxNbPages                 %d\n", Config.MaxNbPages);
    CODEC_TRACE("MaxPageSize                %d\n", Config.MaxPageSize);
    CODEC_TRACE("NbSamplesOut               %d\n", Config.NbSamplesOut);

    return Codec_MmeAudio_c::FillOutTransformerGlobalParameters (GlobalParams_p);
}
//}}}

//{{{  COMMENT
#if 0
//{{{  FillOutTransformerInitializationParameters
////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's initialization parameters for Vorbis audio.
///
CodecStatus_t   Codec_MmeAudioVorbis_c::FillOutTransformerInitializationParameters( void )
{
    CodecStatus_t                       Status;
    MME_LxAudioDecoderInitParams_t     &Params                  = AudioDecoderInitializationParameters;

    MMEInitializationParameters.TransformerInitParamsSize       = sizeof(Params);
    MMEInitializationParameters.TransformerInitParams_p         = &Params;

    CODEC_TRACE ("Codec_MmeAudioVorbis_c::%s\n", __FUNCTION__);

    Status                                                      = Codec_MmeAudio_c::FillOutTransformerInitializationParameters();
    if (Status != CodecNoError)
        return Status;

    return FillOutTransformerGlobalParameters (&Params.GlobalParams);
}
//}}}
//{{{  FillOutDecodeCommand
////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's MME_TRANSFORM parameters for Vorbis audio.
///
CodecStatus_t   Codec_MmeAudioVorbis_c::FillOutDecodeCommand(       void )
{
    VorbisAudioCodecDecodeContext_t*    VorbisContext   = (VorbisAudioCodecDecodeContext_t*)DecodeContext;

    CODEC_DEBUG("%s\n", __FUNCTION__);

    // Initialize the frame parameters
    memset (&VorbisContext->DecodeParameters, 0, sizeof(VorbisContext->DecodeParameters));

    // Zero the reply structure
    memset (&VorbisContext->DecodeStatus, 0, sizeof(VorbisContext->DecodeStatus));

    // Fillout the actual command
    VorbisContext->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize  = sizeof(VorbisContext->DecodeStatus);
    VorbisContext->BaseContext.MMECommand.CmdStatus.AdditionalInfo_p    = (MME_GenericParams_t)(&VorbisContext->DecodeStatus);
    VorbisContext->BaseContext.MMECommand.ParamSize                     = sizeof(VorbisContext->DecodeParameters);
    VorbisContext->BaseContext.MMECommand.Param_p                       = (MME_GenericParams_t)(&VorbisContext->DecodeParameters);

    CODEC_DEBUG("AdditionalInfoSize         %d\n", VorbisContext->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize);

    return CodecNoError;
}
//}}}
//{{{  FillOutSendBuffersCommand
////////////////////////////////////////////////////////////////////////////
///
/// Populate the AUDIO_DECODER's MME_SEND_BUFFERS parameters for Vorbis audio.
///
CodecStatus_t   Codec_MmeAudioVorbis_c::FillOutSendBuffersCommand (void)
{
    VorbisAudioCodecDecodeContext_t*    VorbisContext   = (VorbisAudioCodecDecodeContext_t*)DecodeContext;

    CODEC_DEBUG("%s\n", __FUNCTION__);

    memset (&VorbisContext->DecodeStatus, 0, sizeof(VorbisContext->DecodeStatus) );

    // Initialize the frame parameters
    memset (&VorbisContext->DecodeParameters, 0, sizeof(VorbisContext->DecodeParameters));

    // Fillout the actual command
    VorbisContext->MMEExtraCommand.CmdStatus.AdditionalInfoSize = sizeof(VorbisContext->DecodeStatus);
    VorbisContext->MMEExtraCommand.CmdStatus.AdditionalInfo_p   = (MME_GenericParams_t)(&VorbisContext->DecodeStatus);
    VorbisContext->MMEExtraCommand.ParamSize                    = sizeof(VorbisContext->DecodeParameters);
    VorbisContext->MMEExtraCommand.Param_p                      = (MME_GenericParams_t)(&VorbisContext->DecodeParameters);

    CODEC_DEBUG("AdditionalInfoSize         %d\n", VorbisContext->BaseContext.MMECommand.CmdStatus.AdditionalInfoSize);

    return CodecNoError;
}
//}}}
//{{{  FillOutDecodeContext
////////////////////////////////////////////////////////////////////////////
///
/// OVER-RIDE THE SUPERCLASS VERSION TO SUIT MME_SEND_BUFFERS!
///
/// Populate the DecodeContext structure with parameters for Vorbis audio.
/// Both input and output buffers are populated by the base audio function.
///
CodecStatus_t Codec_MmeAudioVorbis_c::FillOutDecodeContext( void )
{
    VorbisAudioCodecDecodeContext_t*    VorbisContext   = (VorbisAudioCodecDecodeContext_t*)DecodeContext;

    CODEC_DEBUG ("%s\n", __FUNCTION__);

    // Set up all default buffer pointers etc
    Codec_MmeAudio_c::FillOutDecodeContext ();

    // Add extra setup for send buffers command
    memset (&VorbisContext->MMEExtraCommand, 0x00, sizeof(MME_Command_t));
    VorbisContext->MMEExtraCommand.NumberInputBuffers   = 1;
    VorbisContext->MMEExtraCommand.NumberOutputBuffers  = 0;
    VorbisContext->MMEExtraCommand.DataBuffers_p        = DecodeContext->MMEBufferList;

    // Override base setup
    VorbisContext->BaseContext.MMECommand.NumberInputBuffers        = 0;
    VorbisContext->BaseContext.MMECommand.NumberOutputBuffers       = 1;

    CODEC_DEBUG ("DecodeContext->MMEBuffers[0].TotalSize      %6u\n", VorbisContext->BaseContext.MMEBuffers[0].TotalSize);
    CODEC_DEBUG ("DecodeContext->MMEBuffers[0].Page_p         %p\n", VorbisContext->BaseContext.MMEPages[0].Page_p);
    CODEC_DEBUG ("DecodeContext->MMEBuffers[0].Size           %6u\n", VorbisContext->BaseContext.MMEPages[0].Size);

    CODEC_DEBUG ("DecodeContext->MMEBuffers[1].TotalSize      %6u\n", VorbisContext->BaseContext.MMEBuffers[1].TotalSize);
    CODEC_DEBUG ("DecodeContext->MMEBuffers[1].Page_p         %p\n", VorbisContext->BaseContext.MMEPages[1].Page_p);
    CODEC_DEBUG ("DecodeContext->MMEBuffers[1].Size           %6u\n", VorbisContext->BaseContext.MMEPages[1].Size);


    return CodecNoError;

}
//}}}

//{{{  SendMMEDecodeCommand
CodecStatus_t   Codec_MmeAudioVorbis_c::SendMMEDecodeCommand   (void)
{
    CodecStatus_t                       Status;
    VorbisAudioCodecDecodeContext_t*    Context         = (VorbisAudioCodecDecodeContext_t*)DecodeContext;
    VorbisAudioFrameParameters_s*       FrameParams     = NULL;
    Buffer_t                            AttachedCodedDataBuffer;
    ParsedFrameParameters_t*            ParsedFrameParams;
    unsigned int                        PTSFlag         = 0;
    unsigned long long int              PTS             = ACC_NO_PTS_DTS;

    CODEC_DEBUG("%s\n", __FUNCTION__);
    OS_AutoLockMutex mutex( &DecodeContextPoolMutex );

    // pass the PTS to the firmware...

    Status              = DecodeContext->DecodeContextBuffer->ObtainAttachedBufferReference (CodedFrameBufferType, &AttachedCodedDataBuffer);
    if (Status == BufferNoError)
    {
        Status          = AttachedCodedDataBuffer->ObtainMetaDataReference (Player->MetaDataParsedFrameParametersType,
                                                                           (void**)(&ParsedFrameParams));
        if (Status == BufferNoError)
        {
            FrameParams = (VorbisAudioFrameParameters_s*)ParsedFrameParameters->FrameParameterStructure;
            if (ParsedFrameParams->NormalizedPlaybackTime != INVALID_TIME)
            {
                PTS     = ParsedFrameParams->NativePlaybackTime;
                // inform the firmware a pts is present
                PTSFlag = ACC_PTS_PRESENT;
            }
            else
                CODEC_ERROR("PTS = INVALID_TIME\n" );
        }
        else
            CODEC_ERROR("No ObtainMetaDataReference\n" );
    }
    else
        CODEC_ERROR("No ObtainAttachedBufferReference\n" );


    Context->DecodeParameters.BufferFlags.Bits.PTS_DTS_FLAG     = PTSFlag;
    Context->DecodeParameters.BufferFlags.Bits.PTS_Bit32        = PTS >> 32;
    Context->DecodeParameters.PTS                               = PTS;

    SendBuffersCommandsIssued++;

    Status = Codec_MmeBase_c::SendMMEDecodeCommand();
    //CODEC_TRACE("%s: SendBuffersCommandsIssued = %d\n", __FUNCTION__, SendBuffersCommandsIssued);

    if (1)//((SendBuffersCommandsIssued & 0xfffffff0) == 0)          // Vorbis demands that the pump is primed with at least 8 buffers
    {
        CODEC_TRACE("%s: Setting Event SendBuffersCommandsIssued = %d\n", __FUNCTION__, SendBuffersCommandsIssued);
        OS_SetEvent (&IssueWmaTransformCommandEvent);
    }

    return Status;
}
//}}}

//{{{  ValidateDecodeContext
////////////////////////////////////////////////////////////////////////////
///
/// Validate the ACC status structure and squawk loudly if problems are found.
/// 
/// \return CodecSuccess
///
CodecStatus_t   Codec_MmeAudioVorbis_c::ValidateDecodeContext (CodecBaseDecodeContext_t* Context)
{
    VorbisAudioCodecDecodeContext_t*    VorbisContext   = (VorbisAudioCodecDecodeContext_t*)Context;
    MME_LxAudioDecoderFrameStatus_t    &Status          = VorbisContext->DecodeStatus;
    ParsedAudioParameters_t*            AudioParameters;
    Buffer_t                            TheCurrentDecodeBuffer;
    PlayerStatus_t                      PlayerStatus;
    ParsedFrameParameters_t*            DecodedFrameParsedFrameParameters;

    CODEC_TRACE ("%s: DecStatus %d\n", __FUNCTION__, Status.DecStatus);

    if (ENABLE_CODEC_DEBUG)
    {
        //DumpCommand(bufferIndex);
    }

    if (Status.DecStatus != ACC_HXR_OK)
    {
        CODEC_ERROR("VORBIS audio decode error (muted frame): %d\n", Status.DecStatus);
        //DumpCommand(bufferIndex);
        // don't report an error to the higher levels (because the frame is muted)
    }

    // SYSFS
    AudioDecoderStatus                          = Status;

    //
    // Attach any codec derived metadata to the output buffer (or verify the
    // frame analysis if the frame analyser already filled everything in for
    // us).
    //

    AudioParameters                             = BufferState[VorbisContext->BaseContext.BufferIndex].ParsedAudioParameters;

    AudioParameters->Source.BitsPerSample       = AudioOutputSurface->BitsPerSample;
    AudioParameters->Source.ChannelCount        = AudioOutputSurface->ChannelCount;
    AudioParameters->Organisation               = Status.AudioMode;

    AudioParameters->SampleCount                = Status.NbOutSamples;

    int SamplingFreqCode                        = Status.SamplingFreq;
    if (SamplingFreqCode < ACC_FS_reserved)
    {
        AudioParameters->Source.SampleRateHz    = ACC_SamplingFreqLUT[SamplingFreqCode];
    }
    else
    {
        AudioParameters->Source.SampleRateHz    = 0;
        CODEC_ERROR("VORBIS audio decode bad sampling freq returned: 0x%x\n", SamplingFreqCode);
    }

    CODEC_TRACE("AudioParameters                               %p\n", AudioParameters);
    CODEC_TRACE("AudioParameters->Source.BitsPerSample         %d\n", AudioParameters->Source.BitsPerSample);
    CODEC_TRACE("AudioParameters->Source.ChannelCount          %d\n", AudioParameters->Source.ChannelCount);
    CODEC_TRACE("AudioParameters->Organisation                 %d\n", AudioParameters->Organisation);
    CODEC_TRACE("AudioParameters->SampleCount                  %d\n", AudioParameters->SampleCount);
    CODEC_TRACE("AudioParameters->Source.SampleRateHz          %d\n", AudioParameters->Source.SampleRateHz);

    //! This is probably the right time to synthesise a PTS

    TheCurrentDecodeBuffer                      = BufferState[VorbisContext->BaseContext.BufferIndex].Buffer; //= CurrentDecodeBuffer;
    PlayerStatus                                = TheCurrentDecodeBuffer->ObtainMetaDataReference (Player->MetaDataParsedFrameParametersReferenceType, (void**)(&DecodedFrameParsedFrameParameters));

    if (PlayerStatus == PlayerNoError)
    {
        long long       CalculatedDelta         = ((unsigned long long)Status.NbOutSamples * 1000000ull) /
                                                  ((unsigned long long)ConvertCodecSamplingFreq(Status.SamplingFreq));

        //
        // post-decode the DTS isn't interesting (especially given we had to fiddle with it in the
        // frame parser) so we discard it entirely rather then extracting it from the codec's reply.
        //

        DecodedFrameParsedFrameParameters->NativeDecodeTime     = INVALID_TIME;
        DecodedFrameParsedFrameParameters->NormalizedDecodeTime = INVALID_TIME;

        //
        // suck out the PTS from the codec's reply
        //

        if (ACC_isPTS_PRESENT(Status.PTSflag.Bits.PTS_DTS_FLAG))
        {
            unsigned long long  Temp    = (unsigned long long)Status.PTS + (unsigned long long)(((unsigned long long)Status.PTSflag.Bits.PTS_Bit32) << 32);
            DecodedFrameParsedFrameParameters->NativePlaybackTime       = Temp;
            DecodedFrameParsedFrameParameters->NormalizedPlaybackTime   = ((Temp*1000000)/90000);
        }
        else if (LastNormalizedPlaybackTime != UNSPECIFIED_TIME )
        {
            // synthesise a PTS
            unsigned long long  Temp    = LastNormalizedPlaybackTime + CalculatedDelta;
            DecodedFrameParsedFrameParameters->NativePlaybackTime       = (Temp * 90000)/100000;
            DecodedFrameParsedFrameParameters->NormalizedPlaybackTime   = Temp;
        }

        // Squawk if time does not progress quite as expected.
        if (LastNormalizedPlaybackTime != UNSPECIFIED_TIME)
        {
            long long   RealDelta                       = DecodedFrameParsedFrameParameters->NormalizedPlaybackTime - LastNormalizedPlaybackTime;
            long long   DeltaDelta                      = RealDelta - CalculatedDelta;
            long long   PtsJitterTollerenceThreshold    = 1000;      // Changed to 1ms by nick, because some file formats specify pts times to 1 ms accuracy

            // Check that the predicted and actual times deviate by no more than the threshold
            if (DeltaDelta < -PtsJitterTollerenceThreshold || DeltaDelta > PtsJitterTollerenceThreshold) 
            {
                CODEC_ERROR( "Unexpected change in playback time. Expected %lldus, got %lldus (deltas: exp. %lld  got %lld )\n",
                             LastNormalizedPlaybackTime + CalculatedDelta,
                             DecodedFrameParsedFrameParameters->NormalizedPlaybackTime,
                             CalculatedDelta, RealDelta);

            }     
        }

        LastNormalizedPlaybackTime      = DecodedFrameParsedFrameParameters->NormalizedPlaybackTime;

    }
    else
        CODEC_ERROR ("Codec_MmeAudioVorbis_c::%s - ObtainMetaDataReference failed\n", __FUNCTION__);

    return CodecNoError;
}
//}}}
//{{{  DumpSetStreamParameters
// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the set stream 
//      parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioVorbis_c::DumpSetStreamParameters(           void    *Parameters )
{
    CODEC_ERROR("Not implemented\n");
    return CodecNoError;
}
//}}}
//{{{  DumpDecodeParameters
// /////////////////////////////////////////////////////////////////////////
//
//      Function to dump out the decode
//      parameters from an mme command.
//

CodecStatus_t   Codec_MmeAudioVorbis_c::DumpDecodeParameters(              void    *Parameters )
{
    CODEC_TRACE("%s: TotalSize[0]                  %d\n", __FUNCTION__, DecodeContext->MMEBuffers[0].TotalSize);
    CODEC_TRACE("%s: Page_p[0]                     %p\n", __FUNCTION__, DecodeContext->MMEPages[0].Page_p);
    CODEC_TRACE("%s: TotalSize[1]                  %d\n", __FUNCTION__, DecodeContext->MMEBuffers[1].TotalSize);
    CODEC_TRACE("%s: Page_p[1]                     %p\n", __FUNCTION__, DecodeContext->MMEPages[1].Page_p);

    return CodecNoError;
}
//}}}

//{{{  CallbackFromMME
// /////////////////////////////////////////////////////////////////////////
//
//      Callback function from MME
//
//

void   Codec_MmeAudioVorbis_c::CallbackFromMME (MME_Event_t Event, MME_Command_t *CallbackData)
{
    CODEC_TRACE("Callback!  Event %x, CmdId %x  CmdCode %s  State %d  Error %d\n",
                Event,
                CallbackData->CmdStatus.CmdId,
                CallbackData->CmdCode == MME_SET_GLOBAL_TRANSFORM_PARAMS ? "MME_SET_GLOBAL_TRANSFORM_PARAMS" :
                CallbackData->CmdCode == MME_SEND_BUFFERS ? "MME_SEND_BUFFERS" :
                CallbackData->CmdCode == MME_TRANSFORM ? "MME_TRANSFORM" : "UNKNOWN",
                CallbackData->CmdStatus.State,
                CallbackData->CmdStatus.Error);

    Codec_MmeBase_c::CallbackFromMME( Event, CallbackData );

    //
    // Switch to perform appropriate actions per command
    //

    switch (CallbackData->CmdCode )
    {
        case MME_SEND_BUFFERS:
        {
            VorbisAudioCodecDecodeContext_t*    VorbisContext   = (VorbisAudioCodecDecodeContext_t*)((unsigned int)CallbackData - sizeof(CodecBaseDecodeContext_t));

            CODEC_TRACE("CallbackData %p, VorbisContxt %p, TransformIssued %d, DecodeID %d\n", CallbackData, VorbisContext, VorbisContext->TransformIssued, VorbisContext->DecodeId);
            if (!VorbisContext->TransformIssued)        // Release decode context if just a send buffers command
                ReleaseDecodeContext ((CodecBaseDecodeContext_t*)VorbisContext);
            SendBuffersCommandsCompleted++;

            break;
        }
        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
        case MME_TRANSFORM:
        default:
            break;
    }
}

//}}}
#endif
//}}}

