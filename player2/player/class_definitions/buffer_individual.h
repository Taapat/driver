/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_individual.h
Author :           Nick

Definition of the pure virtual class defining the interface to a buffer
module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
04-Jul-06   Created                                         Nick

************************************************************************/

#ifndef H_BUFFER_INDIVIDUAL
#define H_BUFFER_INDIVIDUAL

#include "osinline.h"

// ---------------------------------------------------------------------
//
// Class definition
//

class Buffer_c : public BaseComponentClass_c
{
public:

    //
    // Meta data activities
    //

    virtual BufferStatus_t	 AttachMetaData(	MetaDataType_t	  Type,
							unsigned int	  Size				= UNSPECIFIED_SIZE,
							void		 *MemoryBlock			= NULL,
							char		 *DeviceMemoryPartitionName	= NULL ) = 0;

    virtual BufferStatus_t	 DetachMetaData(	MetaDataType_t	  Type ) = 0;

//

    virtual BufferStatus_t	 ObtainMetaDataReference(
							MetaDataType_t	  Type,
							void		**Pointer ) = 0;

    //
    // Buffer manipulators
    //

    virtual BufferStatus_t	 SetUsedDataSize(	unsigned int	  DataSize ) = 0;

    virtual BufferStatus_t	 ShrinkBuffer(		unsigned int	  NewSize ) = 0;

    virtual BufferStatus_t	 ExpandBuffer(		unsigned int	  NewSize ) = 0;

    virtual BufferStatus_t	 PartitionBuffer(	unsigned int	  LeaveInFirstPartitionSize,
							bool		  DuplicateMetaData,
							Buffer_t	 *SecondPartition,
							unsigned int	  SecondOwnerIdentifier	= UNSPECIFIED_OWNER,
							bool		  NonBlocking 		= false ) = 0;

    //
    // Reference manipulation, and ownership control
    //

    virtual BufferStatus_t	 RegisterDataReference(	unsigned int	  BlockSize,
							void		 *Pointer,
							AddressType_t	  AddressType = CachedAddress ) = 0;

    virtual BufferStatus_t	 RegisterDataReference(	unsigned int	  BlockSize,
							void		 *Pointers[3] ) = 0;

    virtual BufferStatus_t	 ObtainDataReference(	unsigned int	 *BlockSize,
							unsigned int	 *UsedDataSize,
							void		**Pointer,
							AddressType_t	  AddressType = CachedAddress ) = 0;

    virtual BufferStatus_t	 TransferOwnership(	unsigned int	  OwnerIdentifier0,
							unsigned int	  OwnerIdentifier1	= UNSPECIFIED_OWNER ) = 0;

    virtual BufferStatus_t	 IncrementReferenceCount(
							unsigned int	  OwnerIdentifier	= UNSPECIFIED_OWNER ) = 0;

    virtual BufferStatus_t	 DecrementReferenceCount(
							unsigned int	  OwnerIdentifier 	= UNSPECIFIED_OWNER ) = 0;

    //
    // linking of other buffers to this buffer - for increment/decrement management
    //

    virtual BufferStatus_t	 AttachBuffer(		Buffer_t	  Buffer ) = 0;
    virtual BufferStatus_t	 DetachBuffer(		Buffer_t	  Buffer ) = 0;
    virtual BufferStatus_t	 ObtainAttachedBufferReference(
							BufferType_t	  Type,
							Buffer_t	 *Buffer ) = 0;

    //
    // Usage/query/debug methods
    //

    virtual BufferStatus_t	 GetType(		BufferType_t	 *Type ) = 0;

    virtual BufferStatus_t	 GetIndex(		unsigned int	 *Index ) = 0;

    virtual BufferStatus_t	 GetMetaDataCount( 	unsigned int	 *Count ) = 0;

    virtual BufferStatus_t	 GetMetaDataList(	unsigned int	  ArraySize,
							unsigned int	 *ArrayOfMetaDataTypes ) = 0;

    virtual BufferStatus_t	 GetOwnerCount( 	unsigned int	 *Count ) = 0;

    virtual BufferStatus_t	 GetOwnerList(		unsigned int	  ArraySize,
							unsigned int	 *ArrayOfOwnerIdentifiers ) = 0;

    //
    // Status dump/reporting
    //

    virtual void		 Dump( 			unsigned int	  Flags	= DumpAll ) = 0;

    virtual void		DumpToRelayFS ( unsigned int id, unsigned int type, void *param ) = 0;

};
#endif
