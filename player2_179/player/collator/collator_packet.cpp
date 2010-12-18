/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : collator_packet.cpp
Author :           Nick

Implementation of the generic packet collator class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
08-Jul-08   Created                                         Nick

************************************************************************/

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_packet.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

////////////////////////////////////////////////////////////////////////////
//
// Initialize the class by resetting it.
//

Collator_Packet_c::Collator_Packet_c( void )
{
    if( InitializationStatus != CollatorNoError )
	return;

    Collator_Packet_c::Reset();
}


////////////////////////////////////////////////////////////////////////////
//
// Resets and configures according to the requirements of this stream content
//

CollatorStatus_t Collator_Packet_c::Reset( void )
{
CollatorStatus_t Status;

//

    Status = Collator_Base_c::Reset();
    if( Status != CollatorNoError )
	return Status;

    Configuration.GenerateStartCodeList      = false;					// Packets have no start codes
    Configuration.MaxStartCodes              = 0;
    Configuration.StreamIdentifierMask       = 0x00;
    Configuration.StreamIdentifierCode       = 0x00;
    Configuration.BlockTerminateMask         = 0x00;
    Configuration.BlockTerminateCode         = 0x00;
    Configuration.IgnoreCodesRangeStart      = 0x00;
    Configuration.IgnoreCodesRangeEnd        = 0x00;	
    Configuration.InsertFrameTerminateCode   = false;
    Configuration.TerminalCode               = 0x00;
    Configuration.ExtendedHeaderLength       = 0;
    Configuration.DeferredTerminateFlag      = false;

    return CollatorNoError;
}


////////////////////////////////////////////////////////////////////////////
//
// Input, simply take the supplied packet and pass it on
//

CollatorStatus_t   Collator_Packet_c::Input(	PlayerInputDescriptor_t  *Input,
						unsigned int		  DataLength,
						void                     *Data )
{
CollatorStatus_t Status;

//

    AssertComponentState( "Collator_Packet_c::Input", ComponentRunning );

    //
    // Extract the descriptor timing information
    //

    ActOnInputDescriptor( Input );

    //
    // Transfer the packet to the next coded data frame and pass on
    //

    Status	= AccumulateData( DataLength, (unsigned char *)Data );
    if( Status != CollatorNoError )
	return Status;

    Status	= FrameFlush();
    if( Status != CollatorNoError )
        return Status;

//

    return CollatorNoError;
}

