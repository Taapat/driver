/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : ring_unprotected.h
Author :           Nick

Definition of the class defining the interface to a simple ring
storage device.


Date        Modification                                    Name
----        ------------                                    --------
11-Dec-03   Created                                         Nick

************************************************************************/

#ifndef H_RING_UNPROTECTED
#define H_RING_UNPROTECTED

//

#include "osinline.h"
#include "ring.h"

//

class RingUnprotected_c : public Ring_c
{
private:

    unsigned int         Limit;
    unsigned int         NextExtract;
    unsigned int         NextInsert;
    unsigned int        *Storage;

public:

    RingUnprotected_c( unsigned int MaxEntries = 16 );
    ~RingUnprotected_c( void );

    RingStatus_t Insert( unsigned int    Value );
    RingStatus_t Extract( unsigned int  *Value );
    RingStatus_t Flush( void );
    bool         NonEmpty( void );
};

#endif

