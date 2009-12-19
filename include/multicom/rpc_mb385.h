/*
 * rpc_mb385.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Replacement OS21 board support for MB385.
 */

#if ! defined _RPC_MB385_H_
#define _RPC_MB385_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <os21.h>

extern interrupt_name_t OS21_INTERRUPT_TIMER_0;
extern interrupt_name_t OS21_INTERRUPT_TIMER_1;
extern interrupt_name_t OS21_INTERRUPT_TIMER_2;

extern interrupt_name_t OS21_INTERRUPT_MBOX1_0;
extern interrupt_name_t OS21_INTERRUPT_MBOX1_1;

extern interrupt_name_t OS21_INTERRUPT_REMOTE_0;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_1;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_2;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_3;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_4;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_5;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_6;
extern interrupt_name_t OS21_INTERRUPT_REMOTE_7;

#ifdef __cplusplus
}
#endif

#endif /* _RPC_MB385_H_ */
