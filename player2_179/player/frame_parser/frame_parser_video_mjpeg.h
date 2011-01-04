/************************************************************************
COPYRIGHT (C) ST Microelectronics 2009

Source file name : frame_parser_video_mjpeg.h
Author :           Andy

Definition of the frame parser video MJPEG class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------

************************************************************************/

#ifndef H_FRAME_PARSER_VIDEO_MJPEG
#define H_FRAME_PARSER_VIDEO_MJPEG

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "mjpeg.h"
#include "frame_parser_video.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//


// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_VideoMjpeg_c : public FrameParser_Video_c
{
private:

    // Data

    MjpegStreamParameters_t     CopyOfStreamParameters;
    MjpegStreamParameters_t*    StreamParameters;
    MjpegFrameParameters_t*     FrameParameters;

    // Functions

    FrameParserStatus_t         ReadStreamMetadata(                             void );
    FrameParserStatus_t         ReadStartOfFrame(                               void );
#if 0
    FrameParserStatus_t         ReadQuantizationMatrices(                       void );
    FrameParserStatus_t         ReadRestartInterval(                            void );
    FrameParserStatus_t         ReadHuffmanTables(                              void );
    FrameParserStatus_t         ReadStartOfScan(                                void );
#endif

    FrameParserStatus_t         CommitFrameForDecode(                           void );

public:

    // Constructor function
    FrameParser_VideoMjpeg_c( void );
    ~FrameParser_VideoMjpeg_c( void );

    // Overrides for component base class functions
    FrameParserStatus_t         Reset(                                          void );

    // FrameParser class functions
    FrameParserStatus_t         RegisterOutputBufferRing(                       Ring_t          Ring );

    // Stream specific functions
    FrameParserStatus_t         ReadHeaders(                                    void );
    //FrameParserStatus_t         RevPlayProcessDecodeStacks(                     void );
    //FrameParserStatus_t         RevPlayGeneratePostDecodeParameterSettings(     void );

};

#endif

