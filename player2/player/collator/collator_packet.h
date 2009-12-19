/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006,2008

Source file name : collator_packet.h
Author :           Nick

Definition of the base collator packet class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
08-Jul-08   Created                                         Nick

************************************************************************/

#ifndef H_COLLATOR_PACKET
#define H_COLLATOR_PACKET

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_base.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_Packet_c : public Collator_Base_c
{
protected:

    // Data

    // Functions

//

public:

    //
    // Constructor/Destructor methods
    //

    Collator_Packet_c( 	void );

    //
    // Base class overrides
    //

    CollatorStatus_t   Reset( void );

    //
    // Collator class functions
    //

    CollatorStatus_t   Input(	PlayerInputDescriptor_t  *Input,
				unsigned int		  DataLength,
				void                     *Data );
};

#endif


