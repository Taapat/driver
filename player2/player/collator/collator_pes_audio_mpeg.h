/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_mpeg.h
Author :           Daniel

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
19-Apr-07   Created from existing collator_pes_video.h      Daniel

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_MPEG
#define H_COLLATOR_PES_AUDIO_MPEG

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioMpeg_c : public Collator_PesAudio_c
{
protected:

    CollatorStatus_t FindNextSyncWord( int *CodeOffset );
    CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
    void             SetPesPrivateDataLength(unsigned char SpecificCode);
    
public:

    Collator_PesAudioMpeg_c();

    CollatorStatus_t   Reset(			void );
};

#endif // H_COLLATOR_PES_AUDIO_MPEG

