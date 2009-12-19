#ifndef __FP_H
#define __FP_H

#define FP_IOCTL_GETID			0
#define FP_IOCTL_POWEROFF		1
#define FP_IOCTL_REBOOT			2
#define FP_IOCTL_LCD_DIMM		3
#define FP_IOCTL_LED			4
#define FP_IOCTL_GET_WAKEUP_TIMER	5
#define FP_IOCTL_SET_WAKEUP_TIMER	6
#define FP_IOCTL_GET_VCR		7
#define FP_IOCTL_GET_REGISTER		8
#define FP_IOCTL_IS_WAKEUP		9
#define FP_IOCTL_CLEAR_WAKEUP_TIMER	10
#define FP_IOCTL_SET_REGISTER		11
#define FP_IOCTL_LCD_AUTODIMM		12

#define RC_IOCTL_BCODES			0

#ifdef __KERNEL__

#define FP_MINOR	0
#define RC_MINOR	1

#include "dbox2_fp_reset.h"
#include "dbox2_fp_sec.h"
#include "dbox2_fp_tuner.h"

#endif
#endif
