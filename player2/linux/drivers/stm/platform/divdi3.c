/*
 * Simple __divdi3 function which doesn't use FPU.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/math64.h>

extern u64 __xdiv64_32(u64 n, u32 d);

s64 __divdi3(s64 n, s64 d)
{
    int c = 0;
    s64 res;

    if (n < 0LL)
    {
        c = ~c;
        n = -n;
    }

    if (d < 0LL)
    {
        c = ~c;
        d = -d;
    }

    if (unlikely(d & 0xffffffff00000000LL))
    {
        printk(KERN_WARNING "Workaround for 64-bit/64-bit division");
        uint32_t di = d;

        /* Scale divisor to 32 bits */
        if (d > 0xffffffffULL)
        {
            unsigned int shift = fls(d >> 32);
            di = d >> shift;
            n >>= shift;
        }

        /* avoid 64 bit division if possible */
        if (n >> 32)
        {
            do_div(n, di);
            return d;
        }
    }

    res = __xdiv64_32(n, (u32)d);

    if (c)
    {
        res = -res;
    }

    return res;
}

EXPORT_SYMBOL(__divdi3);

MODULE_DESCRIPTION("Player2 64Bit Divide Driver");
MODULE_AUTHOR("STMicroelectronics Limited");
MODULE_VERSION("0.9");
MODULE_LICENSE("GPL");
