/************************************************************************
COPYRIGHT (C) STMicroelectronics R&D Ltd 2007

Source file name : frame_parser_video_wmv.h
Author :           Julian

Definition of the frame parser video vc1 class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
26-Nov-07   Created                                         Julian

************************************************************************/

#ifndef H_FRAME_PARSER_VIDEO_WMV
#define H_FRAME_PARSER_VIDEO_WMV

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "vc1.h"
#include "frame_parser_video_vc1.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

/// Frame parser for VC1 main/simple profile video.
class FrameParser_VideoWmv_c : public FrameParser_VideoVc1_c
{
private:
    unsigned int                RoundingControl;

    FrameParserStatus_t         ReadPictureHeaderSimpleMainProfile(     void );

    unsigned int                RangeReduction;

public:

    FrameParser_VideoWmv_c( void );

    FrameParserStatus_t         ReadHeaders(                            void );

};

#endif

