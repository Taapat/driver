/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_manager.h
Author :           Nick

Definition of the pure virtual class defining the interface to a buffer management
module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
03-Jul-06   Created                                         Nick

************************************************************************/

#ifndef H_BUFFER_MANAGER
#define H_BUFFER_MANAGER

//

typedef enum
{
    NoAllocation		= 0,
    AllocateFromOSMemory,
    AllocateFromDeviceMemory,			// Allocator
    AllocateFromDeviceVideoLMIMemory,		// Allocator
    AllocateFromSuppliedBlock,
    AllocateIndividualSuppliedBlocks,
    AllocateFromNamedDeviceMemory		// Allocator with memory partition name
} BufferAllocationSource_t;

// ---------------------------------------------------------------------
//
// Descriptor record, for pre-defined types, and user defined type
//

typedef enum
{
    BufferDataTypeBase		= 0x0000,
    MetaDataTypeBase		= 0x8000,
} BufferPredefinedType_t;

//

typedef struct BufferDataDescriptor_s
{
    const char			*TypeName;
    BufferType_t		 Type;

    BufferAllocationSource_t	 AllocationSource;
    unsigned int		 RequiredAllignment;
    unsigned int		 AllocationUnitSize;

    bool			 HasFixedSize;
    bool			 AllocateOnPoolCreation;
    unsigned int		 FixedSize;
} BufferDataDescriptor_t;

//

#define InitializeBufferDataDescriptor(d)	d = { NULL, BufferDataTypeBase, NoAllocation,         32, 4096, false, false, NOT_SPECIFIED }
#define InitializeMetaDataDescriptor(d)		d = { NULL, MetaDataTypeBase,   AllocateFromOSMemory, 16,    0, true,  false, NOT_SPECIFIED }

// ---------------------------------------------------------------------
//
// Class definition
//

class BufferManager_c : public BaseComponentClass_c
{
public:

    //
    // Add to the defined types
    //

    virtual BufferStatus_t	CreateBufferDataType(
						BufferDataDescriptor_t	 *Descriptor,
						BufferType_t		 *Type ) = 0;

    virtual BufferStatus_t	FindBufferDataType(
						const char		 *TypeName,
						BufferType_t		 *Type ) = 0;

    virtual BufferStatus_t	GetDescriptor(	BufferType_t		  Type,
						BufferPredefinedType_t	  RequiredKind,
						BufferDataDescriptor_t	**Descriptor ) = 0;

    //
    // Create/destroy a pool of buffers rather than overload the create function, 
    // I have included multiple possible parameters, but with default values.
    //
    // For example no call should specify both a memory pool and an array of memory blocks.
    //

    virtual BufferStatus_t	 CreatePool(	BufferPool_t	 *Pool,
						BufferType_t	  Type,
					    	unsigned int	  NumberOfBuffers		= UNRESTRICTED_NUMBER_OF_BUFFERS,
						unsigned int	  Size				= UNSPECIFIED_SIZE,
					    	void		 *MemoryPool[3]			= NULL,
					    	void		 *ArrayOfMemoryBlocks[][3]	= NULL,
						char		 *DeviceMemoryPartitionName	= NULL ) = 0;

    virtual BufferStatus_t	 DestroyPool(	BufferPool_t	  Pool ) = 0;

    //
    // Status dump/reporting
    //

    virtual void		 Dump( 		unsigned int	  Flags	= DumpAll ) = 0;
};

#endif

