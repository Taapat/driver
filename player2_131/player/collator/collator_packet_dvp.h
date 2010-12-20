/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : collator_packet_dvp.h
Author :           Nick

Definition of the dvp extension to the basic packet input


Date        Modification                                    Name
----        ------------                                    --------
08-Jul-08   Created					    Nick

************************************************************************/

#ifndef H_COLLATOR_PACKET_DVP
#define H_COLLATOR_PACKET_DVP

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_packet.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PacketDvp_c : public Collator_Packet_c
{
protected:

public:

    //
    // Collator class functions
    //

    CollatorStatus_t   Input(	PlayerInputDescriptor_t  *Input,
				unsigned int		  DataLength,
				void                     *Data );
};

#endif

