/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_aac.h
Author :           Adam

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
05-Jul-07   Created from existing collator_pes_video.h      Adam

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_AAC
#define H_COLLATOR_PES_AUDIO_AAC

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"
#include "aac_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioAac_c : public Collator_PesAudio_c
{
protected:

    CollatorStatus_t FindNextSyncWord( int *CodeOffset );
    CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
    void             SetPesPrivateDataLength(unsigned char SpecificCode);

    AacFormatType_t FormatType;
    
public:

    Collator_PesAudioAac_c();

    CollatorStatus_t   Reset(			void );
};

#endif // H_COLLATOR_PES_AUDIO_AAC

