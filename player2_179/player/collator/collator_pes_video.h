/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes_video.h
Author :           Nick

Definition of the base collator pes class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
19-Apr-07   Created from existing collator_pes_video.h      Daniel

************************************************************************/

#ifndef H_COLLATOR_PES_VIDEO
#define H_COLLATOR_PES_VIDEO

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

class Collator_PesVideo_c : public Collator_Pes_c
{
public:

    //
    // Collator class functions
    //

    virtual CollatorStatus_t   Input(   PlayerInputDescriptor_t  *Input,
					unsigned int              DataLength,
					void                     *Data );

    CollatorStatus_t   FrameFlush(      void );

    //
    // A frame flush overload that allows the specific setting of a flushed by terminate flag.
    // This allows such things as the frame parser to know that it can give display indices to 
    // all frames including this one, because any frames that follow will be part of what is 
    // strictly a new stream. This ensures that last framnes will be seen.
    //

    CollatorStatus_t   FrameFlush(      bool                    FlushedByStreamTerminate );

    //
    // Some strange additions by someone
    //

protected:

     bool TerminationFlagIsSet;

public:
	Collator_PesVideo_c() { TerminationFlagIsSet = false; }

};

#endif

