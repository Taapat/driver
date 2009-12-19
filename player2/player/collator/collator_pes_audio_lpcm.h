/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_audio_lpcm.h
Author :           Adam

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
09-Jul-07   Created from existing collator_pes_audio_mpeg.h Adam

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_LPCM
#define H_COLLATOR_PES_AUDIO_LPCM

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"
#include "lpcm_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioLpcm_c : public Collator_PesAudio_c
{
    unsigned char CurrentPesPrivateData[8];	//PDA for building current frame (from GOF)
    unsigned char NextPesPrivateData[8];	//PDA from pes with GOF boundary, for when moving to next frame
    LpcmAudioParsedFrameHeader_t ParsedFrameHeader; //break down of PDA values
    unsigned int SubFrameNumber;     //for tracking when to flush a quantity of subframes
//    unsigned int RealSubFrameNumber; //for tracking whether next PDA is synced

protected:

    CollatorStatus_t FindNextSyncWord( int *CodeOffset );
    CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
    void             SetPesPrivateDataLength(unsigned char SpecificCode);
    CollatorStatus_t HandlePesPrivateData(unsigned char *PesPrivateData);
    
public:

    Collator_PesAudioLpcm_c();

    CollatorStatus_t   Reset(			void );
};

#endif // H_COLLATOR_PES_AUDIO_LPCM

