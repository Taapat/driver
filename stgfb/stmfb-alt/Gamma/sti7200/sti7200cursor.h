/***********************************************************************
 *
 * File: stgfb/Gamma/sti7200/sti7200cursor.h
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef STi7200CURSOR_H
#define STi7200CURSOR_H

#include <Gamma/GammaCompositorCursor.h>

class CSTi7200Cursor: public CGammaCompositorCursor
{
public:
    CSTi7200Cursor(ULONG baseAddr);
    ~CSTi7200Cursor();
};

#endif // STi7200CURSOR_H
