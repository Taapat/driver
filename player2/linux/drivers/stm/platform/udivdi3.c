/*
 * Simple __udivdi3 function which doesn't use FPU.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#ifdef __TDT__
#include <linux/version.h>
#endif

#include <asm/div64.h>

extern u64 __xdiv64_32(u64 n, u32 d);

u64 __udivdi3(u64 n, u64 d) {
	if (unlikely(d & 0xffffffff00000000LL)) {
		printk(KERN_WARNING "Workaround for 64-bit/64-bit division.");
		uint32_t di = d;
		/* Scale divisor to 32 bits */
		if (d > 0xffffffffULL) {
			unsigned int shift = fls(d >> 32);
			di = d >> shift;
			n >>= shift;
		}
		/* avoid 64 bit division if possible */
		if (n >> 32) {
			do_div(n, di);
			return d;
		}
	}
	return __xdiv64_32(n, (u32)d);
}

#if defined(__TDT__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
#warning This implementation of __umoddi3 may be redundant because __umoddi3 is available under stlinux24 which does not provide this symbol explicittly either.
uint32_t __umoddi3(u64 xp, u32 y)
{
        uint32_t rem;
        uint64_t q = __xdiv64_32(xp, y);

        rem = xp - q * y;

        return rem;
}

EXPORT_SYMBOL(__umoddi3);
EXPORT_SYMBOL(__udivdi3);
#else
#if defined (CONFIG_KERNELVERSION) /* STLinux 2.3 or later */
EXPORT_SYMBOL(__udivdi3);
#endif
#endif
