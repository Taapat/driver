/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_Rma.h
Author :           Julian

Definition of the Real Media Rma collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
27-Jan-09   Created from existing collator_pes_video_wmv.h  Julian

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_RMA
#define H_COLLATOR_PES_AUDIO_RMA

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
The Real Media Rma is very similar to the wmv collator because .ra files
contain the audio in frame chunks.  The frame length is passed from the container parser
in a private structure. This is passed into the player using the private data
structure in the pes header.
*/

class Collator_PesAudioRma_c : public Collator_PesFrame_c
{
};

#endif /* H_COLLATOR_PES_AUDIO_RMA */

