/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2007

Source file name : dvp.h
Author :           Chris

Definition of the types and constants that are used by several components
dealing with DVP captured video


Date        Modification                                    Name
----        ------------                                    --------
16-Oct-07   Created                                         Chris

************************************************************************/

#ifndef __DVP_H
#define __DVP_H

#define DVP_COLOUR_MODE_DEFAULT		0
#define DVP_COLOUR_MODE_601		1
#define DVP_COLOUR_MODE_709		2

typedef struct StreamInfo_s {
    unsigned int width;
    unsigned int height;
    unsigned int interlaced;
    unsigned int pixel_aspect_ratio;	
    unsigned int top_field_first;
    unsigned int h_offset;
    unsigned int v_offset;
    unsigned int* buffer;
    unsigned int* buffer_class;

    // Nicks additions
    unsigned long long		FrameRateNumerator;
    unsigned long long		FrameRateDenominator;

    unsigned int		VideoFullRange;
    unsigned int		ColourMode;

}  __attribute__ ((packed)) StreamInfo_t;

#endif // __DVP_H
