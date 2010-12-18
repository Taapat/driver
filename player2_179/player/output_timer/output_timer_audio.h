/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : output_timer_audio.h
Author :           Nick

Basic instance of the output timer class module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
26-Apr-07   Created (from output_timer_video.h)             Daniel

************************************************************************/

#ifndef H_OUTPUT_TIMER_AUDIO
#define H_OUTPUT_TIMER_AUDIO

#include "output_timer_base.h"

// ---------------------------------------------------------------------
//
// Local type definitions
//

typedef struct OutputTimerConfiguration_Audio_s
{
    unsigned int		  MinimumManifestorLatencyInSamples;
    unsigned int                  MinimumManifestorSamplingFrequency;
} OutputTimerConfiguration_Audio_t;

// ---------------------------------------------------------------------
//
// Class definition
//

/// Audio specific output timer.
class OutputTimer_Audio_c : public OutputTimer_Base_c
{
protected:

    // Data

    OutputTimerConfiguration_Audio_t	  AudioConfiguration;
    
    AudioOutputSurfaceDescriptor_t	 *AudioOutputSurfaceDescriptor;

    unsigned int			  SamplesInLastFrame;
    unsigned int			  LastSampleRate;

    Rational_t				  LastAdjustedSpeedAfterFrameDrop;

    Rational_t				  CountMultiplier;
    Rational_t				  SampleDurationTime;
    Rational_t				  AccumulatedError;

    unsigned int			  ExtendSamplesForSynchronization;
    unsigned int			  SynchronizationCorrectionUnits;
    unsigned int			  SynchronizationOneTenthCorrectionUnits;

public:

    //
    // Constructor/Destructor methods
    //

    OutputTimer_Audio_c(		void );
    ~OutputTimer_Audio_c(		void );

    //
    // Base component class overrides
    //

    OutputTimerStatus_t   Halt(		void );
    OutputTimerStatus_t   Reset(	void );

    //
    // Audio specific functions
    //

protected:

    OutputTimerStatus_t   InitializeConfiguration(	void );

    OutputTimerStatus_t   FrameDuration(		void			 *ParsedAudioVideoDataParameters,
							unsigned long long	 *Duration );

    OutputTimerStatus_t   FillOutFrameTimingRecord(	unsigned long long	  SystemTime,
							void			 *ParsedAudioVideoDataParameters,
							void			 *AudioVideoDataOutputTiming );

    OutputTimerStatus_t   CorrectSynchronizationError(	void );

    //
    // Audio specific functions
    //
    
    unsigned int	  LookupMinimumManifestorLatency(unsigned int SamplingFreqency);
};
#endif

