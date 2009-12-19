/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_wma.h
Author :           Adam

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created from existing collator_pes_audio_mpeg.h Adam

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_WMA
#define H_COLLATOR_PES_AUDIO_WMA

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"
#include "wma_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioWma_c : public Collator_PesAudio_c
{

protected:

    CollatorStatus_t FindNextSyncWord( int *CodeOffset );
    CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
    void             SetPesPrivateDataLength(unsigned char SpecificCode);
    CollatorStatus_t HandlePesPrivateData(unsigned char *PesPrivateData);
    
public:

    Collator_PesAudioWma_c();

    CollatorStatus_t   Reset(			void );
};

#endif // H_COLLATOR_PES_AUDIO_WMA

