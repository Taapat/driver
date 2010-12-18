/************************************************************************
COPYRIGHT (C) STMicroelectronics (R&D) Limited 2005

Source file name : osdev_device.c
Author :           Daniel

Takes all the inline symbols in osdev_device.h and make them available
to the rest of the kernel as normal functions. This allows C++
applications access to these symbols without having to include the
Linux headers (which are not C++ safe).

Note that it is not possible to export all the OSDEV_ functions as
functions since some will break quite badly when un-inlined.


Date        Modification                                    Name
----        ------------                                    --------
10-Oct-05   Created                                         Daniel

************************************************************************/

#define EXPORT_SYMBOLS_FOR_CXX

#include "osdev_device.h"

OSDEV_ExportSymbol(OSDEV_Malloc);
OSDEV_ExportSymbol(OSDEV_Free);
OSDEV_ExportSymbol(OSDEV_TranslateAddressToUncached);
OSDEV_ExportSymbol(OSDEV_PurgeCacheAll);
OSDEV_ExportSymbol(OSDEV_FlushCacheRange);
OSDEV_ExportSymbol(OSDEV_InvalidateCacheRange);
OSDEV_ExportSymbol(OSDEV_SnoopCacheRange);
OSDEV_ExportSymbol(OSDEV_PurgeCacheRange);
