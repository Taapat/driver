/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : osdev_user.h
Author :           Nick

Contains the useful operating system functions/types
that allow us to implement OS independent device functionality.


Date        Modification                                    Name
----        ------------                                    --------
07-Mar-05   Created                                         Julian

************************************************************************/

#ifndef H_OSDEV_USER
#define H_OSDEV_USER

// ====================================================================================================================
//      The preamble,
//
// Useful constants
//

#define OSDEV_INVALID_DEVICE            NULL

//
// The enumeration types
//

#ifndef H_OSDEV_DEVICE
typedef enum
{
    OSDEV_NoError       = 0,
    OSDEV_Error
} OSDEV_Status_t;
#endif

//
// Simple types
//

typedef void                                    *DeviceHandle_t;

typedef struct OSDEV_OpenContext_s              *OSDEV_DeviceIdentifier_t;

//
// structures
//

typedef struct OSDEV_PollDevice_s
{
    OSDEV_DeviceIdentifier_t    DeviceId;       /* file descriptor */
    short                       Events;         /* requested events */
    short                       Revents;        /* returned events */
} OSDEV_PollDevice_t;

#define OSDEV_MAP_UNCACHED 1

// ====================================================================================================================
//      The user level device functions

#ifdef __cplusplus
extern "C" {
#endif
OSDEV_Status_t   OSDEV_Stat(      const char                      *Name );
OSDEV_Status_t   OSDEV_Open(      const char                      *Name,
				  OSDEV_DeviceIdentifier_t        *Handle );
OSDEV_Status_t   OSDEV_Close(     OSDEV_DeviceIdentifier_t         Handle );
OSDEV_Status_t   OSDEV_Read(      OSDEV_DeviceIdentifier_t         Handle,
				  void                            *Buffer,
				  unsigned int                     SizeToRead,
				  unsigned int                    *SizeRead );
OSDEV_Status_t   OSDEV_Write(     OSDEV_DeviceIdentifier_t         Handle,
				  void                            *Buffer,
				  unsigned int                     SizeToWrite,
				  unsigned int                    *SizeWritten );
OSDEV_Status_t   OSDEV_Ioctl(     OSDEV_DeviceIdentifier_t         Handle,
				  unsigned int                     Command,
				  void                            *Argument,
				  unsigned int                     ArgumentSize );
OSDEV_Status_t   OSDEV_Poll(      OSDEV_PollDevice_t*              Handles,
				  unsigned int                     Count,
				  unsigned int*                    NumberReady,
				  unsigned int                     Timeout );
OSDEV_Status_t   OSDEV_Map(       OSDEV_DeviceIdentifier_t         Handle,
				  unsigned int                     Offset,
				  unsigned int                     Length,
				  void                           **MapAddress,
				  unsigned int                     Flags );
OSDEV_Status_t   OSDEV_UnMap(     OSDEV_DeviceIdentifier_t         Handle,
				  void                            *MapAddress,
				  unsigned int                     Length );
#ifdef __cplusplus
}
#endif

#endif
