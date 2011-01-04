/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : owner_identifiers.h
Author :           Nick

A list of pre-defined owner identifiers used to record the placement of buffers



Date        Modification                                    Name
----        ------------                                    --------
17-Nov-06   Created                                         Nick

************************************************************************/

#ifndef H_OWNER_IDENTIFIERS
#define H_OWNER_IDENTIFIERS

enum
{
    IdentifierGetInjectBuffer	= 1,
    IdentifierInSequenceCall,
    IdentifierDrain,

    IdentifierCollator,
    IdentifierFrameParser,
    IdentifierCodec,
    IdentifierManifestor,

    IdentifierProcessCollateToParse,
    IdentifierProcessParseToDecode,
    IdentifierProcessDecodeToManifest,
    IdentifierProcessPostManifest,

    IdentifierAttachedToOtherBuffer,
    IdentifierReverseDecodeStack,
    IdentifierFrameParserMarkerFrame,
    IdentifierNonDecodedFrameList,

    IdentifierH264Intermediate,
    IdentifierExternal
};

#endif



