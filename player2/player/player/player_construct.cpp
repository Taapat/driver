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

#ifdef __TDT__
extern int discardlateframe;
#endif

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

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesAfterSynchronize );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyVideoStartImmediate,					PolicyValueApply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyRebaseOnFailureToDeliverDataInTime,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyRebaseOnFailureToDecodeInTime,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyLowerCodecDecodeLimitsOnFailureToDecodeInTime,	PolicyValueDisapply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264AllowNonIDRResynchronization,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264AllowBadPreProcessedFrames,			PolicyValueApply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyH264TreatDuplicateDpbValuesAsNonReferenceFrameFirst,	PolicyValueApply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2ApplicationType,				PolicyValueMPEG2ApplicationDvb );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2DoNotHonourProgressiveFrameFlag,		PolicyValueDisapply );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyClockPullingLimit2ToTheNPartsPerMillion,		8 );

    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyLimitInputInjectAhead,				PolicyValueDisapply );

#if 0
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyAVDSynchronization,   				PolicyValueDisapply );
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesNever );
//    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMasterClock,                                         PolicyValueSystemClockMaster );
#endif

//
#ifdef __TDT__
    // Overwrite default settings (see class_definitions/player_types.h for definitions
    // and descriptions).

    // Discarding late frames significantly improves the H.264 deadlock behavior.
    // Obviously late frames cause the player to run out of buffers if not discarded
    // immediately. It even does not matter how many decode buffers are available -
    // all of them will be used (up to 32 if enough memory is availble in LMI_VID).
    if(discardlateframe == 0)
    	SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesNever );
    else if(discardlateframe == 1)
    	SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesAlways );
    else
    	SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyDiscardLateFrames,	 				PolicyValueDiscardLateFramesAfterSynchronize );
    // Usage of the immediate start depends on the LateFrame policy. Though, it is not
    // quite clear what is meant by "agressive policy".
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyVideoStartImmediate,					PolicyValueApply );

    // This will fix N24 an hopefully every other interlaced-progressive miss interpretertation 
    SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyMPEG2DoNotHonourProgressiveFrameFlag,		PolicyValueApply );

    //SetPolicy( PlayerAllPlaybacks, PlayerAllStreams, PolicyLimitInputInjectAhead,				PolicyValueApply );
#endif

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


