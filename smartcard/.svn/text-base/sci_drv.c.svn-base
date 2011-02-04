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

/*****************************
 * INCLUDES
 *****************************/
 
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/fs.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#endif
#include <asm/system.h>
#include <linux/cdev.h>

#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>

#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/stm/pio.h>
#include <linux/ioport.h>
#include <asm/signal.h>
#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>


#include "sci_types.h"
#include "sci.h"


/*****************************
 * MACROS
 *****************************/

static struct cdev sci_cdev;

static unsigned char byte_invert[] = {
		0xFF, 0x7F, 0xBF, 0x3F, 0xDF, 0x5F, 0x9F, 0x1F,
		0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F, 0x0F,
		0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97, 0x17,
		0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87, 0x07,
		0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B, 0x1B,
		0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B, 0x0B,
		0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93, 0x13,
		0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83, 0x03,
		0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D,
		0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D,
		0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15,
		0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85, 0x05,
		0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99, 0x19,
		0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89, 0x09,
		0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91, 0x11,
		0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81, 0x01,
		0xFE, 0x7E, 0xBE, 0x3E, 0xDE, 0x5E, 0x9E, 0x1E,
		0xEE, 0x6E, 0xAE, 0x2E, 0xCE, 0x4E, 0x8E, 0x0E,
		0xF6, 0x76, 0xB6, 0x36, 0xD6, 0x56, 0x96, 0x16,
		0xE6, 0x66, 0xA6, 0x26, 0xC6, 0x46, 0x86, 0x06,
		0xFA, 0x7A, 0xBA, 0x3A, 0xDA, 0x5A, 0x9A, 0x1A,
		0xEA, 0x6A, 0xAA, 0x2A, 0xCA, 0x4A, 0x8A, 0x0A,
		0xF2, 0x72, 0xB2, 0x32, 0xD2, 0x52, 0x92, 0x12,
		0xE2, 0x62, 0xA2, 0x22, 0xC2, 0x42, 0x82, 0x02,
		0xFC, 0x7C, 0xBC, 0x3C, 0xDC, 0x5C, 0x9C, 0x1C,
		0xEC, 0x6C, 0xAC, 0x2C, 0xCC, 0x4C, 0x8C, 0x0C,
		0xF4, 0x74, 0xB4, 0x34, 0xD4, 0x54, 0x94, 0x14,
		0xE4, 0x64, 0xA4, 0x24, 0xC4, 0x44, 0x84, 0x04,
		0xF8, 0x78, 0xB8, 0x38, 0xD8, 0x58, 0x98, 0x18,
		0xE8, 0x68, 0xA8, 0x28, 0xC8, 0x48, 0x88, 0x08,
		0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10,
		0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00};

/* Prototype */
SCI_ERROR sci_hw_init(ULONG sci_id);
SCI_ERROR sci_hw_init_special(ULONG sci_id);
/****************/


INT debug = 0;

/**< Main struct that contains the sc control block */
SCI_CONTROL_BLOCK   sci_cb[SCI_NUMBER_OF_CONTROLLERS];

/**< Indicates if the driver has been correctly initialized */
ULONG               sci_driver_init;


/**************************************************************************
 *  Registers ops
 **************************************************************************/
 
/**
 * @brief  Read the value of a RO or RW register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */
ULONG _get_reg(ULONG sci_id, BASE_ADDR base_address, ULONG reg)
{
    ULONG reg_address = 0x0;

    switch (base_address)
    {
        case BASE_ADDRESS_SYSCFG7:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_syscfg7 + reg);
            break;
        case BASE_ADDRESS_PIO0:
        case BASE_ADDRESS_PIO1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_pio0 + reg);
            break;
        case BASE_ADDRESS_ASC0:
        case BASE_ADDRESS_ASC1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_asc + reg);
            break;
        case BASE_ADDRESS_SCI0:
        case BASE_ADDRESS_SCI1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_sci + reg);
            break;
        default:
            break;
    }
    return ctrl_inl(reg_address);
}

/**
 * @brief  Write a RW register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @param  mask The mask
 * @return The value of the register
 */
void _set_reg (ULONG sci_id, BASE_ADDR base_address, ULONG reg, UINT bits, UINT mask)
{
    ULONG reg_address = 0x0;
    UINT val;

    switch (base_address)
    {
        case BASE_ADDRESS_SYSCFG7:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_syscfg7 + reg);
            break;
        case BASE_ADDRESS_PIO0:
        case BASE_ADDRESS_PIO1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_pio0 + reg);
            break;
        case BASE_ADDRESS_ASC0:
        case BASE_ADDRESS_ASC1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_asc + reg);
            break;
        case BASE_ADDRESS_SCI0:
        case BASE_ADDRESS_SCI1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_sci + reg);
            break;
        default:
            break;
    }
    val = ctrl_inl(reg_address);
    val &= ~(mask);
    val |= bits;
    ctrl_outl(val, reg_address);
}

/**
 * @brief  Write a WO register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */
void _set_reg_writeonly(ULONG sci_id, BASE_ADDR base_address, ULONG reg, UINT bits)
{
    ULONG reg_address = 0x0;
    
    PDEBUG(" ...\n");
    switch (base_address)
    {
        case BASE_ADDRESS_SYSCFG7:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_syscfg7 + reg);
            break;
        case BASE_ADDRESS_PIO0:
        case BASE_ADDRESS_PIO1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_pio0 + reg);
            break;
        case BASE_ADDRESS_ASC0:
        case BASE_ADDRESS_ASC1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_asc + reg);
            break;
        case BASE_ADDRESS_SCI0:
        case BASE_ADDRESS_SCI1:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_sci + reg);
            break;
        /* Test SC0 voltage to 3v. */
        case BASE_ADDRESS_PIO4:
        case BASE_ADDRESS_PIO3:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_pio4 + reg);
            break;

        default:
            break;
    }
    PDEBUG("reg_address=%lx, bits=%x\n", reg_address, bits);
    ctrl_outl(bits, reg_address);
}

/**
 * @brief  Write a WO 16 bit register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */
void _set_reg_writeonly16(ULONG sci_id, BASE_ADDR base_address, ULONG reg, USHORT bits)
{
    ULONG reg_address = 0x0;
    
    PDEBUG(" ...\n");
    switch (base_address)
    {
        case BASE_ADDRESS_FPGA:
            reg_address = (ULONG)(sci_cb[sci_id].base_address_fpga + reg);
            break;
        default:
            break;
    }
    PDEBUG("reg_address=%lx, bits=%x\n", reg_address, bits);
    ctrl_outw(bits, reg_address);
}

/*******************************/
/* Select the serial interrupt */
/*******************************/
static void set_serial_irq(unsigned char id, unsigned char type)
{
	if(type==RX_FULL_IRQ)
	{
		sci_cb[id].irq_mode=RX_FULL_IRQ;
		_set_reg(id, BASE_ADDRESS_ASC0, ASC0_INT_EN, RX_FULL_IRQ , 0x1FF);
		PDEBUG(" ### Set RX_FULL_IRQ interrupt\n");
	}
	else if(type==TX_EMPTY_IRQ)
	{
		sci_cb[id].irq_mode=TX_EMPTY_IRQ;
		_set_reg(id, BASE_ADDRESS_ASC0, ASC0_INT_EN,TX_EMPTY_IRQ , 0x1FF);
		PDEBUG(" ### Set TX_EMPTY_IRQ interrupt\n");
	}
	else if(type==RX_FULL_TX_EMPTY_IRQ)
	{
		sci_cb[id].irq_mode=RX_FULL_TX_EMPTY_IRQ;
		_set_reg(id, BASE_ADDRESS_ASC0, ASC0_INT_EN, RX_FULL_TX_EMPTY_IRQ , 0x1FF);
		PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ interrupt\n");
	}
	else if(type==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))
	{
		sci_cb[id].irq_mode=(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ);
		_set_reg(id, BASE_ADDRESS_ASC0, ASC0_INT_EN, (TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ) , 0x1FF);
		PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ and TX_HALF_EMPTY_IRQ interrupt\n");
	}
	
	else
		PDEBUG("Type of interrupt is not implemented\n");
}

/**
 * @brief  Reset the smartcard setting the signal high-low-high <br/>
 *         Between the low-high we wait 100 ms and reset the RX FIFO
 * @param  sci_id zero-based number to identify smart card controller
 */
void smartcard_reset(ULONG sci_id, unsigned char wait)
{
    PDEBUG(" ...\n");
    if (sci_id == 0) 
    {
        disable_irq(SCI0_INT_RX_TX);
        /**********/
        mdelay(6);
        sci_cb[0].rx_rptr           = 0;
    	sci_cb[0].rx_wptr           = 0;
    	sci_cb[0].tx_rptr           = 0;
    	sci_cb[0].tx_wptr           = 0;

        /**********/
        /* Reset high */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO0, PIO_SET_P0OUT, 0x10);
		mdelay(7);
        /* Reset low */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO0, PIO_CLR_P0OUT, 0x10);
        /* Wait 100 ms */
        mdelay(100);

        /* Reset RX FIFO */
   		_set_reg_writeonly(sci_id, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
        _set_reg_writeonly(sci_id, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);   
        set_serial_irq((unsigned char)0,RX_FULL_IRQ);
		if(wait)
			msleep(1000);
        enable_irq(SCI0_INT_RX_TX);
    	mdelay(11);
        /* Reset high */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO0, PIO_SET_P0OUT, 0x10);
    }
    else if (sci_id == 1)
    {
        disable_irq(SCI1_INT_RX_TX);
        /**********/
		mdelay(6);
        sci_cb[1].rx_rptr           = 0;
    	sci_cb[1].rx_wptr           = 0;
    	sci_cb[1].tx_rptr           = 0;
    	sci_cb[1].tx_wptr           = 0;
        /**********/
        /* Reset high */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO1, PIO_SET_P1OUT, 0x10);
        mdelay(7);
        /* Reset low */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x10);
        /* Wait 100 ms */
        mdelay(100);
        /* Reset RX FIFO */
    	_set_reg_writeonly(sci_id, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
        _set_reg_writeonly(sci_id, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
    	set_serial_irq((unsigned char)1,RX_FULL_IRQ);
		if(wait)
			msleep(1000);
		enable_irq(SCI1_INT_RX_TX);
		mdelay(11);
        /* Reset high */
        _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO1, PIO_SET_P1OUT, 0x10);
    }
    else
        PERROR("Invalid SC ID controller '%ld'", sci_id);
}

/**
 * @brief  Set the Vcc to 3V or 5V <br/>
 * @param  sci_id zero-based number to identify smart card controller
 * @param  vcc The voltage
 * @return SCI_ERROR_OK if there was no error <br/>
 *         SCI_ERROR_VCC_INVALID if voltage is != 3|5
 *         SCI_ERROR_SCI_INVALID if invalid SC
 */
INT smartcard_voltage_config(ULONG sci_id, UINT vcc)
{
    PDEBUG(" ...\n");
    
    if (sci_id == 0)
    {
        if (vcc == SCI_VCC_3)
        {
            sci_cb[sci_id].sci_atr_class=SCI_CLASS_B;
            _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_CLR_P4OUT, 0x40);
        }
        else if (vcc == SCI_VCC_5)
        {
            sci_cb[sci_id].sci_atr_class=SCI_CLASS_A;
            _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_SET_P4OUT, 0x40);
        }
        else
        {
            PERROR("Invalid Vcc value '%d', set Vcc 5V", vcc);
            sci_cb[sci_id].sci_atr_class=SCI_CLASS_A;
            _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_SET_P4OUT, 0x40);            
            return SCI_ERROR_VCC_INVALID;
        }
    }
    else if (sci_id == 1)
    {
        if (vcc == SCI_VCC_3)
        {
			sci_cb[sci_id].sci_atr_class=SCI_CLASS_B;
            _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_CLR_P3OUT, 0x40);

        }
        else if (vcc == SCI_VCC_5)
        {
			sci_cb[sci_id].sci_atr_class=SCI_CLASS_A;
            _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_SET_P3OUT, 0x40);
        }
        else 
        {
            PERROR("Invalid Vcc value '%d', set Vcc 5V", vcc);
            sci_cb[sci_id].sci_atr_class=SCI_CLASS_A;
            _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_CLR_P3OUT, 0x40);
            return SCI_ERROR_VCC_INVALID;
        }
    }
    else
    {
        PERROR("Invalid SC ID controller '%ld'", sci_id);
        return SCI_ERROR_SCI_INVALID;
    }
    
    return SCI_ERROR_OK;
}

/**
 * @brief  Set the Clock to 3.579MHz or 6.25MHz <br/>
 * @param  sci_id zero-based number to identify smart card controller
 * @param  clock The clock
 * @return SCI_ERROR_OK if there was no error <br/>
 *         SCI_ERROR_VCC_INVALID if clock is != 1|2
 *         SCI_ERROR_SCI_INVALID if invalid SC
 */
static INT smartcard_clock_config(ULONG sci_id, UINT clock)
{
    PDEBUG(" ...\n");
    if (sci_id == 0)
    {
        if (clock == SCI_CLOCK_3579)
            _set_reg_writeonly16(0, BASE_ADDRESS_FPGA, FPGA_SCI0_CLOCK, 0x0701);
        else if (clock == SCI_CLOCK_625)
        {
            _set_reg_writeonly16(0, BASE_ADDRESS_FPGA, FPGA_SCI0_CLOCK, 0x0401);
            debug("\n\nSET CLOCK TO 6 mhz \n");
        }
        else
        {
            PERROR("Invalid Clock value '%d'", clock);
            return SCI_ERROR_VCC_INVALID;
        }
    }
    else if (sci_id == 1)
    {
        if (clock == SCI_CLOCK_3579)
            _set_reg_writeonly16(1, BASE_ADDRESS_FPGA, FPGA_SCI1_CLOCK, 0x0701);
        else if (clock == SCI_CLOCK_625)
        {
            _set_reg_writeonly16(1, BASE_ADDRESS_FPGA, FPGA_SCI1_CLOCK, 0x0401);
            debug("\n\nSET CLOCK TO 6 mhz \n");
        }
        else 
        {
            PERROR("Invalid Clock value '%d'", clock);
            return SCI_ERROR_VCC_INVALID;
        }
    }
    else
    {
        PERROR("Invalid SC ID controller '%ld'", sci_id);
        return SCI_ERROR_SCI_INVALID;
    }
    mdelay(3);
    
    return SCI_ERROR_OK;
}

/**************************************************************************
 *  IRQ handling
 **************************************************************************/
#if 0 
If RX interrupt is actived.... 
 rx_rptr -> pointer when the byte is read from the card
 rx_wptr -> pointer when the byte is written to user

If TX interrupt is actived.... 
 tx_rptr -> pointer when the byte is read from user
 tx_wptr -> pointer when the byte is written to the card
 
#endif 

/**
 * @brief Interrupt (top-half) called when we have something to read
 * @param irq The irq number
 * @param dev_id Ptr to the dev data struct
 */
/************************************/
void red_read(unsigned char id)
{
	ULONG data;
	unsigned char nv=0;

	for(nv=0;nv<16;nv++)
	{
		data = _get_reg(id, BASE_ADDRESS_ASC0, ASC0_RX_BUF);
		if( (sci_cb[id].atr_status!=SCI_ATR_READY) && (sci_cb[id].byte_invert==NO_DEFINED) )
		{
			if((UCHAR)data==0x03)
			{
				sci_cb[id].byte_invert=INVERT;
			}
			else if((UCHAR)data==0x3B)
			{
				sci_cb[id].byte_invert=UNINVERT;
			}
		}
		if(sci_cb[id].byte_invert==INVERT)
			data=byte_invert[(UCHAR)data];
		sci_cb[id].read_buf[sci_cb[id].rx_rptr] = (UCHAR)data;
		sci_cb[id].rx_rptr++;
	}
}
/************************************/
irqreturn_t sci_irq1_rx_tx_handler (int irq, void *dev_id)
{
    ULONG res, data;

    disable_irq(SCI1_INT_RX_TX);
	res = _get_reg(1, BASE_ADDRESS_ASC1, ASC1_STA);

    if( (res & RX_FULL_IRQ) && /* Rx interrupt active */
	    ((sci_cb[1].irq_mode==RX_FULL_IRQ) || (sci_cb[1].irq_mode==RX_FULL_TX_EMPTY_IRQ) || (sci_cb[1].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )
    {
	    do
	    {
	        if (res & 0x1)
	        {
	            /* Skip parity error check. Must not be considered when reading the ATR */
	            /* Frame error not checked*/
	            /* Overrun error */
	            if (res & 0x20) 
	            {
	                PERROR("Data are received when the buffer is full\n");
					red_read(1);
					res=0;
	                enable_irq(SCI1_INT_RX_TX);
	                return IRQ_HANDLED;
	            }
	            else
	            { 
     			    data = _get_reg(1, BASE_ADDRESS_ASC1, ASC1_RX_BUF);
	                if( (sci_cb[1].atr_status!=SCI_ATR_READY) && (sci_cb[1].byte_invert==NO_DEFINED) )
	                {
	                	if((UCHAR)data==0x03)
	                	{
	                		sci_cb[1].byte_invert=INVERT;
	                	}
	                	else if((UCHAR)data==0x3B)
	                	{
	                		sci_cb[1].byte_invert=UNINVERT;
	                    }
	                }
	                if(sci_cb[1].byte_invert==INVERT)
	                	data=byte_invert[(UCHAR)data];
	                sci_cb[1].read_buf[sci_cb[1].rx_rptr] = (UCHAR)data;
	                sci_cb[1].rx_rptr++;
				}
	        }
			//break;
			res = _get_reg(1, BASE_ADDRESS_ASC1, ASC1_STA);
	    } while (res & 0x1);
	    PDEBUG("Read byte in INT: 0x%02x\n", sci_cb[1].read_buf[sci_cb[1].rx_rptr - 1]);
	}
	if( (res & TX_EMPTY_IRQ) &&	/* Tx interrupt active */
		((sci_cb[1].irq_mode==TX_EMPTY_IRQ) || (sci_cb[1].irq_mode==RX_FULL_TX_EMPTY_IRQ) || (sci_cb[1].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )
	{
		if((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i=0, size=0;
			if((sci_cb[1].tx_rptr-sci_cb[1].tx_wptr)>HW_FIFO_SIZE)
				size=HW_FIFO_SIZE;
			else
				size=(sci_cb[1].tx_rptr-sci_cb[1].tx_wptr);

			for(i=0;i<size;i++)
			{
				if(sci_cb[1].byte_invert==INVERT)
		        {
		           	_set_reg_writeonly(1, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci_cb[1].write_buf[sci_cb[1].tx_wptr]]);
				}
				else
				{
					_set_reg_writeonly(1, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci_cb[1].write_buf[sci_cb[1].tx_wptr]);
		        }
		        sci_cb[1].tx_wptr++;
		        sci_cb[1].rx_wptr++;
			}
			if (sci_cb[1].tx_rptr==sci_cb[1].tx_wptr)
				set_serial_irq((unsigned char)1,RX_FULL_IRQ);
		}
	}
	else if( (res & TX_HALF_EMPTY_IRQ) && ((sci_cb[1].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )	/* Tx HALF interrupt active */
	{
		if((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i=0, size=0;
			if((sci_cb[1].tx_rptr-sci_cb[1].tx_wptr)>(HW_FIFO_SIZE/2))
				size=(HW_FIFO_SIZE/2);
			else
				size=(sci_cb[1].tx_rptr-sci_cb[1].tx_wptr);

			for(i=0;i<size;i++)
			{
				if(sci_cb[1].byte_invert==INVERT)
		        {
		           	_set_reg_writeonly(1, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci_cb[1].write_buf[sci_cb[1].tx_wptr]]);
				}
				else
				{
					_set_reg_writeonly(1, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci_cb[1].write_buf[sci_cb[1].tx_wptr]);
		        }
		        sci_cb[1].tx_wptr++;
		        sci_cb[1].rx_wptr++;
			}
			if (sci_cb[1].tx_rptr==sci_cb[1].tx_wptr)
				set_serial_irq((unsigned char)1,RX_FULL_IRQ);
		}
	}

    enable_irq(SCI1_INT_RX_TX);

    return IRQ_HANDLED;
}

/**
 * @brief 
 */
irqreturn_t sci_irq1_handler (int irq, void *dev_id)
{
    disable_irq(SCI1_INT_DETECT);
    
    /* Debounce time for the OFF pin in the TDA8024 */
    mdelay(8);
    
    if (sci_cb[1].card_detect == SCI_CARD_PRESENT) 
    {
        printk("Removing Smartcard 1!\n");
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_CLR_P1COMP, 0x80);
        sci_cb[1].card_detect = SCI_CARD_NOT_PRESENT;
		sci_cb[1].atr_status = SCI_WITHOUT_ATR;

        /* VCC cmd low */
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x20);

        sci_hw_init_special(1);

        /* Clean data */
        _set_reg_writeonly(1, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
        _set_reg_writeonly(1, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
        memset(sci_cb[1].read_buf, 0, SCI_BUFFER_SIZE);
        sci_cb[1].rx_rptr = 0;
        sci_cb[1].rx_wptr = 0;
        memset(sci_cb[1].write_buf, 0, SCI_BUFFER_SIZE);
        sci_cb[1].tx_rptr = 0;
        sci_cb[1].tx_wptr = 0;

        /* Set the interrupt in RX_MOD */
        set_serial_irq((unsigned char)1,RX_FULL_IRQ);

        /*Reset*/
        mdelay(20);
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x10);
        /**************/    
    }
    else 
    {
        printk("Inserting Smartcard 1!\n");
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_SET_P1COMP, 0x80);

		sci_hw_init_special(1);
        sci_cb[1].card_detect = SCI_CARD_PRESENT;
	
		mdelay(57);//wait to set the power supply
        /* VCC cmd high */
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_SET_P1OUT, 0x20);
        
		/* Clean data */
        _set_reg_writeonly(1, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
    	_set_reg_writeonly(1, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
    }
    
    enable_irq(SCI1_INT_DETECT);
    return IRQ_HANDLED;
}
 
/**
 * @brief Interrupt (top-half) called when we have something to read
 * @param irq The irq number
 * @param dev_id Ptr to the dev data struct
 */
irqreturn_t sci_irq0_rx_tx_handler (int irq, void *dev_id)
{
    ULONG res, data;
    
    disable_irq(SCI0_INT_RX_TX);
	res = _get_reg(0, BASE_ADDRESS_ASC0, ASC0_STA);

    if( (res & RX_FULL_IRQ) && /* Rx interrupt active */
	    ((sci_cb[0].irq_mode==RX_FULL_IRQ) || (sci_cb[0].irq_mode==RX_FULL_TX_EMPTY_IRQ) || (sci_cb[0].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )
    {
	    do
	    {
	        if (res & 0x1)
	        {
	            /* Skip parity error check. Must not be considered when reading the ATR */
	            /* Frame error not checked */
	            /* Overrun error */
	            if (res & 0x20)
	            {
					PDEBUG("Data are received when the buffer is full\n");
					red_read(0);
					res=0;
	                enable_irq(SCI0_INT_RX_TX);
	                return IRQ_HANDLED;
	            }
	            else
	            {                        
	                data = _get_reg(0, BASE_ADDRESS_ASC0, ASC0_RX_BUF);
	                if( (sci_cb[0].atr_status!=SCI_ATR_READY) && (sci_cb[0].byte_invert==NO_DEFINED) )
	                {
	                	if((UCHAR)data==0x03)
	                	{
	                		sci_cb[0].byte_invert=INVERT;
	                	}
	                	else if((UCHAR)data==0x3B)
	                	{
	                		sci_cb[0].byte_invert=UNINVERT;
	                	}
    	            }
	                if(sci_cb[0].byte_invert==INVERT)
                		data=byte_invert[(UCHAR)data];
	                sci_cb[0].read_buf[sci_cb[0].rx_rptr] = (UCHAR)data;
	                sci_cb[0].rx_rptr++;
	            }
	        }
			//break;
   	        res = _get_reg(0, BASE_ADDRESS_ASC0, ASC0_STA);
		}while (res & 0x1);
	    PDEBUG("Read byte in INT: 0x%02x\n", sci_cb[0].read_buf[sci_cb[0].rx_rptr - 1]);
	}
	if( (res & TX_EMPTY_IRQ) &&	/* Tx interrupt active */
		((sci_cb[0].irq_mode==TX_EMPTY_IRQ) || (sci_cb[0].irq_mode==RX_FULL_TX_EMPTY_IRQ) || (sci_cb[0].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )
	{
		if((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i=0, size=0;
			if((sci_cb[0].tx_rptr-sci_cb[0].tx_wptr)>HW_FIFO_SIZE)
				size=HW_FIFO_SIZE;
			else
				size=(sci_cb[0].tx_rptr-sci_cb[0].tx_wptr);

			for(i=0;i<size;i++)
			{
				if(sci_cb[0].byte_invert==INVERT)
		        {
		           	_set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci_cb[0].write_buf[sci_cb[0].tx_wptr]]);
				}
				else
				{
					_set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci_cb[0].write_buf[sci_cb[0].tx_wptr]);
		        }
		        sci_cb[0].tx_wptr++;
		        sci_cb[0].rx_wptr++;
			}
			if (sci_cb[0].tx_rptr==sci_cb[0].tx_wptr)
				set_serial_irq((unsigned char)0,RX_FULL_IRQ);
		}
	}
	else if( (res & TX_HALF_EMPTY_IRQ) && ((sci_cb[0].irq_mode==(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ))) )	/* Tx HALF interrupt active */
	{
		if((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i=0, size=0;
			if((sci_cb[0].tx_rptr-sci_cb[0].tx_wptr)>(HW_FIFO_SIZE/2))
				size=(HW_FIFO_SIZE/2);
			else
				size=(sci_cb[0].tx_rptr-sci_cb[0].tx_wptr);

			for(i=0;i<size;i++)
			{
				if(sci_cb[0].byte_invert==INVERT)
		        {
		           	_set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci_cb[0].write_buf[sci_cb[0].tx_wptr]]);
				}
				else
				{
					_set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci_cb[0].write_buf[sci_cb[0].tx_wptr]);
		        }
		        sci_cb[0].tx_wptr++;
		        sci_cb[0].rx_wptr++;
			}
			if (sci_cb[0].tx_rptr==sci_cb[0].tx_wptr)
				set_serial_irq((unsigned char)0,RX_FULL_IRQ);
		}
	}

    enable_irq(SCI0_INT_RX_TX);

    return IRQ_HANDLED;
}

irqreturn_t sci_irq0_handler (int irq, void *dev_id)
{
    disable_irq(SCI0_INT_DETECT);

    /* Debounce time for the OFF pin in the TDA8024 */
    mdelay(8);

    if (sci_cb[0].card_detect == SCI_CARD_PRESENT)
    {
        printk("Removing Smartcard 0!\n");
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P0COMP, 0x80);
        sci_cb[0].card_detect = SCI_CARD_NOT_PRESENT;
		sci_cb[0].atr_status = SCI_WITHOUT_ATR;

        sci_hw_init_special(0);

        /* Clean data */
        _set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
        _set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);
        memset(sci_cb[0].read_buf, 0, SCI_BUFFER_SIZE);
        sci_cb[0].rx_rptr = 0;
        sci_cb[0].rx_wptr = 0;
        memset(sci_cb[0].write_buf, 0, SCI_BUFFER_SIZE);
        sci_cb[0].tx_rptr = 0;
        sci_cb[0].tx_wptr = 0;

        /* Set the interrupt in RX_MOD */
        set_serial_irq((unsigned char)0,RX_FULL_IRQ);

		/*Reset*/
        mdelay(20);
	_set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P0OUT, 0x10);
        /**************/
    }
    else
    {
        printk("Inserting Smartcard 0!\n");
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_SET_P0COMP, 0x80);

        sci_hw_init_special(0);
        sci_cb[0].card_detect = SCI_CARD_PRESENT;

		/* Clean data */
        _set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
    	_set_reg_writeonly(0, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);
    }
    
    enable_irq(SCI0_INT_DETECT);
    return IRQ_HANDLED;
}

/**
 * @brief Install interrupts in both SCs for card detect and read op
 */
INT sci_irq_install(UCHAR sci_id) 
{
    INT res;

    if (sci_id == 0)
    {
        /* Set SC 0 Detect interrupt */
        res = request_irq(SCI0_INT_DETECT, sci_irq0_handler, IRQF_DISABLED, SCI0_INT_DETECT_NAME, NULL);
        if (res < 0)
        {
            PERROR("Could not request irq '%d' for '%s'. Err %d\n", 
                SCI0_INT_DETECT, SCI0_INT_DETECT_NAME, res);
            return -1;
        }
            
        /* Set SC 0 RX buffer interrupt */
        res = request_irq(SCI0_INT_RX_TX, sci_irq0_rx_tx_handler, IRQF_DISABLED, SCI0_INT_RX_TX_NAME, NULL);
        if (res < 0)
        {
            PERROR("Could not request irq '%d' for '%s'\n", 
                SCI0_INT_RX_TX, SCI0_INT_RX_TX_NAME);
            return -1;
        }
    }
    else if (sci_id == 1)
    {
        /* Set SC 1 Detect interrupt */
        res = request_irq(SCI1_INT_DETECT, sci_irq1_handler, IRQF_DISABLED, SCI1_INT_DETECT_NAME, NULL);
        if (res < 0)
        {
            PERROR("Could not request irq '%d' for '%s'\n", 
                SCI1_INT_DETECT, SCI1_INT_DETECT_NAME);
            return -1;
        }
            
        /* Set SC 1 RX buffer interrupt */
        res = request_irq(SCI1_INT_RX_TX, sci_irq1_rx_tx_handler, IRQF_DISABLED, SCI1_INT_RX_TX_NAME, NULL);
        if (res < 0)
        {
            PERROR("Could not request irq '%d' for '%s'\n", 
                SCI1_INT_RX_TX, SCI1_INT_RX_TX_NAME);
            return -1;
        }
    }
    else
    {
        PERROR("Invalid SC ID controller '%d'", sci_id);
        return -1;
    }
    return 0;
}

/**
 * @brief Free interrupts for all SCs
 */
void sci_irq_uninstall(void)
{
    free_irq(SCI0_INT_DETECT, NULL);
    free_irq(SCI0_INT_RX_TX, NULL);
    
    free_irq(SCI1_INT_DETECT, NULL);
    free_irq(SCI1_INT_RX_TX, NULL);
}

/**************************************************************************
 * Init/Uninit funcs
 **************************************************************************/

/**
 * @brief Initialize and set to default values all the registers needed <br/>
 *          for the Smart Card interface controller
 * @param  sci_id zero-based number to identify smart card controller
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if the sci_id is not valid
 */
SCI_ERROR sci_hw_init(ULONG sci_id) {
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG(" ...\n");

    /* System configuration register SYS_CFG7. Set alternative functions. Datasheet page 196 */
//     _set_reg(0, BASE_ADDRESS_SYSCFG7, SYSCONFIG7, 0x1B0, 0x2000);
    /* SC_COND_VCC must be equal to SC_DETECT, not inverted */
    _set_reg(0, BASE_ADDRESS_SYSCFG7, SYSCONFIG7, 0xB0, 0x2100);

    if (sci_id == 0)
    {
		 /*Reset*/
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P0OUT, 0x10);
        /**************/

		/* Change SC0 voltage. Set GPIO 4[6]. Datasheet page 213-214 */
        _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_CLR_P4C0, 0x40);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_SET_P4C1, 0x40);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO4, PIO_CLR_P4C2, 0x40);

#ifdef SC_CLOCK_CONTROL_BY_FPGA
	#if defined CONFIG_SH_QBOXHD_MINI_1_0
        /* Disable pin of internal clock */
		/* Set pin 0.2 and 0.6 in output because they are the reset pin for tuners */
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C0, 0x08, 0xBF);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C1, 0x35, 0x8A);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C2, 0xAB, 0x14);
		/* With this configuration, they should be ok....but feach case reset in output...*/ 
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P4C0, 0x44);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_SET_P4C1, 0x44);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P4C2, 0x44);
	#else
        /* Disable pin of internal clock */
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C0, 0x08, 0xBF);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C1, 0x35, 0x8A);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C2, 0xAB, 0x14);
	#endif

#else
	#if defined CONFIG_SH_QBOXHD_MINI_1_0
        /* Bit order works according to the alternative function. Datasheet page 63 */
        /* PIO bit configuration encoding. Datasheet page 214 */
		/* Set pin 0.2 and 0.6 in output because they are the reset pin for tuners */
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C0, 0x00, 0xBF);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C1, 0x3D, 0x82);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C2, 0xAB, 0x14);
		/* With this configuration, they should be ok....but feach case reset in output...*/ 
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P4C0, 0x44);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_SET_P4C1, 0x44);
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P4C2, 0x44);
	#else
        /* Bit order works according to the alternative function. Datasheet page 63 */
        /* PIO bit configuration encoding. Datasheet page 214 */
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C0, 0x00, 0xBF);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C1, 0x3D, 0x82);
        _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0C2, 0xAB, 0x14);

	#endif
#endif
        
        /* Set SC0_C8: Datasheet qboxhd gpio interface */
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_SET_P0OUT, 0x4);
        
        if (!sci_driver_init)
        {
            /* Set Smartcard Detect interrupt. Datasheet page 215 */
            _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0COMP, 0, 0x80);
            _set_reg(0, BASE_ADDRESS_PIO0, PIO_P0MASK, 0x80, 0);
            
            ///* Set Smartcard RX buffer interrupt. Datasheet page 913 */
            //_set_reg(0, BASE_ADDRESS_ASC0, ASC0_INT_EN, 0x01, 0);
            set_serial_irq((unsigned char)0,RX_FULL_IRQ);
        }
    
        /* Asynchronous Serial Controller 0 registers. Datasheet page 908 */
//         _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x38F, 0x3FFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x2787, 0x3FFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
        /* Smartcard interface registers. Datasheet page 922*/
#if 1


#ifdef SC_CLOCK_CONTROL_BY_FPGA

		/* Disable internal clock, enable externa clock */		
		_set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x0, 0x3);
//        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x01, 0x0);
        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x00, 0x0);


		smartcard_clock_config( 0, SCI_CLOCK_3579 );
		sci_cb[0].sci_clock=SCI_CLOCK_3579;
#else

        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x0, 0x3);
        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x02, 0x0);
        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_VAL, 14, 0x1F);
#endif

#else
        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_CTRL, 0x0, 0x1);
        _set_reg(0, BASE_ADDRESS_SCI0, SCI_CLK_VAL, 14, 0x1F);
#endif

        smartcard_voltage_config(0, SCI_VCC_5);
    } 
    else if (sci_id == 1) {

		 /*Reset*/
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x10);
        /**************/
        
        /* Change SC1 voltage. Set GPIO 3[6]. Datasheet page 213-214 */
        _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_CLR_P3C0, 0x40);
        _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_SET_P3C1, 0x40);
        _set_reg_writeonly(1, BASE_ADDRESS_PIO3, PIO_CLR_P3C2, 0x40);
        
#ifdef SC_CLOCK_CONTROL_BY_FPGA
	#if defined CONFIG_SH_QBOXHD_MINI_1_0
        /* Disable pin of internal clock */
		/* Set pin 1.2 in input (spare pin), now it is in output because it is used by internal clk pin */
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C0, 0x0C, 0xBF);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C1, 0x31, 0x8E);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C2, 0x8F, 0x30);
	#else
        /* Disable pin of internal clock */
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C0, 0x08, 0xBF);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C1, 0x35, 0x8A);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C2, 0x8B, 0x34);
	#endif
#else
		/* Pin 1.2 it is the internal clk pin so,if the driver uses the internal clk this pin must be set to 1.2 */	

        /* Bit order works according to the alternative function. Datasheet page 63 */
        /* PIO bit configuration encoding. Datasheet page 214 */
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C0, 0x00, 0xBF);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C1, 0x3D, 0x82);
        _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1C2, 0x8B, 0x34);
#endif        
           
         /* VCC cmd low */
         _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x20); 

        /* Set SC0_C8: Datasheet qboxhd gpio interface */
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_SET_P1OUT, 0x4);
        
        if (!sci_driver_init)
        {
            /* Set Smartcard Detect interrupt. Datasheet page 215 */
            _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1COMP, 0, 0x80);
            _set_reg(1, BASE_ADDRESS_PIO1, PIO_P1MASK, 0x80, 0);
            
            ///* Set Smartcard RX buffer interrupt. Datasheet page 913 */
            //_set_reg(1, BASE_ADDRESS_ASC1, ASC1_INT_EN, 0x01, 0);
			set_serial_irq((unsigned char)1,RX_FULL_IRQ);
        }
    
        /* Asynchronous Serial Controller 0 registers. Datasheet page 908 */
    //    _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x38F, 0x3FFF);
		_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL, 0x2787, 0x3FFF);
        //_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL, 0x2387, 0x3FFF);
        _set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
        _set_reg(1, BASE_ADDRESS_ASC1, ASC1_GUARDTIME, GT_DEFAULT, 0x1FF);
        
#ifdef SC_CLOCK_CONTROL_BY_FPGA

			/* Disable internal clock, enable externa clock */	
        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_CTRL, 0x0, 0x3);
//        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_CTRL, 0x01, 0x0);
        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_CTRL, 0x00, 0x0);

		smartcard_clock_config( 1, SCI_CLOCK_3579 );
		sci_cb[1].sci_clock=SCI_CLOCK_3579;

#else
        /* Smartcard interface registers. Datasheet page 922*/
        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_CTRL, 0x0, 0x3);
        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_CTRL, 0x02, 0x0);
        _set_reg(1, BASE_ADDRESS_SCI1, SCI_CLK_VAL, 14, 0x1F);

#endif
        
        smartcard_voltage_config(1, SCI_VCC_5);   
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    return rc;
}
/*************************************************************************/
SCI_ERROR sci_hw_init_special(ULONG sci_id) 
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if (sci_id == 0)
    {
		 /*Reset*/
        _set_reg_writeonly(0, BASE_ADDRESS_PIO0, PIO_CLR_P0OUT, 0x10);
        /**************/

        /* Asynchronous Serial Controller 0 registers. Datasheet page 908 */
//         _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x38F, 0x3FFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x2787, 0x3FFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
        _set_reg(0, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
                
        smartcard_clock_config( 0, SCI_CLOCK_3579 );
		sci_cb[0].sci_clock=SCI_CLOCK_3579;
		sci_cb[0].sci_parameters.ETU=372;

        /* Smartcard interface registers. Datasheet page 922*/
        smartcard_voltage_config(0, SCI_VCC_5);
    } 
    else if (sci_id == 1) {

		 /*Reset*/
        _set_reg_writeonly(1, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x10);
        /**************/
           
         /* VCC cmd low */
         _set_reg_writeonly(sci_id, BASE_ADDRESS_PIO1, PIO_CLR_P1OUT, 0x20); 

        /* Asynchronous Serial Controller 0 registers. Datasheet page 908 */
    //    _set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x38F, 0x3FFF);
        _set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL, 0x2787, 0x3FFF);
        _set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
        _set_reg(1, BASE_ADDRESS_ASC1, ASC1_GUARDTIME, GT_DEFAULT, 0x1FF);
        
		smartcard_clock_config( 1, SCI_CLOCK_3579 );
		sci_cb[1].sci_clock=SCI_CLOCK_3579;
        sci_cb[1].sci_parameters.ETU=372;
        
        /* Smartcard interface registers. Datasheet page 922*/
        smartcard_voltage_config(1, SCI_VCC_5);     
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    return rc;
}


void sci_cb_init(ULONG sci_id)
{
    PDEBUG(" ...\n");
    if (sci_id > SCI_NUMBER_OF_CONTROLLERS)
        return;

    if (!sci_driver_init)
        sci_cb[sci_id].card_detect       = SCI_CARD_NOT_PRESENT;
            
    sci_cb[sci_id].atr_status        = SCI_WITHOUT_ATR;
    sci_cb[sci_id].waiting           = 0;
    sci_cb[sci_id].sci_modes.emv2000 = 0;
    sci_cb[sci_id].sci_modes.dma     = 0;
    sci_cb[sci_id].sci_modes.man_act = 0;
    //sci_cb[i].sci_modes.rw_mode = SCI_SYNC | SCI_DATA_ANY;
    sci_cb[sci_id].sci_modes.rw_mode = SCI_SYNC;
    sci_cb[sci_id].error             = SCI_ERROR_OK;
    
    /* Set default ATR parameters defined by standard */
    sci_cb[sci_id].sci_parameters.T      = SCI_ATR_T;
    sci_cb[sci_id].sci_parameters.f      = SCI_ATR_F;
    sci_cb[sci_id].sci_parameters.ETU    = SCI_ATR_ETU;
    sci_cb[sci_id].sci_parameters.WWT    = SCI_ATR_WWT;
    sci_cb[sci_id].sci_parameters.CWT    = SCI_ATR_CWT;
    sci_cb[sci_id].sci_parameters.BWT    = SCI_ATR_BWT;
    sci_cb[sci_id].sci_parameters.EGT    = SCI_ATR_EGT;
    sci_cb[sci_id].sci_parameters.clock_stop_polarity = SCI_CLOCK_STOP_DISABLED;
    sci_cb[sci_id].sci_parameters.check  = SCI_ATR_CHECK;
    sci_cb[sci_id].sci_parameters.P      = SCI_ATR_P;
    sci_cb[sci_id].sci_parameters.I      = SCI_ATR_I;
    sci_cb[sci_id].sci_parameters.U      = SCI_ATR_U;
    
    sci_cb[sci_id].rx_rptr           = 0;
    sci_cb[sci_id].rx_wptr           = 0;
    sci_cb[sci_id].tx_rptr           = 0;
    sci_cb[sci_id].tx_wptr           = 0;

    sci_cb[sci_id].atr_len           = 0;
    
    sci_cb[sci_id].byte_invert		 = NO_DEFINED;
    sci_cb[sci_id].sci_atr_class	 = SCI_UNKNOWN_CLASS;

//	sci_cb[sci_id].sci_clock=SCI_CLOCK_3579;

    PDEBUG(" = OK\n");
}

/**
 * @brief Uninitialize the Smart Card interface controllers and driver <br/>
 *        and enter deac state.
 */
void sci_osd_uninit (void)
{
    /* Remove mappings from SC 0 */
    iounmap((void *)sci_cb[0].base_address_syscfg7);
    iounmap((void *)sci_cb[0].base_address_pio0);
    iounmap((void *)sci_cb[0].base_address_asc);
    iounmap((void *)sci_cb[0].base_address_sci);
	iounmap((void *)sci_cb[0].base_address_fpga);
    
    /* Remove mappings from SC 1 */
    iounmap((void *)sci_cb[1].base_address_syscfg7);
    iounmap((void *)sci_cb[1].base_address_pio0);
    iounmap((void *)sci_cb[1].base_address_asc);
    iounmap((void *)sci_cb[1].base_address_sci);
	iounmap((void *)sci_cb[1].base_address_fpga);

    sci_irq_uninstall();
    
    sci_driver_init = 0;
}

//#define DEBUG
#ifdef DEBUG
void *checked_ioremap(unsigned long offset, unsigned long size)
{
    void *rv = ioremap(offset, size);

    if(rv == NULL)
        PERROR("IOREMAP %lx failed.\n", offset);
    else
        PDEBUG("IOREMAP %lx-%lx ok.\n", offset, offset+size);
    return rv;
}
#else
#define checked_ioremap(x,y) ioremap(x,y)
#endif

/**
 * @brief  Initialize the Smart Card interface controller(s) and driver <br/>
 *         and enter deac state.
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_KERNEL_FAIL: if interrupt handler install fails or <br/>
 *             reader demon task creation fails
 */
SCI_ERROR sci_osd_init(void)
{
    SCI_ERROR rc = SCI_ERROR_OK;
    UCHAR i;

    /* Init and set to defaults the main sci control struct and the hw registers */
    for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
    {
        PDEBUG(" ...\n");
        if (i == 0)
        {
            sci_cb[0].base_address_syscfg7 = (ULONG)checked_ioremap(SYSCONFIG_BASE_ADDRESS, SCI_IO_SIZE);
            /* Map from 0xb8020000 (PIO0_BASE_ADDRESS) to 0xb8025000 (PIO4_BASE_ADDRESS) */
            sci_cb[0].base_address_pio0    = (ULONG)checked_ioremap(PIO0_BASE_ADDRESS, SCI_IO_SIZE * 5);
            sci_cb[0].base_address_asc     = (ULONG)checked_ioremap(ASC0_BASE_ADDRESS, SCI_IO_SIZE);
            sci_cb[0].base_address_sci     = (ULONG)checked_ioremap(SCI0_BASE_ADDRESS, SCI_IO_SIZE);       
            //sci_cb[0].base_address_pio4    = PIO4_BASE_ADDRESS;
            sci_cb[0].base_address_pio4    = (ULONG)checked_ioremap(PIO4_BASE_ADDRESS, SCI_IO_SIZE * 5);
			sci_cb[0].base_address_fpga    = (ULONG)checked_ioremap(FPGA_BASE_ADDRESS, SCI_IO_SIZE);
            
            sci_cb[0].driver_inuse = 0;
        }
        else if (i == 1)
        {
            sci_cb[1].base_address_syscfg7 = (ULONG)checked_ioremap(SYSCONFIG_BASE_ADDRESS, SCI_IO_SIZE);
            /* Map from 0xb8020000 (PIO0_BASE_ADDRESS) to 0xb8024000 (PIO3_BASE_ADDRESS) */
            sci_cb[1].base_address_pio0    = (ULONG)checked_ioremap(PIO1_BASE_ADDRESS, SCI_IO_SIZE * 4);
            sci_cb[1].base_address_asc     = (ULONG)checked_ioremap(ASC1_BASE_ADDRESS, SCI_IO_SIZE);
            sci_cb[1].base_address_sci     = (ULONG)checked_ioremap(SCI1_BASE_ADDRESS, SCI_IO_SIZE);
            //sci_cb[1].base_address_pio4    = PIO3_BASE_ADDRESS;
            sci_cb[1].base_address_pio4    = (ULONG)checked_ioremap(PIO3_BASE_ADDRESS, SCI_IO_SIZE * 5);
			sci_cb[1].base_address_fpga    = (ULONG)checked_ioremap(FPGA_BASE_ADDRESS, SCI_IO_SIZE);
            
            sci_cb[1].driver_inuse = 0;
        }
    
        sci_cb_init(i);
        if (sci_hw_init(i) != SCI_ERROR_OK)
            PERROR("Could not initialize hw of sc[%d]\n", i);
    }
    
    for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
    {
        if (sci_irq_install(i) < 0)
        {
            sci_osd_uninit();
            return SCI_ERROR_DRIVER_NOT_INITIALIZED;
        }
    }
    
    sci_driver_init = 1;    

    return(rc);
}

/**************************************************************************
 * Read/Write ops
 **************************************************************************/

/**
 * @brief  Write data to the SC
 * @param  sci_id zero-based number to identify smart card controller
 *         p_buffer input pointer to write buffer
 *         num_bytes number of bytes to write from p_buffer
 *         mode_flags flags to indicate behavior of write
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or <br/>
 *             p_buffer is zero or num_bytes is zero <br/>
 *         SCI_ERROR_CARD_NOT_PRESENT: if card is not activated <br/>
 *         SCI_ERROR_NO_ATR: if there isn't an ATR
 */
#if 0
/* It isn't used ...*/
SCI_ERROR sci_osd_write (ULONG sci_id,
                         UCHAR *p_buffer,
                         ULONG num_bytes,
                         ULONG mode_flags
)
{

}
#endif

/**
 * @brief  Read data from the Smart Card.
 * @param  sci_id zero-based number to identify smart card controller
 * @param  p_buffer input pointer to read buffer
 * @param  num_bytes number of bytes to read into p_buffer
 * @param  p_bytes_read number of bytes actually read into p_buffer
 * @param  mode_flags flags to indicate behavior of read
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or <br/>
 *             p_buffer is zero or num_bytes is zero or p_bytes_read is zero <br/>
 *         SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
 */

SCI_ERROR sci_osd_read (ULONG sci_id,
                        UCHAR **p_buffer,
                        ULONG num_bytes,
                        ULONG *p_bytes_read,
                        ULONG flags)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	INT real_num_bytes=0;

	PDEBUG("sc[%d] enter: userspace wants to read %ld bytes\n", (UINT)sci_id, num_bytes);
	PDEBUG("rx_rptr: 0x%02x, rx_wptr: 0x%02x\n", sci_cb[sci_id].rx_rptr, sci_cb[sci_id].rx_wptr);
          
	SCI_CHECK_INIT_COND(sci_id, rc);
	if (rc != SCI_ERROR_OK)
	{
		PERROR("sc[%d] error=%d\n", (UINT)sci_id, rc);
	}
	else if ((p_buffer == 0) || (num_bytes == 0) || (p_bytes_read == 0))
	{
		rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		PERROR("sc[%d] error=%d\n", (UINT)sci_id, rc);
	}
	else
	{
		real_num_bytes=sci_cb[sci_id].rx_rptr - sci_cb[sci_id].rx_wptr;
		if (real_num_bytes>num_bytes)
			real_num_bytes=num_bytes;
		*p_bytes_read = real_num_bytes;
		*p_buffer = &(sci_cb[sci_id].read_buf[sci_cb[sci_id].rx_wptr]); 
		sci_cb[sci_id].rx_wptr += real_num_bytes;
	}
	return(rc);
}

/*****************************************************************************
** Function:    sci_osd_deactivate
**
** Purpose:     Initiate a deactivation (enter deac state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osd_deactivate(ULONG sci_id)
{
    //ULONG k_state;
    SCI_ERROR rc = SCI_ERROR_OK;
#if 0
    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_cb[sci_id].state != SCI_STATE_DEAC)
            {
                k_state = os_enter_critical_section();
                /* abort any current transactions                         */
                /* assign abort error code if one is not already assigned */
                if(sci_cb[sci_id].waiting == 1)
                {
                    sci_cb[sci_id].waiting = 0;
                    wake_up_interruptible(sci_wait_q[sci_id]);
                    //os_release_mutex(sci_cb[sci_id].mutex);
                    sci_cb[sci_id].error = SCI_ERROR_TRANSACTION_ABORTED;
                    sci_cb[sci_id].rx_complete = 1;
                }
                sci_atom_deactivate(sci_id);
                sci_cb[sci_id].state = SCI_STATE_DEAC;
                os_leave_critical_section(k_state);
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);
#endif
	PDEBUG("Deactivate is dummy ...\n");
    return(rc);
}


/**
 * @brief  Initiate a reset (enter atr state).
 * @param  sci_id zero-based number to identify smart card controller
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE:  if sci_id is invalid <br/>
 *         SCI_ERROR_CARD_NOT_PRESENT: if no Smart Card is <br/>
 *             present in the reader
 */
SCI_ERROR sci_osd_reset(ULONG sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("sc[%d] enter\n", (UINT) sci_id);

    SCI_CHECK_INIT_COND(sci_id, rc);
    if (rc != SCI_ERROR_OK)
    {
        PDEBUG("sc[%d] error=%d\n", (UINT) sci_id, rc);
        return rc;
    }
    /* FIXME: Add wake_up_interruptible() */
                
    smartcard_reset(sci_id,0);
    
    PDEBUG("sc[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/**
 * @brief  Determine if a card is present in the reader
 * @param  sci_id zero-based number to identify smart card controller
 * @return 0: card is not present  <br/>
 *         1: card is present
 */
SCI_ERROR sci_osi_is_card_present(ULONG sci_id)
{
    UCHAR rc;

    if (sci_driver_init == 1)
        rc = sci_cb[sci_id].card_detect;
    else
    {
        /* sc not present or an error occurred while detecting the sc */
        rc = SCI_CARD_NOT_PRESENT;
    }
    
    return(rc);
}

/**
 * @brief  Compatibility func. It calls sci_osi_is_card_present()
 * @param  sci_id zero-based number to identify smart card controller
 * @return 0: card is not present <br/>
 *         1: card is present
 */
SCI_ERROR sci_osi_is_card_activated (ULONG sci_id)
{ 
    return sci_osi_is_card_present(sci_id);
}


/*****************************************************************************
** Function:    sci_osi_clock_stop
**
** Purpose:     Stop the SCI/Smart Card clock at a given polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
**              SCI_ERROR_CLOCK_STOP_DISABLED: if clock stop is disabled
*****************************************************************************/
SCI_ERROR sci_osi_clock_stop(ULONG sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_osi_is_card_activated(sci_id) == 1)
            {
                /* check for clock stop enabled */
                if(sci_cb[sci_id].sci_parameters.clock_stop_polarity !=
                   SCI_CLOCK_STOP_DISABLED)
                {
                    //sci_atom_clock_stop(sci_id); not use
                }
                else
                {
                    rc = SCI_ERROR_CLOCK_STOP_DISABLED;
                }
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_clock_start
**
** Purpose:     Start the SCI/Smart Card clock.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osi_clock_start(unsigned long sci_id)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if(sci_id < SCI_NUMBER_OF_CONTROLLERS)
        {
            if(sci_osi_is_card_activated(sci_id) == 1)
            {
                /* start the clock */
                //sci_atom_clock_start(sci_id); not use
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_modes
**
** Purpose:     Set the current Smart Card driver modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_modes: input pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_modes(ULONG sci_id, SCI_MODES *p_sci_modes)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_modes != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            if((p_sci_modes->emv2000 == 0) || (p_sci_modes->emv2000 == 1))
            {
/* old code....*/
//                 if((p_sci_modes->emv2000 == 1) && 
//                    (sci_drv_modes.emv_supported == 0))
//                 {
//                     sci_cb[sci_id].sci_modes.emv2000 = 0;
//                     rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
//                 }
//                 else
//                 {
                     sci_cb[sci_id].sci_modes.emv2000 = p_sci_modes->emv2000;
//                 }
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if((p_sci_modes->dma == 0) || (p_sci_modes->dma == 1))
            {
                /*sci_cb[sci_id].sci_modes.dma = p_sci_modes->dma;*/
                /* not yet supported */
                if(p_sci_modes->dma == 1)
                {
                    printk("DMA mode is not supported\n");
                    //rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
                }
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if((p_sci_modes->man_act == 0) || (p_sci_modes->man_act == 1))
            {
                sci_cb[sci_id].sci_modes.man_act = p_sci_modes->man_act;
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }

            if (p_sci_modes->rw_mode < 4)
            {
                sci_cb[sci_id].sci_modes.rw_mode = p_sci_modes->rw_mode;
            }
            else
            {
                rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }
    
    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Function:    sci_osi_get_modes
**
** Purpose:     Retrieve the current Smart Card modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              sci_get_modes: output pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_osi_get_modes(ULONG sci_id, SCI_MODES *p_sci_modes)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("sc[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_modes != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            p_sci_modes->emv2000 = sci_cb[sci_id].sci_modes.emv2000;
            p_sci_modes->dma     = sci_cb[sci_id].sci_modes.dma;
            p_sci_modes->man_act = sci_cb[sci_id].sci_modes.man_act;
            p_sci_modes->rw_mode = sci_cb[sci_id].sci_modes.rw_mode;
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;

        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PERROR("sc[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("sc[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/**************************************************************************
 * Driver File Operations
 **************************************************************************/

/**
 * @brief POSIX device open
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @return =0: success <br/>
 *         <0: if any error occur
 */
static int sci_inf_open(struct inode *inode, struct file *filp)
{
    ULONG sci_id = MINOR(inode->i_rdev);
    int rc = 0;

    PDEBUG("Opening device!\n");
    if (sci_id <= SCI_NUMBER_OF_CONTROLLERS)
    {
        if (sci_cb[sci_id].driver_inuse == 0)
        {
            sci_cb[sci_id].driver_inuse++;
            PDEBUG("Smartcard[%d] char device available: %d\n", 
                (UINT)sci_id, sci_cb[sci_id].driver_inuse);
            
            if (sci_osi_is_card_present(sci_id) == SCI_CARD_NOT_PRESENT)
            {
                PERROR("sc[%d] not present or an error occurred while detecting it\n", 
                    (UINT) sci_id);
            }
        }
        else
        {
            PERROR("sc[%d] is busy\n", (UINT) sci_id);
            rc = -EBUSY;
        }
    }
    else
    {
        PERROR("Couldn't open device for sc[%d]\n", (UINT) sci_id);
        rc = -ENODEV;
    }

    return (rc);
}

/**
 * @brief POSIX device close
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @return =0: success <br/>
 *         <0: if any error occur
 */
static int sci_inf_release(struct inode *inode, struct file *filp)
{
    ULONG sci_id = MINOR(inode->i_rdev);
    int rc = 0;

    PDEBUG("Closing device!\n");
    if (sci_id <= SCI_NUMBER_OF_CONTROLLERS)
    {
        if (sci_cb[sci_id].driver_inuse > 0)
        {
            sci_cb[sci_id].driver_inuse--;
        }
        else
        {
            PERROR("sc[%d] device is not opend: %d\n", 
                (UINT)sci_id, sci_cb[sci_id].driver_inuse);
            rc = -EINVAL;
        }
    }
    else
    {
        PERROR("Couldn't close sc[%d]\n", (UINT) sci_id);
        rc = -ENODEV;
    }

    return rc;
}

/**
 * @brief  POSIX smart card read 
 * @param  file input pointer to the file handle
 * @param  buffer output pointer to the output data buffer
 * @param  length the expect characters number
 * @param  offset offset of the file
 * @return >=0: the actually number of read characters <br/>
 *         <0: if any error occur
 */
static ssize_t sci_inf_read(struct file *file,
                            char *buffer, 
                            size_t length, 
                            loff_t * offset
)
{
    ULONG sci_id;
    ULONG real_num_bytes;
  
    /* sci_id is the Minor Num of this device */
    sci_id = MINOR(file->f_dentry->d_inode->i_rdev);

//	while( (sci_cb[sci_id].rx_wptr == sci_cb[sci_id].rx_rptr) &&
//			(sci_cb[sci_id].card_detect==SCI_CARD_PRESENT) )

	while ( ((sci_cb[sci_id].rx_wptr >= sci_cb[sci_id].rx_rptr) && (sci_cb[sci_id].card_detect==SCI_CARD_PRESENT)) ||
			((sci_cb[sci_id].irq_mode==RX_FULL_TX_EMPTY_IRQ) && (sci_cb[sci_id].card_detect==SCI_CARD_PRESENT)) )
	{
		if ((file->f_flags&O_NONBLOCK)>>0xB)
		{
			return -EWOULDBLOCK;
		}
		mdelay(57);
	}
	
	if(sci_cb[sci_id].card_detect!=SCI_CARD_PRESENT)
		return 0;

	real_num_bytes=sci_cb[sci_id].rx_rptr - sci_cb[sci_id].rx_wptr;

/**********************/
	if( !((file->f_flags&O_NONBLOCK)>>0xB) )
	{
		if(real_num_bytes<length)
		{
			unsigned char cnt_tmp=0;
			do{	
				mdelay(10);
				cnt_tmp++;
				real_num_bytes=sci_cb[sci_id].rx_rptr - sci_cb[sci_id].rx_wptr;
			}while ( (real_num_bytes<length) && (cnt_tmp<100) );	/* Wait a second */
		}
	}
/**********************/

	if (real_num_bytes>length)
		real_num_bytes=length;
	if (length>SCI_BUFFER_SIZE)
		real_num_bytes=SCI_BUFFER_SIZE;

	PDEBUG("Request bytes: %d , Available bytes: %ld\n",length,real_num_bytes);
/*
	printk("-------->Request bytes in read: %d\n",length);
	int p=0;
	printk("DATA TO READ...\n");
	for(p=0;p<real_num_bytes;p++)
		printk("0x%02X ",sci_cb[sci_id].read_buf[sci_cb[sci_id].rx_wptr+p]);
	printk("\n");
*/

	copy_to_user((void *)buffer, (const void *)&(sci_cb[sci_id].read_buf[sci_cb[sci_id].rx_wptr]), real_num_bytes);
	sci_cb[sci_id].rx_wptr += real_num_bytes;
	return (ssize_t) real_num_bytes;
}

/**
 * @brief  POSIX smart card write
 * @param  file:   input pointer to the file handle
 * @param  buffer: input pointer to the input data buffer
 * @param  length: the characters number of input buffer
 * @param  offset: offset of the file
 * @return >=0: the actually number of writen characters <br/>
 *         <0: if any error occur
 */
static ssize_t sci_inf_write(struct file *file, 
                             const char *buffer, 
                             size_t length, 
                             loff_t * offset
)
{
    ULONG sci_id;
    INT count=0;
   
    /* sci_id is the Minor Num of this device */
    sci_id = MINOR(file->f_dentry->d_inode->i_rdev);

	while( (sci_cb[sci_id].tx_wptr != sci_cb[sci_id].tx_rptr) && /* A writen in progress */
			(sci_cb[sci_id].card_detect==SCI_CARD_PRESENT) )
	{
		if ((file->f_flags&O_NONBLOCK)>>0xB)
		{
			return -EWOULDBLOCK;
		}
		mdelay(57);
	}
	
	if(sci_cb[sci_id].card_detect!=SCI_CARD_PRESENT)
		return 0;
	
	if(sci_id==0)
		disable_irq(SCI0_INT_RX_TX);
	else if(sci_id==1)
		disable_irq(SCI1_INT_RX_TX);
	
	if (length <= SCI_BUFFER_SIZE)
		count=length;
	else
		count=SCI_BUFFER_SIZE;

	memset(&(sci_cb[sci_id].write_buf),0,SCI_BUFFER_SIZE);
	copy_from_user((void *)&(sci_cb[sci_id].write_buf), (const void *)buffer, count);

	_set_reg_writeonly(sci_id, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
    _set_reg_writeonly(sci_id, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);	


	PDEBUG("Bytes to write: %d\n",count);
	
/* 
	int p=0;
	printk("DATA TO SEND...%d bytes\n", count);
	for(p=0;p<count;p++)
		printk("0x%02X ",(unsigned char)buffer[p]);
	printk("\n");
 */

	sci_cb[sci_id].tx_rptr=count;
	sci_cb[sci_id].tx_wptr=0;
	sci_cb[sci_id].rx_rptr=0;
	sci_cb[sci_id].rx_wptr=0;
	//	set_serial_irq(sci_id,TX_EMPTY_IRQ);
	if(count<=HW_FIFO_SIZE)
		set_serial_irq((unsigned char)sci_id,RX_FULL_TX_EMPTY_IRQ);
	else
		set_serial_irq((unsigned char)sci_id,(TX_HALF_EMPTY_IRQ|RX_FULL_TX_EMPTY_IRQ));
	
	if(sci_id==0)
		enable_irq(SCI0_INT_RX_TX);
	else if(sci_id==1)
		enable_irq(SCI1_INT_RX_TX);

	return count;
}
/*****************************************************/
/* WARNING: it is used to prevent hardware failures */
void check_atr(unsigned char id)
{
	if( (sci_cb[id].read_buf[sci_cb[id].rx_wptr]==0x3F) &&
		(sci_cb[id].read_buf[sci_cb[id].rx_wptr+1]==0xFD) &&
		(sci_cb[id].read_buf[sci_cb[id].rx_wptr+2]==0x13) )
	{
		smartcard_voltage_config(id, SCI_VCC_3);
	}
	else
	{
		smartcard_voltage_config(id, SCI_VCC_5);
	}
}
/*****************************************************/

int detect_ATR( unsigned char id )
{
	int rc=0;

	sci_cb_init(id);
	
	smartcard_clock_config(id, SCI_CLOCK_3579 );

	_set_reg(id, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
	_set_reg(id, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
        _set_reg(id, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
	
	if ( sci_osd_reset(id) == SCI_ERROR_OK )
		rc = 0;
	else
		rc = -1;
	mdelay(1100);

	/* Change the clock */
	if( (sci_cb[id].read_buf[sci_cb[id].rx_wptr]!=0x3B) &&
		(sci_cb[id].read_buf[sci_cb[id].rx_wptr]!=0x3F) )
	{
		memset(sci_cb[id].read_buf, 0, SCI_BUFFER_SIZE);				
		smartcard_clock_config( id, SCI_CLOCK_625 );
                
		if(id==0)
		{
			_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x271, 0xFFFF);
			sci_cb[0].sci_parameters.ETU = 372;
			sci_cb[0].sci_clock=SCI_CLOCK_625;
		}
		else if(id==1)
		{
			_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x271, 0xFFFF);
			sci_cb[1].sci_parameters.ETU = 372;
			sci_cb[1].sci_clock=SCI_CLOCK_625;
		}

		if (sci_osd_reset(id) == SCI_ERROR_OK)
			rc = 0;
		else
			rc = -1;
		mdelay(750);
	}
	return (rc);
}
/*****************************************************/

/**
 * @brief  POSIX ioctl
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @param  ioctl_num:   special operation number of this device
 * @param  ioctl_param: input/output parameters of ioctl
 * @return =0: success <br/>
 *         <0: if any error occur
 */


int sci_inf_ioctl(struct inode *inode, 
                  struct file *file, 
                  unsigned int ioctl_num,    /* The number of ioctl */
                  unsigned long ioctl_param  /* The parameter to it */
)
{
    ULONG sci_id;
    INT rc = -1;
    SCI_MODES sci_mode;
    SCI_PARAMETERS sci_param;
    UINT sci_rc;

    sci_id = MINOR(inode->i_rdev);

    switch (ioctl_num)
    {
        case IOCTL_SET_ONLY_RESET:
                if(sci_cb[sci_id].card_detect != SCI_CARD_PRESENT)
                        return -1;

                smartcard_reset(sci_id, 1);
                rc = 0;
                break;

        case IOCTL_SET_RESET://1
            PDEBUG("ioctl IOCTL_SET_RESET: sci_id[%d]\n", (INT)sci_id);
	
			if(sci_cb[sci_id].card_detect != SCI_CARD_PRESENT)
				return -1;

			rc=detect_ATR(sci_id);
			
			/* Check the ATR and change the voltage for hw security */
			check_atr(sci_id);
			/* Set the parity */
			if (sci_cb[sci_id].byte_invert==INVERT)
			{
				if(sci_id==0)
				{
					_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, PARITY_ODD, 0);
				}
				else if(sci_id==1)
				{
					_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL, PARITY_ODD, 0);
				}
			}
			else if (sci_cb[sci_id].byte_invert==UNINVERT)
			{
				if(sci_id==0)
				{
					_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL, 0, PARITY_ODD);
				}
				else if(sci_id==1)
				{
					_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL, 0, PARITY_ODD);
				}
			}
           break;
           
        case IOCTL_SET_MODES://2
            PDEBUG("ioctl IOCTL_SET_MODES (dummy): sci_id[%d]\n", (INT)sci_id);
            copy_from_user((void *) &sci_param, 
                           (const void *) ioctl_param,
                           sizeof(SCI_MODES));
            if (sci_osi_set_modes(sci_id, &sci_mode) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
			rc=0;
            break;

        case IOCTL_GET_MODES://3
	        PDEBUG("ioctl IOCTL_GET_MODES (dummy): sci_id[%d]\n", (INT)sci_id);
            if (sci_osi_get_modes(sci_id, &sci_mode) == SCI_ERROR_OK)
            {
                copy_to_user((void *) ioctl_param, 
                             (const void *) &sci_mode,
                             sizeof(SCI_MODES));
                rc = 0;
            }
            else
            {
                rc = -1;
            }
			rc=0;
            break;
            
        case IOCTL_SET_PARAMETERS://4
            PDEBUG("ioctl IOCTL_SET_PARAMETERS: sci_id[%d]\n", (INT)sci_id);
            copy_from_user((void *)&sci_param, (const void *)ioctl_param, sizeof(SCI_PARAMETERS));

            if (sci_osi_set_parameters(sci_id, &sci_param) == SCI_ERROR_OK)
                rc = 0;
            else
                rc = -1;
            break;

 /**/       case IOCTL_GET_PARAMETERS://5
	        PDEBUG("ioctl IOCTL_GET_PARAMETERS: sci_id[%d]\n", (INT)sci_id);
            if (sci_osi_get_parameters(sci_id, &sci_param) == SCI_ERROR_OK)
            {
                copy_to_user((void *) ioctl_param, 
                             (const void *) &sci_param,
                             sizeof(SCI_PARAMETERS));
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

		case IOCTL_SET_CLOCK://13
			{
				ULONG clock;
	            PDEBUG("ioctl IOCTL_SET_CLOCK: sci_id[%d]\n", (INT)sci_id);
	            copy_from_user((void *)&clock, (const void *)ioctl_param, sizeof(clock));
	            if (smartcard_clock_config(sci_id, clock)== SCI_ERROR_OK)
	            {
	                rc = 0;
	            }
	            else
	            {
    	            rc = -1;
	            }
			}
			break;

  /**/      case IOCTL_SET_CLOCK_START://6
            PDEBUG("ioctl IOCTL_SET_CLOCK_START (dummy): sci_id[%d]\n", (INT)sci_id);
            if (sci_osi_clock_start(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

   /**/     case IOCTL_SET_CLOCK_STOP://7
        	PDEBUG("ioctl IOCTL_SET_CLOCK_STOP (dummy): sci_id[%d]\n", (INT)sci_id);
            if (sci_osi_clock_stop(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_GET_IS_CARD_PRESENT://8
            //PDEBUG("ioctl IOCTL_GET_IS_CARD_PRESENT: sci_id[%d]\n", (INT)sci_id);
            sci_rc = sci_osi_is_card_present(sci_id);
            copy_to_user((void *)ioctl_param, (const void *)&sci_rc, sizeof(UINT));
            rc = 0;
            break;

        case IOCTL_GET_IS_CARD_ACTIVATED://9
			//PDEBUG("ioctl IOCTL_GET_IS_CARD_ACTIVATED (dummy): sci_id[%d]\n", (INT)sci_id);
			sci_rc = sci_osi_is_card_present(sci_id);
            copy_to_user((void *)ioctl_param, (const void *)&sci_rc, sizeof(UINT));
            rc = 0;
            break;

		case IOCTL_SET_DEACTIVATE:	//10
			PDEBUG("ioctl IOCTL_SET_DEACTIVATE (dummy): sci_id[%d]\n",(INT)sci_id);
            if (sci_osd_deactivate(sci_id) == SCI_ERROR_OK)
            {
                rc = 0;
            }
            else
            {
                rc = -1;
            }
            break;

        case IOCTL_SET_ATR_READY://11
            PDEBUG("ioctl IOCTL_SET_ATR_READY: sci_id[%d]\n", (INT)sci_id);
            sci_cb[sci_id].atr_status = SCI_ATR_READY;
           
            rc = 0;
            break;

        case IOCTL_GET_ATR_STATUS://12
            PDEBUG("ioctl IOCTL_GET_ATR_STATUS: sci_id[%d]\n", (INT)sci_id);
            copy_to_user((void *)ioctl_param, 
                (const void *)&(sci_cb[sci_id].atr_status),
                sizeof(SCI_ATR_STATUS));
            rc = 0;
            break;

        default:
            PDEBUG("error ioctl_num %d\n", ioctl_num);
            rc = -1;
    }

    if (rc != 0)
    {
        PERROR("ioctl failed\n");
    }

    return rc;
}


static unsigned int sci_inf_poll(struct file *file, poll_table *wait)
{
    ULONG sci_id;
    sci_id = MINOR(file->f_dentry->d_inode->i_rdev);

	PDEBUG("POLL is called\n");
     
    return (sci_cb[sci_id].rx_rptr-sci_cb[sci_id].rx_wptr);
}

#ifdef CONFIG_PROC_FS
#define MAXBUF 256
/**
 * @brief  procfs file handler
 * @param  buffer:
 * @param  start:
 * @param  offset:
 * @param  size:
 * @param  eof:
 * @param  data:
 * @return =0: success <br/>
 *         <0: if any error occur
 */
int sci_read_proc(char *buffer,
                  char **start,
                  off_t offset,
                  int size,
                  int *eof,
                  void *data)
{
    char outbuf[MAXBUF] = "[ sci driver, version 3.0 ]\n";
    int blen, ix;

    for(ix = 0; ix < SCI_NUMBER_OF_CONTROLLERS; ix++)
    {
        blen = strlen(outbuf);
        if(sci_osi_is_card_present(ix) == SCI_CARD_NOT_PRESENT)
            sprintf(outbuf+blen, "sc%d: no card\n", ix);
        else
            sprintf(outbuf+blen, "sc%d: card detected\n", ix);
    }

    blen = strlen(outbuf);

    if(size < blen)
        return -EINVAL;

    /*
     * If file position is non-zero, then assume the string has
     * been read and indicate there is no more data to be read.
     */
    if (offset != 0)
        return 0;

    /*
     * We know the buffer is big enough to hold the string.
     */
    strcpy(buffer, outbuf);

    /*
     * Signal EOF.
     */
    *eof = 1;

    return blen;

}
#endif



static struct file_operations Fops = 
{
    .owner   = THIS_MODULE,
    .open    = sci_inf_open,
    .release = sci_inf_release,
    .read    = sci_inf_read,
    .write   = sci_inf_write,
    .ioctl   = sci_inf_ioctl,
    .poll    = sci_inf_poll,
    .llseek  = NULL
};

/**************************************************************************
 * Module init/exit
 **************************************************************************/
 
/**
 * @brief Init module 
 */
static int __init sci_module_init(void)
{
    dev_t dev = MKDEV(MAJOR_NUM, MINOR_START);

    sci_driver_init = 0;

    if (sci_osd_init() == SCI_ERROR_OK)
    {
		if(register_chrdev_region(dev, SCI_NUMBER_OF_CONTROLLERS, DEVICE_NAME) < 0)
        {
            PERROR("Couldn't register '%s' driver region\n", DEVICE_NAME);
            return -1;
        }
        cdev_init(&sci_cdev, &Fops);
        sci_cdev.owner = THIS_MODULE;
        sci_cdev.ops = &Fops;
        if (cdev_add(&sci_cdev, dev, SCI_NUMBER_OF_CONTROLLERS) < 0)
        {
            PERROR("Couldn't register '%s' driver\n", DEVICE_NAME);
            return -1;
        }
        PINFO("Registering device '%s', major '%d'\n", DEVICE_NAME, MAJOR_NUM);
    }
    else
    {
        PERROR("Couldn't register '%s' driver\n", DEVICE_NAME);
        return -1;
    }

#ifdef CONFIG_PROC_FS
    if(create_proc_read_entry(SCI_PROC_FILENAME, 0, NULL, sci_read_proc, NULL) == 0)
    {
        PERROR("Unable to register '%s' proc file\n", SCI_PROC_FILENAME);
        return -ENOMEM;
    }
#endif
	printk("Version of smartcard driver: [%s]\n", MYVERSION);
    return 0;
}

/**
 * @brief Cleanup module 
 */
 static void __exit sci_module_cleanup(void)
{
    dev_t dev = MKDEV(MAJOR_NUM, MINOR_START);

#ifdef CONFIG_PROC_FS
    remove_proc_entry(SCI_PROC_FILENAME, NULL);
#endif

    sci_osd_uninit();

    PINFO("Unregistering device '%s', major '%d'\n", DEVICE_NAME, MAJOR_NUM);

    unregister_chrdev_region(dev, SCI_NUMBER_OF_CONTROLLERS);
    
    cdev_del(&sci_cdev);
	printk("Driver smartcard v.[%s] removed. \n",MYVERSION );
}

module_init(sci_module_init);
module_exit(sci_module_cleanup);

#ifdef CONFIG_SH_QBOXHD_1_0
#define MOD               "-HD"
#elif  CONFIG_SH_QBOXHD_MINI_1_0
#define MOD               "-Mini"
#else
#define MOD               ""
#endif

#define SMARTCARD_VERSION       "0.0.9"MOD

MODULE_VERSION(SMARTCARD_VERSION);

module_param(debug, int, S_IRUGO);
MODULE_PARM_DESC(debug, "Turn on/off SmartCard debugging (default:off)");

MODULE_AUTHOR("dvr-1, dvr-2");
MODULE_DESCRIPTION("Driver for the Smart Card Interface");
MODULE_LICENSE("GPL");

