/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

Source file name : collator_pes_video_h264.h
Author :           Julian

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
26-Jul-07   Created from existing mpeg2 version		    Nick

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_H264
#define H_COLLATOR_PES_VIDEO_H264

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_pes_video.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesVideoH264_c : public Collator_PesVideo_c
{
protected:

public:

    Collator_PesVideoH264_c();

    CollatorStatus_t   Reset(                   void );
};

#endif // H_COLLATOR_PES_VIDEO_H264

