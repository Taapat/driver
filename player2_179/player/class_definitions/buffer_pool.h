/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_pool.h
Author :           Nick

Definition of the pure virtual class defining the interface to a buffer pool
module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
03-Jul-06   Created                                         Nick

************************************************************************/

#ifndef H_BUFFER_POOL
#define H_BUFFER_POOL

// ---------------------------------------------------------------------
//
// Class definition
//

class BufferPool_c : public BaseComponentClass_c
{
public:

    //
    // Global operations on all pool members
    //

    virtual BufferStatus_t	 AttachMetaData(	MetaDataType_t	  Type,
							unsigned int	  Size				= UNSPECIFIED_SIZE,
							void		 *MemoryPool			= NULL,
							void		 *ArrayOfMemoryBlocks[]		= NULL,
							char		 *DeviceMemoryPartitionName	= NULL ) = 0;

    virtual BufferStatus_t	 DetachMetaData(	MetaDataType_t	  Type ) = 0;

    //
    // Get/Release a buffer
    //

    virtual BufferStatus_t	 GetBuffer(		Buffer_t	 *Buffer,
							unsigned int	  OwnerIdentifier	= UNSPECIFIED_OWNER,
							unsigned int	  RequiredSize		= UNSPECIFIED_SIZE,
							bool		  NonBlocking 		= false ) = 0;

    virtual BufferStatus_t	 AbortBlockingGetBuffer(void ) = 0;

    virtual BufferStatus_t	 ReleaseBuffer(		Buffer_t	  Buffer ) = 0;

    //
    // Usage/query/debug methods
    //

    virtual BufferStatus_t	 GetType(		BufferType_t	 *Type ) = 0;

    virtual BufferStatus_t	 GetPoolUsage(		unsigned int	 *BuffersInPool,
							unsigned int     *BuffersWithNonZeroReferenceCount	= NULL,
							unsigned int	 *MemoryInPool				= NULL,
							unsigned int	 *MemoryAllocated			= NULL,
							unsigned int	 *MemoryInUse				= NULL,
							unsigned int	 *LargestFreeMemoryBlock		= NULL ) = 0;

    virtual BufferStatus_t	 CountBuffersReferencedBy( 
							unsigned int	  OwnerIdentifier,
							unsigned int	 *Count ) = 0;

    virtual BufferStatus_t	 GetAllUsedBuffers(	unsigned int	  ArraySize,
							Buffer_t	 *ArrayOfBuffers,
							unsigned int	  OwnerIdentifier ) = 0;

    //
    // Status dump/reporting
    //

    virtual void		 Dump( 		unsigned int	  Flags	= DumpAll ) = 0;
};
#endif
