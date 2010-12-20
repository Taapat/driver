/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : collator_pes_audio_wma.cpp
Author :           Adam

Implementation of the pes collator class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created                                         Adam

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Collator_PesAudioWma_c
///
/// Implements WMA audio sync word scanning and frame length analysis.
/// Or rather - it doesn't.
/// Each pes should contain a 'Stream Properties Object' or a 'WMA Data Block'
/// and we want to just pass these on for the frame parser to distinguish
/// and treat accordingly. AFAICT.
///

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#define ENABLE_COLLATOR_DEBUG 0

#include "collator_pes_audio_wma.h"
#include "frame_parser_audio_wma.h"
#include "wma_audio.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//
#define WMA_HEADER_SIZE 0

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
Collator_PesAudioWma_c::Collator_PesAudioWma_c( void )
{
    if( InitializationStatus != CollatorNoError )
	return;

    Collator_PesAudioWma_c::Reset();
}

////////////////////////////////////////////////////////////////////////////
///
/// Search for the WMA audio synchonrization word and, if found, report its offset.
///
/// No sync word, just fine to carry on as is.
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioWma_c::FindNextSyncWord( int *CodeOffset )
{
    *CodeOffset = 0;
    return CollatorNoError;
}

////////////////////////////////////////////////////////////////////////////
///
/// As far as I understand it thus far, we just want to hand the entire
/// pes data on to be processed by the frame_parser
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioWma_c::DecideCollatorNextStateAndGetLength( unsigned int *FrameLength )
{
    CollatorStatus_t Status = CollatorNoError;

    *FrameLength = RemainingElementaryLength;
    CollatorState = GotCompleteFrame;

    return Status;
}


////////////////////////////////////////////////////////////////////////////
///
/// Sets the extended pes header length according to the stream_id field of the pes header
///
/// \return void
///
void  Collator_PesAudioWma_c::SetPesPrivateDataLength(unsigned char SpecificCode)
{
  /* do nothing, configuration already set to the right value... */
}

////////////////////////////////////////////////////////////////////////////
///
/// Examine the PES private data header.
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t Collator_PesAudioWma_c::HandlePesPrivateData(unsigned char *PesPrivateData)
{
  /* do nothing, configuration already set to the right value... */
    return CollatorNoError;
}

CollatorStatus_t Collator_PesAudioWma_c::Reset( void )
{
    CollatorStatus_t Status;

    COLLATOR_DEBUG(">><<\n");

    Status = Collator_PesAudio_c::Reset();
    if( Status != CollatorNoError )
	return Status;

    // FrameHeaderLength belongs to Collator_PesAudio_c so we must set it after the class has been reset    
    FrameHeaderLength = WMA_HEADER_SIZE;

    Configuration.StreamIdentifierMask       = PES_START_CODE_MASK;
    Configuration.StreamIdentifierCode       = PES_START_CODE_AUDIO;
    Configuration.BlockTerminateMask         = 0xff;         // Picture
    Configuration.BlockTerminateCode         = 0x00;
    Configuration.IgnoreCodesRangeStart      = 0x01; // All slice codes
    Configuration.IgnoreCodesRangeEnd        = PES_START_CODE_PRIVATE_STREAM_1 - 1;
    Configuration.InsertFrameTerminateCode   = false;
    Configuration.TerminalCode               = 0;
    Configuration.ExtendedHeaderLength       = 0;       //private data area after pes header
    Configuration.DeferredTerminateFlag      = false;
    //Configuration.DeferredTerminateCode[0]   =  Configuration.BlockTerminateCode;
    //Configuration.DeferredTerminateCode[1]   =  Configuration.BlockTerminateCode;

    PassPesPrivateDataToElementaryStreamHandler = false;

    return CollatorNoError;
}
