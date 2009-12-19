/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : collator_pes_audio_lpcm.cpp
Author :           Adam

Implementation of the pes collator class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
09-Jul-07   Created                                         Adam

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Collator_PesAudioLpcm_c
///
/// Implements LPCM audio sync word scanning and frame length analysis.
///

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#define ENABLE_COLLATOR_DEBUG 0

#include "collator_pes_audio_lpcm.h"
#include "frame_parser_audio_lpcm.h"
#include "lpcm_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
#define LPCM_HEADER_SIZE 0

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

////////////////////////////////////////////////////////////////////////////
///
/// Initialize the class by resetting it.
///
/// During a constructor calls to virtual methods resolve to the current class (because
/// the vtable is still for the class being constructed). This means we need to call
/// ::Reset again because the calls made by the sub-constructors will not have called
/// our reset method.
///
Collator_PesAudioLpcm_c::Collator_PesAudioLpcm_c( void )
{
    if( InitializationStatus != CollatorNoError )
	return;

    Collator_PesAudioLpcm_c::Reset();
}

////////////////////////////////////////////////////////////////////////////
///
/// Search for the LPCM audio synchonrization word and, if found, report its offset.
///
/// Not relevant - OR IS IT?
///                don't we need to make sure we skip until the pes/pda that represents
///                the start of a 20frame collection?
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioLpcm_c::FindNextSyncWord( int *CodeOffset )
{
    int audio_frame_number      = (RemainingElementaryData[-3] & 0x1F);
    int number_of_frame_headers = RemainingElementaryData[-6];

    COLLATOR_DEBUG("LPCM: FINDSYNC - RemainingElementaryData %02x %02x %02x %02x %02x %02x %02x \n",
       RemainingElementaryData[-7],RemainingElementaryData[-6],RemainingElementaryData[-5],RemainingElementaryData[-4],
       RemainingElementaryData[-3],RemainingElementaryData[-2],RemainingElementaryData[-1]
    );

    *CodeOffset = (((int)RemainingElementaryData[-5] << 8) | (int)RemainingElementaryData[-4]) - 4; //first_access_unit_pointer-4
												    //-4 because we don't need to worry
												    //about rest of PDA

    //examine the PDA 'audio_frame_number' and if not start of GOF (group of frames)
    //then do not establish sync. Perhaps we can set SubFrameNumber appropriately and
    //insert a modified PDA to save wasting data, but I expect the maximum waste of
    //19/600 of a second is live-with-able
    if(audio_frame_number != 0) {
	//now, just because it's not zero, doesn't mean a GOF doesn't start in this pes.
	if((audio_frame_number + number_of_frame_headers) > 20) {
	    //yay! it DOES start in this pes.
	    //now, calculate the offset for audio_frame_number 0

	    //first, how many frames in is it?
	    int audio_frame_number_zero = 20 - audio_frame_number;

	    //second, find the frame size
	    int audio_frame_size = ((((RemainingElementaryData[-2] & 0x30) + 1) >> 4) * 80) *  //sample freq/ num samples
				     ((RemainingElementaryData[-2] & 0x07) + 1);               //num channels

	    switch( ((RemainingElementaryData[-2] & 0xC0)>>6) ) {       //quantization
		case 0: audio_frame_size *= 2;                          //16 bit
			break;
		case 1: audio_frame_size *= 5;
			audio_frame_size = audio_frame_size >> 1;       //20 bit
			break;
		case 2: audio_frame_size *= 3;                          //24 bit
			break;
	    }
//            report(severity_error, "20 1stAU Ptr %x audio_frame_number_zero %d audio_frame_size %x  - %x\n",
//                                       *CodeOffset, audio_frame_number_zero,   audio_frame_size,
//                                       (*CodeOffset+(audio_frame_number_zero*audio_frame_size)) );
	    //third, add on the offset
	    *CodeOffset += (audio_frame_number_zero * audio_frame_size);

	} else {
	    //of course, if we're in 96kHz, we only want bundles of 10 anyway...
	    if( ((audio_frame_number + number_of_frame_headers) > 10) && (((RemainingElementaryData[-2] & 0x30) >> 4) ==1) ) {
		//yay! it DOES start in this pes.
		//now, calculate the offset for audio_frame_number 0

		//first, how many frames in is it?
		int audio_frame_number_ten = 10 - audio_frame_number;

		//second, find the frame size
//                int audio_frame_size = (((RemainingElementaryData[-2] & 0x30) >> 4) * 80) *   //sample freq/ num samples
//                                        ((RemainingElementaryData[-2] & 0x07) +  1);          //num channels
		int audio_frame_size = ((((RemainingElementaryData[-2] & 0x30) >> 4) + 1) * 80) *  //sample freq/ num samples
					 ((RemainingElementaryData[-2] & 0x07) + 1);               //num channels

		switch( ((RemainingElementaryData[-2] & 0xC0)>>6) ) {   //quantization
		    case 0: audio_frame_size *= 2;                          //16 bit
			    break;
		    case 1: audio_frame_size *= 5;
			    audio_frame_size = audio_frame_size >> 1;       //20 bit
			    break;
		    case 2: audio_frame_size *= 3;                          //24 bit
			    break;
		}
//    report(severity_error, "LPCM: FINDSYNC - RemainingElementaryData %02x %02x %02x %02x %02x %02x %02x \n",
//       RemainingElementaryData[-7],RemainingElementaryData[-6],RemainingElementaryData[-5],RemainingElementaryData[-4],
//       RemainingElementaryData[-3],RemainingElementaryData[-2],RemainingElementaryData[-1]
//    );
//                report(severity_error, "10 1stAU  sample freq/ num samples %d  num channels %d\n",
//                                          ((RemainingElementaryData[-2] & 0x30) + 1),((RemainingElementaryData[-2] & 0x07) + 1) );
//                report(severity_error, "10 1stAU Ptr %x audio_frame_number_ten %d audio_frame_size %x  - %x\n",
//                                          *CodeOffset, audio_frame_number_ten,   audio_frame_size,
//                                          (*CodeOffset+(audio_frame_number_ten*audio_frame_size)) );

		//third, add on the offset
		*CodeOffset += (audio_frame_number_ten * audio_frame_size);

		//do set up as if DecideCollatorNextStateAndGetLength had already been through
		//for 10 subframes and one flush
		memcpy( CurrentPesPrivateData, NextPesPrivateData, 7 );
		SubFrameNumber = 11;
	    } else {
		//oh well, we tried almost everything
		return CollatorError;
	    }
	}
    }
    return CollatorNoError;

}

////////////////////////////////////////////////////////////////////////////
///
/// Determine the new state of the collator according to the incoming sub frame
/// Also returns this sub frame length
///
/// If in 96kHz mode, we'll set state to GotCompleteFrame after counting up
/// 10 subframes - and we'll need to AccumulateData the private data header
/// again for the subsequent 10 frames. (with new PTS too) 
///
/// Otherwise, we just go 
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioLpcm_c::DecideCollatorNextStateAndGetLength( unsigned int *FrameLength )
{
    CollatorStatus_t Status = CollatorNoError;

    unsigned int Flush = 0;
    unsigned int NewPesNumSubFrames = 0;
    unsigned char NewPesPrivateData[8];

    COLLATOR_DEBUG("LPCM: NEXT-STATE&LENGTH\n");

    //BTW, this variable needs a better name, as it counts subframes AND flushes
    SubFrameNumber++;

    PlaybackTime += 150; //add 1/600th of a second to PES PTS for each subframe we advance
			 //so that when pass frame on, we pass with corrected PTS that
			 //should fit with the synthetic PTS it will compare with.

    if(ParsedFrameHeader.AudioSamplingFrequency == LPCM_SAMPLING_FREQ_96KHZ) {

	switch(SubFrameNumber) {
	    case 1:
		//get PDA for new frame (previously stored by HandlePesPrivateData)
		//copy off PDA here, so retained for the fake PDA we
		//generate when accumulated 10 or 20 frames - just in case overwritten
		//by new pes. Not likely, but 16bit mono would be 10*2*80 = 1600bytes,
		//so it could happen.
		memcpy( CurrentPesPrivateData, NextPesPrivateData, 7 );
		NewPesNumSubFrames = 10;  //need to insert modified PDA 
		break;
	    case 11:
		Flush = 1;                //need to flush frame
		break;
	    case 12:
		NewPesNumSubFrames = 10;  //need to insert modified PDA 
		break;
	    case 22:
		Flush = 1;                //need to flush frame
	       break;
	    default:
		break;                    //business as usual
	}

    } else {

	if(SubFrameNumber == 1) {
	     //get PDA for new frame (previously stored by HandlePesPrivateData)
	    memcpy( CurrentPesPrivateData, NextPesPrivateData, 7 );
	    NewPesNumSubFrames = 20;      //need to insert modified PDA
	} else if(SubFrameNumber == 21) {
	    Flush = 1;                    //need to flush frame
	} //else, business as usual

    }

    //Insert modified PDA if requested
    if(NewPesNumSubFrames) {
	//get PDA for new frame
	memcpy( NewPesPrivateData, CurrentPesPrivateData, 7 );
	//modify
	NewPesPrivateData[1] = NewPesNumSubFrames;             //number_of_frame_headers
	NewPesPrivateData[2] = 0;                              //first_access_unit_pointer
	NewPesPrivateData[3] = 4;                              //first_access_unit_pointer
	NewPesPrivateData[4] = (NewPesPrivateData[4] & 0xE0);  //audio_frame_number 

	//insert
	Status = AccumulateData( (unsigned int) 8, (unsigned char*)&NewPesPrivateData );
	COLLATOR_DEBUG("LPCM: NEXT-StateLen - FakePDA (AccumDat ret %d)\n",Status);
    } 

    //Flush subframes (10 or 20 of them)  OR  go and Accumulate another subframe
    if(Flush) {
	//need to flush frame to parser
	if(  (ParsedFrameHeader.AudioSamplingFrequency == LPCM_SAMPLING_FREQ_96KHZ) &&
	     (SubFrameNumber == 11)  )
	{
	    COLLATOR_DEBUG("LPCM: NEXT-StateLen - Flush - sf:%d (w/o subfrm cnt reset)\n",SubFrameNumber);
	} else {
	    COLLATOR_DEBUG("LPCM: NEXT-StateLen - Flush - sf:%d \n",SubFrameNumber);
	    SubFrameNumber = 0;
	}
	*FrameLength   = 0;
	CollatorState = GotCompleteFrame;
    } else {
	COLLATOR_DEBUG("LPCM: NEXT-StateLen - NxtSubFrame - sf:%d \n",SubFrameNumber);
	//business as usual
	*FrameLength  = ParsedFrameHeader.Length;

	CollatorState = ReadSubFrame;
    }

    return Status;
}


////////////////////////////////////////////////////////////////////////////
///
/// Sets the extended pes header length according to the stream_id field of the pes header
///
/// \return void
///
void  Collator_PesAudioLpcm_c::SetPesPrivateDataLength(unsigned char SpecificCode)
{
  /* do nothing, configuration already set to the right value... */
}

////////////////////////////////////////////////////////////////////////////
///
/// Examine the PES private data header.
///
/// Here we shall compare with the previous, to figure out if this is the
/// continuance of a GOF (Group Of Frames), or the start of a new one.
/// If the same, do nothing as AccumulateData shall continue to munch through
/// the PES with out interference.
/// If is a new GOF, we need to calculate the offset in subframes, from the start
/// of the first complete subframe to the start of the GOF indicated by the PDA.
/// This value will need to be multiplied by 1/600 sec to adjust the PTS for the frame/GOF
///
/// This is also true if in 96kHz mode, and splitting 20 subframe GOFs into 2 lots of 10.
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioLpcm_c::HandlePesPrivateData(unsigned char *PesPrivateData)
{
    CollatorStatus_t Status = CollatorNoError;

	COLLATOR_DEBUG("LPCM: NEW PDA --==<< %02x %02x %02x %02x - %02x %02x %02x %02x >>==--\n",
			PesPrivateData[0],PesPrivateData[1],PesPrivateData[2],PesPrivateData[3],
			PesPrivateData[4],PesPrivateData[5],PesPrivateData[6],PesPrivateData[7]);

    if( (PesPrivateData[2] == 0) &&
	(PesPrivateData[3] == 4) &&
	((PesPrivateData[4] & 0x1F)==0)  ) {
	COLLATOR_DEBUG("LPCM: NEW PDA-CLEAN FRAME START\n");
	goto mismatch;  //frame number less
    }

    if(NextPesPrivateData[0] != PesPrivateData[0]) {
	COLLATOR_DEBUG("LPCM: NEW PDA-SUBSTREAM ID CHANGED\n");
	goto mismatch; //different substream?
    }

    if(NextPesPrivateData[5] != PesPrivateData[5]) {
	COLLATOR_DEBUG("LPCM: NEW PDA-FREQ/ETC CHANGED\n");
	goto mismatch; //quant/samp rate/num channels different
    }

    if((PesPrivateData[1] + (PesPrivateData[4] & 0x1F)) > 20) {
	COLLATOR_DEBUG("LPCM: NEW PDA-WRAP SUBFRAME (pes %d, subfr %d)\n",(PesPrivateData[4] & 0x1F),SubFrameNumber);
	goto mismatch;  //frame number less
    }

    return Status;

mismatch:

    memcpy( NextPesPrivateData, PesPrivateData, 7 );

    Status = FrameParser_AudioLpcm_c::ParseFrameHeader( PesPrivateData, &ParsedFrameHeader );

    COLLATOR_DEBUG("LPCM: Started new frame NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW\n");

    return Status;
}

CollatorStatus_t Collator_PesAudioLpcm_c::Reset( void )
{
    CollatorStatus_t Status;

    COLLATOR_DEBUG(">><<\n");

    Status = Collator_PesAudio_c::Reset();
    if( Status != CollatorNoError )
	return Status;

    // FrameHeaderLength belongs to Collator_PesAudio_c so we must set it after the class has been reset    
    FrameHeaderLength = LPCM_HEADER_SIZE;

    Configuration.StreamIdentifierMask       = PES_START_CODE_MASK;
    Configuration.StreamIdentifierCode       = PES_START_CODE_PRIVATE_STREAM_1; //PES_START_CODE_AUDIO;
    Configuration.BlockTerminateMask         = 0xff;         // Picture
    Configuration.BlockTerminateCode         = 0x00;
    Configuration.IgnoreCodesRangeStart      = 0x01; // All slice codes
    Configuration.IgnoreCodesRangeEnd        = PES_START_CODE_PRIVATE_STREAM_1 - 1;
    Configuration.InsertFrameTerminateCode   = false;
    Configuration.TerminalCode               = 0;
    Configuration.ExtendedHeaderLength       = 7;       //private data area after pes header
    Configuration.DeferredTerminateFlag      = false;

    PassPesPrivateDataToElementaryStreamHandler = false;

    SubFrameNumber     = 0;

    return CollatorNoError;
}
