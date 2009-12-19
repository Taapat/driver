/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_pool_generic.h
Author :           Nick

Implementation of the class definition of the buffer pool class for use in player 2


Date        Modification                                    Name
----        ------------                                    --------
14-Jul-06   Created                                         Nick

************************************************************************/

#ifndef H_BUFFER_POOL_GENERIC
#define H_BUFFER_POOL_GENERIC

//

struct BlockDescriptor_s
{
    BlockDescriptor_t		  Next;
    BufferDataDescriptor_t	 *Descriptor;
    bool			  AttachedToPool;

    unsigned int		  Size;
    void			 *Address[3];		// 3 address types (cached uncached physical) - only cached for meta data

    allocator_device_t		  MemoryAllocatorDevice;
    unsigned int		  PoolAllocatedOffset;
};

//

class BufferPool_Generic_c : public BufferPool_c
{
friend class BufferManager_Generic_c;
friend class Buffer_Generic_c;

private:

    // Friend Data

    BufferManager_Generic_t	  Manager;
    BufferPool_Generic_t	  Next;

    // Data

    OS_Mutex_t			  Lock;

    BufferDataDescriptor_t	 *BufferDescriptor;
    unsigned int		  NumberOfBuffers;
    unsigned int		  CountOfBuffers;		// In effect when NumberOfBuffers == NOT_SPECIFIED
    unsigned int		  Size;

    Buffer_Generic_t		  ListOfBuffers;
    Ring_c			 *FreeBuffer;
    void			 *MemoryPool[3];		// If memory pool is specified, its three addresses
    Allocator_c			 *MemoryPoolAllocator;
    allocator_device_t		  MemoryPoolAllocatorDevice;
    char			  MemoryPartitionName[ALLOCATOR_MAX_PARTITION_NAME_SIZE];

    BlockDescriptor_t		  BufferBlock;
    BlockDescriptor_t		  ListOfMetaData;

    bool			  AbortGetBuffer;		// Flag to abort a get buffer call
    bool			  BufferReleaseSignalWaitedOn;
    OS_Event_t			  BufferReleaseSignal;

    // Functions

    void		TidyUp( 		void );

    BufferStatus_t	CheckMemoryParameters(	BufferDataDescriptor_t	 *Descriptor,
						bool			  ArrayAllocate,
						unsigned int		  Size,
						void			 *MemoryPool,
						void			 *ArrayOfMemoryBlocks,
						char			 *MemoryPartitionName,
						const char		 *Caller,
						unsigned int		 *ItemSize	= NULL );

    BufferStatus_t	AllocateMemoryBlock(	BlockDescriptor_t	  Block,
						bool			  ArrayAllocate,
						unsigned int		  Index,
						Allocator_c		 *PoolAllocator,
						void			 *MemoryPool,
						void			 *ArrayOfMemoryBlocks,
						char			 *MemoryPartitionName,
						const char		 *Caller );

    BufferStatus_t	DeAllocateMemoryBlock(	BlockDescriptor_t	  Block );

    BufferStatus_t	ShrinkMemoryBlock( 	BlockDescriptor_t	  Block,
						unsigned int		  NewSize );

    BufferStatus_t	ExpandMemoryBlock( 	BlockDescriptor_t	  Block,
						unsigned int		  NewSize );


public:

    BufferPool_Generic_c(			BufferManager_Generic_t	  Manager,
						BufferDataDescriptor_t	 *Descriptor, 
					    	unsigned int		  NumberOfBuffers,
						unsigned int		  Size,
					    	void			 *MemoryPool[3],
					    	void			 *ArrayOfMemoryBlocks[][3],
						char			 *DeviceMemoryPartitionName );

    ~BufferPool_Generic_c( void );

    //
    // Global operations on all pool members
    //

    BufferStatus_t	 AttachMetaData(	MetaDataType_t	  Type,
						unsigned int	  Size				= UNSPECIFIED_SIZE,
						void		 *MemoryPool			= NULL,
						void		 *ArrayOfMemoryBlocks[]		= NULL,
						char		 *DeviceMemoryPartitionName	= NULL );

    BufferStatus_t	 DetachMetaData(	MetaDataType_t	  Type );

    //
    // Get/Release a buffer - overloaded get
    //

    BufferStatus_t	 GetBuffer(		Buffer_t	 *Buffer,
						unsigned int	  OwnerIdentifier	= UNSPECIFIED_OWNER,
						unsigned int	  RequiredSize		= UNSPECIFIED_SIZE,
						bool		  NonBlocking 		= false );

    BufferStatus_t	 AbortBlockingGetBuffer(void );

    BufferStatus_t	 ReleaseBuffer(		Buffer_t	  Buffer );

    //
    // Usage/query/debug methods
    //

    BufferStatus_t	 GetType(		BufferType_t	 *Type );

    BufferStatus_t	 GetPoolUsage(		unsigned int	 *BuffersInPool,
						unsigned int     *BuffersWithNonZeroReferenceCount,
						unsigned int	 *MemoryInPool,
						unsigned int	 *MemoryAllocated,
						unsigned int	 *MemoryInUse );

    BufferStatus_t	 CountBuffersReferencedBy( 
						unsigned int	  OwnerIdentifier,
						unsigned int	 *Count );

    BufferStatus_t	 GetAllUsedBuffers(	unsigned int	  ArraySize,
						Buffer_t	 *ArrayOfBuffers,
						unsigned int	  OwnerIdentifier );

    //
    // Status dump/reporting
    //

    void		 Dump( 			unsigned int	  Flags	= DumpAll );
};

#endif
