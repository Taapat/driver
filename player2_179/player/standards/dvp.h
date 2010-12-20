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

typedef struct DvpRectangle_s
{
    unsigned int	 X;
    unsigned int	 Y;
    unsigned int	 Width;
    unsigned int	 Height;
} DvpRectangle_t;

//

struct Ratio_s
{
    unsigned int                Numerator;
    unsigned int                Denominator;
};

//

typedef struct StreamInfo_s {
    unsigned int width;
    unsigned int height;
    unsigned int interlaced;	
    unsigned int top_field_first;
    unsigned int h_offset;
    unsigned int v_offset;
    unsigned int* buffer;
    unsigned int* buffer_class;

    struct Ratio_s pixel_aspect_ratio;
    
    // Nicks additions
    unsigned long long		FrameRateNumerator;
    unsigned long long		FrameRateDenominator;

    unsigned int		VideoFullRange;
    unsigned int		ColourMode;

    DvpRectangle_t		InputWindow;
    DvpRectangle_t		OutputWindow;
}  __attribute__ ((packed)) StreamInfo_t;

#endif // __DVP_H
