/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : collator_pes_audio_lpcm.h
Author :           Sylvain

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
12-Jul-07   Creation                                        Sylvain

************************************************************************/

#ifndef H_COLLATOR_PES_AUDIO_LPCM
#define H_COLLATOR_PES_AUDIO_LPCM

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_audio.h"
#include "lpcm.h"

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
 private:
  ///
  LpcmAudioParsedFrameHeader_t ParsedFrameHeader;
  LpcmAudioParsedFrameHeader_t NextParsedFrameHeader;
  unsigned int                 GuessedNextFirstAccessUnit; ///< In debug mode, try to guess the next first access unit pointer
  bool                         IsPesPrivateDataAreaNew;    ///< Indicates if the private dara area of the pes has some key parameters 
                                                           ///< different from the previous one
  bool                         AccumulatePrivateDataArea;  ///< Indicates if it there is a ned to store the pda at the beginning of the frame
  bool                         IsPesPrivateDataAreaValid;  ///< Validity of Private Data Area
  bool                         IsFirstPacket;

  unsigned char                NewPesPrivateDataArea[LPCM_MAX_PRIVATE_HEADER_LENGTH];
  int                          AccumulatedFrameNumber;
  unsigned char                StreamId;
  int                          RemainingDataLength;
  int                          PesPrivateToSkip;
  int                          GlobbedFramesOfNewPacket;

protected:
  
  CollatorStatus_t FindNextSyncWord( int *CodeOffset );
  
  CollatorStatus_t DecideCollatorNextStateAndGetLength( unsigned int *FrameLength );
  void             SetPesPrivateDataLength(unsigned char SpecificCode);
  CollatorStatus_t HandlePesPrivateData(unsigned char *PesPrivateData);
  void             ResetCollatorStateAfterForcedFrameFlush();
    
public:

  LpcmStreamType_t StreamType;

  Collator_PesAudioLpcm_c( LpcmStreamType_t StreamType );
  
  CollatorStatus_t   Reset(			void );

};

#endif // H_COLLATOR_PES_AUDIO_LPCM

