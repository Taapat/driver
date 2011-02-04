/*
 * 	Copyright (C) 2010 Duolabs Srl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _SCI_H
#define _SCI_H

#define MYVERSION "0.0.9"

//#define DEBUG

#ifdef DEBUG
#define debug(args...) printk(args)
#else
#define debug(args...)
#endif


/*****************************
 * INCLUDES
 *****************************/

#include <linux/kernel.h>

#include "sci_types.h"

/*****************************
 * MACROS
 *****************************/

/******* SC generic *******/

#define SC_CLOCK_CONTROL_BY_FPGA


#define SCI_NUMBER_OF_CONTROLLERS   2           /**< Num of SC controllers */

#define SCI_BUFFER_SIZE             0x400 /* 1024*/ //512

#define SCI_INT_MEM_REG             0x1000

#define SCI_IO_SIZE                 0x1000      /**< Size of I/O mem: 4096 */

#define SYSCONFIG_BASE_ADDRESS      0x19001000
#define SYSCONFIG7                  0x11c

#define SCI_CLK_GLOBAL              100000000   /* 100 MHz */
#define SCI_CLK_VAL                 0x00
#define SCI_CLK_CTRL                0x04
#define CLKEN                       0x02

#define FPGA_BASE_ADDRESS           0x03800000   /* Base addr of Fpga */
#define FPGA_SCI0_CLOCK             0x1c        /* Register for Change clock of SC0 */ 
#define FPGA_SCI1_CLOCK             0x1e        /* Register for Change clock of SC1 */


#define SCI_UNKNOWN_CLASS			0           /**  */

#define SCI_CLASS_A                 1           /**< Not used: only 5V Vcc to SC */
#define SCI_CLASS_B                 2           /**< Not used: only 3V Vcc to SC */
#define SCI_CLASS_AB                3           /**< Not used: 5V or 3V Vcc to SC */

#define SCI_VCC_3                   3           /**< 3V to SC */
#define SCI_VCC_5                   5           /**< 3V to SC */

#define SCI_CLOCK_3579              1           /** Clock as 3.579 MHz to SC */
#define SCI_CLOCK_625               2           /** Clock as 6.25  MHz to SC */


#define SCI_PROC_FILENAME           "sc"

/******* SC 0 *******/

#define SCI0_INT_DETECT             80
#define SCI0_INT_DETECT_NAME        "sc0_detect"

#define SCI0_INT_RX_TX              123
#define SCI0_INT_RX_TX_NAME         "sc0_rx_tx"

#define PIO0_BASE_ADDRESS           0x18020000
#define PIO_P0C0                    0x20
#define PIO_P0C1                    0x30
#define PIO_P0C2                    0x40
#define PIO_SET_P0OUT               0x04
#define PIO_CLR_P0OUT               0x08
#define PIO_P0MASK                  0x60
#define PIO_P0COMP                  0x50
#define PIO_SET_P0COMP              0x54
#define PIO_CLR_P0COMP              0x58

#define SCI0_BASE_ADDRESS           0x18048000

#define ASC0_BASE_ADDRESS           0x18030000
#define ASC0_BAUDRATE               0x00
#define ASC0_TX_BUF                 0x004
#define ASC0_RX_BUF                 0x008
#define ASC0_CTRL                   0x00C
#define ASC0_INT_EN                 0x010
#define ASC0_STA                    0x014
#define ASC0_GUARDTIME              0x018
#define ASC0_TIMEOUT                0x01C
#define ASC0_TX_RST                 0x020
#define ASC0_RX_RST                 0x024
#define ASC0_RETRIES                0x028

/* Test SC0 voltage to 3v. */
#define PIO4_BASE_ADDRESS           0x18024000

#define PIO_CLR_P4C0                0x28
#define PIO_CLR_P4C1                0x38
#define PIO_CLR_P4C2                0x48
#define PIO_SET_P4C0                0x24
#define PIO_SET_P4C1                0x34
#define PIO_SET_P4C2                0x44
#define PIO_SET_P4OUT               0x04
#define PIO_CLR_P4OUT               0x08

/******* SC 1 *******/

#define SCI1_INT_DETECT             84
#define SCI1_INT_DETECT_NAME        "sc1_detect"

#define SCI1_INT_RX_TX              122
#define SCI1_INT_RX_TX_NAME         "sc1_rx_tx"

#define PIO1_BASE_ADDRESS           0x18021000
#define PIO_P1C0                    0x20
#define PIO_P1C1                    0x30
#define PIO_P1C2                    0x40
#define PIO_SET_P1OUT               0x04
#define PIO_CLR_P1OUT               0x08
#define PIO_P1MASK                  0x60
#define PIO_P1COMP                  0x50
#define PIO_SET_P1COMP              0x54
#define PIO_CLR_P1COMP              0x58

#define SCI1_BASE_ADDRESS           0x18049000

#define ASC1_BASE_ADDRESS           0x18031000
#define ASC1_BAUDRATE               0x00
#define ASC1_TX_BUF                 0x004
#define ASC1_RX_BUF                 0x008
#define ASC1_CTRL                   0x00C
#define ASC1_INT_EN                 0x010
#define ASC1_STA                    0x014
#define ASC1_GUARDTIME              0x018
#define ASC1_TIMEOUT                0x01C
#define ASC1_TX_RST                 0x020
#define ASC1_RX_RST                 0x024
#define ASC1_RETRIES                0x028

/* Test SC1 voltage to 3v. */
#define PIO3_BASE_ADDRESS           0x18023000

#define PIO_CLR_P3C0                0x28
#define PIO_CLR_P3C1                0x38
#define PIO_CLR_P3C2                0x48
#define PIO_SET_P3C0                0x24
#define PIO_SET_P3C1                0x34
#define PIO_SET_P3C2                0x44
#define PIO_SET_P3OUT               0x04
#define PIO_CLR_P3OUT               0x08

/******* Board-specific defines *******/

#define ACTIVE_HIGH                 1
#define ACTIVE_LOW                  0
#define SCI_CLASS                   SCI_CLASS_A     /**< Operating class of SCI */

/******* ATR parameters by standard *******/

#define SCI_MAX_ATR_SIZE            33              /**< Max size of an ATR */

#define SCI_ATR_T                   0
#define SCI_ATR_F                   4500000
#define SCI_ATR_ETU                 372
#define SCI_ATR_WWT                 9600
#define SCI_ATR_CWT                 8203

//#define SCI_ATR_BWT                 15371
#define SCI_ATR_BWT                 0

#define SCI_ATR_EGT                 0
#define SCI_ATR_CHECK               1
#define SCI_ATR_P                   5
#define SCI_ATR_I                   50
#define SCI_ATR_U                   SCI_CLASS_A

#define SCI_MAX_F                   80000000
#define SCI_MAX_ETU                 0xFFF
#define SCI_MAX_WWT                 0xFFFFFFFF
#define SCI_MAX_CWT                 0xFFFF
#define SCI_MAX_BWT                 0xFFFFFFFF
#define SCI_MAX_EGT                 0xFF

#define SCI_MIN_F                   1000000
#define SCI_MIN_ETU                 8
#define SCI_MIN_WWT                 12
#define SCI_MIN_CWT                 12
#define SCI_MIN_BWT                 971
#define SCI_MIN_EGT                 0

/* R/W mode flags */
#define SCI_SYNC                    0x00000001
#define SCI_DATA_ANY                0x00000002

/******* Clock states *******/
#define SCI_CLOCK_STOP_DISABLED     0
#define SCI_CLOCK_STOP_LOW          1
#define SCI_CLOCK_STOP_HIGH         2

/******* Inverter code ******/
#define	UNINVERT			0x00
#define	INVERT				0x01	
#define	NO_DEFINED			0xFF	

#define PARITY_ODD			0x20
#define STOP_2_BIT			0x18	/* The [4-3] bits: 11 -> 2 bits of stop ; 00 -> 0,5 bit of stop */

/***** Guard Time Parameters *****/
#define GT_DEFAULT			0x02	/* It should be 0 but for sti7101, it must be 2 */

#define	TA1_MASK			0x10
#define	TB1_MASK			0x20
#define	TC1_MASK			0x40
#define	TD1_MASK			0x80

/******* Char dev driver *******/
#define DEVICE_NAME                 "sc"
#define MAJOR_NUM			        169     /**< Major num of char dev */
#define MINOR_START                 0       /**< Starting minor for char dev */

/* Ioctl cmd table */
#define SCI_IOW_MAGIC			    's'

#define IOCTL_SET_RESET			   		_IOW(SCI_IOW_MAGIC, 1,  ULONG)
#define IOCTL_SET_MODES			   		_IOW(SCI_IOW_MAGIC, 2,  SCI_MODES)
#define IOCTL_GET_MODES			  		_IOW(SCI_IOW_MAGIC, 3,  SCI_MODES)
#define IOCTL_SET_PARAMETERS			_IOW(SCI_IOW_MAGIC, 4,  SCI_PARAMETERS)
#define IOCTL_GET_PARAMETERS			_IOW(SCI_IOW_MAGIC, 5,  SCI_PARAMETERS)
#define IOCTL_SET_CLOCK_START			_IOW(SCI_IOW_MAGIC, 6,  ULONG)
#define IOCTL_SET_CLOCK_STOP			_IOW(SCI_IOW_MAGIC, 7,  ULONG)
#define IOCTL_GET_IS_CARD_PRESENT		_IOW(SCI_IOW_MAGIC, 8,  ULONG)
#define IOCTL_GET_IS_CARD_ACTIVATED		_IOW(SCI_IOW_MAGIC, 9,  ULONG)



/* new ioctl*/
#define IOCTL_SET_DEACTIVATE        _IOW(SCI_IOW_MAGIC, 10, ULONG)
/************/

#define IOCTL_SET_ATR_READY		    _IOW(SCI_IOW_MAGIC, 11, ULONG)
#define IOCTL_GET_ATR_STATUS		_IOW(SCI_IOW_MAGIC, 12, ULONG)
#define IOCTL_SET_CLOCK				_IOW(SCI_IOW_MAGIC, 13, ULONG)

/* new ioctl */
#define IOCTL_DUMP_REGS             _IOW(SCI_IOW_MAGIC, 20, ULONG)
/*************/

#define IOCTL_SET_ONLY_RESET		_IOW(SCI_IOW_MAGIC, 100,  ULONG)	// add for single reset, without change any of the params clock & baud

/******* Check operation conditions *******/

/* Check if driver is initialized, if the number of SC controllers is right 
 * and if the card is present. Used in most of the funcs 
 */
#define SCI_CHECK_INIT_COND(sci_id, rc)                     \
    if (sci_driver_init != 1)                               \
        rc = SCI_ERROR_CARD_NOT_PRESENT;                    \
    if (sci_id > SCI_NUMBER_OF_CONTROLLERS)                 \
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;              \
    if (sci_cb[sci_id].card_detect == SCI_CARD_NOT_PRESENT) \
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;

/******* Debugging *******/

#define PDEBUG(fmt, args...)                                             \
    do {                                                                 \
        if (debug) printk(KERN_NOTICE "[%s]: " fmt, __FUNCTION__, ##args); \
    } while (0)

//#define PINFO(fmt, args...)         printk(KERN_INFO " " fmt, ##args)
//#define PERROR(fmt, args...)        printk(KERN_ERR "[%s]: " fmt, __FUNCTION__, ##args)

#define PINFO	PDEBUG
#define PERROR	PDEBUG

// FIXME Remove it
// This value is defined only to compile but it is a 'dummy' value
#define __STB_SYS_CLK               (63000000)      // 63MHz SYS_CLK of xxx

/*****************************
 * DATA TYPES
 *****************************/

/* Enums */

/**< Base addresses of the different registers */
typedef enum base_addr_en
{
    BASE_ADDRESS_SYSCFG7=0, 
    BASE_ADDRESS_PIO0, 
    BASE_ADDRESS_PIO1, 
    BASE_ADDRESS_ASC0,
    BASE_ADDRESS_ASC1,
    BASE_ADDRESS_SCI0,
    BASE_ADDRESS_SCI1,
    BASE_ADDRESS_PIO4,
    BASE_ADDRESS_PIO3,
	BASE_ADDRESS_FPGA
} BASE_ADDR;

/**< Card present */
typedef enum sci_card_status_en
{
    SCI_CARD_NOT_PRESENT,
    SCI_CARD_PRESENT
} SCI_CARD_STATUS;

/**< SCI ATR status */
typedef enum sci_atr_status_en
{
    SCI_WITHOUT_ATR = 0,
    SCI_ATR_READY
} SCI_ATR_STATUS;

/**< Error codes */
typedef enum sci_error_en
{
    SCI_ERROR_OK = 0,
    SCI_ERROR_DRIVER_NOT_INITIALIZED = -1000,
    SCI_ERROR_FAIL,
    SCI_ERROR_KERNEL_FAIL,
    SCI_ERROR_NO_ATR,
    SCI_ERROR_CRC_FAIL,
    SCI_ERROR_CARD_NOT_PRESENT,
    SCI_ERROR_CARD_NOT_ACTIVATED,
    SCI_ERROR_AWT_TIMEOUT,
    SCI_ERROR_WWT_TIMEOUT,
    SCI_ERROR_CWT_TIMEOUT,
    SCI_ERROR_BWT_TIMEOUT,
    SCI_ERROR_PARAMETER_OUT_OF_RANGE,
    SCI_ERROR_TRANSACTION_ABORTED,
    SCI_ERROR_CLOCK_STOP_DISABLED,
    SCI_ERROR_ATR_PENDING,
    SCI_ERROR_VCC_INVALID,
    SCI_ERROR_SCI_INVALID
} SCI_ERROR;

/* Structs */

/**< SCI modes (not used) */
typedef struct sci_modes_st
{
    INT emv2000;
    INT dma;
    INT man_act;
    INT rw_mode;
} SCI_MODES;

/**< SCI communication parameters for T1 */
typedef struct sci_parameters_st
{
    UCHAR T;
    ULONG f;
    ULONG ETU;
    ULONG WWT;
    ULONG CWT;
    ULONG BWT;
    ULONG EGT;
    ULONG clock_stop_polarity;
    UCHAR check;
    UCHAR P;
    UCHAR I;
    UCHAR U;
} SCI_PARAMETERS;

/**************************************/
#define	HW_FIFO_SIZE			16
/* Used in exclusive mode */
#define	RX_FULL_IRQ				0x01
#define	TX_EMPTY_IRQ			0x02

/* Usend with both interrupt (rx and tx) */
#define	RX_FULL_TX_EMPTY_IRQ	0x03

/* When there are more 16 bytes to trasmit */
#define	TX_HALF_EMPTY_IRQ		0x04
/*************************************/

#define MAX_ATR_LEN 33

/**< SCI control block */
typedef struct sci_control_block_st
{
    ULONG           base_address_syscfg7;
    ULONG           base_address_pio0;
    ULONG           base_address_asc;
    ULONG           base_address_sci;
    ULONG           base_address_pio4;
    ULONG 			base_address_fpga;	

    SCI_CARD_STATUS card_detect;
    INT             error;
    ULONG           waiting;
    UCHAR           *p_read;
    UCHAR           *p_write;
    SCI_MODES       sci_modes;
    SCI_PARAMETERS  sci_parameters;
    UCHAR           write_buf[SCI_BUFFER_SIZE];
    UCHAR           read_buf[SCI_BUFFER_SIZE];
    UINT            driver_inuse;
    SCI_ATR_STATUS  atr_status;
    UINT16          rx_rptr;
    UINT16          rx_wptr;
    UINT16          tx_rptr;
    UINT16          tx_wptr;
    UCHAR           atr_len;
    UCHAR           atr[MAX_ATR_LEN];
    UCHAR			byte_invert;
    UCHAR			sci_clock;
    UCHAR			irq_mode;
    UCHAR			sci_atr_class;
} SCI_CONTROL_BLOCK;

/*****************************
 * FUNCTION DECLARATIION
 *****************************/

SCI_ERROR sci_osi_set_parameters(ULONG, SCI_PARAMETERS *);
SCI_ERROR sci_osi_get_parameters(ULONG, SCI_PARAMETERS *);
SCI_ERROR sci_osi_is_card_activated (ULONG);

void _set_reg_writeonly16(ULONG sci_id, BASE_ADDR base_address, ULONG reg, USHORT bits);

#endif  /* _SCI_H */
