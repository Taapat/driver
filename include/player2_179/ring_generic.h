/************************************************************************
COPYRIGHT (C) STMicroelectronics 2004

Source file name : ring_blocking.h
Author :           Nick

Definition of the class defining the interface to a simple ring
storage device, that blocks on extraction, and has protected insertion.
It has the inbuild assumption that there can be only one extractor of data.


Date        Modification                                    Name
----        ------------                                    --------
23-Sep-04   Created                                         Nick
15-Nov-06   Revised                                         Nick

************************************************************************/

#ifndef H_RING_GENERIC
#define H_RING_GENERIC

//

#include "ring.h"
#include "osinline.h"

//

class RingGeneric_c : public Ring_c
{
private:

    OS_Mutex_t		 Lock;
    OS_Event_t		 Signal;
    unsigned int 	 Limit;
    unsigned int 	 NextExtract;
    unsigned int 	 NextInsert;
    unsigned int	*Storage;

public:

    RingGeneric_c( unsigned int MaxEntries = 16 );
    ~RingGeneric_c( void );

    RingStatus_t Insert(  unsigned int	 Value );
    RingStatus_t Extract( unsigned int	*Value,
			  unsigned int   BlockingPeriod = OS_INFINITE );
    RingStatus_t Flush( void );
    bool         NonEmpty( void );
};

#endif


