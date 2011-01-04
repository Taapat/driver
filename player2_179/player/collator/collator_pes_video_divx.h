/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

Source file name : collator_pes_video_divx.h
Author :           Chris

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Jul-07   Created from existing collator_pes_video.h      Chris

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_DIVX
#define H_COLLATOR_PES_VIDEO_DIVX

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

class Collator_PesVideoDivx_c : public Collator_PesVideo_c
{
private:

    bool                IgnoreCodes;
    unsigned char       Version;

protected:

public:

    Collator_PesVideoDivx_c();

    CollatorStatus_t   Reset(   void );

    CollatorStatus_t   Input(   PlayerInputDescriptor_t  *Input,
				unsigned int                  DataLength,
				void                             *Data );
};

#endif // H_COLLATOR_PES_VIDEO_DIVX

