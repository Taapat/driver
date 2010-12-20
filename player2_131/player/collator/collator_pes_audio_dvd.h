/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2009

Source file name : collator_pes_audio_dvd.h
Author :           Daniel

Add DVD specific features to the audio bass class.


Date        Modification                                    Name
----        ------------                                    --------
08-Jan-09   Created.                                        Daniel

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_DVD
#define H_COLLATOR_PES_AUDIO_DVD

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesAudioDvd_c : public Collator_PesAudio_c
{
private:
    int SyncWordPrediction; ///< Expected location of the sync word (proves we are a DVD-style PES stream)
    int RemainingWildcardsPermitted; ///< Used to avoid getting stuck in *really* extreme cases.

    /// Used to force streams with broken first access unit pointers to work.
    int RemainingMispredictionsBeforeAutomaticWildcard;
    
protected:
    enum
    {
	WILDCARD_PREDICTION = -1,
	INVALID_PREDICTION = -2,
    };

    void AdjustDvdSyncWordPredictionAfterConsumingData(int Adjustment);
    void MakeDvdSyncWordPrediction(int Prediction);
    void ResetDvdSyncWordHeuristics();
    void VerifyDvdSyncWordPrediction(int Offset);

public:
    
    Collator_PesAudioDvd_c();
    CollatorStatus_t Reset();
};

#endif // H_COLLATOR_PES_AUDIO_DVD

