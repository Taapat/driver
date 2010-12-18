/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2003

Source file name : allocatorio.h
Author :           Nick

Contains the ioctl interface between the allocator user level components
(allocinline.h) and the allocator device level components.

Date        Modification                                    Name
----        ------------                                    --------
14-Jan-03   Created                                         Nick
12-Nov-07   Changed to use partition names for BPA2         Nick

************************************************************************/

#ifndef H_ALLOCATORIO
#define H_ALLOCATORIO

#define ALLOCATOR_MAX_PARTITION_NAME_SIZE	64

#define ALLOCATOR_IOCTL_ALLOCATE_DATA           1

#define ALLOCATOR_DEVICE                        "/dev/allocator"

//

typedef struct allocator_ioctl_allocate_s
{
    unsigned int         RequiredSize;
    char		 PartitionName[ALLOCATOR_MAX_PARTITION_NAME_SIZE];
    unsigned char	*CachedAddress;
    unsigned char	*UnCachedAddress;
    unsigned char       *PhysicalAddress;
} allocator_ioctl_allocate_t;

//

#endif
