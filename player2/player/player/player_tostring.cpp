/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2009

Source file name : to_string.cpp
Author :           Daniel

Massively overloaded to_string() method to decode various types.
This is mostly for debugging/logging purposes.

Date        Modification                                    Name
----        ------------                                    --------
7-Jan-09    Created                                         Daniel

************************************************************************/

#include "player_types.h"

#define CASE(x) case x: return #x
#define DEFAULT default: return "UNKNOWN"

const char *ToString(PlayerStreamType_t StreamType)
{
    switch (StreamType) {
	CASE(StreamTypeNone);
	CASE(StreamTypeAudio);
	CASE(StreamTypeVideo);
	CASE(StreamTypeOther);
	DEFAULT;
    }
}
