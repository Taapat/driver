/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : demultiplexor_base.h
Author :           Nick

Definition of the base demultiplexor class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------
13-Nov-06   Created                                         Nick

************************************************************************/

#ifndef H_DEMULTIPLEXOR_BASE
#define H_DEMULTIPLEXOR_BASE

// /////////////////////////////////////////////////////////////////////
//
//	Include any component headers

#include "player.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

#define	DEMULTIPLEXOR_MAX_STREAMS	4

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

typedef struct DemultiplexorBaseStreamContext_s
{
    PlayerStream_t	Stream;
    unsigned int	Identifier;
    Collator_t		Collator;
} DemultiplexorBaseStreamContext_t;

//

struct DemultiplexorBaseContext_s
{
    OS_Mutex_t				  Lock;

    unsigned int			  LastStreamSet;
    PlayerInputDescriptor_t		 *Descriptor;
    unsigned int			  BufferLength;
    unsigned char			 *BufferData;
    DemultiplexorBaseStreamContext_t	  Streams[DEMULTIPLEXOR_MAX_STREAMS];
};

//

typedef struct DemultiplexorBaseContext_s   *DemultiplexorBaseContext_t;

// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class Demultiplexor_Base_c : public Demultiplexor_c
{
protected:

    // Data

    unsigned int	  SizeofContext;

    // Functions

    void		  SetContextSize(	unsigned int	SizeofContext );

public:

    //
    // Constructor/Destructor methods
    //

    Demultiplexor_Base_c( 	void );
    ~Demultiplexor_Base_c( 	void );

    //
    // Context managment functions
    //

    DemultiplexorStatus_t   CreateContext(	DemultiplexorContext_t	 *Context );

    DemultiplexorStatus_t   DestroyContext(	DemultiplexorContext_t	  Context );

    DemultiplexorStatus_t   AddStream(		DemultiplexorContext_t	  Context,
						PlayerStream_t		  Stream,
						unsigned int		  StreamIdentifier );

    DemultiplexorStatus_t   RemoveStream(	DemultiplexorContext_t	  Context,
						unsigned int		  StreamIdentifier );

    DemultiplexorStatus_t   InputJump(		DemultiplexorContext_t	  Context );

    DemultiplexorStatus_t   Demux(		PlayerPlayback_t	  Playback,
						DemultiplexorContext_t	  Context,
						Buffer_t		  Buffer );
};

#endif

