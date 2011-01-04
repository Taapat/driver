/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_vorbis.h
Author :           Julian

Definition of the Ogg Vorbis collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
27-Jan-09   Created from existing collator_pes_video_wmv.h  Julian

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_VORBIS
#define H_COLLATOR_PES_AUDIO_VORBIS

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
The Ogg Vorbis is very similar to the wmv collator because .ogg files
contain the audio in frame chunks.  The frame length is passed from the container parser
in a private structure. This is passed into the player using the private data
structure in the pes header.
*/

class Collator_PesAudioVorbis_c : public Collator_PesFrame_c
{
};

#endif /* H_COLLATOR_PES_AUDIO_VORBIS */

