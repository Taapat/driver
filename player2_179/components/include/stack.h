/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : stack.h
Author :           Nick

Definition of the pure virtual class defining the interface to a simple stack
storage device.


Date        Modification                                    Name
----        ------------                                    --------
08-Jan-07   Created                                         Nick

************************************************************************/

#ifndef H_STACK
#define H_STACK

#include "osinline.h"

typedef enum 
{
    StackNoError		= 0,
    StackNoMemory,
    StackTooManyEntries,
    StackNothingToGet
} StackStatus_t;

//

class Stack_c
{
public:

    StackStatus_t	InitializationStatus;

    virtual ~Stack_c( void ) {};

    virtual StackStatus_t Push(	unsigned int	 Value ) = 0;
    virtual StackStatus_t Pop( 	unsigned int	*Value ) = 0;
    virtual StackStatus_t Flush( void ) = 0;
    virtual bool          NonEmpty( void ) = 0;
};

typedef class Stack_c	*Stack_t;
#endif
