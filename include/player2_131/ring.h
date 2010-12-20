/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : ring.h
Author :           Nick

Definition of the pure virtual class defining the interface to a simple ring
storage device.


Date        Modification                                    Name
----        ------------                                    --------
11-Dec-03   Created                                         Nick

************************************************************************/

#ifndef H_RING
#define H_RING

#include "osinline.h"

typedef enum 
{
    RingNoError		= 1,
    RingNoMemory,
    RingTooManyEntries,
    RingNothingToGet
} RingStatus_t;

//

#define	RING_NONE_BLOCKING	0

//

class Ring_c
{
public:

    RingStatus_t	InitializationStatus;

    virtual ~Ring_c( void ) {};

    virtual RingStatus_t Insert(  unsigned int	 Value ) = 0;
    virtual RingStatus_t Extract( unsigned int  *Value,
                                  unsigned int   BlockingPeriod = RING_NONE_BLOCKING ) = 0;
    virtual RingStatus_t Flush( void ) = 0;
    virtual bool         NonEmpty( void ) = 0;
};

typedef class Ring_c	*Ring_t;
#endif
