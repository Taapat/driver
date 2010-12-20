/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_video_frame.h
Author :           Julian

Definition of the wmv collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created from existing collator_pes_audio_mpeg.h Julian

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_WMV
#define H_COLLATOR_PES_VIDEO_WMV

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
The wmv collator is a frame collator as Wmv streams
do not contain start codes so frame length is passed from the container parser
in a private structure. This is passed into the player using the privade data
structure in the pes header.
*/

class Collator_PesVideoWmv_c : public Collator_PesFrame_c
{
};

#endif /* H_COLLATOR_PES_VIDEO_WMV*/

