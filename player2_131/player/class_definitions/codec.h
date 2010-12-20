/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : codec.h
Author :           Nick

Definition of the codec class module for player 2.


Date        Modification                                    Name
----        ------------                                    --------
02-Nov-06   Created                                         Nick

************************************************************************/

#ifndef H_CODEC
#define H_CODEC

#include "player.h"

// ---------------------------------------------------------------------
//
// Local type definitions
//

enum
{
    CodecNoError				= PlayerNoError,
    CodecError					= PlayerError,

    CodecUnknownFrame				= BASE_CODEC
};

typedef PlayerStatus_t	CodecStatus_t;

//

enum
{
    CodecFnGetCodedFrameBufferPool	= BASE_CODEC,
    CodecFnRegisterOutputBufferRing,
    CodecFnOutputPartialDecodeBuffers,
    CodecFnReleaseReferenceFrame,
    CodecFnReleaseDecodeBuffer,
    CodecFnInput,

    CodecFnSetModuleParameters
};


// ---------------------------------------------------------------------
//
// Class definition
//

#define	CODEC_RELEASE_ALL		INVALID_INDEX

class Codec_c : public BaseComponentClass_c
{
public:

    virtual CodecStatus_t   GetCodedFrameBufferPool(	BufferPool_t		 *Pool,
							unsigned int		 *MaximumCodedFrameSize ) = 0;

    virtual CodecStatus_t   GetTrickModeParameters(	CodecTrickModeParameters_t	**TrickModeParameters ) = 0;

    virtual CodecStatus_t   RegisterOutputBufferRing(	Ring_t			  Ring ) = 0;

    virtual CodecStatus_t   OutputPartialDecodeBuffers(	void ) = 0;

    virtual CodecStatus_t   DiscardQueuedDecodes( 	void ) = 0;

    virtual CodecStatus_t   ReleaseReferenceFrame(	unsigned int		  ReferenceFrameDecodeIndex ) = 0;

    virtual CodecStatus_t   CheckReferenceFrameList(	unsigned int		  NumberOfReferenceFrameLists,
							ReferenceFrameList_t	  ReferenceFrameList[] ) = 0;

    virtual CodecStatus_t   ReleaseDecodeBuffer(	Buffer_t		  Buffer ) = 0;

    virtual CodecStatus_t   Input(			Buffer_t		  CodedBuffer ) = 0;
};

// ---------------------------------------------------------------------
//
// Docuentation
//

/*! \class Codec_c
\brief Responsible for taking individual parsed coded frames and decoding them.

The codec class is responsible for taking individual parsed coded 
frames and decoding them. It manages the decode buffers, and when 
decode buffer is complete (all fields + all slices) it places the 
decode buffer on its output ring. This is a list of its entrypoints, 
and a partial list of the calls it makes, and the data structures it 
accesses, these are in addition to the standard component class 
entrypoints, and the complete list of support entrypoints in the Player 
class.

The partial list of entrypoints used by this class:

- Empty list

The partial list of meta data types used by this class:

- Attached to input buffers:
  - <b>ParsedFrameParameters</b>, Describes the coded frame to be decoded.
  - <b>StartCodeList</b>, Optional input for those codecs that require a list of start codes (IE mpeg2 soft decoder).
  - <b>Parsed[Video|Audio|Data]Parameters</b>, Optional input attachment to be transferred to the output.

- Added to output buffers:
  - <b>ParsedFrameParameters</b>, Copied from the input, or merged with any current data for video decoding of partial frames.
  - <b>Parsed[Video|Audio|Data]Parameters</b>, Optional Copied from the input, or merged with any current data for video decoding of partial frames.

*/

/*! \fn CodecStatus_t Codec_c::GetCodedFrameBufferPool(BufferPool_t *Pool, unsigned int *MaximumCodedFrameSize)
\brief Gain access to (and potentially allocate) the coded frame buffer pool.

<b>WARNING: This function was documented by Daniel and has not been subject to review by Mr. Haydock.</b>

On the first call to this function after class construction, or a reset,
the function will ensure that the coded frame buffer pool (collator
buffers) exists (this may involve the actual creation of the pool). On 
the first and all subsequent calls to this function it will return the 
buffer pool.

Buffers within the coded frame buffer pool are allocated at the maximum 
size for a coded frame.The pool must use an allocation method that 
allows the collator to request a fixed size buffer, and shrink that 
size after the actual usage has been established. In practice this 
probably means that the allocation method should be
<b>AllocateFromSuppliedBlock</b>.

\param Pool A pointer to a variable to hold the instance of the buffer pool.
\param MaximumCodedFrameSize A pointer to a variable to hold the maximum coded frame size.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t   GetTrickModeParameters(     CodecTrickModeParameters_t      **TrickModeParameters )
\brief Gain access to parameters describing trick mode capabilities.

This function gains access to a datastructuture held within the codec, that 
describes the abilities, of the codec, with respec to supporting trick modes. 
In particular it supplies information that will be used by the player to select 
the ranges in which particular fast forward algorithms operate.

The function retruns a pointer to memory held within the class, this memory must 
remain valid for the lifetime of the class.

\param TrickModeParameters A pointer to a variable to hold the pointer to the parameter structure.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::RegisterOutputBufferRing(Ring_t Ring) 
\brief Register a ring on which decoded frames are to be placed.

\param Ring Pointer to a Ring_c instance

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::OutputPartialDecodeBuffers()
\brief Request for incompletely decoded buffers to be immediately output.

Passes onto the output ring any decode buffers that are partially
filled, this includes buffers with only one field decoded, or a number 
of slices. In the event that several slices have been queued but not 
decoded, they should be decoded and the relevent buffer passed on.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::DiscardQueuedDecodes()
\brief Discard any ongoing decodes.

This function is non-blocking.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::ReleaseReferenceFrame(unsigned int ReferenceFrameDecodeIndex)
\brief Release a reference frame.

<b>TODO: This method is not adequately documented.</b>

\param ReferenceFrameDecodeIndex Decode index of the frame to be released.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::CheckReferenceFrameList(
						unsigned int		  NumberOfReferenceFrameLists,
						ReferenceFrameList_t	  ReferenceFrameList[] )

\brief Check a reference frame list

This function checks that all the frames mentioned in the supplied lists are available 
for use as reference frames. It is used, when frames are being discarded due to trick modes, 
to ensure that a frame can be decoded.

\param NumberOfReferenceFrameLists, Number of reference frame lists (one for mpeg2 et al, two for H264)
\param ReferenceFrameList, the array of reference frame lists

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::ReleaseDecodeBuffer(Buffer_t Buffer)
\brief Release a decode buffer.

<b>TODO: This method is not adequately documented.</b>

\param Buffer A pointer to the Buffer_c instance.

\return Codec status code, CodecNoError indicates success.
*/

/*! \fn CodecStatus_t Codec_c::Input(Buffer_t CodedBuffer)
\brief Accept coded data for decode.

The buffer provided to this function is the output from a frame parser.

\param CodedBuffer A pointer to a Buffer_c instance of a parsed coded frame buffer.

\return Codec status code, CodecNoError indicates success.
*/

#endif
