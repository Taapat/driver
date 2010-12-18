/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : demultiplexor_ts.h
Author :           Nick

Definition of the transport stream demultiplexor class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
13-Nov-06   Created                                         Nick

************************************************************************/

#ifndef H_DEMULTIPLEXOR_TS
#define H_DEMULTIPLEXOR_TS

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers
#include "demultiplexor_base.h"
#include "dvb.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
//#define DO_NICKS_SUGGESTED_IMPROVEMENTS
#ifdef DO_NICKS_SUGGESTED_IMPROVEMENTS
#define ACCUMULATION_BUFFER_SIZE 8192
#endif

#define DEMULTIPLEXOR_SELECT_ON_PRIORITY	0x00002000
#define DEMULTIPLEXOR_PRIORITY_HIGH		0x00004000
#define DEMULTIPLEXOR_PRIORITY_LOW		0x00000000

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

typedef struct DemultiplexorStreamContext_s
{
    bool		ValidExpectedContinuityCount;
    unsigned char	ExpectedContinuityCount;
#ifdef DO_NICKS_SUGGESTED_IMPROVEMENTS
    unsigned char       AccumulationBuffer[ACCUMULATION_BUFFER_SIZE];
    int                 AccumulationBufferPointer;
#endif
    bool                SelectOnPriority;
    bool                DesiredPriority;
} DemultiplexorStreamContext_t;

//

struct DemultiplexorContext_s
{
    struct DemultiplexorBaseContext_s	Base;

    DemultiplexorStreamContext_t	Streams[DEMULTIPLEXOR_MAX_STREAMS];
    unsigned char			PidTable[DVB_MAX_PIDS];
};


// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Demultiplexor_Ts_c : public Demultiplexor_Base_c
{
private:

    // Data

    // Functions

public:

    //
    // Constructor/Destructor methods 
    //

    Demultiplexor_Ts_c( void );

    //
    // API functions
    //

    DemultiplexorStatus_t   GetHandledMuxType(	PlayerInputMuxType_t	 *HandledType );

    DemultiplexorStatus_t   AddStream(		DemultiplexorContext_t	  Context,
						PlayerStream_t		  Stream,
						unsigned int		  StreamIdentifier );

    DemultiplexorStatus_t   RemoveStream(	DemultiplexorContext_t	  Context,
						unsigned int		  StreamIdentifier );

    DemultiplexorStatus_t   InputJump(		DemultiplexorContext_t	  Context );

    DemultiplexorStatus_t   Demux(		PlayerPlayback_t	  Playback,
						DemultiplexorContext_t	  Context,
						Buffer_t		  Buffer );
};

#endif

