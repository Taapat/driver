/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : havana_stream.cpp - derived from havana_player.cpp
Author :           Julian

Implementation of the stream module for havana.


Date        Modification                                    Name
----        ------------                                    --------
14-Feb-07   Created                                         Julian

************************************************************************/

#include "osinline.h"
#include "osdev_user.h"
#include "mpeg2.h"
#include "backend_ops.h"
#include "display.h"
#include "player.h"
#include "collator_base.h"
#include "output_timer_base.h"
#include "codec_mme_base.h"
#include "manifestor_audio.h"
#include "manifestor_video.h"
#include "demultiplexor_ts.h"
#include "havana_player.h"
#include "havana_playback.h"
#include "havana_stream.h"
#include "pes.h"
#include <stmdisplay.h>

//{{{  HavanaStream_c
HavanaStream_c::HavanaStream_c (void)
{
    HAVANA_DEBUG("\n");

    PlayerStreamType            = StreamTypeNone;
    PlayerStream                = NULL;
    DemuxContext                = NULL;

    HavanaPlayer                = NULL;
    Player                      = NULL;
    Demultiplexor               = NULL;
    Collator                    = NULL;
    FrameParser                 = NULL;
    Codec                       = NULL;
    OutputTimer                 = NULL;
    Manifestor                  = NULL;
    DecodeBufferPool            = NULL;

    DemuxId                     = DEMUX_INVALID_ID;
    StreamId                    = DEMUX_INVALID_ID;

    EventSignalCallback         = NULL;
    CallbackContext             = NULL;
    memset (&StreamEvent, 0, sizeof (struct stream_event_s));
    StreamEvent.code            = STREAM_EVENT_INVALID;

    LockInitialised             = false;
}
//}}}  
//{{{  ~HavanaStream_c
HavanaStream_c::~HavanaStream_c (void)
{
    HAVANA_DEBUG("\n");

    if (PlayerStream != NULL)
    {
        // tell player we want to bin rest of stream
        Player->SetPolicy (PlayerPlayback, PlayerStream, PolicyPlayoutOnTerminate, PolicyValueDiscard);
        if ((Demultiplexor != NULL) && (DemuxContext != NULL))
            // remove stream from multiplexor
            Demultiplexor->RemoveStream (DemuxContext, StreamId);
        // finally remove stream from player
        Player->RemoveStream (PlayerStream, true);
    }
#if 0
    if ((Manifestor != NULL) && (PlayerStreamType == StreamTypeVideo))
    {
        class Manifestor_Video_c*       VideoManifestor = (class Manifestor_Video_c*)Manifestor;
        VideoManifestor->CloseOutputSurface ();
    }
#endif
    if (Collator != NULL)
        delete Collator;
    if (FrameParser != NULL)
        delete FrameParser;
    if (Codec != NULL)
        delete Codec;
    if (OutputTimer != NULL)
        delete OutputTimer;
#if 0
    if (Manifestor != NULL)
        delete Manifestor;
#endif

    if (LockInitialised)
    {
        OS_TerminateMutex (&InputLock);
        LockInitialised         = false;
    }

    EventSignalCallback         = NULL;
    CallbackContext             = NULL;

    PlayerStreamType            = StreamTypeNone;
    PlayerStream                = NULL;
    DemuxContext                = NULL;

    Demultiplexor               = NULL;
    Collator                    = NULL;
    FrameParser                 = NULL;
    Codec                       = NULL;
    OutputTimer                 = NULL;
    Manifestor                  = NULL;
    DecodeBufferPool            = NULL;

}
//}}}  
//{{{  Init
//{{{  doxynote
/// \brief Create and initialise all of the necessary player components for a new player stream
/// \param HavanaPlayer         Grandparent class
/// \param Player               The player
/// \param PlayerPlayback       The player playback to which the stream will be added
/// \param Media                Textual description of media (audio or video)
/// \param Format               Stream packet format (PES)
/// \param Encoding             The encoding of the stream content (MPEG2/H264 etc)
/// \param Multiplex            Name of multiplex if present (TS)
/// \return                     Havana status code, HavanaNoError indicates success.
//}}}  
HavanaStatus_t HavanaStream_c::Init    (class HavanaPlayer_c*   HavanaPlayer,
                                        class Player_c*         Player,
                                        PlayerPlayback_t        PlayerPlayback,
                                        char*                   Media,
                                        char*                   Format,
                                        char*                   Encoding,
                                        unsigned int            SurfaceId)
{
    HavanaStatus_t      Status          = HavanaNoError;
    PlayerStatus_t      PlayerStatus    = PlayerNoError;

    HAVANA_DEBUG("Stream %s %s %s\n", Media, Format, Encoding);

    this->Player                = Player;
    this->PlayerPlayback        = PlayerPlayback;
    this->HavanaPlayer          = HavanaPlayer;

    if (!LockInitialised)
    {
        if (OS_InitializeMutex (&InputLock) != OS_NO_ERROR)
        {
            STREAM_ERROR ("Failed to initialize InputLock mutex\n");
            return HavanaNoMemory;
        }
        LockInitialised         = true;
    }

    if (strcmp (Media, BACKEND_AUDIO_ID) == 0)
        PlayerStreamType        = StreamTypeAudio;
    else if (strcmp (Media, BACKEND_VIDEO_ID) == 0)
        PlayerStreamType        = StreamTypeVideo;
    else
        PlayerStreamType        = StreamTypeOther;

    if (Collator == NULL)
        Status  = HavanaPlayer->CallFactory (Format, Encoding, PlayerStreamType, ComponentCollator, (void**)&Collator);
    if (Status != HavanaNoError)
        return Status;

    if (FrameParser == NULL)
        Status  = HavanaPlayer->CallFactory (Encoding, FACTORY_ANY_ID, PlayerStreamType, ComponentFrameParser, (void**)&FrameParser);
    if (Status != HavanaNoError)
        return Status;

    if (Codec == NULL)
        Status  = HavanaPlayer->CallFactory (Encoding, FACTORY_ANY_ID, PlayerStreamType, ComponentCodec, (void**)&Codec);
    if (Status != HavanaNoError)
        return Status;
    {
        CodecParameterBlock_t           CodecParameters;
        CodecStatus_t                   CodecStatus;

        CodecParameters.ParameterType   = CodecSelectTransformer;
        CodecParameters.Transformer     = (SurfaceId == DISPLAY_ID_MAIN) ? 0 : 1;

        //if (PlayerStreamType == StreamTypeVideo)
        //{
        //    CodecParameters.ParameterType   = CodecSpecifyCodedMemoryPartition;
        //    strcpy (CodecParameters.PartitionName, "BPA2_Region1" );
        //}

        CodecStatus                     = Codec->SetModuleParameters (sizeof(CodecParameterBlock_t), &CodecParameters);
        if (CodecStatus != CodecNoError)
        {
            STREAM_ERROR("Failed to set codec parameters (%08x)\n", CodecStatus);
            return HavanaNoMemory;
        }
        STREAM_DEBUG("Selecting transformer %d for stream %d\n", SurfaceId, PlayerStreamType);

        if (FIRST_AUDIO_COPROCESSOR && PlayerStreamType == StreamTypeAudio)
        {
            CodecParameterBlock_t AudioParameters = { CodecSpecifyTransformerPostFix };

            for (int i=0; i<NUMBER_AUDIO_COPROCESSORS; i++)
            {
                AudioParameters.TransformerPostFix.Transformer = i;
                AudioParameters.TransformerPostFix.PostFix = FIRST_AUDIO_COPROCESSOR + i;

                // ignore the return values (some codecs have immutable transformer names)
                (void) Codec->SetModuleParameters (sizeof(CodecParameterBlock_t), &AudioParameters);
            }
        }

    }

    if (OutputTimer == NULL)
        Status  = HavanaPlayer->CallFactory (Media, FACTORY_ANY_ID, PlayerStreamType, ComponentOutputTimer, (void**)&OutputTimer);
    if (Status != HavanaNoError)
        return Status;

    if (Manifestor == NULL)
    {
        Status  = HavanaPlayer->GetManifestor (Media, Encoding, SurfaceId, &Manifestor);
        if (Status != HavanaNoError)
            return Status;
    }

    PlayerStatus        = Player->AddStream    (PlayerPlayback,
                                                &PlayerStream,
                                                PlayerStreamType,
                                                Collator,
                                                FrameParser,
                                                Codec,
                                                OutputTimer,
                                                Manifestor,
                                                true);
    if (PlayerStatus != PlayerNoError)
    {
        STREAM_ERROR("Unable to create player stream\n");
        return HavanaNoMemory;
    }

    PlayerStatus        = Player->GetDecodeBufferPool (PlayerStream, &DecodeBufferPool);
    if (PlayerStatus != PlayerNoError)
    {
        STREAM_ERROR("Unable to access decode buffer pool\n");
        return HavanaNoMemory;
    }

    Player->SpecifySignalledEvents (PlayerPlayback, PlayerStream, EventAllEvents);
    Player->SetPolicy (PlayerPlayback, PlayerStream, PolicyManifestFirstFrameEarly, PolicyValueApply);
    Player->SetPolicy (PlayerPlayback, PlayerStream, PolicyVideoBlankOnShutdown, PolicyValueApply);

    return HavanaNoError;
}
//}}}  
//{{{  InjectData
HavanaStatus_t HavanaStream_c::InjectData      (const unsigned char*    Data,
                                                unsigned int            DataLength)
{
    Buffer_t                    Buffer;
    PlayerInputDescriptor_t*    InputDescriptor;

    OS_LockMutex (&InputLock);
    Player->GetInjectBuffer (&Buffer);
    Buffer->ObtainMetaDataReference (Player->MetaDataInputDescriptorType, (void**)&InputDescriptor);

    InputDescriptor->MuxType                    = MuxTypeUnMuxed;
    InputDescriptor->UnMuxedStream              = PlayerStream;

    InputDescriptor->PlaybackTimeValid          = false;
    InputDescriptor->DecodeTimeValid            = false;
    InputDescriptor->DataSpecificFlags          = 0;

    Buffer->RegisterDataReference (DataLength, (void*)Data);
    Buffer->SetUsedDataSize (DataLength);

    Player->InjectData (PlayerPlayback, Buffer);
    OS_UnLockMutex (&InputLock);

    return HavanaNoError;
}
//}}}  
//{{{  InjectDataPacket
HavanaStatus_t HavanaStream_c::InjectDataPacket(const unsigned char*    Data,
                                                unsigned int            DataLength,
                                                bool                    PlaybackTimeValid,
                                                unsigned long long      PlaybackTime )
{
    Buffer_t                    Buffer;
    PlayerInputDescriptor_t*    InputDescriptor;

    OS_LockMutex (&InputLock);
    Player->GetInjectBuffer (&Buffer);
    Buffer->ObtainMetaDataReference (Player->MetaDataInputDescriptorType, (void**)&InputDescriptor);

    InputDescriptor->MuxType                    = MuxTypeUnMuxed;
    InputDescriptor->UnMuxedStream              = PlayerStream;

    InputDescriptor->PlaybackTimeValid          = PlaybackTimeValid;
    InputDescriptor->PlaybackTime               = PlaybackTime;
    InputDescriptor->DecodeTimeValid            = false;
    InputDescriptor->DataSpecificFlags          = 0;

    Buffer->RegisterDataReference (DataLength, (void*)Data);
    Buffer->SetUsedDataSize (DataLength);

    Player->InjectData (PlayerPlayback, Buffer);
    OS_UnLockMutex (&InputLock);

    return HavanaNoError;
}
//}}}  
//{{{  Discontinuity
HavanaStatus_t HavanaStream_c::Discontinuity   (bool                    ContinuousReverse,
                                                bool                    SurplusData)
{
    //STREAM_DEBUG("Surplus %d, reverse %d\n", SurplusData, ContinuousReverse);

    OS_LockMutex   (&InputLock);
    Player->InputJump (PlayerPlayback, PlayerStream, SurplusData, ContinuousReverse);
    OS_UnLockMutex (&InputLock);

    return HavanaNoError;
}
//}}}  
//{{{  Drain
HavanaStatus_t HavanaStream_c::Drain   (bool            Discard)
{
    unsigned int        PolicyValue     = Discard ? PolicyValueDiscard : PolicyValuePlayout;

    Player->SetPolicy   (PlayerPlayback, PlayerStream, PolicyPlayoutOnDrain, PolicyValue);
    Player->DrainStream (PlayerStream); //NonBlocking=false, SignalEvent=false, EventUserData=NULL);
                                        // i.e. we wait rather than use the event
    return HavanaNoError;
}
//}}}  
//{{{  Enable
HavanaStatus_t HavanaStream_c::Enable  (bool    Manifest)
{
    ManifestorStatus_t  ManifestorStatus;

    //STREAM_DEBUG("Id = %x\n", Id);
    if (PlayerStreamType == StreamTypeVideo)
    {
        class Manifestor_Video_c*       VideoManifestor = (class Manifestor_Video_c*)Manifestor;
        if (Manifest)
            ManifestorStatus        = VideoManifestor->Enable ();
        else
            ManifestorStatus        = VideoManifestor->Disable ();
        if (ManifestorStatus != ManifestorNoError)
        {
            STREAM_ERROR("Failed to enable video output surface\n");
            return HavanaError;
        }
    }
    else if (PlayerStreamType == StreamTypeAudio)
    {
        ManifestorAudioParameterBlock_t MuteParameters = { ManifestorAudioSetEmergencyMuteState };
        MuteParameters.EmergencyMute = !Manifest;

        ManifestorStatus = Manifestor->SetModuleParameters( sizeof( MuteParameters ), &MuteParameters );
        if (ManifestorStatus != ManifestorNoError)
        {
            STREAM_ERROR("Failed to mute/unmute the audio output\n");
            return HavanaError;
        }
    }
    else
    {
        STREAM_ERROR("Unknown stream type\n");
        return HavanaError;
    }

    return HavanaNoError;
}
//}}}  
//{{{  SetId
HavanaStatus_t HavanaStream_c::SetId   (unsigned int            DemuxId,
                                        unsigned int            Id)
{
    STREAM_DEBUG("DemuxId = %d, Id = 0x%x\n", DemuxId, Id);

    if ((DemuxId != DEMUX_INVALID_ID) && ((Id & DMX_FILTER_BY_PRIORITY_MASK) != 0))
    {
        if ((Id & DMX_FILTER_BY_PRIORITY_HIGH) != 0)
            Id          = (Id & ~DMX_FILTER_BY_PRIORITY_MASK) | DEMULTIPLEXOR_SELECT_ON_PRIORITY | DEMULTIPLEXOR_PRIORITY_HIGH;
        else
            Id          = (Id & ~DMX_FILTER_BY_PRIORITY_MASK) | DEMULTIPLEXOR_SELECT_ON_PRIORITY | DEMULTIPLEXOR_PRIORITY_LOW;
    }

    if (HavanaPlayer->GetDemuxContext (DemuxId, &Demultiplexor, &DemuxContext) == HavanaNoError)
        Demultiplexor->AddStream (DemuxContext, PlayerStream, Id);

    this->DemuxId       = DemuxId;
    StreamId            = Id;

    return HavanaNoError;
}
//}}}  
//{{{  ChannelSelect
HavanaStatus_t HavanaStream_c::ChannelSelect   (channel_select_t        Channel)
{
    //STREAM_DEBUG("Surplus %d, reverse %d\n", SurplusData, ContinuousReverse);

    if ((PlayerStreamType == StreamTypeAudio) && (Codec != NULL))
    {
        CodecParameterBlock_t           CodecParameters;
        CodecStatus_t                   CodecStatus;

        CodecParameters.ParameterType   = CodecSelectChannel;
        CodecParameters.Channel         = (Channel == CHANNEL_MONO_LEFT)  ? ChannelSelectLeft :
                                          (Channel == CHANNEL_MONO_RIGHT) ? ChannelSelectRight :
                                                                            ChannelSelectStereo;

        CodecStatus                     = Codec->SetModuleParameters (sizeof(CodecParameterBlock_t), &CodecParameters);
        if (CodecStatus != CodecNoError)
        {
            STREAM_ERROR("Failed to set codec parameters (%08x)\n", CodecStatus);
            return HavanaError;
        }
        STREAM_DEBUG("Selecting channel %d for stream %d\n", Channel, PlayerStreamType);
    }

    return HavanaNoError;
}
//}}}  
//{{{  SetOption
HavanaStatus_t HavanaStream_c::SetOption       (play_option_t           Option,
                                                unsigned int            Value)
{
    unsigned char       PolicyValue = 0;
    PlayerPolicy_t      PlayerPolicy;
    PlayerStatus_t      Status;
    PlayerStream_t      Stream  = PlayerStream;

    switch (Option)
    {
        case PLAY_OPTION_VIDEO_ASPECT_RATIO:
            PlayerPolicy    = PolicyDisplayAspectRatio;
            switch (Value)
            {
                case VIDEO_FORMAT_16_9:
                    PolicyValue         = PolicyValue16x9;
                    break;
                default:
                    PolicyValue         = PolicyValue4x3;
                    break;
            }
            break;
        case PLAY_OPTION_VIDEO_DISPLAY_FORMAT:
            PlayerPolicy    = PolicyDisplayFormat;
            switch (Value)
            {
                case VIDEO_PAN_SCAN:
                    PolicyValue         = PolicyValuePanScan;
                    break;
                case VIDEO_LETTER_BOX:
                    PolicyValue         = PolicyValueLetterBox;
                    break;
                case VIDEO_CENTER_CUT_OUT:
                    PolicyValue         = PolicyValueCentreCutOut;
                    break;
                default:
                    PolicyValue         = PolicyValueFullScreen;
                    break;
            }
            break;
        case PLAY_OPTION_VIDEO_BLANK:
            PlayerPolicy        = PolicyVideoBlankOnShutdown;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueBlankScreen : PolicyValueLeaveLastFrameOnScreen;
            break;
        case PLAY_OPTION_AV_SYNC:
            PlayerPolicy        = PolicyAVDSynchronization;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_TRICK_MODE_AUDIO:
            PlayerPolicy        = PolicyTrickModeAudio;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_PLAY_24FPS_VIDEO_AT_25FPS:
            PlayerPolicy        = PolicyPlay24FPSVideoAt25FPS;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_MASTER_CLOCK:
            PlayerPolicy        = PolicyMasterClock;
            switch (Value)
            {
                case PLAY_OPTION_VALUE_VIDEO_CLOCK_MASTER:
                    PolicyValue = PolicyValueVideoClockMaster;
                    break;
                case PLAY_OPTION_VALUE_AUDIO_CLOCK_MASTER:
                    PolicyValue = PolicyValueAudioClockMaster;
                    break;
                default:
                    PolicyValue = PolicyValueSystemClockMaster;
                    break;
            }
            break;
        case PLAY_OPTION_EXTERNAL_TIME_MAPPING:
            PlayerPolicy        = PolicyExternalTimeMapping;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            Stream              = PlayerAllStreams;
            break;
        case PLAY_OPTION_DISPLAY_FIRST_FRAME_EARLY:
            PlayerPolicy        = PolicyManifestFirstFrameEarly;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_STREAM_ONLY_KEY_FRAMES:
            PlayerPolicy        = PolicyStreamOnlyKeyFrames;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_STREAM_SINGLE_GROUP_BETWEEN_DISCONTINUITIES:
            PlayerPolicy        = PolicyStreamSingleGroupBetweenDiscontinuities;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_PLAYOUT_ON_TERMINATE:
            PlayerPolicy        = PolicyPlayoutOnTerminate;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValuePlayout : PolicyValueDiscard;
            break;
        case PLAY_OPTION_PLAYOUT_ON_SWITCH:
            PlayerPolicy        = PolicyPlayoutOnSwitch;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValuePlayout : PolicyValueDiscard;
            break;
        case PLAY_OPTION_PLAYOUT_ON_DRAIN:
            PlayerPolicy        = PolicyPlayoutOnDrain;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValuePlayout : PolicyValueDiscard;
            break;
        case PLAY_OPTION_TRICK_MODE_DOMAIN:
            PlayerPolicy        = PolicyTrickModeDomain;
            switch (Value)
            {
                case PLAY_OPTION_VALUE_TRICK_MODE_AUTO:
                    PolicyValue = PolicyValueTrickModeAuto;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL:
                    PolicyValue = PolicyValueTrickModeDecodeAll;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL_DEGRADE_NON_REFERENCE_FRAMES:
                    PolicyValue = PolicyValueTrickModeDecodeAllDegradeNonReferenceFrames;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_START_DISCARDING_NON_REFERENCE_FRAMES:
                    PolicyValue = PolicyValueTrickModeStartDiscardingNonReferenceFrames;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_DECODE_REFERENCE_FRAMES_DEGRADE_NON_KEY_FRAMES:
                    PolicyValue = PolicyValueTrickModeDecodeReferenceFramesDegradeNonKeyFrames;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_DECODE_KEY_FRAMES:
                    PolicyValue = PolicyValueTrickModeDecodeKeyFrames;
                    break;
                case PLAY_OPTION_VALUE_TRICK_MODE_DISCONTINUOUS_KEY_FRAMES:
                    PolicyValue = PolicyValueTrickModeDiscontinuousKeyFrames;
                    break;
            }
            break;
            break;
        case PLAY_OPTION_DISCARD_LATE_FRAMES:
            PlayerPolicy        = PolicyDiscardLateFrames;
            switch (Value)
            {
                case PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_NEVER:
                    PolicyValue = PolicyValueDiscardLateFramesNever;
                    break;
                case PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_ALWAYS:
                    PolicyValue = PolicyValueDiscardLateFramesAlways;
                    break;
                default:
                    PolicyValue = PolicyValueDiscardLateFramesAfterSynchronize;
                    break;
            }
            break;
        case PLAY_OPTION_VIDEO_START_IMMEDIATE:
            PlayerPolicy        = PolicyVideoStartImmediate;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_REBASE_ON_DATA_DELIVERY_LATE:
            PlayerPolicy        = PolicyRebaseOnFailureToDeliverDataInTime;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_REBASE_ON_FRAME_DECODE_LATE:
            PlayerPolicy        = PolicyRebaseOnFailureToDecodeInTime;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_LOWER_CODEC_DECODE_LIMITS_ON_FRAME_DECODE_LATE:
            PlayerPolicy        = PolicyLowerCodecDecodeLimitsOnFailureToDecodeInTime;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_H264_ALLOW_NON_IDR_RESYNCHRONIZATION:
            PlayerPolicy        = PolicyH264AllowNonIDRResynchronization;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_MPEG2_IGNORE_PROGESSIVE_FRAME_FLAG:
            PlayerPolicy        = PolicyMPEG2DoNotHonourProgressiveFrameFlag;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_CLAMP_PLAYBACK_INTERVAL_ON_PLAYBACK_DIRECTION_CHANGE:
            PlayerPolicy        = PolicyClampPlaybackIntervalOnPlaybackDirectionChange;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_H264_ALLOW_BAD_PREPROCESSED_FRAMES:
            PlayerPolicy        = PolicyH264AllowBadPreProcessedFrames;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_H264_TREAT_DUPLICATE_DPB_AS_NON_REFERENCE_FRAME_FIRST:
            PlayerPolicy        = PolicyH264TreatDuplicateDpbValuesAsNonReferenceFrameFirst;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        case PLAY_OPTION_CLOCK_RATE_ADJUSTMENT_LIMIT_2_TO_THE_N_PARTS_PER_MILLION:
            PlayerPolicy        = PolicyClockPullingLimit2ToTheNPartsPerMillion;
            PolicyValue         = Value;
            break;
        case PLAY_OPTION_LIMIT_INPUT_INJECT_AHEAD:
            PlayerPolicy        = PolicyLimitInputInjectAhead;
            PolicyValue         = (Value == PLAY_OPTION_VALUE_ENABLE) ? PolicyValueApply : PolicyValueDisapply;
            break;
        default:
            STREAM_ERROR("Unknown option %d\n", Option);
            return HavanaError;

    }

    Status  = Player->SetPolicy (PlayerPlayback, Stream, PlayerPolicy, PolicyValue);
    if (Status != PlayerNoError)
    {
        STREAM_ERROR("Unable to set stream option %x, %x\n", PlayerPolicy, PolicyValue);
        return HavanaError;
    }
    return HavanaNoError;
}
//}}}  
//{{{  GetOption
HavanaStatus_t HavanaStream_c::GetOption       (play_option_t           Option,
                                                unsigned int*           Value)
{
    unsigned char       PolicyValue;
    PlayerPolicy_t      PlayerPolicy;

    switch (Option)
    {
        case PLAY_OPTION_VIDEO_ASPECT_RATIO:
            PlayerPolicy    = PolicyDisplayAspectRatio;
            break;
        case PLAY_OPTION_VIDEO_DISPLAY_FORMAT:
            PlayerPolicy    = PolicyDisplayFormat;
            break;
        case PLAY_OPTION_VIDEO_BLANK:
            PlayerPolicy        = PolicyVideoBlankOnShutdown;
            break;
        case PLAY_OPTION_AV_SYNC:
            PlayerPolicy        = PolicyAVDSynchronization;
            break;
        case PLAY_OPTION_EXTERNAL_TIME_MAPPING:
            PlayerPolicy        = PolicyExternalTimeMapping;
            break;
        case PLAY_OPTION_DISCARD_LATE_FRAMES:
            PlayerPolicy	= PolicyDiscardLateFrames;
            break;
        default:
            STREAM_ERROR("Unknown option %d\n", Option);
            return HavanaError;
    }

    PolicyValue = Player->PolicyValue (PlayerPlayback, PlayerStream, PlayerPolicy);
    switch (Option)
    {
        case PLAY_OPTION_VIDEO_ASPECT_RATIO:
            *Value      = (PolicyValue == PolicyValue16x9) ? VIDEO_FORMAT_16_9 : VIDEO_FORMAT_4_3;
            break;
        case PLAY_OPTION_VIDEO_DISPLAY_FORMAT:
            switch (PolicyValue)
            {
                case PolicyValuePanScan:
                    *Value              = VIDEO_PAN_SCAN;
                    break;
                case PolicyValueLetterBox:
                    *Value              = VIDEO_LETTER_BOX;
                    break;
                case PolicyValueCentreCutOut:
                    *Value              = VIDEO_CENTER_CUT_OUT;
                    break;
                default:
                    *Value              = VIDEO_FULL_SCREEN;
                    break;
            }
            break;
        case PLAY_OPTION_VIDEO_BLANK:
            *Value      = (PolicyValue == PolicyValueBlankScreen) ? true : false;
            break;
        case PLAY_OPTION_AV_SYNC:
            *Value      = (PolicyValue == PolicyValueApply) ? true : false;
            break;
        case PLAY_OPTION_DISCARD_LATE_FRAMES:
            switch (PolicyValue)
            {
                case PolicyValueDiscardLateFramesNever:
                    *Value = PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_NEVER;
                    break;
                case PolicyValueDiscardLateFramesAlways:
                    *Value = PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_ALWAYS;
                    break;
                default:
                    *Value = PLAY_OPTION_VALUE_DISCARD_LATE_FRAMES_AFTER_SYNCHRONIZE;
                    break;
            }
            break;
        default:
            STREAM_ERROR("Unknown option %d\n", Option);
            return HavanaError;
    }

    return HavanaNoError;
}
//}}}  
//{{{  Step
HavanaStatus_t HavanaStream_c::Step (void)
{
    PlayerStatus_t      Status;

    Status      = Player->StreamStep (PlayerStream);
    if (Status != PlayerNoError)
    {
            STREAM_ERROR("Failed to step stream\n");
            return HavanaError;
    }

    return HavanaNoError;
}
//}}}  
//{{{  SetOutputWindow
HavanaStatus_t HavanaStream_c::SetOutputWindow (unsigned int            X,
                                                unsigned int            Y,
                                                unsigned int            Width,
                                                unsigned int            Height)
{
    ManifestorStatus_t  ManifestorStatus;

    //STREAM_DEBUG("\n");
    if (PlayerStreamType == StreamTypeVideo)
    {
        class Manifestor_Video_c*       VideoManifestor = (class Manifestor_Video_c*)Manifestor;

        ManifestorStatus        = VideoManifestor->SetOutputWindow (X, Y, Width, Height);
        if (ManifestorStatus != ManifestorNoError)
        {
            STREAM_ERROR("Failed to set output window dimensions\n");
            return HavanaError;
        }
    }

    return HavanaNoError;
}
//}}}  
//{{{  SetInputWindow
HavanaStatus_t HavanaStream_c::SetInputWindow  (unsigned int            X,
                                                unsigned int            Y,
                                                unsigned int            Width,
                                                unsigned int            Height)
{
    ManifestorStatus_t  ManifestorStatus;

    //STREAM_DEBUG("\n");
    if (PlayerStreamType == StreamTypeVideo)
    {
        class Manifestor_Video_c*       VideoManifestor = (class Manifestor_Video_c*)Manifestor;

        ManifestorStatus        = VideoManifestor->SetInputWindow (X, Y, Width, Height);
        if (ManifestorStatus != ManifestorNoError)
        {
            //STREAM_ERROR("Failed to set input window dimensions\n");
            return HavanaError;
        }
    }

    return HavanaNoError;
}
//}}}  
//{{{  GetPlayInfo
HavanaStatus_t HavanaStream_c::GetPlayInfo (struct play_info_s* PlayInfo)
{
    ManifestorStatus_t          Status = ManifestorError;

    PlayInfo->pts               = INVALID_TIME;
    PlayInfo->presentation_time = INVALID_TIME;
    PlayInfo->system_time       = INVALID_TIME;
    PlayInfo->frame_count       = 0ull;

    if (Manifestor != NULL)
        Status  = Manifestor->GetNativeTimeOfCurrentlyManifestedFrame (&PlayInfo->pts);

    Player->RetrieveNativePlaybackTime (PlayerPlayback, &PlayInfo->presentation_time);

    PlayInfo->system_time       = OS_GetTimeInMicroSeconds();

    if ((Manifestor != NULL) && (Status == ManifestorNoError))
        Status  = Manifestor->GetFrameCount (&PlayInfo->frame_count);

//report( severity_info, "Playtime : %016llx %016llx\n", PlayTime->pts, PlayTime->presentation_time );

    return (Status == ManifestorNoError) ? HavanaNoError : HavanaError;
}
//}}}  
//{{{  GetDecodeBuffer
HavanaStatus_t HavanaStream_c::GetDecodeBuffer (buffer_handle_t*        DecodeBuffer,
                                                unsigned char**         Data,
                                                unsigned int            Format,
                                                unsigned int            DimensionCount,
                                                unsigned int            Dimensions[],
                                                unsigned int*           Index,
                                                unsigned int*           Stride)
{
    ManifestorStatus_t  Status;
    BufferStructure_t   BufferStructure;
    class Buffer_c*     Buffer;
    unsigned int BuffIndex;

//    STREAM_DEBUG("\n");

    //
    // Fill out the buffer structure request
    //

    switch( Format )
    {
        case SURF_YCBCR422R:    BufferStructure.Format  = FormatVideo422_Raster;
                                BufferStructure.DimensionCount  = DimensionCount;
                                memcpy( BufferStructure.Dimension, Dimensions, sizeof(BufferStructure.Dimension) );
                                break;

        default:                STREAM_ERROR("Unsupported decode buffer format request\n");
                                return HavanaError;
    }

    //
    // Request the buffer
    //

    Status      = Manifestor->GetDecodeBuffer( &BufferStructure, &Buffer );
    if (Status != ManifestorNoError)
    {
        STREAM_ERROR ("Failed to get decode buffer\n");
        return HavanaError;
    }

    /* Change the owner ID so we can track the buffer through the system */
    Status = Buffer->TransferOwnership(IdentifierExternal);
    if (Status != BufferNoError)
    {
        STREAM_ERROR ("Failed to transfer ownership of the buffer to %x\n",IdentifierExternal);
        ReturnDecodeBuffer( (buffer_handle_t *)Buffer );
        return HavanaError;
    }


    Buffer->GetIndex(&BuffIndex);

    if (Index != NULL)
        *Index = BuffIndex;

    // We now need to return physical address as we can convert that into anything
    // unlike before when we could do that with a virtual address
    Status      = Buffer->ObtainDataReference(NULL, NULL, (void**)Data,PhysicalAddress);
    if (Status != BufferNoError)
    {
        STREAM_ERROR ("Failed to get decode buffer data reference\n");
        ReturnDecodeBuffer( (buffer_handle_t *)Buffer );
        return HavanaError;
    }
    *DecodeBuffer       = (buffer_handle_t)Buffer;

    if (Stride)
        *Stride         = BufferStructure.Strides[0][0];

    return HavanaNoError;
}
//}}}  
//{{{  ReturnDecodeBuffer
HavanaStatus_t HavanaStream_c::ReturnDecodeBuffer (buffer_handle_t        DecodeBuffer)
{
    BufferStatus_t      Status;
    class Buffer_c*     Buffer = (Buffer_c*)DecodeBuffer;

//    STREAM_DEBUG("\n");

    Status = Buffer->DecrementReferenceCount();
    if (Status != BufferNoError)
    {
        STREAM_ERROR ("Failed to decrement buffer reference count\n");
        return HavanaError;
    }

//

    return HavanaNoError;
}


//}}}  
//{{{  GetDecodeBufferPoolStatus
HavanaStatus_t HavanaStream_c::GetDecodeBufferPoolStatus (unsigned int* BuffersInPool,
                                                          unsigned int* BuffersWithNonZeroReferenceCount)
{
    PlayerStatus_t   Status;
    unsigned int         MemoryInPool;
    unsigned int         MemoryAllocated;
    unsigned int         MemoryInUse;

//
// Nick changed the implementation of this function, instead of
// returning buffers in pool obtained from get pool usage, we now return the
// manifestor obtained value of how many buffers we can reasonably expect to allocate
//


//    STREAM_DEBUG("\n");
    Status = DecodeBufferPool->GetPoolUsage( NULL,
                                             BuffersWithNonZeroReferenceCount,
                                             &MemoryInPool,
                                             &MemoryAllocated,
                                             &MemoryInUse);
    if (Status != BufferNoError)
    {
        STREAM_ERROR ("Failed to get decode buffer pool usage\n");
        return HavanaError;
    }

//

    Status = Manifestor->GetDecodeBufferCount( BuffersInPool );
    if (Status != ManifestorNoError)
    {
        STREAM_ERROR ("Failed to get manifestor decode buffer count\n");
        return HavanaError;
    }

//

    return HavanaNoError;
}

//}}}  
//{{{  CheckEvent
HavanaStatus_t HavanaStream_c::CheckEvent      (struct PlayerEventRecord_s*     PlayerEvent)
{

    // Check if event is for us and we are interested.
    if ((PlayerEvent->Playback == PlayerPlayback) &&
        (PlayerEvent->Stream   == PlayerStream)   &&
        ((PlayerEvent->Code & STREAM_SPECIFIC_EVENTS) != 0))
    {
        //{{{  Translate from a Player2 event record to the external play event record.
        memset (&StreamEvent, 0, sizeof(struct stream_event_s));
        StreamEvent.timestamp           = PlayerEvent->PlaybackTime;
        switch (PlayerEvent->Code)
        {
            case EventSourceSizeChangeManifest:
                StreamEvent.code                        = STREAM_EVENT_SIZE_CHANGED;
                StreamEvent.u.size.width                = PlayerEvent->Value[0].UnsignedInt;
                StreamEvent.u.size.height               = PlayerEvent->Value[1].UnsignedInt;
                // There are only three choices so make the arbitrary decision that
                // less than 3:2 = 4:3, less than 2:1 = 16:9 otherwise 2.21:1
                if (PlayerEvent->Rational < Rational_t (3, 2))
                    StreamEvent.u.size.aspect_ratio     = ASPECT_RATIO_4_3;
                else if (PlayerEvent->Rational < Rational_t (2, 1))
                    StreamEvent.u.size.aspect_ratio     = ASPECT_RATIO_16_9;
                else
                    StreamEvent.u.size.aspect_ratio     = ASPECT_RATIO_221_1;

                // The manifestor inserts the pixel aspect ratio into the event rational.
                StreamEvent.u.size.pixel_aspect_ratio_numerator         = (unsigned int)PlayerEvent->Rational.GetNumerator ();
                StreamEvent.u.size.pixel_aspect_ratio_denominator       = (unsigned int)PlayerEvent->Rational.GetDenominator ();
                break;

            case EventSourceFrameRateChangeManifest:
                StreamEvent.code                        = STREAM_EVENT_FRAME_RATE_CHANGED;
                StreamEvent.u.frame_rate                = IntegerPart(PlayerEvent->Rational * PLAY_FRAME_RATE_MULTIPLIER);
                break;

            case EventFirstFrameManifested:
                StreamEvent.code                        = STREAM_EVENT_FIRST_FRAME_ON_DISPLAY;
                break;

            case EventStreamUnPlayable:
                StreamEvent.code                        = STREAM_EVENT_STREAM_UNPLAYABLE;
                StreamEvent.u.reason                    = REASON_UNKNOWN;
                break;

            case EventFailedToQueueBufferToDisplay:
                StreamEvent.code                        = STREAM_EVENT_FATAL_ERROR;
                StreamEvent.u.reason                    = REASON_UNKNOWN;
                break;

            case EventFailedToDecodeInTime:
                StreamEvent.code                        = STREAM_EVENT_FRAME_DECODED_LATE;
                break;

            case EventFailedToDeliverDataInTime:
                StreamEvent.code                        = STREAM_EVENT_DATA_DELIVERED_LATE;
                break;

            case EventTrickModeDomainChange:
                StreamEvent.code                        = STREAM_EVENT_TRICK_MODE_CHANGE;
                switch (PlayerEvent->Value[0].UnsignedInt)
                {
                     case PolicyValueTrickModeAuto:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_AUTO;
                         break;
                     case PolicyValueTrickModeDecodeAll:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL;
                         break;
                     case PolicyValueTrickModeDecodeAllDegradeNonReferenceFrames:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_DECODE_ALL_DEGRADE_NON_REFERENCE_FRAMES;
                         break;
                     case PolicyValueTrickModeStartDiscardingNonReferenceFrames:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_START_DISCARDING_NON_REFERENCE_FRAMES;
                         break;
                     case PolicyValueTrickModeDecodeReferenceFramesDegradeNonKeyFrames:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_DECODE_REFERENCE_FRAMES_DEGRADE_NON_KEY_FRAMES;
                         break;
                     case PolicyValueTrickModeDecodeKeyFrames:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_DECODE_KEY_FRAMES;
                         break;
                     case PolicyValueTrickModeDiscontinuousKeyFrames:
                         StreamEvent.u.trick_mode_domain        = PLAY_OPTION_VALUE_TRICK_MODE_DISCONTINUOUS_KEY_FRAMES;
                         break;
                }
                break;
            default:
                STREAM_DEBUG("Unexpected event %x\n", PlayerEvent->Code);
                StreamEvent.code                = STREAM_EVENT_INVALID;
                return HavanaError;
        }
        //}}}  
        if ((EventSignalCallback != NULL) && (CallbackContext != NULL))
        {
            EventSignalCallback (CallbackContext, &StreamEvent);        // Pass event on to outside world
            StreamEvent.code                    = STREAM_EVENT_INVALID; // and reset to invalid so not sent twice.
            return HavanaNoError;
        }
        else
            STREAM_DEBUG("Callback pointers not valid %p, %p\n", EventSignalCallback, CallbackContext);
    }

    return HavanaError;
}
//}}}  
//{{{  GetOutputWindow
HavanaStatus_t HavanaStream_c::GetOutputWindow (unsigned int*           X,
                                                unsigned int*           Y,
                                                unsigned int*           Width,
                                                unsigned int*           Height)
{
    ManifestorStatus_t  ManifestorStatus;

    //STREAM_DEBUG("\n");
    if (PlayerStreamType == StreamTypeVideo)
    {
        class Manifestor_Video_c*       VideoManifestor = (class Manifestor_Video_c*)Manifestor;

        ManifestorStatus        = VideoManifestor->GetOutputWindow (X, Y, Width, Height);
        if (ManifestorStatus != ManifestorNoError)
        {
            STREAM_ERROR("Failed to get output window dimensions\n");
            return HavanaError;
        }
    }

    return HavanaNoError;
}
//}}}  
//{{{  RegisterEventSignalCallback
stream_event_signal_callback HavanaStream_c::RegisterEventSignalCallback       (context_handle_t                Context,
                                                                                stream_event_signal_callback    Callback)
{
    stream_event_signal_callback        PreviousCallback        = EventSignalCallback;

    STREAM_DEBUG("\n");
    CallbackContext             = Context;
    EventSignalCallback         = Callback;

    if ((EventSignalCallback != NULL) && (CallbackContext != NULL) && (StreamEvent.code != STREAM_EVENT_INVALID))
    {
        EventSignalCallback (CallbackContext, &StreamEvent);            // Pass event on to outside world
        StreamEvent.code        = STREAM_EVENT_INVALID;                 // and reset to invalid so not sent twice.
    }

    return PreviousCallback;
}
//}}}  
//{{{  GetPlayerEnvironment
HavanaStatus_t HavanaStream_c::GetPlayerEnvironment    (PlayerPlayback_t*               PlayerPlayback,
                                                        PlayerStream_t*                 PlayerStream)
{
    //STREAM_DEBUG("\n");

    if ((this->PlayerStream == NULL) || (this->PlayerPlayback == NULL))
    {
        STREAM_ERROR ("One of the parameters is null. (%p, %p) \n", this->PlayerPlayback, this->PlayerStream);
        return HavanaError;
    }

    *PlayerPlayback     = this->PlayerPlayback;
    *PlayerStream       = this->PlayerStream;

    return HavanaNoError;
}
//}}}  


