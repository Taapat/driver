/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_video_rmv.h
Author :           Julian

Definition of the Real Media video collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
24-Oct-08   Created from existing collator_pes_video_wmv.h  Julian

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_RMV
#define H_COLLATOR_PES_VIDEO_RMV

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_pes_frame.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/*
The Real Video collator is very similar to the wmv collator because Real Video streams
do not contain start codes so frame length is passed from the container parser
in a private structure. This is passed into the player using the privade data
structure in the pes header.
*/

class Collator_PesVideoRmv_c : public Collator_PesFrame_c
{
};

#endif /* H_COLLATOR_PES_VIDEO_RMV */

