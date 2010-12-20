/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : stack_generic.h
Author :           Nick

Implementation of the class defining the interface to a simple stack
storage device.


Date        Modification                                    Name
----        ------------                                    --------
08-Jan-07   Created                                         Nick

************************************************************************/

#ifndef H_STACK_GENERIC
#define H_STACK_GENERIC

#include "stack.h"

//

class StackGeneric_c : public Stack_c
{
private:

    OS_Mutex_t		 Lock;
    unsigned int 	 Limit;
    unsigned int 	 Level;
    unsigned int	*Storage;

public:

    StackGeneric_c( unsigned int MaxEntries = 16 );
    ~StackGeneric_c( void );

    StackStatus_t Push(	unsigned int	 Value );
    StackStatus_t Pop( 	unsigned int	*Value );
    StackStatus_t Flush( void );
    bool          NonEmpty( void );
};
#endif
