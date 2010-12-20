/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2006

Source file name : lmimalloc.h
Author :           Nick

Contains the prototypes of the VERY simple lmi malloc, used by the allocator.

Date        Modification                                    Name
----        ------------                                    --------
14-Mar-06   Created                                         Nick

************************************************************************/

#ifndef H_LMIMALLOC
#define H_LMIMALLOC

//

#define LMI_MALLOC_MAX_DISCONTIGUOUS_BLOCKS	16
#define LMI_PAGE_SIZE				4096

//

typedef struct LmiBlock_s
{
    bool		 InUse;
    unsigned int	 Size;
    unsigned char	*Base;
} LmiBlock_t;

//

OSDEV_Status_t	LmiMallocInitialize(	unsigned int	 MemorySize,
					unsigned char	*MemoryBase );

void    LmiMallocTerminate(		void );

unsigned char	*LmiMalloc(		unsigned int	 Size );

void	LmiFree(			unsigned int	 Size,
					unsigned char	*Memory );

#endif


