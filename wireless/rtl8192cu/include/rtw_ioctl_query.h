#ifndef __IOCTL_QUERY_H
#define __IOCTL_QUERY_H

#include <drv_conf.h>
#include <drv_types.h>


#ifdef PLATFORM_WINDOWS

u8 query_802_11_capability(_adapter*	padapter,u8*	pucBuf,u32 *	pulOutLen);
u8 query_802_11_association_information (_adapter * padapter, PNDIS_802_11_ASSOCIATION_INFORMATION pAssocInfo);

#endif


#endif

