
#ifndef __DRV_CONF_H__
#define __DRV_CONF_H__
#include "autoconf.h"

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

//#include <rtl871x_byteorder.h>

#endif // __DRV_CONF_H__

