/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : frame_parser_video_flv1.h
Author :           Julian

Definition of the frame parser video flv1 class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
19-May_08   Created                                         Julian

************************************************************************/

#ifndef H_FRAME_PARSER_VIDEO_FLV1
#define H_FRAME_PARSER_VIDEO_FLV1

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "h263.h"
#include "frame_parser_video_h263.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//


// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/// Frame parser for Flv1
class FrameParser_VideoFlv1_c : public FrameParser_VideoH263_c
{
private:

    FrameParserStatus_t         FlvReadPictureHeader(           void );

public:

    FrameParser_VideoFlv1_c(                                    void );

    FrameParserStatus_t         ReadHeaders(                    void );

};

#endif

