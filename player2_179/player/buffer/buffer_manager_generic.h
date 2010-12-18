/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_manager_generic.h
Author :           Nick

Implementation of the class definition of the buffer manager class for use in player 2


Date        Modification                                    Name
----        ------------                                    --------
14-Jul-06   Created                                         Nick

************************************************************************/

#ifndef H_BUFFER_MANAGER_GENERIC
#define H_BUFFER_MANAGER_GENERIC

//

class BufferManager_Generic_c : public BufferManager_c
{
friend class BufferPool_Generic_c;
friend class Buffer_Generic_c;

private:

    // Data

    OS_Mutex_t			Lock;

    unsigned int		TypeDescriptorCount;
    BufferDataDescriptor_t	TypeDescriptors[MAX_BUFFER_DATA_TYPES];

    BufferPool_Generic_t	ListOfBufferPools;

    // Functions

public:

    BufferManager_Generic_c( void );
    ~BufferManager_Generic_c( void );

    //
    // Add to the defined types
    //

    BufferStatus_t	CreateBufferDataType(	BufferDataDescriptor_t	 *Descriptor,
						BufferType_t		 *Type );

    BufferStatus_t	FindBufferDataType(	const char		 *TypeName,
						BufferType_t		 *Type );

    BufferStatus_t	GetDescriptor( 		BufferType_t		  Type,
						BufferPredefinedType_t	  RequiredKind,
						BufferDataDescriptor_t	**Descriptor );

    //
    // Create/destroy a pool of buffers overloaded creation function
    //

    BufferStatus_t	 CreatePool(		BufferPool_t	 *Pool,
						BufferType_t	  Type,
					    	unsigned int	  NumberOfBuffers		= UNRESTRICTED_NUMBER_OF_BUFFERS,
						unsigned int	  Size				= UNSPECIFIED_SIZE,
					    	void		 *MemoryPool[3]			= NULL,
					    	void		 *ArrayOfMemoryBlocks[][3]	= NULL,
						char		 *DeviceMemoryPartitionName	= NULL );

    BufferStatus_t	 DestroyPool(		BufferPool_t		  Pool );

    //
    // Status dump/reporting
    //

    void		 Dump( 			unsigned int	  Flags	= DumpAll );

};

#endif
