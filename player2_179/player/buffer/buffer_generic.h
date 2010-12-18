/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : buffer_generic.h
Author :           Nick

Common header file bringing together the buffer headers for the generic 
implementation, note the sub-includes are order dependent, at least that 
is the plan.


Date        Modification                                    Name
----        ------------                                    --------
14-Jul-06   Created                                         Nick

************************************************************************/


#ifndef H_BUFFER_GENERIC
#define H_BUFFER_GENERIC

//

#include "player.h"
#include "allocinline.h"
#include "allocator.h"

//

#define MAX_BUFFER_DATA_TYPES		256		// Number of data types, no restriction, uses minimal memory
#define MAX_BUFFER_OWNER_IDENTIFIERS	4
#define MAX_ATTACHED_BUFFERS		4

#define BUFFER_MAXIMUM_EVENT_WAIT	50		// ms

#define TYPE_INDEX_MASK			(MAX_BUFFER_DATA_TYPES - 1)
#define TYPE_TYPE_MASK			(~TYPE_INDEX_MASK)

//

typedef struct BlockDescriptor_s 	*BlockDescriptor_t;

typedef  class Buffer_Generic_c		*Buffer_Generic_t;
typedef  class BufferPool_Generic_c	*BufferPool_Generic_t;
typedef  class BufferManager_Generic_c	*BufferManager_Generic_t;

//

#include "buffer_individual_generic.h"
#include "buffer_pool_generic.h"
#include "buffer_manager_generic.h"

//

#endif
