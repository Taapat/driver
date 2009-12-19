/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : output_timer_video.h
Author :           Nick

Basic instance of the output timer class module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
09-Mar-07   Created                                         Nick

************************************************************************/

#ifndef H_OUTPUT_TIMER_VIDEO
#define H_OUTPUT_TIMER_VIDEO

#include "output_timer_base.h"

// ---------------------------------------------------------------------
//
// Local type definitions
//

// ---------------------------------------------------------------------
//
// Class definition
//

class OutputTimer_Video_c : public OutputTimer_Base_c
{
protected:

    // Data

    VideoOutputSurfaceDescriptor_t	 *VideoOutputSurfaceDescriptor;

    bool				  ThreeTwoPulldownDetected;

    bool				  SeenInterlacedContentOnInterlacedDisplay;
    unsigned int			  LoseFramesForSynchronization;

    Rational_t				  LastAdjustedSpeedAfterFrameDrop;

    Rational_t				  FrameRate;
    Rational_t				  PreviousFrameRate;
    Rational_t				  DisplayFrameRate;
    Rational_t				  PreviousDisplayFrameRate;
    bool				  PreviousContentProgressive;

    Rational_t				  CountMultiplier;
    unsigned long long			  FrameDurationTime;
    unsigned long long			  SourceFrameDurationTime;
    Rational_t				  AccumulatedError;

    // Functions

public:

    //
    // Constructor/Destructor methods
    //

    OutputTimer_Video_c(		void );
    ~OutputTimer_Video_c(		void );

    //
    // Base component class overrides
    //

    OutputTimerStatus_t   Halt(		void );
    OutputTimerStatus_t   Reset(	void );

    //
    // Video specific functions
    //

protected:

    OutputTimerStatus_t   InitializeConfiguration(	void );

    OutputTimerStatus_t   FrameDuration(		void			 *ParsedAudioVideoDataParameters,
							unsigned long long	 *Duration );

    OutputTimerStatus_t   FillOutFrameTimingRecord(	unsigned long long	  SystemTime,
							void			 *ParsedAudioVideoDataParameters,
							void			 *AudioVideoDataOutputTiming );

    OutputTimerStatus_t   CorrectSynchronizationError(	void );
};
#endif

