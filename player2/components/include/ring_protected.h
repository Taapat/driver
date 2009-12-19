/************************************************************************
COPYRIGHT (C) STMicroelectronics 2004

Source file name : ring_protected.h
Author :           Nick

Definition of the class defining the interface to a simple ring
storage device.


Date        Modification                                    Name
----        ------------                                    --------
11-Feb-03   Created                                         Nick

************************************************************************/

#ifndef H_RING_PROTECTED
#define H_RING_PROTECTED

//

#include "ring.h"
#include "osinline.h"

//

class RingProtected_c : public Ring_c
{
private:

    OS_Mutex_t           Lock;
    unsigned int         Limit;
    unsigned int         NextExtract;
    unsigned int         NextInsert;
    unsigned int        *Storage;

public:

    RingProtected_c( unsigned int MaxEntries = 16 );
    ~RingProtected_c( void );

    RingStatus_t Insert( unsigned int    Value );
    RingStatus_t Extract( unsigned int  *Value );
    RingStatus_t Flush( void );
    bool         NonEmpty( void );
};

#endif

