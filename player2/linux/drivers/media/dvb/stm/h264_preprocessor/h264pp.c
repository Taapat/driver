/*
** h264pp.c - implementation of a simple H264 pre-processor driver
**
** Copyright 2005 STMicroelectronics, all right reserved.
**
**          MAJOR  MINOR  DESCRIPTION
**          -----  -----  ----------------------------------------------
**          246     0     Drive the H264 pre-processor hardware
*/

#define MAJOR_NUMBER    246
#define MINOR_NUMBER    0

#include "osdev_device.h"

/* --- */

#include "h264ppio.h"

int            h264_pp_workaround_gnbvd42331;
int            h264_pp_per_instance;
int            h264_pp_number_of_pre_processors;
unsigned long  h264_pp_register_base[2];
int            h264_pp_interrupt[2];

// ////////////////////////////////////////////////////////////////////////////////
//
// defines

// ////////////////////////////////////////////////////////////////////////////////
//
// Static Data

static OSDEV_Semaphore_t        Lock;
static unsigned int             OpenMask;
static unsigned int             RegisterBase[2];

#define H264_PP_MAPPED_REGISTER_BASE    RegisterBase
#include "h264pp.h"

// ////////////////////////////////////////////////////////////////////////////////
//
//

static OSDEV_OpenEntrypoint(  H264ppOpen );
static OSDEV_CloseEntrypoint( H264ppClose );
static OSDEV_IoctlEntrypoint( H264ppIoctl );
static OSDEV_MmapEntrypoint(  H264ppMmap );

static OSDEV_Descriptor_t H264ppDeviceDescriptor =
{
	Name:           "H264 Pre-processor Module",
	MajorNumber:    MAJOR_NUMBER,

	OpenFn:         H264ppOpen,
	CloseFn:        H264ppClose,
	IoctlFn:        H264ppIoctl,
	MmapFn:         H264ppMmap
};

/* --- External entrypoints --- */

OSDEV_PlatformLoadEntrypoint( H264ppLoadModule );
OSDEV_PlatformUnloadEntrypoint( H264ppUnloadModule );

OSDEV_RegisterPlatformDriverFn( "h264pp", H264ppLoadModule, H264ppUnloadModule);

/* --- The context for an opening of the device --- */

//

typedef struct H264ppIndividualContext_s
{
    unsigned int		Busy;
    h264pp_ioctl_queue_t	Parameters;			// Last buffer queued parameters
    unsigned int                BufferProcessedInsert;		// Where to store the details when finished 
    unsigned int		DecodeTime;
    unsigned int		Accumulated_ITS;

    unsigned int		ForceWorkAroundGNBvd42331;	// Workaround flags
    unsigned int		last_mb_adaptive_frame_field_flag;
} H264ppIndividualContext_t;

//

typedef struct H264ppProcessedBuffer_s
{
    boolean			Filled;
    unsigned int		PP;
    unsigned int		PP_ITS;
    unsigned int		QueueIdentifier;
    unsigned int		OutputSize;
    unsigned int		Field;
    unsigned int		DecodeTime;
} H264ppProcessedBuffer_t;

//

typedef struct H264ppContext_s
{
    unsigned int                 Index;
    boolean                      Closing;

    OSDEV_Semaphore_t            PreProcessor;                          // Semaphore for claiming the preprocessor hardware (associated with this context)
    OSDEV_Semaphore_t            ProcessedBufferListEntry;              // Semaphore for claiming a filled processed buffer list entry.
    OSDEV_Semaphore_t            ProcessedBuffer;                       // Semaphore for claiming a filled processed buffer.

    unsigned int                 BufferProcessedInsert;
    unsigned int                 BufferProcessedExtract;
    H264ppProcessedBuffer_t	 BufferProcessed[H264_PP_MAX_SUPPORTED_BUFFERS];

    H264ppIndividualContext_t    SubContext[H264_PP_MAX_SUPPORTED_PP];      // Context for each running pre-processor per open
} H264ppContext_t;

//

static void H264ppWorkAroundGNBvd42331( H264ppIndividualContext_t	 *SubContext,
					unsigned int			  N );

// ////////////////////////////////////////////////////////////////////////////////
//
//    The Initialize module function

OSDEV_PlatformLoadEntrypoint( H264ppLoadModule )
{
OSDEV_Status_t  Status;
int             n;
//

    OSDEV_LoadEntry();

    h264_pp_workaround_gnbvd42331	= 1;
    h264_pp_per_instance		= 1;

    h264_pp_number_of_pre_processors	= min(PlatformData->NumberBaseAddresses,PlatformData->NumberInterrupts);

    for (n=0;n<h264_pp_number_of_pre_processors;n++)
    {
	    h264_pp_register_base[n]		= PlatformData->BaseAddress[n];
	    h264_pp_interrupt[n]		= PlatformData->Interrupt[n];
    }

    //
    // Initailize the global static data
    //

    OpenMask            = 0;
    
    for (n=0;n<h264_pp_number_of_pre_processors;n++)
	    RegisterBase[n]        = (unsigned int)OSDEV_IOReMap( H264_PP_REGISTER_BASE(n) , H264_PP_REGISTER_SIZE );

    OSDEV_InitializeSemaphore( &Lock, 1 );

    //
    // Register our device
    //

    Status = OSDEV_RegisterDevice( &H264ppDeviceDescriptor );
    if( Status != OSDEV_NoError )
    {
	OSDEV_Print( "H264ppLoadModule : Unable to get major %d\n", MAJOR_NUMBER );
	OSDEV_LoadExit( OSDEV_Error );
    }

    OSDEV_LinkDevice(  H264_PP_DEVICE, MAJOR_NUMBER, MINOR_NUMBER );

//

    OSDEV_Print( "H264ppLoadModule : H264pp device loaded\n" );
    OSDEV_LoadExit( OSDEV_NoError );
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The Terminate module function

OSDEV_PlatformUnloadEntrypoint( H264ppUnloadModule )
{
OSDEV_Status_t          Status;

/* --- */

    OSDEV_UnloadEntry();


/* --- */

    Status = OSDEV_DeRegisterDevice( &H264ppDeviceDescriptor );
    if( Status != OSDEV_NoError )
	OSDEV_Print( "H264ppUnloadModule : Unregister of device failed\n");

/* --- */

    OSDEV_DeInitializeSemaphore( &Lock );

/* --- */

    OSDEV_Print( "H264pp device unloaded\n" );
    OSDEV_UnloadExit(OSDEV_NoError);
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The Interrupt handler

OSDEV_InterruptHandlerEntrypoint( H264ppInterruptHandler )
{
H264ppContext_t                 *H264ppContext  = (H264ppContext_t *)((unsigned int)Parameter & ~3);
unsigned int                     ParticularPP   = ((unsigned int)Parameter & 3);
H264ppIndividualContext_t       *SubContext;
H264ppProcessedBuffer_t		*Record;
unsigned int                     N;
unsigned int                     PP_ITS;
unsigned int                     PP_WDL;
unsigned int                     PP_ISBG;
unsigned int                     PP_IPBG;

    //
    // Read and clear the interrupt status
    //

    N                   = H264ppContext->Index + ParticularPP;
    SubContext          = &H264ppContext->SubContext[ParticularPP];

    SubContext->DecodeTime	= OSDEV_GetTimeInMilliSeconds() - SubContext->DecodeTime;

    PP_ITS              = OSDEV_ReadLong( PP_ITS(N) );
    PP_ISBG             = OSDEV_ReadLong(PP_ISBG(N));
    PP_IPBG             = OSDEV_ReadLong(PP_IPBG(N));
    PP_WDL              = OSDEV_ReadLong(PP_WDL(N));

    OSDEV_WriteLong( PP_ITS(N), PP_ITS );               // Clear interrupt status

#if 0
if( PP_ITS != PP_ITM__DMA_CMP )
{
printk( "$$$$$$ H264ppInterruptHandler (PP %d)  - Took %dms $$$$$$\n", ParticularPP, SubContext->DecodeTime );

printk( "       PP_READ_START           = %08x\n", OSDEV_ReadLong(PP_READ_START(N)) );
printk( "       PP_READ_STOP            = %08x\n", OSDEV_ReadLong(PP_READ_STOP(N)) );
printk( "       PP_BBG                  = %08x\n", OSDEV_ReadLong(PP_BBG(N)) );
printk( "       PP_BBS                  = %08x\n", OSDEV_ReadLong(PP_BBS(N)) );
printk( "       PP_ISBG                 = %08x\n", PP_ISBG );
printk( "       PP_IPBG                 = %08x\n", OSDEV_ReadLong(PP_IPBG(N)) );
printk( "       PP_IBS                  = %08x\n", OSDEV_ReadLong(PP_IBS(N)) );
printk( "       PP_WDL                  = %08x\n", PP_WDL );
printk( "       PP_CFG                  = %08x\n", OSDEV_ReadLong(PP_CFG(N)) );
printk( "       PP_PICWIDTH             = %08x\n", OSDEV_ReadLong(PP_PICWIDTH(N)) );
printk( "       PP_CODELENGTH           = %08x\n", OSDEV_ReadLong(PP_CODELENGTH(N)) );
printk( "       PP_MAX_OPC_SIZE         = %08x\n", OSDEV_ReadLong(PP_MAX_OPC_SIZE(N)) );
printk( "       PP_MAX_CHUNK_SIZE       = %08x\n", OSDEV_ReadLong(PP_MAX_CHUNK_SIZE(N)) );
printk( "       PP_MAX_MESSAGE_SIZE     = %08x\n", OSDEV_ReadLong(PP_MAX_MESSAGE_SIZE(N)) );
printk( "       PP_ITM                  = %08x\n", OSDEV_ReadLong(PP_ITM(N)) );
printk( "       PP_ITS                  = %08x\n", PP_ITS );
}
#endif

    //
    // If appropriate, signal the code.
    //

    SubContext->Accumulated_ITS	|= PP_ITS;
    if( ((PP_ITS & PP_ITM__DMA_CMP) != 0) && SubContext->Busy )
    {
#if 0
static unsigned int	 Frame = 0;
unsigned int		 i;
unsigned char	 	*Output	= (unsigned char *)SubContext->Parameters.BufferCachedAddress;
unsigned int		 Total;

    Total	= 0;
    for( i=0; i<(PP_WDL - PP_IPBG); i++ )
	Total	+= Output[i+H264_PP_SESB_SIZE];

    printk( "Frame %4d - %08x %08x %08x %08x %08x %08x\n", Frame++, SubContext->Parameters.InputSize, (PP_WDL - PP_IPBG), SubContext->Accumulated_ITS, Output, SubContext->Parameters.BufferPhysicalAddress, Total );
#endif
	//
	// Record the completion of this process
	//

	Record	= &H264ppContext->BufferProcessed[SubContext->BufferProcessedInsert];

	Record->PP		= N;
	Record->PP_ITS		= SubContext->Accumulated_ITS;
	Record->QueueIdentifier	= SubContext->Parameters.QueueIdentifier;
        Record->OutputSize	= (PP_WDL - PP_ISBG);
	Record->Field		= SubContext->Parameters.Field;
	Record->DecodeTime	= SubContext->DecodeTime;
	Record->Filled		= 1;

	OSDEV_ReleaseSemaphore( &H264ppContext->ProcessedBuffer );

	//
	// If we have seen an error condition, then force the pre-processor
	// to launch the H264ppWorkAroundGNBvd42331 on the next pass.
	//

	if( h264_pp_workaround_gnbvd42331 && (PP_ITS != PP_ITM__DMA_CMP) )
	    SubContext->ForceWorkAroundGNBvd42331	= 1;

	//
	// Indicate that the pre-processor is free
	//

	SubContext->Busy	= 0;
	OSDEV_ReleaseSemaphore( &H264ppContext->PreProcessor );
    }

//

    return IRQ_HANDLED;
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The Open function

static OSDEV_OpenEntrypoint(  H264ppOpen )
{
unsigned int      i,j;
H264ppContext_t  *H264ppContext;
int               Status;
unsigned int      N;

    //
    // Entry
    //

    OSDEV_OpenEntry();

    //
    // Is there a set of PPs to get.
    //

    OSDEV_ClaimSemaphore( &Lock );
    if( OpenMask == ((1 << H264_PP_NUMBER_OF_PRE_PROCESSORS) - 1) )
    {
	OSDEV_Print( "H264ppOpen - Too many opens.\n" );
	OSDEV_ReleaseSemaphore( &Lock );
	OSDEV_OpenExit( OSDEV_Error );
    }

    //
    // Allocate and initialize the private data structure.
    //

    H264ppContext    = (H264ppContext_t *)OSDEV_Malloc( sizeof(H264ppContext_t) );
    OSDEV_PrivateData   = (void *)H264ppContext;

    if( H264ppContext == NULL )
    {
	OSDEV_Print( "H264ppOpen - Unable to allocate memory for open context.\n" );
	OSDEV_ReleaseSemaphore( &Lock );
	OSDEV_OpenExit( OSDEV_Error );
    }

//

    memset( H264ppContext, 0x00, sizeof(H264ppContext_t) );

    H264ppContext->Closing              = false;

    //
    // Find a free pair of PPs
    //

    for( H264ppContext->Index   = 0;
	 ((OpenMask & (H264_PP_PER_INSTANCE_MASK<<H264ppContext->Index)) != 0);
	 H264ppContext->Index += H264_PP_PER_INSTANCE );

    OpenMask    |= (H264_PP_PER_INSTANCE_MASK<<H264ppContext->Index);

    //
    // Initialize the processed buffer list indices
    //

    H264ppContext->BufferProcessedInsert	= 0;
    H264ppContext->BufferProcessedExtract	= 0;

    //
    // Initialize the semaphores
    //

    OSDEV_InitializeSemaphore( &H264ppContext->PreProcessor, H264_PP_PER_INSTANCE ); 				// No one currently using the pre-processors - so they are free
    OSDEV_InitializeSemaphore( &H264ppContext->ProcessedBufferListEntry, H264_PP_MAX_SUPPORTED_BUFFERS );	// The table contains this many entries
    OSDEV_InitializeSemaphore( &H264ppContext->ProcessedBuffer, 0 );                    			// There are no processed buffers to signal

    //
    // Ensure we will have no interrupt surprises and install the interrupt handler
    //

    for( i=0; i<H264_PP_PER_INSTANCE; i++ )
    {
	N = H264ppContext->Index + i;

	OSDEV_WriteLong( PP_ITS(N),                     0xffffffff );           // Clear interrupt status
	OSDEV_WriteLong( PP_ITM(N),                     0x00000000 );

	//
	// Initialize subcontect data
	//

	H264ppContext->SubContext[i].ForceWorkAroundGNBvd42331		= 1;
	H264ppContext->SubContext[i].last_mb_adaptive_frame_field_flag	= 0;	// Doesn't matter, will be initialized on first frame

	//
	// Perform soft reset
	//

	OSDEV_WriteLong( PP_SRS(N),			1 );			// Perform a soft reset
	for( j=0; j<H264_PP_RESET_TIME_LIMIT; j++ )
	{
	    if( (OSDEV_ReadLong(PP_ITS(N)) & PP_ITM__SRS_COMP) != 0 )
		break;

	    OSDEV_SleepMilliSeconds( 1 );
	}

	if( j == H264_PP_RESET_TIME_LIMIT )
	    OSDEV_Print( "H264ppOpen - Failed to soft reset PP %d.\n", N );

	OSDEV_WriteLong( PP_ITS(N),                     0xffffffff );           // Clear interrupt status

//

	OSDEV_WriteLong( PP_MAX_OPC_SIZE(N),            5 );                    // Setup the ST bus parameters
	OSDEV_WriteLong( PP_MAX_CHUNK_SIZE(N),          0 );
	OSDEV_WriteLong( PP_MAX_MESSAGE_SIZE(N),        3 );

//

	Status  = request_irq( H264_PP_INTERRUPT(N), H264ppInterruptHandler, 0, "H264 PP", (void *)((unsigned int)H264ppContext + i) );
	if( Status != 0 )
	{
	    OSDEV_Print( "H264ppOpen - Unable to request an IRQ for PP %d.\n", N );

	    OSDEV_DeInitializeSemaphore( &H264ppContext->PreProcessor );
	    OSDEV_DeInitializeSemaphore( &H264ppContext->ProcessedBufferListEntry );
	    OSDEV_DeInitializeSemaphore( &H264ppContext->ProcessedBuffer );

	    for( j=0; j<i; j++ )
		free_irq( H264_PP_INTERRUPT((H264ppContext->Index + j)), (void *)((unsigned int)H264ppContext+j) );

	    OpenMask    ^= (H264_PP_PER_INSTANCE_MASK<<H264ppContext->Index);

	    OSDEV_ReleaseSemaphore( &Lock );
	    OSDEV_OpenExit( OSDEV_Error );
	}
    }

//

    OSDEV_ReleaseSemaphore( &Lock );
    OSDEV_OpenExit( OSDEV_NoError );
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The Release function

static OSDEV_CloseEntrypoint(  H264ppClose )
{
unsigned int             i;
H264ppContext_t         *H264ppContext;
unsigned int             N;

//

    OSDEV_CloseEntry();
    H264ppContext               = (H264ppContext_t  *)OSDEV_PrivateData;

    H264ppContext->Closing      = true;

    OSDEV_ClaimSemaphore( &Lock );

    //
    // Turn off interrupts
    //

    for( i=0; i<H264_PP_PER_INSTANCE; i++ )
    {
	N               = H264ppContext->Index + i;

	OSDEV_WriteLong( PP_ITM(N),             0x00000000 );
	OSDEV_WriteLong( PP_ITS(N),             0xffffffff );           // Clear interrupt status

	free_irq( H264_PP_INTERRUPT(N), (void *)((unsigned int)H264ppContext + i) );
    }

    //
    // Make sure everyone exits
    //

    OSDEV_ReleaseSemaphore( &H264ppContext->PreProcessor );
    OSDEV_ReleaseSemaphore( &H264ppContext->ProcessedBufferListEntry );
    OSDEV_ReleaseSemaphore( &H264ppContext->ProcessedBuffer );

    OSDEV_SleepMilliSeconds( 2 );

    //
    // De-initialize the semaphores
    //

    OSDEV_DeInitializeSemaphore( &H264ppContext->PreProcessor );
    OSDEV_DeInitializeSemaphore( &H264ppContext->ProcessedBufferListEntry );
    OSDEV_DeInitializeSemaphore( &H264ppContext->ProcessedBuffer );

    //
    // Free up the particular PP cells, and free the context memory
    //

    OpenMask    ^= (H264_PP_PER_INSTANCE_MASK<<H264ppContext->Index);

    OSDEV_Free( H264ppContext );

//

    OSDEV_ReleaseSemaphore( &Lock );
    OSDEV_CloseExit( OSDEV_NoError );
}

// ////////////////////////////////////////////////////////////////////////////////
//
//    The mmap function to make the H264pp buffers available to the user for writing

static OSDEV_MmapEntrypoint( H264ppMmap )
{
H264ppContext_t  *H264ppContext;
OSDEV_Status_t    MappingStatus;

//

    OSDEV_MmapEntry();
    H264ppContext    = (H264ppContext_t  *)OSDEV_PrivateData;

//

    OSDEV_Print( "H264ppMmap - No allocated memory to map.\n" );
    MappingStatus	= OSDEV_Error;

//

    OSDEV_MmapExit( MappingStatus );
}

// ////////////////////////////////////////////////////////////////////////////////
//
//    The ioctl function to handle queueing of a buffer to the preprocessor

static OSDEV_Status_t H264ppIoctlQueueBuffer(   H264ppContext_t         *H264ppContext,
						unsigned int             ParameterAddress )
{
unsigned int                     i;
H264ppIndividualContext_t       *SubContext;
h264pp_ioctl_queue_t            *params;
unsigned int                     N;
unsigned int                     BufferBase;
unsigned int                     SourceAddress;
unsigned int                     EndAddress;
unsigned int                     SliceErrorStatusAddress;
unsigned int                     IntermediateAddress;
unsigned int                     IntermediateEndAddress;

    //
    // Claim resources
    //

    OSDEV_ClaimSemaphore( &H264ppContext->ProcessedBufferListEntry );   // Claim somewhere to store a record of the preprocessor output
    OSDEV_ClaimSemaphore( &H264ppContext->PreProcessor );               // Claim the preprocessor hardware

    if( H264ppContext->Closing )
    {
	OSDEV_Print( "H264ppIoctlQueueBuffer - device closing.\n" );
	return OSDEV_Error;
    }

    for( i=0; i<H264_PP_PER_INSTANCE; i++ )
	if( !H264ppContext->SubContext[i].Busy )
	{
	    SubContext          = &H264ppContext->SubContext[i];
	    N                   = H264ppContext->Index + i;
	    SubContext->Busy    = 1;
	    break;
	}

    if( i >= H264_PP_PER_INSTANCE )
    {
	OSDEV_Print( "H264ppIoctlQueueBuffer - No free sub-context - implementation error (should be one, the semaphore says there is).\n" );
	return OSDEV_Error;
    }

//

    params			= &SubContext->Parameters;
    OSDEV_CopyToDeviceSpace( params, ParameterAddress, sizeof(h264pp_ioctl_queue_t) );

    //
    // Optionally check for and perform the workaround to GNBvd42331
    //

    if( h264_pp_workaround_gnbvd42331 )
	H264ppWorkAroundGNBvd42331( SubContext, N );

    //
    // Calculate the address values
    //

    BufferBase                  = (unsigned int)params->BufferPhysicalAddress;

    SliceErrorStatusAddress     = BufferBase;
    IntermediateAddress         = BufferBase + H264_PP_SESB_SIZE;
    IntermediateEndAddress      = IntermediateAddress + H264_PP_OUTPUT_SIZE - 1;
    SourceAddress               = IntermediateAddress + H264_PP_OUTPUT_SIZE;
    EndAddress                  = SourceAddress + params->InputSize - 1;

    //
    // Program the preprocessor
    //
    // Standard pre-processor initialization
    //

    flush_cache_all();
    OSDEV_WriteLong( PP_ITS(N),                 0xffffffff );                           // Clear interrupt status

#if 1
    OSDEV_WriteLong( PP_ITM(N),                 PP_ITM__BIT_BUFFER_OVERFLOW |           // We are interested in every interrupt
						PP_ITM__BIT_BUFFER_UNDERFLOW |
						PP_ITM__INT_BUFFER_OVERFLOW |
						PP_ITM__ERROR_BIT_INSERTED |
						PP_ITM__ERROR_SC_DETECTED |
						PP_ITM__SRS_COMP |
						PP_ITM__DMA_CMP );
#else
    OSDEV_WriteLong( PP_ITM(N),                 PP_ITM__DMA_CMP );
#endif

    //
    // Setup the decode
    //

    OSDEV_WriteLong( PP_BBG(N),                 (SourceAddress & 0xfffffff8) );
    OSDEV_WriteLong( PP_BBS(N),                 (EndAddress    | 0x7) );
    OSDEV_WriteLong( PP_READ_START(N),          SourceAddress );
    OSDEV_WriteLong( PP_READ_STOP(N),           EndAddress );

    OSDEV_WriteLong( PP_ISBG(N),                SliceErrorStatusAddress );
    OSDEV_WriteLong( PP_IPBG(N),                IntermediateAddress );
    OSDEV_WriteLong( PP_IBS(N),                 IntermediateEndAddress );

    OSDEV_WriteLong( PP_CFG(N),                 PP_CFG__CONTROL_MODE__START_STOP | params->Cfg );
    OSDEV_WriteLong( PP_PICWIDTH(N),            params->PicWidth );
    OSDEV_WriteLong( PP_CODELENGTH(N),          params->CodeLength );

    //
    // Launch the pre-processor
    //

    SubContext->BufferProcessedInsert		= H264ppContext->BufferProcessedInsert++;
    SubContext->DecodeTime             		= OSDEV_GetTimeInMilliSeconds();
    SubContext->Accumulated_ITS			= 0;

    OSDEV_WriteLong( PP_START(N),               1 );

//

    H264ppContext->BufferProcessedInsert	%= H264_PP_MAX_SUPPORTED_BUFFERS;

//

    return OSDEV_NoError;
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The ioctl function to get a buffer that has been handled by the pre-processor

static OSDEV_Status_t H264ppIoctlGetPreprocessedBuffer( H264ppContext_t         *H264ppContext,
							unsigned int             ParameterAddress )
{
h264pp_ioctl_dequeue_t           params;
H264ppProcessedBuffer_t		*Record;

//

    do
    {
	//
	// Anything to report
	//

	if( H264ppContext->BufferProcessed[H264ppContext->BufferProcessedExtract].Filled )
	{
	    Record					 = &H264ppContext->BufferProcessed[H264ppContext->BufferProcessedExtract++];
	    H264ppContext->BufferProcessedExtract	%= H264_PP_MAX_SUPPORTED_BUFFERS;

#if 0
	    if( Record->PP_ITS != PP_ITM__DMA_CMP )
		printk( "H264ppIoctlGPB (PP %d)  - Took %dms - ITS = %04x - QID = %d\n", Record->PP, Record->DecodeTime, Record->PP_ITS, Record->QueueIdentifier );
#endif

	    //
	    // Take the buffer
	    //

	    params.QueueIdentifier	= Record->QueueIdentifier;
	    params.OutputSize		= Record->OutputSize;
	    params.ErrorMask		= Record->PP_ITS & ~(PP_ITM__SRS_COMP | PP_ITM__DMA_CMP);
	    OSDEV_CopyToUserSpace( ParameterAddress, &params, sizeof(h264pp_ioctl_dequeue_t) );

	    //
	    // Free up the record for re-use
	    //

	    Record->Filled		= 0;
	    OSDEV_ReleaseSemaphore( &H264ppContext->ProcessedBufferListEntry );

	    return OSDEV_NoError;
	}

	//
	// Wait for a buffer completed signal
	//

	OSDEV_ClaimSemaphore( &H264ppContext->ProcessedBuffer );

    } while( !H264ppContext->Closing );

//

    OSDEV_Print( "H264ppIoctlGetPreprocessedBuffer - device closing.\n" );
    return OSDEV_Error;
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The ioctl function

static OSDEV_IoctlEntrypoint( H264ppIoctl )
{
OSDEV_Status_t           Status;
H264ppContext_t      *H264ppContext;

/* --- */

    OSDEV_IoctlEntry();
    H264ppContext    = (H264ppContext_t  *)OSDEV_PrivateData;

    switch( OSDEV_IoctlCode )
    {
	case H264_PP_IOCTL_QUEUE_BUFFER:
			Status = H264ppIoctlQueueBuffer(                H264ppContext, OSDEV_ParameterAddress );
			break;

	case H264_PP_IOCTL_GET_PREPROCESSED_BUFFER:
			Status = H264ppIoctlGetPreprocessedBuffer(      H264ppContext, OSDEV_ParameterAddress );
			break;

	default:        OSDEV_Print( "H264ppIoctl : Invalid ioctl %08x\n", OSDEV_IoctlCode );
			Status = OSDEV_Error;
    }

/* --- */

    OSDEV_IoctlExit( Status );
}


// ////////////////////////////////////////////////////////////////////////////////
//
//    The ioctl function

#define GNBvd42331_CFG          0x4DB9C923
#define GNBvd42331_PICWIDTH     0x00010001
#define GNBvd42331_CODELENGTH   0x000001D0

static unsigned char	 GNBvd42331Data[] = 
{
	0x00, 0x00, 0x00, 0x01, 0x27, 0x64, 0x00, 0x15, 0x08, 0xAC, 0x1B, 0x16, 0x39, 0xB2, 0x00, 0x00,
	0x00, 0x01, 0x28, 0x03, 0x48, 0x47, 0x86, 0x83, 0x50, 0x13, 0x02, 0xC1, 0x4A, 0x15, 0x00, 0x00,
	0x00, 0x01, 0x65, 0xB0, 0x34, 0x80, 0x00, 0x00, 0x03, 0x01, 0x6F, 0x70, 0x00, 0x14, 0x0A, 0xFF,
	0xF6, 0xF7, 0xD0, 0x01, 0xAE, 0x5E, 0x3D, 0x7C, 0xCA, 0xA9, 0xBE, 0xCC, 0xB3, 0x3B, 0x50, 0x92,
	0x27, 0x47, 0x24, 0x34, 0xE5, 0x24, 0x84, 0x53, 0x7C, 0xF5, 0x2C, 0x6E, 0x7B, 0x48, 0x1F, 0xC9,
	0x8D, 0x73, 0xA8, 0x3F, 0x00, 0x00, 0x01, 0x0A, 0x03
};

static unsigned char	*GNBvd42331DataPhysicalAddress	= NULL;

//

static void H264ppWorkAroundGNBvd42331( H264ppIndividualContext_t	 *SubContext,
					unsigned int			  N )
{
unsigned int	i;
unsigned int	mb_adaptive_frame_field_flag;
unsigned int	entropy_coding_mode_flag;
unsigned int	PerformWorkaround;
unsigned int	SavedITM;
unsigned int	BufferBase;
unsigned int	SourceAddress;
unsigned int	EndAddress;
unsigned int	SliceErrorStatusAddress;
unsigned int	IntermediateAddress;
unsigned int	IntermediateEndAddress;

    //
    // Do we have to worry.
    //

    mb_adaptive_frame_field_flag			= ((SubContext->Parameters.Cfg & 1) != 0);
    entropy_coding_mode_flag				= ((SubContext->Parameters.Cfg & 2) != 0);

    PerformWorkaround					= !mb_adaptive_frame_field_flag && 
							  SubContext->last_mb_adaptive_frame_field_flag &&
							  entropy_coding_mode_flag;

    SubContext->last_mb_adaptive_frame_field_flag	= mb_adaptive_frame_field_flag;

    if( !PerformWorkaround && !SubContext->ForceWorkAroundGNBvd42331 )
	return;

//OSDEV_Print( "H264ppWorkAroundGNBvd42331 - Deploying GNBvd42331 workaround block to PP %d - %08x.\n", N, SubContext->Parameters.Cfg );

    SubContext->ForceWorkAroundGNBvd42331	= 0;

    //
    // we transfer the workaround stream to the output buffer (offset by 64k to not interfere with the output).
    //

    memcpy( (void *)((unsigned int)SubContext->Parameters.BufferCachedAddress + 0x10000), GNBvd42331Data, sizeof(GNBvd42331Data) );

    GNBvd42331DataPhysicalAddress	= (unsigned char *)SubContext->Parameters.BufferPhysicalAddress + 0x10000;

#ifdef __TDT__
/* found somewhere this patch which should increase performance about 1%.
 * ->should be revised!
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
    dma_cache_wback((SubContext->Parameters.BufferCachedAddress + 0x10000),sizeof(GNBvd42331Data));
#else
    writeback_ioremap_region(0, (SubContext->Parameters.BufferCachedAddress + 0x10000),
    		0, sizeof(GNBvd42331Data));
#endif

#else
    flush_cache_all();
#endif

    //
    // Derive the pointers - we use the next buffer to be queued as output as our output 
    //

    BufferBase                  = (unsigned int)SubContext->Parameters.BufferPhysicalAddress;

    SliceErrorStatusAddress     = BufferBase;
    IntermediateAddress         = BufferBase + H264_PP_SESB_SIZE;
    IntermediateEndAddress      = IntermediateAddress + H264_PP_OUTPUT_SIZE - 1;
    SourceAddress               = (unsigned int)GNBvd42331DataPhysicalAddress;
    EndAddress                  = (unsigned int)GNBvd42331DataPhysicalAddress + sizeof(GNBvd42331Data) - 1;

    //
    // Launch the workaround block
    //

    SavedITM		= OSDEV_ReadLong(PP_ITM(N));			// Turn off interrupts
    OSDEV_WriteLong( PP_ITM(N), 0 );

    OSDEV_WriteLong( PP_BBG(N),                 (SourceAddress & 0xfffffff8) );
    OSDEV_WriteLong( PP_BBS(N),                 (EndAddress    | 0x7) );
    OSDEV_WriteLong( PP_READ_START(N),          SourceAddress );
    OSDEV_WriteLong( PP_READ_STOP(N),           EndAddress );

    OSDEV_WriteLong( PP_ISBG(N),                SliceErrorStatusAddress );
    OSDEV_WriteLong( PP_IPBG(N),                IntermediateAddress );
    OSDEV_WriteLong( PP_IBS(N),                 IntermediateEndAddress );

    OSDEV_WriteLong( PP_CFG(N),                 GNBvd42331_CFG );
    OSDEV_WriteLong( PP_PICWIDTH(N),            GNBvd42331_PICWIDTH );
    OSDEV_WriteLong( PP_CODELENGTH(N),          GNBvd42331_CODELENGTH );

    OSDEV_WriteLong( PP_ITS(N),                 0xffffffff );		// Clear interrupt status
    OSDEV_WriteLong( PP_START(N),               1 );

    //
    // Wait for it to complete
    //

    for( i=0; i<H264_PP_RESET_TIME_LIMIT; i++ )
    {
	OSDEV_SleepMilliSeconds( 1 );

	if( (OSDEV_ReadLong(PP_ITS(N)) & PP_ITM__DMA_CMP) != 0 )
	    break;

    }

    if( (i == H264_PP_RESET_TIME_LIMIT) || (OSDEV_ReadLong(PP_ITS(N)) != PP_ITM__DMA_CMP) )
	OSDEV_Print( "H264ppWorkAroundGNBvd42331 - Failed to execute GNBvd42331 workaround block to PP %d (ITS %08x).\n", N, OSDEV_ReadLong(PP_ITS(N)) );

    //
    // Restore the interrupts
    //

    OSDEV_WriteLong( PP_ITS(N),                     0xffffffff );           // Clear interrupt status
    OSDEV_WriteLong( PP_ITM(N), SavedITM );

}
