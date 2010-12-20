/************************************************************************
COPYRIGHT (C) STMicroelectronics 2008

Source file name : frame_parser_video_divx_hd.h
Author :           Chris

Definition of the DivX HD frame parser video class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
18-Jun-07   Created                                         Chris

************************************************************************/

#ifndef H_FRAME_PARSER_VIDEO_DIVX_HD
#define H_FRAME_PARSER_VIDEO_DIVX_HD

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "frame_parser_video_divx.h"

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_VideoDivxHd_c : public FrameParser_VideoDivx_c
{

protected:

    FrameParserStatus_t  ReadVopHeader(       Mpeg4VopHeader_t         *Vop );
    FrameParserStatus_t  ReadHeaders(         void );

};

#endif // H_FRAME_PARSER_VIDEO_DIVX_HD
