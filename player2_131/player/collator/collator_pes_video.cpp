/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : collator_pes.cpp
Author :           Nick

Implementation of the pes collator class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
19-Apr-07   Created from existing collator_pes_video.cpp    Daniel

************************************************************************/

// /////////////////////////////////////////////////////////////////////////
///     \class Collator_PesVideo_c
///
///     Specialized PES collator implementing video specific features.
///
///     For typical video that is uses MPEG start codes to demark different
///	sections we we can identify frame boundaries by studying the start
///	codes it contains. This class combines the extraction of elementary
///	stream with the process of identifying frame boundaries.
///

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "collator_pes_video.h"
#include "st_relay.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#define ZERO_START_CODE_HEADER_SIZE	7		// Allow us to see 00 00 01 00 00 01 <other code>

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

////////////////////////////////////////////////////////////////////////////
///
/// 	Handle input, by scanning the start codes, and chunking the data
///
///  \par Note 1:
///
///	Since we are accepting pes encapsulated data, and 
///	junking the pes header, we need to accumulate the 
///	pes header, before parsing and junking it.
///
///  \par Note 2:
///
///	We also need to accumulate and parse padding headers 
///	to allow the skipping of pad data.
///
///  \par Note 3:
///
///	In general we deal with only 4 byte codes, so we do not
///	need to accumulate more than 4 bytes for a code.
///	A special case for this is code Zero. Pes packets may
///	partition the input at any point, so standard start 
///	codes can span a pes packet header, this is no problem 
///	so long as we check the accumulated bytes alongside new 
///	bytes after skipping a pes header. 
///
///  \par Note 4:
///
///	The zero code special case, when we see 00 00 01 00, we
///	need to accumulate a further 3 bytes this is because we
///	can have the special case of 
///
///		00 00 01 00 00 01 <pes/packing start code>
///
///	where the first start code lead in has a terminal byte 
///	later in the stream which may lead to a completely different 
///	code. If we see 00 00 01 00 00 01 we always ignore the first 
///	code, as in a legal DVB stream this must be followed by a
///	pes/packing code of some sort, we accumulate the 3rd byte to
///	determine which
///
///	\todo This function weighs in at over 450 lines...
///
CollatorStatus_t   Collator_PesVideo_c::Input(
					PlayerInputDescriptor_t	 *Input,
					unsigned int		  DataLength,
					void			 *Data )
{
unsigned int		i;
CollatorStatus_t	Status;
unsigned int		HeaderSize;
unsigned int		Transfer;
unsigned int		Skip;
unsigned int		SpanningWord;
unsigned int		StartingWord;
unsigned int		SpanningCount;
unsigned int		CodeOffset;
unsigned char		Code;
unsigned char		C1, C2, C3;
//
    st_relayfs_write(ST_RELAY_TYPE_PES_VIDEO_BUFFER, ST_RELAY_SOURCE_VIDEO_COLLATOR, (unsigned char *)Data, DataLength, 0 );

    AssertComponentState( "Collator_PesVideo_c::Input", ComponentRunning );

    ActOnInputDescriptor( Input );

    //
    // Initialize scan state
    //

    StartCodeCount	= 0;
    RemainingData	= (unsigned char *)Data;
    RemainingLength	= DataLength;

    while( RemainingLength != 0 )
    {
	//
	// Are we accumulating extra data to judge a zero code
	//

	if( GotPartialZeroHeader )
	{
	    HeaderSize		= ZERO_START_CODE_HEADER_SIZE;

	    Transfer	= min( RemainingLength, (HeaderSize - GotPartialZeroHeaderBytes) );
	    memcpy( StoredZeroHeader+GotPartialZeroHeaderBytes, RemainingData, Transfer );

	    GotPartialZeroHeaderBytes	+= Transfer;
	    RemainingData		+= Transfer;
	    RemainingLength		-= Transfer;

	    if( GotPartialZeroHeaderBytes == ZERO_START_CODE_HEADER_SIZE )
	    {
		GotPartialZeroHeader		 = false;				// Lose it so we do not come back here
		if( (StoredZeroHeader[4] == 0x00) && (StoredZeroHeader[5] == 0x01) )	// Do we have a secondary start code
		{
		    AccumulatedDataSize		+= 3;					// Leave the first 00 00 01 in the accumulated buffer
		    if( StoredZeroHeader[6] == PES_PADDING_START_CODE )
		    {
			GotPartialPaddingHeader		 = true;
			GotPartialPaddingHeaderBytes	 = 4;
			StoredPaddingHeader		 = BufferBase + AccumulatedDataSize;
		    }
		    else
		    {
			GotPartialPesHeader		 = true;
			GotPartialPesHeaderBytes	 = 4;
			StoredPesHeader			 = BufferBase + AccumulatedDataSize;
		    }
		}
		else
		{
		    //
		    // Is it a block terminate code
		    //

		    if( Configuration.BlockTerminateCode == 0x00 )
		    {
			C1				 = StoredZeroHeader[4];		// Note StoredZeroHeader will point to invalid memory after the flush
			C2		 		 = StoredZeroHeader[5];
			C3		 		 = StoredZeroHeader[6];

			Status				 = FrameFlush();
			if( Status != CollatorNoError )
			    return Status;

			BufferBase[0]	 		 = 0x00;
			BufferBase[1]	 		 = 0x00;
			BufferBase[2]	 		 = 0x01;
			BufferBase[3]	 		 = 0x00;
			BufferBase[4]	 		 = C1;
			BufferBase[5]	 		 = C2;
			BufferBase[6]	 		 = C3;
			AccumulatedDataSize		 = 0;
			SeekingPesHeader		 = false;
		    }

		    //
		    // Otherwise (and if its a block terminate) accumulate the start code
		    //

		    Status	= AccumulateStartCode( PackStartCode(AccumulatedDataSize,0x00) );
		    if( Status != CollatorNoError )
		    {
			DiscardAccumulatedData();
			return Status;
		    }

		    AccumulatedDataSize		+= ZERO_START_CODE_HEADER_SIZE;
		}
	    }

	    if( RemainingLength == 0 )
		return CollatorNoError;
	}

	//
	// Are we building a pes header
	//

	if( GotPartialPesHeader )
	{
	    HeaderSize		= PES_INITIAL_HEADER_SIZE;
	    if( RemainingLength >= (PES_INITIAL_HEADER_SIZE - GotPartialPesHeaderBytes) )
		HeaderSize	= GotPartialPesHeaderBytes >= PES_INITIAL_HEADER_SIZE ?
					PES_HEADER_SIZE(StoredPesHeader) :
					PES_HEADER_SIZE(RemainingData-GotPartialPesHeaderBytes);

	    Transfer	= min( RemainingLength, (HeaderSize - GotPartialPesHeaderBytes) );
	    memcpy( StoredPesHeader+GotPartialPesHeaderBytes, RemainingData, Transfer );

	    GotPartialPesHeaderBytes	+= Transfer;
	    RemainingData		+= Transfer;
	    RemainingLength		-= Transfer;

	    if( GotPartialPesHeaderBytes == PES_HEADER_SIZE(StoredPesHeader) )
	    {
		//
		// Since we are going to process the partial header, we will not have it in future
		//

		GotPartialPesHeader	= false;

//

		Status	= ReadPesHeader();
		if( Status != CollatorNoError )
		    return Status;

		if( SeekingPesHeader )
		{
		    AccumulatedDataSize		= 0;		// Dump any collected data
		    SeekingPesHeader		= false;
		}
	    }

	    if( RemainingLength == 0 )
		return CollatorNoError;
	}

	//
	// Are we building a padding header
	//

	if( GotPartialPaddingHeader )
	{
	    HeaderSize		= PES_PADDING_INITIAL_HEADER_SIZE;

	    Transfer	= min( RemainingLength, (HeaderSize - GotPartialPaddingHeaderBytes) );
	    memcpy( StoredPaddingHeader+GotPartialPaddingHeaderBytes, RemainingData, Transfer );

	    GotPartialPaddingHeaderBytes	+= Transfer;
	    RemainingData			+= Transfer;
	    RemainingLength			-= Transfer;

	    if( GotPartialPaddingHeaderBytes == PES_PADDING_INITIAL_HEADER_SIZE )
	    {
		Skipping			 = PES_PADDING_SKIP(StoredPaddingHeader);
		GotPartialPaddingHeader		 = false;
	    }

	    if( RemainingLength == 0 )
		return CollatorNoError;
	}

	//
	// Are we skipping padding
	//

	if( Skipping != 0 )
	{
	    Skip		 = min( Skipping, RemainingLength );
	    RemainingData	+= Skip;
	    RemainingLength	-= Skip;
	    Skipping		-= Skip;
	    
	    if( RemainingLength == 0 )
		return CollatorNoError;
	}

	//
	// Check for spanning header
	//

	SpanningWord		 = 0xffffffff << (8 * min(AccumulatedDataSize,3));
	SpanningWord		|= BufferBase[AccumulatedDataSize-3] << 16;
	SpanningWord		|= BufferBase[AccumulatedDataSize-2] << 8;
	SpanningWord		|= BufferBase[AccumulatedDataSize-1];

	StartingWord		 = 0x00ffffff >> (8 * min((RemainingLength-1),3));
	StartingWord		|= RemainingData[0] << 24;
	StartingWord		|= RemainingData[1] << 16;
	StartingWord		|= RemainingData[2] <<  8;

	//
	// Check for a start code spanning, or in the first word
	// record the nature of the span in a counter indicating how many 
	// bytes of the code are in the remaining data. 
	// NOTE the 00 at the bottom indicates we have a byte for the code, 
	//      not what it is.
	//

	SpanningCount		= 0;

	if( (SpanningWord << 8) == 0x00000100 )
	{
	    SpanningCount	= 1;
	}
	else if( ((SpanningWord << 16) | ((StartingWord >> 16) & 0xff00)) == 0x00000100 )
	{
	    SpanningCount	= 2;
	}
	else if( ((SpanningWord << 24) | ((StartingWord >> 8)  & 0xffff00)) == 0x00000100 )
	{
	    SpanningCount	= 3;
	}
	else if( StartingWord == 0x00000100 )
	{
	    SpanningCount	= 4;
	    UseSpanningTime	= false;
	}

	//
	// Check that if we have a spanning code, that the code is not to be ignored
	//

	if( (SpanningCount != 0) && 
	    inrange(RemainingData[SpanningCount-1], Configuration.IgnoreCodesRangeStart, Configuration.IgnoreCodesRangeEnd) )
	{
	    SpanningCount	= 0;
	}

	//
	// Handle a spanning start code
	//

	if( SpanningCount != 0 )
	{
	    //
	    // Copy over the spanning bytes
	    //

	    for( i=0; i<SpanningCount; i++ )
		BufferBase[AccumulatedDataSize + i]	= RemainingData[i];

	    AccumulatedDataSize	+= SpanningCount;
	    RemainingData	+= SpanningCount;
	    RemainingLength	-= SpanningCount;

	    Code		 = BufferBase[AccumulatedDataSize-1];

	    //
	    // Is it a zero code
	    //

	    if( Code == 0x00 )
	    {
		GotPartialZeroHeader		 = true;
		AccumulatedDataSize		-= 4;		// Wind to before it
		GotPartialZeroHeaderBytes	 = 4;
		StoredZeroHeader		 = BufferBase + AccumulatedDataSize;
		continue;
	    }

	    //
	    // Is it a pes header, and is it a pes stream we are interested in
	    //

	    else if( IS_PES_START_CODE_VIDEO(Code) )
	    {
		AccumulatedDataSize		-= 4;		// Wind to before it
		if( (Code & Configuration.StreamIdentifierMask) == Configuration.StreamIdentifierCode )
		{
		    GotPartialPesHeader		= true;
		    GotPartialPesHeaderBytes	 = 4;
		    StoredPesHeader		 = BufferBase + AccumulatedDataSize;
		}
		else
		{
		    SeekingPesHeader		 = true;
		}
		continue;
	    }

	    //
	    // Or is it a padding block
	    //

	    else if( Code == PES_PADDING_START_CODE )
	    {
		GotPartialPaddingHeader		 = true;
		AccumulatedDataSize		-= 4;		// Wind to before it
		GotPartialPaddingHeaderBytes	 = 4;
		StoredPaddingHeader		 = BufferBase + AccumulatedDataSize;
		continue;		
	    }

	    //
	    // Or if we are seeking a pes header, dump what we have and try again
	    //

	    else if( SeekingPesHeader )
	    {
		AccumulatedDataSize		 = 0;		// Wind to before it
		continue;		
	    }

	    //
	    // Or is it a block terminate code
	    //

	    else if( (((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode) && !Configuration.DeferredTerminateFlag) ||
		     (Configuration.StreamTerminateFlushesFrame && (Code == Configuration.StreamTerminationCode)) ||
		     ((Configuration.DeferredTerminateFlag && TerminationFlagIsSet)))
	    {
		AccumulatedDataSize		-=4;

		Status				 = FrameFlush( (Configuration.StreamTerminateFlushesFrame && (Code == Configuration.StreamTerminationCode)) );
		if( Status != CollatorNoError )
		    return Status;

		BufferBase[0]	 		 = 0x00;
		BufferBase[1]	 		 = 0x00;
		BufferBase[2]	 		 = 0x01;
		BufferBase[3]	 		 = Code;
		AccumulatedDataSize		 = 4;
		SeekingPesHeader		 = false;
		TerminationFlagIsSet = false;

	     if ( Configuration.DeferredTerminateFlag && 
		((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode)) TerminationFlagIsSet = true;
	    
	    } else
	      if ( Configuration.DeferredTerminateFlag && 
		((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode)) TerminationFlagIsSet = true;
		
	    //
	    // Otherwise (and if its a block terminate) accumulate the start code
	    //

	    Status	= AccumulateStartCode( PackStartCode(AccumulatedDataSize-4,Code) );
	    if( Status != CollatorNoError )
	    {
		DiscardAccumulatedData();
		return Status;
	    }
	}

	//
	// If we had no spanning code, but we had a spanning PTS, and we 
	// had no normal PTS for this frame, the copy the spanning time 
	// to the normal time.
	//

	else if( !PlaybackTimeValid )
	{
	    PlaybackTimeValid		= SpanningPlaybackTimeValid;
	    PlaybackTime		= SpanningPlaybackTime;
	    DecodeTimeValid		= SpanningDecodeTimeValid;
	    DecodeTime			= SpanningDecodeTime;
	    UseSpanningTime		= false;
	    SpanningPlaybackTimeValid	= false;
	    SpanningDecodeTimeValid	= false;
	}

	//
	// Now enter the loop processing start codes
	//

	while( true )
	{
	    Status	= FindNextStartCode( &CodeOffset );
	    if( Status != CollatorNoError )
	    {
		//
		// Terminal code after start code processing copy remaining data into buffer
		//

		Status	= AccumulateData( RemainingLength, RemainingData );
		if( Status != CollatorNoError )
		    DiscardAccumulatedData();

		RemainingLength		= 0;
		return Status;
	    }

	    //
	    // Got one accumulate upto and including it
	    //

	    Status	= AccumulateData( CodeOffset+4, RemainingData );
	    if( Status != CollatorNoError )
	    {
		DiscardAccumulatedData();
		return Status;
	    }

	    Code				 = RemainingData[CodeOffset+3];
	    RemainingLength			-= CodeOffset+4;
	    RemainingData			+= CodeOffset+4;

	    //
	    // Is it a zero code
	    //

	    if( Code == 0x00 )
	    {
		GotPartialZeroHeader		 = true;
		AccumulatedDataSize		-= 4;		// Wind to before it
		GotPartialZeroHeaderBytes	 = 4;
		StoredZeroHeader		 = BufferBase + AccumulatedDataSize;
		break;
	    }

	    //
	    // Is it a pes header, and is it a pes stream we are interested in
	    //

	    else if( IS_PES_START_CODE_VIDEO(Code) )
	    {
		AccumulatedDataSize		-= 4;		// Wind to before it
		if( (Code & Configuration.StreamIdentifierMask) == Configuration.StreamIdentifierCode )
		{
		    GotPartialPesHeader		= true;
		    GotPartialPesHeaderBytes	 = 4;
		    StoredPesHeader		 = BufferBase + AccumulatedDataSize;
		}
		else
		{
		    SeekingPesHeader		 = true;
		}
		break;
	    }

	    //
	    // Or is it a padding block
	    //

	    else if( Code == PES_PADDING_START_CODE )
	    {
		GotPartialPaddingHeader		 = true;
		AccumulatedDataSize		-= 4;		// Wind to before it
		GotPartialPaddingHeaderBytes	 = 4;
		StoredPaddingHeader		 = BufferBase + AccumulatedDataSize;
		break;
	    }

	    //
	    // Or if we are seeking a pes header, dump what we have and try again
	    //

	    else if( SeekingPesHeader )
	    {
		AccumulatedDataSize		 = 0;		// Wind to before it
		break;		
	    }

		
	    //
	    // Or is it a block terminate code
	    //

	    else if( (((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode) && !Configuration.DeferredTerminateFlag) ||
		     (Configuration.StreamTerminateFlushesFrame && (Code == Configuration.StreamTerminationCode)) ||
		     ((Configuration.DeferredTerminateFlag && TerminationFlagIsSet)))
	    {
		AccumulatedDataSize		-=4;

		Status				 = FrameFlush( (Configuration.StreamTerminateFlushesFrame && (Code == Configuration.StreamTerminationCode)) );
		if( Status != CollatorNoError )
		    return Status;

		BufferBase[0]	 		 = 0x00;
		BufferBase[1]	 		 = 0x00;
		BufferBase[2]	 		 = 0x01;
		BufferBase[3]	 		 = Code;
		AccumulatedDataSize		 = 4;
		SeekingPesHeader		 = false;
		TerminationFlagIsSet = false;
		if( Configuration.DeferredTerminateFlag && 
		    ((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode) )
		    TerminationFlagIsSet = true;
	    }

	   else if( Configuration.DeferredTerminateFlag && 
		    ((Code & Configuration.BlockTerminateMask) == Configuration.BlockTerminateCode) )
	   {
		TerminationFlagIsSet = true;
	   }
		
	    //
	    // Otherwise (and if its a block terminate) accumulate the start code
	    //

	    Status	= AccumulateStartCode( PackStartCode(AccumulatedDataSize-4,Code) );
	    if( Status != CollatorNoError )
	    {
		DiscardAccumulatedData();
		return Status;
	    }
	}
    }

    return CollatorNoError;
}


// /////////////////////////////////////////////////////////////////////////
//
// 	The Frame Flush functions
//

CollatorStatus_t   Collator_PesVideo_c::FrameFlush( bool	FlushedByStreamTerminate )
{
    CodedFrameParameters->FollowedByStreamTerminate     = FlushedByStreamTerminate;
    return FrameFlush();
}


// -----------------------

CollatorStatus_t   Collator_PesVideo_c::FrameFlush(		void )
{
CollatorStatus_t	Status;

//

    AssertComponentState( "Collator_PesVideo_c::FrameFlush", ComponentRunning );

//

    Status					= Collator_Pes_c::FrameFlush();
    if( Status != CodecNoError )
	return Status;

    SeekingPesHeader				= true;
    GotPartialZeroHeader			= false;
    GotPartialPesHeader				= false;
    GotPartialPaddingHeader			= false;
    Skipping					= 0;

    TerminationFlagIsSet = false;
	
    //
    // at this point we sit (approximately) between frames and should update the PTS/DTS with the values
    // last extracted from the PES header. UseSpanningTime will (or at least should) be true when the
    // frame header spans two PES packets, at this point the frame started in the previous packet and
    // should therefore use the older PTS.
    //
    if( UseSpanningTime )
    {
	CodedFrameParameters->PlaybackTimeValid	= SpanningPlaybackTimeValid;
	CodedFrameParameters->PlaybackTime	= SpanningPlaybackTime;
	SpanningPlaybackTimeValid		= false;
	CodedFrameParameters->DecodeTimeValid	= SpanningDecodeTimeValid;
	CodedFrameParameters->DecodeTime	= SpanningDecodeTime;
	SpanningDecodeTimeValid			= false;
    }
    else
    {
	CodedFrameParameters->PlaybackTimeValid	= PlaybackTimeValid;
	CodedFrameParameters->PlaybackTime	= PlaybackTime;
	PlaybackTimeValid			= false;
	CodedFrameParameters->DecodeTimeValid	= DecodeTimeValid;
	CodedFrameParameters->DecodeTime	= DecodeTime;
	DecodeTimeValid				= false;
    }

    return CodecNoError;
}


