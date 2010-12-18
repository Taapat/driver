/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : codec_dummy.h
Author :           Nick

Definition of the dummy codec class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
16-Jan-07   Created                                         Nick

************************************************************************/

#ifndef H_CODEC_DUMMY
#define H_CODEC_DUMMY

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "player.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Codec_Dummy_c : public Codec_c
{
public:

    CodecTrickModeParameters_t	  CodecTrickModeParameters;

    //
    // Constructor/Destructor methods
    //

    Codec_Dummy_c( 	void ){ report( severity_info, "Codec_Dummy_c - Called\n" ); }
    ~Codec_Dummy_c( 	void ){ report( severity_info, "~Codec_Dummy_c - Called\n" ); }

    //
    // Other functions
    //

    CodecStatus_t   GetTrickModeParameters(	CodecTrickModeParameters_t *TrickModeParameters )
	    {
		CodecTrickModeParameters.SubstandardDecodeSupported	= false;
		CodecTrickModeParameters.MaximumNormalDecodeRate	= 2;
		CodecTrickModeParameters.MaximumSubstandardDecodeRate	= 3;
		CodecTrickModeParameters.DefaultGroupSize		= 12;
		CodecTrickModeParameters.DefaultGroupReferenceFrameCount= 4;

		*TrickModeParameters	= CodecTrickModeParameters;
		return CodecNoError;
	    }

//

    CodecStatus_t   SetModuleParameters(	unsigned int   ParameterBlockSize,
						void          *ParameterBlock )
		{ report( severity_info, "Codec_Dummy_c::SetModuleParameters - Called\n" ); return CodecNoError; }

    CodecStatus_t   DiscardQueuedDecodes(       void )
		{ report( severity_info, "Codec_Dummy_c::DiscardQueuedDecodes - Called\n" ); return CodecNoError; }

    CodecStatus_t   CheckReferenceFrameList(    unsigned int              NumberOfReferenceFrameLists,
                                                ReferenceFrameList_t      ReferenceFrameList[] )
		{ report( severity_info, "Codec_Dummy_c::CheckReferenceFrameList - Called\n" ); return CodecNoError; }

    CodecStatus_t   RegisterOutputBufferRing(	Ring_t		  Ring )
		{ report( severity_info, "Codec_Dummy_c::RegisterOutputBufferRing - Called\n" ); return CodecNoError; }

    CodecStatus_t   OutputPartialDecodeBuffers(	void )
		{ report( severity_info, "Codec_Dummy_c::OutputPartialDecodeBuffers - Called\n" ); return CodecNoError; };

    CodecStatus_t   ReleaseReferenceFrame(	unsigned int	  ReferenceFrameDecodeIndex )
		{ report( severity_info, "Codec_Dummy_c::ReleaseReferenceFrame - Called\n" ); return CodecNoError; }

    CodecStatus_t   ReleaseDecodeBuffer(	Buffer_t	  Buffer )
		{ report( severity_info, "Codec_Dummy_c::ReleaseDecodeBuffer - Called\n" ); return CodecNoError; }

    CodecStatus_t   Input(			Buffer_t	  CodedBuffer )
		{ report( severity_info, "Codec_Dummy_c::Input - Called\n" ); /*BufferManager->Dump(DumpAll);*/ return CodecNoError; }
};

#endif

