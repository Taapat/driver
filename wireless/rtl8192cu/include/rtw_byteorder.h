#ifndef _RTL871X_BYTEORDER_H_
#define _RTL871X_BYTEORDER_H_

#include <drv_conf.h>

#if defined (CONFIG_LITTLE_ENDIAN) && defined (CONFIG_BIG_ENDIAN)
#error "Shall be CONFIG_LITTLE_ENDIAN or CONFIG_BIG_ENDIAN, but not both!\n"
#endif

#if defined (CONFIG_LITTLE_ENDIAN)
#ifndef CONFIG_PLATFORM_MSTAR389
#  include <byteorder/little_endian.h>
#endif
#elif defined (CONFIG_BIG_ENDIAN)
#  include <byteorder/big_endian.h>
#else
#  error "Must be LITTLE/BIG Endian Host"
#endif

#endif /* _RTL871X_BYTEORDER_H_ */

