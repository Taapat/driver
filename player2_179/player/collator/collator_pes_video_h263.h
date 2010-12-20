/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_video_h263.h
Author :           Julian

Definition of the h263 collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
20-May-08   Created from existing collator_pes_video_wmv.h  Julian

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_H263
#define H_COLLATOR_PES_VIDEO_H263

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
The h263 collator is very similar to the wmv collator because h263 streams
do not contain start codes so frame length is passed from the container parser
in a private structure. This is passed into the player using the privade data
structure in the pes header.
*/

class Collator_PesVideoH263_c : public Collator_PesFrame_c
{
};

#endif /* H_COLLATOR_PES_VIDEO_H263 */

