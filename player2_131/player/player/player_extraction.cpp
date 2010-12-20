/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : player_extraction.cpp
Author :           Nick

Implementation of the data extraction related functions of the
generic class implementation of player 2


Date        Modification                                    Name
----        ------------                                    --------
06-Nov-06   Created                                         Nick

************************************************************************/

#include "player_generic.h"

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Useful defines/macros that need not be user visible
//

// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Request capture of a decode buffer
//

PlayerStatus_t   Player_Generic_c::RequestDecodeBufferReference(
						PlayerStream_t		  Stream,
						unsigned long long	  NativeTime,
						void			 *EventUserData )
{
    return PlayerNoError;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////
//
//      Release a previously captured decode buffer
//

PlayerStatus_t   Player_Generic_c::ReleaseDecodeBufferReference(
						PlayerEventRecord_t	 *Record )
{
    return PlayerNoError;
}





