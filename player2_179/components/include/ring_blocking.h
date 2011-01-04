/************************************************************************
COPYRIGHT (C) STMicroelectronics 2004

Source file name : ring_blocking.h
Author :           Nick

Definition of the class defining the interface to a simple ring
storage device.


Date        Modification                                    Name
----        ------------                                    --------
23-Sep-04   Created                                         Nick

************************************************************************/

#ifndef H_RING_BLOCKING
#define H_RING_BLOCKING

//

#include "ring.h"
#include "osinline.h"

//

class RingBlocking_c : public Ring_c
{
private:

    OS_Event_t           Signal;
    unsigned int         Limit;
    unsigned int         NextExtract;
    unsigned int         NextInsert;
    unsigned int        *Storage;

public:

    RingBlocking_c( unsigned int MaxEntries = 16 );
    ~RingBlocking_c( void );

    RingStatus_t Insert( unsigned int    Value );
    RingStatus_t Extract( unsigned int  *Value );
    RingStatus_t Flush( void );
    bool         NonEmpty( void );
};

#endif

