/***********************************************************************
 *
 * File: stmfb/Gamma/sti7106/sti7106device.h
 * Copyright (c) 2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7106DEVICE_H
#define _STI7106DEVICE_H

#ifdef __cplusplus

#include <Gamma/sti7105/sti7105device.h>


class CSTi7106Device : public CSTi7105Device
{
public:
  CSTi7106Device  (void);
  ~CSTi7106Device (void);

protected:
  bool CreateHDMIOutput(void);

private:
  CSTi7106Device (const CSTi7106Device &);
  CSTi7106Device& operator= (const CSTi7106Device &);
};

#endif /* __cplusplus */


#endif // _STI7105DEVICE_H
