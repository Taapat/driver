/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_frame.h
Author :           Julian

Definition of the wmv collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
11-Sep-07   Created from existing collator_pes_audio_mpeg.h Julian

************************************************************************/

#ifndef H_COLLATOR_PES_FRAME
#define H_COLLATOR_PES_FRAME

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_pes.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Collator_PesFrame_c : public Collator_Pes_c
{
private:

    int         RemainingDataLength;
    int         FrameSize;

protected:

public:
    Collator_PesFrame_c();

    CollatorStatus_t    Input          (PlayerInputDescriptor_t  *Input,
                                        unsigned int              DataLength,
                                        void                     *Data );

    CollatorStatus_t    FrameFlush     (void );
    CollatorStatus_t    FrameFlush     (bool                   FlushedByStreamTerminate);


    CollatorStatus_t    Reset          (void);
};

#endif /* H_COLLATOR_PES_FRAME */

