/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2002

Source file name : Fixed.h
Author :           Julian

Definition of some common defines and structures used in more than one class


Date        Modification                                    Name
----        ------------                                    --------
19-Sep-06   Created                                         Julian

************************************************************************/

#ifndef H_FIXED
#define H_FIXED

#ifdef __cplusplus
extern "C" {
#endif

/*
Fixed point numbers for frame rates aspect ratios etc
We are using a Q16.16 format
*/

typedef unsigned int                    Fixed_t;

#define FIX(a)                          (a << 16)
#define INTEGER_PART(a)                 (a >> 16)
#define TIMES(a, b)                     (Fixed_t)((((unsigned long long)a) * ((unsigned long long)b)) >> 16)
#define DIVIDE(a, b)                    (Fixed_t)((((unsigned long long)a) << 16) / b)
#define FRACTIONAL_PART(a)              (a & 0xffff)
#define FRACTIONAL_DIVISOR              0x10c6f687              /* 0x800000000000 / 500000.5 */
#define FRACTIONAL_DECIMAL(a)           (unsigned int)((((unsigned long long)FRACTIONAL_PART(a)) << 32ULL) / FRACTIONAL_DIVISOR)

#ifdef __cplusplus
}
#endif

#endif

