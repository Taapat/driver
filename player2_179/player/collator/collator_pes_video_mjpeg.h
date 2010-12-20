/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

Source file name : collator_pes_video_mjpeg.h
Author :           Julian

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
12-Jun-07   Created from existing collator_pes_video.h      Julian

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO_MJPEG
#define H_COLLATOR_PES_VIDEO_MJPEG

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

class Collator_PesVideoMjpeg_c : public Collator_PesVideo_c
{
protected:

    // Overrides for base class
    CollatorStatus_t   FindNextStartCode(       unsigned int*           CodeOffset);

public:

    Collator_PesVideoMjpeg_c();

    CollatorStatus_t   Reset(                   void );

    CollatorStatus_t    Input          (PlayerInputDescriptor_t  *Input,
                                        unsigned int              DataLength,
                                        void                     *Data );

};

#endif // H_COLLATOR_PES_VIDEO_MJPEG

