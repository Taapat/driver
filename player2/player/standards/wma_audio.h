/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : wma.h
Author :           Sylvain

Definition of the types and constants that are used by several components
dealing with wma audio decode/display for havana.


Date        Modification                                    Name
----        ------------                                    --------
24-Apr-06   Created                                         Mark
06-June-07  Ported to player2                               Sylvain

************************************************************************/

#ifndef H_WMA_
#define H_WMA_

#include "pes.h"
#include "asf.h"

// ////////////////////////////////////////////////////////////////////////////
//
//  General defines of start codes etc.
//
#define WMA_FRAME_HEADER_SIZE                 0

////////////////////////////////////////////////////////////////////
///
/// Exploded copy of the WMA audio frame header.
///
typedef struct WmaAudioParsedFrameHeader_s
{

} WmaAudioParsedFrameHeader_t;

////////////////////////////////////////////////////////////////

typedef struct WmaAudioStreamParameters_s
{
//    unsigned int Unused;

    unsigned int        StreamNumber;
    unsigned int        FormatTag;
    unsigned int        NumberOfChannels;
    unsigned int        SamplesPerSecond;
    unsigned int        AverageNumberOfBytesPerSecond;
    unsigned int        BlockAlignment;
    unsigned int        BitsPerSample;
    unsigned int        ValidBitsPerSample;
    unsigned int        ChannelMask;
    unsigned int        SamplesPerBlock;
    unsigned int        EncodeOptions;
    unsigned int        SuperBlockAlign;
    // calculated parameters
    unsigned int        SamplesPerFrame;
    unsigned int        SamplingFrequency;
//


} WmaAudioStreamParameters_t;

#define BUFFER_WMA_AUDIO_STREAM_PARAMETERS        "WmaAudioStreamParameters"
#define BUFFER_WMA_AUDIO_STREAM_PARAMETERS_TYPE   {BUFFER_WMA_AUDIO_STREAM_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(WmaAudioStreamParameters_t)}

////////////////////////////////////////////////////////////////
///
/// Meta-data unique to WMA audio.
///
/// \todo This is actually pretty generic stuff; if nothing radical emerges
///       in the other audio codecs perhaps this should be combined. Alternatively
///       just get rid of this type entirely and rename MpegAudioParsedFrameHeader_t.
///
typedef struct WmaAudioFrameParameters_s
{
    /// The bit rate of the frame
    unsigned int BitRate;

    /// Size of the compressed frame (in bytes)
    unsigned int FrameSize;
} WmaAudioFrameParameters_t;

#define BUFFER_WMA_AUDIO_FRAME_PARAMETERS        "WmaAudioFrameParameters"
#define BUFFER_WMA_AUDIO_FRAME_PARAMETERS_TYPE   {BUFFER_WMA_AUDIO_FRAME_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(WmaAudioFrameParameters_t)}


#endif /* H_WMA_ */
