/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2009

Source file name : player_coded_frame.cpp
Author :           Nick

Implementation of the coded frame buffer related functions of the
generic class implementation of player 2


Date        Modification                                    Name
----        ------------                                    --------
23-Feb-09   Created                                         Nick

************************************************************************/

#include <stdarg.h>
#include "player_generic.h"


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Find the coded frame buffer pool associated with a stream playback
//

PlayerStatus_t   Player_Generic_c::GetCodedFrameBufferPool(
						PlayerStream_t	 	  Stream,
						BufferPool_t		 *Pool,
						unsigned int		 *MaximumCodedFrameSize )
{
PlayerStatus_t          Status;
#ifdef __KERNEL__
allocator_status_t      AStatus;
#endif

    //
    // If we haven't already created the buffer pool, do it now.
    //

    if( Stream->CodedFrameBufferPool == NULL )
    {
	//
	// Get the memory and Create the pool with it
	//

#if __KERNEL__
	AStatus = PartitionAllocatorOpen( &Stream->CodedFrameMemoryDevice, Stream->CodedMemoryPartitionName, Stream->CodedMemorySize, true );
	if( AStatus != allocator_ok )
	{
	    report( severity_error, "Player_Generic_c::GetCodedFrameBufferPool - Failed to allocate memory\n" );
	    return PlayerInsufficientMemory;
	}

	Stream->CodedFrameMemory[CachedAddress]         = AllocatorUserAddress( Stream->CodedFrameMemoryDevice );
	Stream->CodedFrameMemory[UnCachedAddress]       = AllocatorUncachedUserAddress( Stream->CodedFrameMemoryDevice );
	Stream->CodedFrameMemory[PhysicalAddress]       = AllocatorPhysicalAddress( Stream->CodedFrameMemoryDevice );
#else
	static unsigned char    Memory[4][4*1024*1024];

	Stream->CodedFrameMemory[CachedAddress]         = Memory[Stream->StreamType];
	Stream->CodedFrameMemory[UnCachedAddress]       = NULL;
	Stream->CodedFrameMemory[PhysicalAddress]       = Memory[Stream->StreamType];
	Stream->CodedMemorySize				= 4*1024*1024;
#endif

//

	Status  = BufferManager->CreatePool( &Stream->CodedFrameBufferPool, Stream->CodedFrameBufferType, Stream->CodedFrameCount, Stream->CodedMemorySize, Stream->CodedFrameMemory );
	if( Status != BufferNoError )
	{
	    report( severity_error, "Player_Generic_c::GetCodedFrameBufferPool - Failed to create the pool.\n" );
	    return PlayerInsufficientMemory;
	}
    }

    //
    // Setup the parameters and return
    //

    if( Pool != NULL )
	*Pool			= Stream->CodedFrameBufferPool;

    if( MaximumCodedFrameSize != NULL )
	*MaximumCodedFrameSize	= Stream->CodedFrameMaximumSize;

    return PlayerNoError;
}




