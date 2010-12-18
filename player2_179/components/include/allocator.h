/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : allocator.h
Author :           Nick

Definition of the pure virtual class defining the interface to a generic
allocator of regions of memory.


Date        Modification                                    Name
----        ------------                                    --------
29-Jun-05   Created                                         Nick

************************************************************************/

#ifndef H_ALLOCATOR
#define H_ALLOCATOR

//

typedef enum
{
    AllocatorNoError            = 0,
    AllocatorError,
    AllocatorNoMemory,
    AllocatorUnableToAllocate
} AllocatorStatus_t;

//

class Allocator_c
{
public:

    AllocatorStatus_t   InitializationStatus;

    virtual ~Allocator_c( void ) {};

    virtual AllocatorStatus_t Allocate( unsigned int      Size,
					unsigned char   **Block,
					bool              NonBlocking   = false ) = 0;

    virtual AllocatorStatus_t Free(     void ) = 0;

    virtual AllocatorStatus_t Free(     unsigned int      Size,
					unsigned char    *Block ) = 0;

    virtual AllocatorStatus_t LargestFreeBlock( 	unsigned int	 *Size ) = 0;
};
#endif

