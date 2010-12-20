/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : collator_packet_dvp.cpp
Author :           Julian

Implementation of the packet collator for dvp video.


Date        Modification                                    Name
----        ------------                                    --------
08-Jul-08   Created                                         Julian

************************************************************************/

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_packet_dvp.h"
#include "dvp.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

///////////////////////////////////////////////////////////////////////////
//
// Resets and configures according to the requirements of this stream content
//

CollatorStatus_t Collator_PacketDvp_c::Input(	PlayerInputDescriptor_t  *Input,
						unsigned int		  DataLength,
						void                     *Data )
{
StreamInfo_t		*CapturedFrameDescriptor = (StreamInfo_t *)Data;
Buffer_t		 CapturedBuffer;

//

    AssertComponentState( "Collator_Packet_c::Input", ComponentRunning );

    //
    // Attach the decode buffer mentioned in the input packet
    // to the current coded data frame, to ensure release
    // at the appropriate time.
    //

    if( DataLength != sizeof(StreamInfo_t) )
    {
	report( severity_error, "Collator_Packet_c::Input - Packet is wrong size (%d != %d)\n", DataLength, sizeof(StreamInfo_t) );
	return CollatorError;
    }

    CapturedBuffer	= (Buffer_t)(CapturedFrameDescriptor->buffer_class);
    CodedFrameBuffer->AttachBuffer( CapturedBuffer );

    //
    // Perform the standard packet handling
    //

    return Collator_Packet_c::Input( Input, DataLength, Data );
}

