/************************************************************************
COPYRIGHT (C) ST Microelectronics 2007

Source file name : collator_pes_audio_mlp.h
Author :           Sylvain

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
08-Oct-07   Creation                                        Sylvain

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_MLP
#define H_COLLATOR_PES_AUDIO_MLP

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio_dvd.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioMlp_c : public Collator_PesAudioDvd_c
{
protected:

    CollatorStatus_t FindNextSyncWord( int *CodeOffset );
    CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
    void             SetPesPrivateDataLength( unsigned char SpecificCode );
    CollatorStatus_t HandlePesPrivateData( unsigned char *PesPrivateData );

    int AccumulatedFrameNumber;   ///< Holds the number of audio frames accumulated
    int NbFramesToGlob;           ///< Number of audio frames to glob (depends on sampling frequency)
    int StuffingBytesLength;      ///< Number of stuffing bytes in the pda (concerns MLP in DVD-Audio)
public:

    Collator_PesAudioMlp_c();

    CollatorStatus_t   Reset(			void );
};

#endif // H_COLLATOR_PES_AUDIO_MLP

