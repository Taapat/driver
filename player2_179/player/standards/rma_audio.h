/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : rma_audio.h
Author :           Julian

Definition of the constants/macros that define useful things associated with 
Real Media Rma audio streams.


Date        Modification                                    Name
----        ------------                                    --------
28-Jan-09   Created                                         Julian

************************************************************************/

#ifndef H_RMA_AUDIO
#define H_RMA_AUDIO

#include "pes.h"

// ////////////////////////////////////////////////////////////////////////////
//
//  General defines of start codes etc.
//
#define RMA_FRAME_HEADER_SIZE                 0
#define RMA_MAX_OPAQUE_DATA_SIZE              80

////////////////////////////////////////////////////////////////////
///
/// Exploded copy of the Rma audio frame header.
///
typedef struct RmaAudioParsedFrameHeader_s
{

} RmaAudioParsedFrameHeader_t;

////////////////////////////////////////////////////////////////

typedef struct RmaAudioStreamParameters_s
{
    unsigned int        Length;
    unsigned int        HeaderSignature;
    unsigned int        Version;
    unsigned int        RaSignature;
    unsigned int        Size;
    unsigned int        Version2;
    unsigned int        HeaderSize;
    unsigned int        CodecFlavour;
    unsigned int        CodedFrameSize;
    unsigned int        SubPacket;
    unsigned int        FrameSize;
    unsigned int        SubPacketSize;
    unsigned int        SampleRate;
    unsigned int        SampleSize;
    unsigned int        ChannelCount;
    unsigned int        InterleaverId;
    unsigned int        CodecId;
    unsigned int        CodecOpaqueDataLength;
    unsigned int        RmaVersion;
    unsigned int        SamplesPerFrame;
} RmaAudioStreamParameters_t;

#define BUFFER_RMA_AUDIO_STREAM_PARAMETERS        "RmaAudioStreamParameters"
#define BUFFER_RMA_AUDIO_STREAM_PARAMETERS_TYPE   {BUFFER_RMA_AUDIO_STREAM_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(RmaAudioStreamParameters_t)}

////////////////////////////////////////////////////////////////
///
/// Meta-data unique to Rma audio.
///
typedef struct RmaAudioFrameParameters_s
{
    /// The bit rate of the frame
    unsigned int        BitRate;

    /// Size of the compressed frame (in bytes)
    unsigned int        FrameSize;
} RmaAudioFrameParameters_t;

#define BUFFER_RMA_AUDIO_FRAME_PARAMETERS        "RmaAudioFrameParameters"
#define BUFFER_RMA_AUDIO_FRAME_PARAMETERS_TYPE   {BUFFER_RMA_AUDIO_FRAME_PARAMETERS, BufferDataTypeBase, AllocateFromOSMemory, 4, 0, true, true, sizeof(RmaAudioFrameParameters_t)}


#endif /* H_RMA_AUDIO_ */
