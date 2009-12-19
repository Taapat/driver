////////////////////////////////////////////////////////////////////////////
/// COPYRIGHT (C) STMicroelectronics 2007
///
/// Source file name : crc32.h
/// Author :           Daniel
///
/// Simple 32-bit CRC implementation for debugging purpose.
///
///
/// Date        Modification                                    Name
/// ----        ------------                                    --------
/// 30-Aug-07   Created                                         Daniel
///
////////////////////////////////////////////////////////////////////////////

#ifndef CRC32_H_
#define CRC32_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned int crc32(unsigned char *data, unsigned int length);

#ifdef __cplusplus
};
#endif

#endif /*CRC32_H_*/
