/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer.h
Author :           Nick

Common header file bringing together the buffer headers, note the sub-includes
are order dependent, at least that is the plan.


Date        Modification                                    Name
----        ------------                                    --------
04-Jul-06   Created                                         Nick

************************************************************************/


#ifndef H_BUFFER
#define H_BUFFER

//

#include "player_types.h"
#include "base_component_class.h"

//

#ifndef NOT_SPECIFIED
#define NOT_SPECIFIED 			0xffffffff
#endif

#define UNRESTRICTED_NUMBER_OF_BUFFERS	NOT_SPECIFIED
#define UNSPECIFIED_SIZE		NOT_SPECIFIED
#define UNSPECIFIED_OWNER		NOT_SPECIFIED

//

enum
{
    BufferNoError				= PlayerNoError,
    BufferError					= PlayerError,

						// GetBuffer
    BufferBlockingCallAborted			= BASE_BUFFER,

    BufferInsufficientMemoryGeneral,
    BufferInsufficientMemoryForPool,
    BufferInsufficientMemoryForBuffer,
    BufferInsufficientMemoryForMetaData,

    BufferUnsupportedAllocationSource,		// CreateBufferDataType
    BufferInvalidDescriptor,			// CreateBufferDataType

    BufferTooManyDataTypes,			// CreateBufferDataType
    BufferDataTypeNotFound,			// FindBufferDataType - CreatePool
    BufferMetaDataTypeNotFound,			// AttachMetaData - DetachMetaData - ObtainMetaDataReference

    BufferParametersIncompatibleWithAllocationSource,
    BufferOperationNotSupportedByThisDescriptor,

    BufferSizeIncompatibleWithDescriptor,	// CreatePool - AttachMetaData - GetBuffer - ShrinkBuffer - ExpandBuffer
    BufferPoolNotFound,				// DestroyPool

    BufferFailedToCreateBuffer,			// GetBuffer - PartitionBuffer
    BufferNoFreeBufferAvailable,		// GetBuffer - PartitionBuffer

    BufferNonZeroReferenceCount,		// ReleaseBuffer

    BufferNotInUse,				// AttachMetaData - DetachMetaData - ShrinkBuffer - ExpandBuffer - PartitionBuffer - ObtainDataReference - IncrementReferenceCount - DecrementReferenceCount

    BufferTooManyAttachments,			// AttachBuffer
    BufferAttachmentNotFound			// DetachBuffer
};

typedef PlayerStatus_t	BufferStatus_t;

//

typedef enum
{
    CachedAddress	= 0,
    UnCachedAddress	= 1,
    PhysicalAddress	= 2
} AddressType_t;

//

typedef enum
{
    DumpBufferTypes	= 1,
    DumpMetaDataTypes	= 2,
    DumpListPools	= 4,
    DumpPoolStates	= 8,
    DumpBufferStates	= 16
} DumpTypeMask_t;

#define DumpAll		0xffffffff

//
// Buffer types
//

typedef unsigned int	BufferType_t;
#define MetaDataType_t	BufferType_t

//

typedef  class Buffer_c		*Buffer_t;
typedef  class BufferPool_c	*BufferPool_t;
typedef  class BufferManager_c	*BufferManager_t;

//

#include "buffer_individual.h"
#include "buffer_pool.h"
#include "buffer_manager.h"

#endif
