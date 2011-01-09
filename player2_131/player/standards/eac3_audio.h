/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : eac3_audio.h
Author :           Nick

Definition of the types and constants that are used by several components
dealing with ac3 audio decode/display for havana.


Date        Modification                                    Name
----        ------------                                    --------
08-Jan-03   Created                                         Nick
18-May-07   Ported to player2                               Daniel

************************************************************************/

#ifndef H_EAC3_AUDIO
#define H_EAC3_AUDIO

#include "pes.h"

// ////////////////////////////////////////////////////////////////////////////
//
//  General defines of start codes etc.
//

#define EAC3_START_CODE                          0x0b77
#define EAC3_START_CODE_MASK                     0xffff
#define EAC3_FRAME_HEADER_SIZE                   2
#define EAC3_EXTRA_INFO_SIZE                     4
#define EAC3_REACH_CONVSYNC                      59 // this is the theoretical number of bytes needed to reach the convsync info...
#define EAC3_BYTES_NEEDED                        (EAC3_FRAME_HEADER_SIZE + EAC3_EXTRA_INFO_SIZE + EAC3_REACH_CONVSYNC)
#define EAC3_NBSAMPLES_NEEDED                    1536
#define EAC3_MAX_INDEPENDANT_SUBFRAME_COUNT      6  // max number of accumulated independant subframes to reach 1536 samples 
#define EAC3_MAX_DEPENDANT_SUBFRAME_COUNT        8
#define EAC3_AC3_MAX_FRAME_SIZE                  3840

// ////////////////////////////////////////////////////////////////////////////
//
//  Definitions of types matching AC3 audio headers
//

////////////////////////////////////////////////////////////////
///
/// EAC3 audio stream type, for synchronization of E-AC3 frames
///
///
typedef enum
{
  TypeEac3Ind,  ///< frame is an e-ac3 independant substream
  TypeAc3,      ///< frame is an ac3 frame
  TypeEac3Dep,  ///< frame is an e-ac3 dependant substream
  TypeNotSynch  ///< frame is not eac3 nor ac3
} Ac3StreamType_t;


///////////////////////////////////////////////////////////////////////////////
///
/// Exploded copy of the first four bytes of the AC3 audio frame header.
///
typedef struct EAc3AudioParsedFrameHeader_s
{
	// Directly interpretted values
	Ac3StreamType_t Type;
	unsigned char   SubStreamId;
	unsigned int    SamplingFrequency; ///< Sampling frequency in Hz.
	
	// Derived values
	unsigned int    NumberOfSamples; ///< Number of samples per channel within the frame.
	unsigned int    Length; ///< Length of frame in bytes (including header).
	bool            FirstBlockForTranscoding; ///< indicates if the subframe is the first block (out of six for transcoding)
	
} EAc3AudioParsedFrameHeader_t;

////////////////////////////////////////////////////////////////

typedef struct EAc3AudioStreamParameters_s
{
    unsigned int Unused;
} EAc3AudioStreamParameters_t;

#define BUFFER_EAC3_AUDIO_STREAM_PARAMETERS        "EAc3AudioStreamParameters"
#define BUFFER_EAC3_AUDIO_STREAM_PARAMETERS_TYPE   {BUFFER_EAC3_AUDIO_STREAM_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(EAc3AudioStreamParameters_t)}

//

////////////////////////////////////////////////////////////////
///
/// Meta-data unique to EAC3 audio.
///
/// \todo This is actually pretty generic stuff; if nothing radical emerges
///       in the other audio codecs perhaps this should be combined. Alternatively
///       just get rid of this type entirely and rename Ac3AudioParsedFrameHeader_t.
///
typedef struct EAc3AudioFrameParameters_s
{
    /// The bit rate of the frame
    unsigned int BitRate;
    
    /// Size of the compressed frame (in bytes)
    unsigned int FrameSize;
} EAc3AudioFrameParameters_t;

#define BUFFER_EAC3_AUDIO_FRAME_PARAMETERS        "EAc3AudioFrameParameters"
#define BUFFER_EAC3_AUDIO_FRAME_PARAMETERS_TYPE   {BUFFER_EAC3_AUDIO_FRAME_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(EAc3AudioFrameParameters_t)}

#endif /* H_AC3_AUDIO */
