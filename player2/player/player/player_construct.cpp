/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : player_construct.cpp
Author :           Nick

Implementation of the constructor and destructor functions of the
generic class implementation of player 2


Date        Modification                                    Name
----        ------------                                    --------
03-Nov-06   Created                                         Nick

************************************************************************/

#include "player_generic.h"
#include "st_relay.h"

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Useful defines/macros that need not be user visible
//

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Contructor function - Initialize our data
//

Player_Generic_c::Player_Generic_c( void )
{
unsigned int    i;

//

    InitializationStatus        = BufferError;

//

    OS_InitializeMutex( &Lock );

    ShutdownPlayer              = false;

    BufferManager               = NULL;
    PlayerControlStructurePool  = NULL;
    InputBufferPool             = NULL;
    DemultiplexorCount          = 0;
    ListOfPlaybacks             = NULL;

    memset( &PolicyRecord, 0x00, sizeof(PlayerPolicyState_t) );

    EventListHead               = INVALID_INDEX;
    EventListTail               = INVALID_INDEX;
    for( i=0; i<PLAYER_MAX_OUTSTANDING_EVENTS; i++ )
	EventList[i].Record.Code = EventIllegalIdentifier;

    for( i=0; i<PLAYER_MAX_EVENT_SIGNALS; i++ )
	ExternalEventSignals[i].Signal  = NULL;

    OS_InitializeEvent( &InternalEventSignal );

//

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, (PlayerPolicy_t)PolicyPlayoutAlwaysPlayout,		PolicyValuePlayout );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, (PlayerPolicy_t)PolicyPlayoutAlwaysDiscard,		PolicyValueDiscard );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMasterClock,						PolicyValueVideoClockMaster );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyManifestFirstFrameEarly,				PolicyValueApply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyExternalTimeMapping,					PolicyValueDisapply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyAVDSynchronization,   				PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyClampPlaybackIntervalOnPlaybackDirectionChange,	PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyTrickModeDomain,	 				PolicyValueTrickModeAuto );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, (PlayerPolicy_t)PolicyStatisticsOnAudio,			PolicyValueDisapply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, (PlayerPolicy_t)PolicyStatisticsOnVideo,			PolicyValueDisapply );

#ifdef player_before_131

#ifdef __TDT__
    // policies proposed to use for live tv - should solve audio issuses after zap
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesNever );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyVideoStartImmediate,					PolicyValueDisapply );
#else
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesAfterSynchronize );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyVideoStartImmediate,					PolicyValueApply );
#endif

#else
//Dagobert wyplay uses default ones so lets try this again!
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesAfterSynchronize );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyVideoStartImmediate,					PolicyValueApply );

#endif

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyRebaseOnFailureToDeliverDataInTime,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyRebaseOnFailureToDecodeInTime,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyLowerCodecDecodeLimitsOnFailureToDecodeInTime,	PolicyValueDisapply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264AllowNonIDRResynchronization,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264AllowBadPreProcessedFrames,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264TreatDuplicateDpbValuesAsNonReferenceFrameFirst,	PolicyValueApply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2ApplicationType,				PolicyValueMPEG2ApplicationDvb );
#ifdef __TDT__
//Thiw will fix N24 an hopefully every other interlaced-progressive miss interpretertation 
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2DoNotHonourProgressiveFrameFlag,		PolicyValueApply );
#else
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2DoNotHonourProgressiveFrameFlag,		PolicyValueDisapply );
#endif

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyClockPullingLimit2ToTheNPartsPerMillion,		8 );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyLimitInputInjectAhead,				PolicyValueDisapply );

#if 0
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyAVDSynchronization,   				PolicyValueDisapply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesNever );
//    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMasterClock,                                         PolicyValueSystemClockMaster );
#endif

//

    InitializationStatus        = BufferNoError;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Destructor function - close down
//

Player_Generic_c::~Player_Generic_c( void )
{
    ShutdownPlayer              = true;

    OS_TerminateEvent( &InternalEventSignal );
    OS_TerminateMutex( &Lock );
}


