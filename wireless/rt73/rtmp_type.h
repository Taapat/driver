/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 5F, No. 36 Taiyuan St.
 * Jhubei City
 * Hsinchu County 302, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2008, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  * 
 * it under the terms of the GNU General Public License as published by  * 
 * the Free Software Foundation; either version 2 of the License, or     * 
 * (at your option) any later version.                                   * 
 *                                                                       * 
 * This program is distributed in the hope that it will be useful,       * 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * 
 * GNU General Public License for more details.                          * 
 *                                                                       * 
 * You should have received a copy of the GNU General Public License     * 
 * along with this program; if not, write to the                         * 
 * Free Software Foundation, Inc.,                                       * 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             * 
 *                                                                       * 
 *************************************************************************
 
    Module Name:
    rtmp_type.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Paul Lin    1-2-2004
*/

#ifndef __RTMP_TYPE_H__
#define __RTMP_TYPE_H__

// Put platform dependent declaration here
// For example, linux type definition

#define PACKED  __attribute__ ((packed))

//#ifdef  Linux
typedef unsigned char		UINT8;
typedef unsigned short      UINT16;
typedef unsigned long       UINT32;
typedef unsigned long long  UINT64;
typedef int					INT32;
//#endif

#ifndef Win32
#define Win32

//#undef  BIG_ENDIAN          // Only little endian for WIN32 system

//
// Following type define have been already define in
// %include%\basetsd.h     eg. c:\winddk\3790\inc\wxp\basetsd.h
//     Defined  Win2k      WinXP
//     UINT16     X           O
//     UINT32     O           O
//     UINT64     X           O
#ifdef NDIS50_MINIPORT

typedef unsigned short      UINT16;
//typedef   unsigned long       UINT32;
typedef unsigned __int64    UINT64;

#endif //#ifdef NDIS50_MINIPORT

#endif //#ifndef Win32


// Endian byte swapping codes
#define SWAP16(x) \
    ((UINT16)( \
    (((UINT16)(x) & (UINT16) 0x00ffU) << 8) | \
    (((UINT16)(x) & (UINT16) 0xff00U) >> 8) ))

#define SWAP32(x) \
    ((UINT32)( \
    (((UINT32)(x) & (UINT32) 0x000000ffUL) << 24) | \
    (((UINT32)(x) & (UINT32) 0x0000ff00UL) <<  8) | \
    (((UINT32)(x) & (UINT32) 0x00ff0000UL) >>  8) | \
    (((UINT32)(x) & (UINT32) 0xff000000UL) >> 24) ))

#define SWAP64(x) \
    ((UINT64)( \
    (UINT64)(((UINT64)(x) & (UINT64) 0x00000000000000ffULL) << 56) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x000000000000ff00ULL) << 40) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x0000000000ff0000ULL) << 24) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x00000000ff000000ULL) <<  8) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x000000ff00000000ULL) >>  8) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x0000ff0000000000ULL) >> 24) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0x00ff000000000000ULL) >> 40) | \
    (UINT64)(((UINT64)(x) & (UINT64) 0xff00000000000000ULL) >> 56) ))

#ifdef BIG_ENDIAN

#define cpu2le64(x) SWAP64((x))
#define le2cpu64(x) SWAP64((x))
#define cpu2le32(x) SWAP32((x))
#define le2cpu32(x) SWAP32((x))
#define cpu2le16(x) SWAP16((x))
#define le2cpu16(x) SWAP16((x))
#define cpu2be64(x) ((UINT64)(x))
#define be2cpu64(x) ((UINT64)(x))
#define cpu2be32(x) ((UINT32)(x))
#define be2cpu32(x) ((UINT32)(x))
#define cpu2be16(x) ((UINT16)(x))
#define be2cpu16(x) ((UINT16)(x))

#else

#define cpu2le64(x) ((UINT64)(x))
#define le2cpu64(x) ((UINT64)(x))
#define cpu2le32(x) ((UINT32)(x))
#define le2cpu32(x) ((UINT32)(x))
#define cpu2le16(x) ((UINT16)(x))
#define le2cpu16(x) ((UINT16)(x))
#define cpu2be64(x) SWAP64((x))
#define be2cpu64(x) SWAP64((x))
#define cpu2be32(x) SWAP32((x))
#define be2cpu32(x) SWAP32((x))
#define cpu2be16(x) SWAP16((x))
#define be2cpu16(x) SWAP16((x))

#endif  // BIG_ENDIAN



// Ralink timer control block
typedef struct  _RALINK_TIMER_STRUCT    {
	BOOLEAN				Valid;			// Set to True when call RTMPInitTimer
    struct timer_list	TimerObj;       // Ndis Timer object
    ULONG               TimerValue;     // Timer value in milliseconds
    BOOLEAN             State;          // True if timer cancelled
    BOOLEAN             Repeat;         // True if periodic timer
	ULONG				cookie;			// os specific object
}   RALINK_TIMER_STRUCT, *PRALINK_TIMER_STRUCT;

typedef struct	PACKED _RSN_IE_HEADER_STRUCT	{
	UCHAR		Eid;
	UCHAR		Length;
	USHORT		Version;	// Little endian format
}	RSN_IE_HEADER_STRUCT, *PRSN_IE_HEADER_STRUCT;

// Cipher suite selector types
typedef struct PACKED _CIPHER_SUITE_STRUCT	{
	UCHAR		Oui[3];
	UCHAR		Type;
}	CIPHER_SUITE_STRUCT, *PCIPHER_SUITE_STRUCT;

// Authentication and Key Management suite selector
typedef struct PACKED _AKM_SUITE_STRUCT	{
	UCHAR		Oui[3];
	UCHAR		Type;
}	AKM_SUITE_STRUCT, *PAKM_SUITE_STRUCT;

#ifdef BIG_ENDIAN
typedef struct	_RSN_CAPABILITY	{
	USHORT		PreAuth:1;
	USHORT		NoPairwise:1;
	USHORT		PTKSAReplayCnt:2;
	USHORT		GTKSAReplayCnt:2;
	USHORT		Rsv:10;
}	RSN_CAPABILITY, *PRSN_CAPABILITY;
#else
// RSN capability
typedef struct	_RSN_CAPABILITY	{
	USHORT		Rsv:10;
	USHORT		GTKSAReplayCnt:2;
	USHORT		PTKSAReplayCnt:2;
	USHORT		NoPairwise:1;
	USHORT		PreAuth:1;
}	RSN_CAPABILITY, *PRSN_CAPABILITY;
#endif /* !BIG_ENDIAN */

typedef struct _CIPHER_KEY {
	UCHAR	BssId[6];
	UCHAR	CipherAlg;			// 0-none, 1:WEP64, 2:WEP128, 3:TKIP, 4:AES, 5:CKIP64, 6:CKIP128
	UCHAR	KeyLen; 			// Key length for each key, 0: entry is invalid
	UCHAR	Key[16];			// right now we implement 4 keys, 128 bits max
	UCHAR	RxMic[8];
	UCHAR	TxMic[8];
	UCHAR	TxTsc[6];			// 48bit TSC value
	UCHAR	RxTsc[6];			// 48bit TSC value
	UCHAR	Type;				// Indicate Pairwise/Group when reporting MIC error
}	CIPHER_KEY, *PCIPHER_KEY;
#endif	// __RTMP_TYPE_H__
