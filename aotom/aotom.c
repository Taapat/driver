/*
 * aotom.c
 *
 * (c) 2010 Spider-Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * Edision argus vip2 frontpanel driver (AOTOM A16311)
 *
 * Devices:
 *	- /dev/vfd (vfd ioctls and read/write function)
 *	- /dev/rc  (reading of key events)
 *
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "aotom.h"

static short paramDebug = 0;
#define TAGDEBUG "[Aotom] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
} while (0)

#define VFD_CS_CLR() {udelay(10); stpio_set_pin(cfg.cs, 0);}
#define VFD_CS_SET() {udelay(10); stpio_set_pin(cfg.cs, 1);}

#define VFD_CLK_CLR() {stpio_set_pin(cfg.clk, 0);udelay(1);}
#define VFD_CLK_SET() {stpio_set_pin(cfg.clk, 1);udelay(1);}

#define VFD_DATA_CLR() {stpio_set_pin(cfg.data, 0);}
#define VFD_DATA_SET() {stpio_set_pin(cfg.data, 1);}

#define INVALID_KEY    	-1
#define LOG_OFF     	0
#define LOG_ON      	1

#define NO_KEY_PRESS    -1
#define KEY_PRESS_DOWN 	1
#define KEY_PRESS_UP   	0

#define REC_NEW_KEY 	34
#define REC_NO_KEY  	0
#define REC_REPEAT_KEY  2

typedef struct
{
	struct file*      fp;
	int               read;
	struct semaphore  sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC             1
#define LASTMINOR                 	    2

static tFrontPanelOpen FrontPanelOpen [LASTMINOR];

typedef enum VFDMode_e{
	VFDWRITEMODE,
	VFDREADMODE
}VFDMode_T;

typedef enum SegNum_e{
	SEGNUM1 = 0,
	SEGNUM2
}SegNum_T;

typedef struct SegAddrVal_s{
	unsigned char Segaddr1;
	unsigned char Segaddr2;
	unsigned char CurrValue1;
	unsigned char CurrValue2;
}SegAddrVal_T;

typedef enum PIO_Mode_e
{
    PIO_Out,
    PIO_In
}PIO_Mode_T;

struct VFD_config
{
	struct stpio_pin*	clk;
	struct stpio_pin*	data;
	struct stpio_pin*	cs;
	int data_pin[2];
	int clk_pin[2];
	int cs_pin[2];
};

struct VFD_config cfg;

#define BUFFERSIZE                256     //must be 2 ^ n

/* structure to queue transmit data is necessary because
 * after most transmissions we need to wait for an acknowledge
 */

struct transmit_s
{
	unsigned char 	buffer[BUFFERSIZE];
	int		len;
	int  		needAck; /* should we increase ackCounter? */

   int      ack_len; /* len of complete acknowledge sequence */
   int      ack_len_header; /* len of ack header ->contained in ack_buffer */
	unsigned char 	ack_buffer[BUFFERSIZE]; /* the ack sequence we wait for */

	int		requeueCount;
};

#define cMaxTransQueue	100

/* I make this static, I think if there are more
 * then 100 commands to transmit something went
 * wrong ... point of no return
 */
struct transmit_s transmit[cMaxTransQueue];

static int transmitCount = 0;

struct receive_s
{
   int           len;
   unsigned char buffer[BUFFERSIZE];
};

#define cMaxReceiveQueue	100
static wait_queue_head_t   wq;

struct receive_s receive[cMaxReceiveQueue];

static int receiveCount = 0;

#define cMaxAckAttempts	150
#define cMaxQueueCount	5

/* waiting retry counter, to stop waiting on ack */
static int 		   waitAckCounter = 0;

static int 		   timeoutOccured = 0;
static int 		   dataReady = 0;

struct semaphore 	   write_sem;
struct semaphore 	   rx_int_sem; /* unused until the irq works */
struct semaphore 	   transmit_sem;
struct semaphore 	   receive_sem;
struct semaphore 	   key_mutex;

struct saved_data_s
{
	int   length;
	char  data[BUFFERSIZE];
};

static struct saved_data_s lastdata;

/* last received ioctl command. we dont queue answers
 * from "getter" requests to the fp. they are protected
 * by a semaphore and the threads goes to sleep until
 * the answer has been received or a timeout occurs.
 */
static char ioctl_data[BUFFERSIZE];

int writePosition = 0;
int readPosition = 0;
unsigned char receivedData[BUFFERSIZE];

unsigned char str[64];
static SegAddrVal_T VfdSegAddr[15];

#define VFD_RW_SEM
#ifdef VFD_RW_SEM
struct rw_semaphore vfd_rws;
#endif

unsigned char CharLib[48][2] =
{
	{0xF1, 0x38},	//A
	{0x74, 0x72},	//B
	{0x01, 0x68},	//C
	{0x54, 0x72},	//D
	{0xE1, 0x68},	//E
	{0xE1, 0x28},	//F
	{0x71, 0x68},	//G
	{0xF1, 0x18},	//H
	{0x44, 0x62},	//I
	{0x45, 0x22},	//J
	{0x46, 0x06},	//K
	{0x01, 0x48},	//L
	{0x51, 0x1D},	//M
	{0x53, 0x19},	//N
	{0x11, 0x78},	//O
	{0xE1, 0x38},	//P
	{0x13, 0x78},	//Q
	{0xE3, 0x38},	//R
	{0xF0, 0x68},	//S
	{0x44, 0x22},	//T
	{0x11, 0x58},	//U
	{0x49, 0x0C},	//V
	{0x5B, 0x18},	//W
	{0x4A, 0x05},	//X
	{0x44, 0x05},	//Y
	{0x48, 0x64},	//Z
	/* A--Z  */

	{0x01, 0x68},
	{0x42, 0x01},
	{0x10, 0x70},	//
	{0x43, 0x09},	//
	{0xE0, 0x00},	//
	{0xEE, 0x07},	//
	{0xE4, 0x02},	//
	{0x50, 0x00},	//
	{0xE0, 0x00},	//
	{0x05, 0x00},	//
	{0x48, 0x04},	//

	{0x11, 0x78},	//
	{0x44, 0x02},	//
	{0xE1, 0x70},	//
	{0xF0, 0x70},	//
	{0xF0, 0x18},	//
	{0xF0, 0x68},	//
	{0xF1, 0x68},	//
	{0x10, 0x30},	//
	{0xF1, 0x78},	//
	{0xF0, 0x78},	//
	/* 0--9  */
	{0x00, 0x00}
};

unsigned char NumLib[10][2] =
{
	{0x77, 0x77},	//{01110111, 01110111},
	{0x24, 0x22},	//{00100100, 00010010},
	{0x6B, 0x6D},	//{01101011, 01101101},
	{0x6D, 0x6B},	//{01101101, 01101011},
	{0x3C, 0x3A},	//{00111100, 00111010},
	{0x5D, 0x5B},	//{01011101, 01011011},
	{0x5F, 0x5F},	//{01011111, 01011111},
	{0x64, 0x62},	//{01100100, 01100010},
	{0x7F, 0x7F},	//{01111111, 01111111},
	{0x7D, 0x7B} 	//{01111101, 01111011},
};

static int AOTOMfp_Set_PIO_Mode(PIO_Mode_T Mode_PIO)
{
   int ret = 0;

   if(Mode_PIO == PIO_Out)
   {
	   stpio_configure_pin(cfg.data, STPIO_OUT);
   }
   else if(Mode_PIO == PIO_In)
   {
	   stpio_configure_pin(cfg.data, STPIO_IN);
   }
   return ret;
}

unsigned char AOTOMfp_RD(void)
{
    int i;
    unsigned char val = 0, data = 0;

#ifdef VFD_RW_SEM
     down_read(&vfd_rws);
#endif

    AOTOMfp_Set_PIO_Mode(PIO_In);
    for (i = 0; i < 8; i++)
    {
        val >>= 1;
		VFD_CLK_CLR();
        udelay(1);
        data = stpio_get_pin(cfg.data);
		VFD_CLK_SET();
        if(data)
        {
            val |= 0x80;
        }
		VFD_CLK_SET();
        udelay(1);
    }
    udelay(1);
    AOTOMfp_Set_PIO_Mode(PIO_Out);

#ifdef VFD_RW_SEM
    up_read(&vfd_rws);
#endif

    return val;
}

static int VFD_WR(unsigned char data)
{
	int i;

#ifdef VFD_RW_SEM
    down_write(&vfd_rws);
#endif

    for(i = 0; i < 8; i++)
	{
		VFD_CLK_CLR();
		if(data & 0x01)
		{
			VFD_DATA_SET();
		}
		else
		{
			VFD_DATA_CLR();
		}
		VFD_CLK_SET();
		data >>= 1;
	}

#ifdef VFD_RW_SEM
    up_write(&vfd_rws);
#endif

    return 0;
}

void VFD_Seg_Addr_Init(void)
{
	unsigned char i, addr = 0xC0;//adress flag
	for(i = 0; i < 13; i++)
	{
		VfdSegAddr[i + 1].CurrValue1 = 0;
		VfdSegAddr[i + 1].CurrValue2 = 0;
		VfdSegAddr[i + 1].Segaddr1 = addr;
		VfdSegAddr[i + 1].Segaddr2 = addr + 1;
		addr += 3;
	}
}

static int VFD_Seg_Dig_Seg(unsigned char dignum, SegNum_T segnum, unsigned char val)
{
	unsigned char  addr=0;
    if(segnum < 0 && segnum > 1)
    {
    	dprintk(2, "bad parameter!\n");
        return -1;
    }

    VFD_CS_CLR();
    if(segnum == SEGNUM1)
	{
        addr = VfdSegAddr[dignum].Segaddr1;
        VfdSegAddr[dignum].CurrValue1 = val ;
	}
    else if(segnum == SEGNUM2)
    {
        addr = VfdSegAddr[dignum].Segaddr2;
        VfdSegAddr[dignum].CurrValue2 = val ;
    }
    VFD_WR(addr);
    udelay(10);
    VFD_WR(val);
    VFD_CS_SET();
    return  0;
}

static int VFD_Set_Mode(VFDMode_T mode)
{
    unsigned char data = 0;

    if(mode == VFDWRITEMODE)
    {
        data = 0x44;
        VFD_CS_CLR();
        VFD_WR(data);
        VFD_CS_SET();
    }
    else if(mode == VFDREADMODE)
    {
        data = 0x46;
        VFD_WR(data);
        udelay(5);
    }
    return 0;
}

static int VFD_Show_Content(void)
{
    VFD_CS_CLR();
    VFD_WR(0x8F);
    VFD_CS_SET();
    return 0;
}

static int VFD_Show_Content_Off(void)
{
    VFD_WR(0x87);
    return 0;
}

void VFD_Clear_All(void)
{
 	int i;
 	for(i = 0; i < 13; i++)
	{
        VFD_Seg_Dig_Seg(i + 1,SEGNUM1,0x00);
        VFD_Show_Content();
		VfdSegAddr[i + 1].CurrValue1 = 0x00;
        VFD_Seg_Dig_Seg(i + 1,SEGNUM2,0x00);
        VFD_Show_Content();
		VfdSegAddr[i + 1].CurrValue2 = 0;
    }
}

void VFD_Draw_Char(char c, unsigned char position)
{
	if(position < 1 || position > 8)
	{
		dprintk(2, "char position error! %d\n", position);
		return;
	}
	if(c >= 65 && c <= 95)
		c = c - 65;
	else if(c >= 97 && c <= 122)
		c = c - 97;
	else if(c >= 42 && c <= 57)
		c = c - 11;
	else if(c == 32)
		c = 47;
	else
	{
		dprintk(2, "unknown char!\n");
		return;
	}
	VFD_Seg_Dig_Seg(position, SEGNUM1, CharLib[(unsigned char)c][0]);
	VFD_Seg_Dig_Seg(position, SEGNUM2, CharLib[(unsigned char)c][1]);

    VFD_Show_Content();
}

void VFD_Draw_Num(unsigned char c, unsigned char position)
{
	int dignum;

	if(position < 1 || position > 4)
	{
		dprintk(2, "num position error! %d\n", position);
		return;
	}
	if(c >  9)
	{
		dprintk(2, "unknown num!\n");
		return;
	}

	dignum =10 - position / 3;
	if(position % 2 == 1)
	{
		if(NumLib[c][1] & 0x01)
			VFD_Seg_Dig_Seg(dignum, SEGNUM1, VfdSegAddr[dignum].CurrValue1 | 0x80);
		else
			VFD_Seg_Dig_Seg(dignum, SEGNUM1, VfdSegAddr[dignum].CurrValue1 & 0x7F);
    		VfdSegAddr[dignum].CurrValue2 = VfdSegAddr[dignum].CurrValue2 & 0x40;//sz
    		VFD_Seg_Dig_Seg(dignum, SEGNUM2, (NumLib[c][1] >> 1) | VfdSegAddr[dignum].CurrValue2);
	}
	else if(position % 2 == 0)
	{
	   if((NumLib[c][0] & 0x01))
        {
            VFD_Seg_Dig_Seg(dignum, SEGNUM2, VfdSegAddr[dignum].CurrValue2 | 0x40);// SZ  08-05-30
	   	}
	   else
			VFD_Seg_Dig_Seg(dignum, SEGNUM2, VfdSegAddr[dignum].CurrValue2 & 0x3F);
    		VfdSegAddr[dignum].CurrValue1 = VfdSegAddr[dignum].CurrValue1 & 0x80;
    		VFD_Seg_Dig_Seg(dignum, SEGNUM1, (NumLib[c][0] >>1 ) | VfdSegAddr[dignum].CurrValue1 );
	}
    VFD_Show_Content();
}

static int VFD_Show_String(char* str, int len)
{
	int i;
	char buf[8];

	if(len < 1){
		dprintk(2, "empty string\n");
		return -1;
	}

	dprintk(5, "VFD String : '%s'\n", str);
	memset(buf,' ',8);

	if(len > 8){
		dprintk(2, "VFD String Length value is over! %d\n", len);
		len = 8;
		memcpy(buf, str, 8);
	}else{
		memcpy(buf, str, len);
	}

 	for(i = 0; i < 8; i++)
	{
	 	VFD_Draw_Char(buf[i], i + 1);
	}

    return 0;
}

static int VFD_Show_Ico(LogNum_T log_num, int log_stat)
{
    int dig_num = 0,seg_num = 0;
    SegNum_T seg_part = 0;
    u8  seg_offset = 0;
    u8  addr = 0,val = 0;

    if(log_num >= LogNum_Max)
    {
    	dprintk(2, "%s bad parameter!\n", __func__);
        return -1;
    }
    dig_num = log_num/16;
    seg_num = log_num%16;
    seg_part = seg_num/9;

    VFD_CS_CLR();
    if(seg_part == SEGNUM1)
	{
        seg_offset = 0x01 << ((seg_num%9) - 1);
        addr = VfdSegAddr[dig_num].Segaddr1;
        if(log_stat == LOG_ON)
        {
           VfdSegAddr[dig_num].CurrValue1 |= seg_offset;
        }
        if(log_stat == LOG_OFF)
        {
           VfdSegAddr[dig_num].CurrValue1 &= (0xFF-seg_offset);
        }
        val = VfdSegAddr[dig_num].CurrValue1 ;
	}
    else if(seg_part == SEGNUM2)
    {
        seg_offset = 0x01 << ((seg_num%8) - 1);
        addr = VfdSegAddr[dig_num].Segaddr2;
        if(log_stat == LOG_ON)
        {
           VfdSegAddr[dig_num].CurrValue2 |= seg_offset;
        }
        if(log_stat == LOG_OFF)
        {
           VfdSegAddr[dig_num].CurrValue2 &= (0xFF-seg_offset);
        }
        val = VfdSegAddr[dig_num].CurrValue2 ;
    }
    VFD_WR(addr);
    udelay(10);
    VFD_WR(val);
    VFD_CS_SET();
    VFD_Show_Content();

    return 0 ;
}

static int VFD_Show_Time(int hh, int mm)
{
    if( (hh > 24) && (mm > 60))
    {
    	dprintk(2, "%s bad parameter!\n", __func__);
    }
    VFD_Draw_Num((hh/10), 1);
    VFD_Draw_Num((hh%10), 2);
    VFD_Draw_Num((mm/10), 3);
    VFD_Draw_Num((mm%10), 4);
    return 0;
}

static int VFD_Show_Time_Off(void)
{
	int ret=0;

    ret = VFD_Seg_Dig_Seg(9, SEGNUM1, 0x00);
    ret = VFD_Seg_Dig_Seg(9, SEGNUM2, 0x00);
    ret = VFD_Seg_Dig_Seg(10,SEGNUM1, 0x00);
    ret = VFD_Seg_Dig_Seg(10,SEGNUM2, 0x00);
    return ret;
}

unsigned char AOTOMfp_Scan_Keyboard(unsigned char read_num)
{
    unsigned char key_val[read_num] ;
    unsigned char i = 0, ret;

    VFD_CS_CLR();
    ret = VFD_Set_Mode(VFDREADMODE);
    if(ret)
    {
    	dprintk(2, "%s DEVICE BUSY!\n", __func__);
        return -1;
    }

    for (i = 0; i < read_num; i++)
    {
    	key_val[i] = AOTOMfp_RD();
    }
    VFD_CS_SET();

    ret = VFD_Set_Mode(VFDWRITEMODE);
    if(ret)
    {
    	dprintk(2, "%s DEVICE BUSY!\n", __func__);
        return -1;
    }
    return key_val[5];
}

static int AOTOMfp_Get_Key_Value(void)
{
	unsigned char byte = 0;
	int key_val = INVALID_KEY;

	byte = AOTOMfp_Scan_Keyboard(6);

	switch(byte)
	{
        case 0x02:
        {
            key_val = KEY_LEFT;
            break;
        }
        case 0x04:
        {
            key_val = KEY_UP;
            break;
        }
        case 0x08:
        {
            key_val = KEY_OK;
            break;
        }
        case 0x10:
        {
            key_val = KEY_RIGHT;
            break;
        }
        case 0x20:
        {
            key_val = KEY_DOWN;
            break;
        }
        case 0x40:
        {
            key_val = KEY_POWER;
            break;
        }
        case 0x80:
        {
            key_val = KEY_MENU;
            break;
        }
        default :
        {
            key_val = INVALID_KEY;
            break;
        }
    }

    return key_val;
}

int aotomSetTime(char* time)
{
   char		buffer[8];
   int      res = 0;

	dprintk(5, "%s >\n", __func__);

	dprintk(5, "%s time: %02d:%02d\n", __func__, time[2], time[3]);
	memset(buffer, 0, 8);
	memcpy(buffer, time, 5);
	VFD_Show_Time(time[2], time[3]);
	dprintk(5, "%s <\n", __func__);

   return res;
}

int vfd_init_func(void)
{
	dprintk(5, "%s >\n", __func__);
	printk("Edision argus vip2 VFD module initializing\n");

	cfg.data_pin[0] = 3;
	cfg.data_pin[1] = 2;
	cfg.clk_pin[0] = 3;
	cfg.clk_pin[1] = 4;
	cfg.cs_pin[0] = 3;
	cfg.cs_pin[1] = 5;

	cfg.cs  = stpio_request_pin (cfg.cs_pin[0], cfg.cs_pin[1], "VFD CS", STPIO_OUT);
	cfg.clk = stpio_request_pin (cfg.clk_pin[0], cfg.clk_pin[1], "VFD CLK", STPIO_OUT);
	cfg.data= stpio_request_pin (cfg.data_pin[0], cfg.data_pin[1], "VFD DATA", STPIO_OUT);

	if(!cfg.cs || !cfg.data || !cfg.clk) {
		printk("vfd_init_func:  PIO errror!\n");
		return -1;
	}

#ifdef VFD_RW_SEM
    init_rwsem(&vfd_rws);
#endif

    VFD_CS_CLR();
    VFD_WR(0x0C);
    VFD_CS_SET();

  	VFD_Set_Mode(VFDWRITEMODE);
    VFD_Seg_Addr_Init();
    VFD_Clear_All();
    VFD_Show_Content();

	return 0;
}

static void VFD_CLR()
{
    VFD_CS_CLR();
    VFD_WR(0x0C);
    VFD_CS_SET();

  	VFD_Set_Mode(VFDWRITEMODE);
    VFD_Seg_Addr_Init();
    VFD_Clear_All();
    VFD_Show_Content();
}

int aotomSetIcon(int which, int on)
{
	char buffer[8];
	int  res = 0, m = 0, n = 0;

	dprintk(5, "%s > %d, %d\n", __func__, which, on);
	if (which < 1 || which > 45)
	{
		printk("VFD/AOTOM icon number out of range %d\n", which);
		return -EINVAL;
	}

	which-=1;
	res = VFD_Show_Ico(((which/15)+11)*16+(which%15)+1, on);

	dprintk(10, "%s <\n", __func__);

   return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(aotomSetIcon);

static ssize_t AOTOMdev_write(struct file *filp, const unsigned char *buff, size_t len, loff_t *off)
{
	char* kernel_buf;
	int minor, vLoop, res = 0;

	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	minor = -1;
  	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
  	{
    	if (FrontPanelOpen[vLoop].fp == filp)
    	{
			minor = vLoop;
		}
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -1; //FIXME
	}

	dprintk(1, "minor = %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC)
		return -EOPNOTSUPP;

	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
	   printk("%s return no mem<\n", __func__);
	   return -ENOMEM;
	}
	copy_from_user(kernel_buf, buff, len);

	if(down_interruptible (&write_sem))
      return -ERESTARTSYS;

	if (kernel_buf[len-1] == '\n') {
		kernel_buf[len-1] = 0;
		VFD_Show_String(kernel_buf, len - 1);
	}
	else
		VFD_Show_String(kernel_buf, len);

	kfree(kernel_buf);

	up(&write_sem);

	dprintk(10, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
	   return res;
	else
	   return len;
}

static ssize_t AOTOMdev_read(struct file *filp, unsigned char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;

	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	minor = -1;
  	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
  	{
    		if (FrontPanelOpen[vLoop].fp == filp)
    		{
			    minor = vLoop;
		   }
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -EUSERS;
	}

	dprintk(1, "minor = %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC)
	{

     while (receiveCount == 0)
	  {
	    if (wait_event_interruptible(wq, receiveCount > 0))
		    return -ERESTARTSYS;
	  }

	  /* 0. claim semaphore */
	  down_interruptible(&receive_sem);

	  /* 1. copy data to user */
     copy_to_user(buff, receive[0].buffer, receive[0].len);

	  /* 2. copy all entries to start and decreas receiveCount */
	  receiveCount--;
	  memmove(&receive[0], &receive[1], 99 * sizeof(struct receive_s));

	  /* 3. free semaphore */
	  up(&receive_sem);

     return 8;
	}

	/* copy the current display string to the user */
 	if (down_interruptible(&FrontPanelOpen[minor].sem))
	{
	   printk("%s return erestartsys<\n", __func__);
   	return -ERESTARTSYS;
	}

	if (FrontPanelOpen[minor].read == lastdata.length)
	{
	    FrontPanelOpen[minor].read = 0;

	    up (&FrontPanelOpen[minor].sem);
	    printk("%s return 0<\n", __func__);
	    return 0;
	}

	if (len > lastdata.length)
		len = lastdata.length;

	/* fixme: needs revision because of utf8! */
	if (len > 16)
		len = 16;

	FrontPanelOpen[minor].read = len;
	copy_to_user(buff, lastdata.data, len);

	up (&FrontPanelOpen[minor].sem);

	dprintk(10, "%s < (len %d)\n", __func__, len);
	return len;
}

int AOTOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(5, "%s >\n", __func__);

    minor = MINOR(inode->i_rdev);

	dprintk(1, "open minor %d\n", minor);

  	if (FrontPanelOpen[minor].fp != NULL)
  	{
		printk("EUSER\n");
    		return -EUSERS;
  	}
  	FrontPanelOpen[minor].fp = filp;
  	FrontPanelOpen[minor].read = 0;

	dprintk(5, "%s <\n", __func__);
	return 0;
}

int AOTOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(5, "%s >\n", __func__);

  	minor = MINOR(inode->i_rdev);

	dprintk(1, "close minor %d\n", minor);

  	if (FrontPanelOpen[minor].fp == NULL)
	{
		printk("EUSER\n");
		return -EUSERS;
  	}
	FrontPanelOpen[minor].fp = NULL;
  	FrontPanelOpen[minor].read = 0;

	dprintk(5, "%s <\n", __func__);
	return 0;
}

static int AOTOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
   static int mode = 0;
   struct aotom_ioctl_data * aotom = (struct aotom_ioctl_data *)arg;
   int res = 0;

   dprintk(5, "%s > 0x%.8x\n", __func__, cmd);

   if(down_interruptible (&write_sem))
      return -ERESTARTSYS;

	switch(cmd) {
	case VFDSETMODE:
		mode = aotom->u.mode.compat;
		break;
	case VFDSETLED:
		break;
	case VFDBRIGHTNESS:
		break;
	case VFDICONDISPLAYONOFF:
		{
			res = aotomSetIcon(aotom->u.icon.icon_nr, aotom->u.icon.on);
		}

		mode = 0;
	case VFDSTANDBY:
	   break;
	case VFDSETTIME:
		if (aotom->u.time.time != 0)
		   res = aotomSetTime(aotom->u.time.time);
		break;
	case VFDGETTIME:
		break;
	case VFDGETWAKEUPMODE:
		break;
	case VFDDISPLAYCHARS:
		if (mode == 0)
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
			res = VFD_Show_String(data->data, data->length);
		} else
		{
			//not suppoerted
		}

		mode = 0;

		break;
	case VFDDISPLAYWRITEONOFF:
		if(aotom->u.mode.compat) // 1 = show, 0 = off
			VFD_Show_Content();
		else
			VFD_Show_Content_Off();
		break;
	case VFDDISPLAYCLR:
		VFD_CLR();
		break;
	default:
		printk("VFD/Aotom: unknown IOCTL 0x%x\n", cmd);

		mode = 0;
		break;
	}

	up(&write_sem);

   dprintk(5, "%s <\n", __func__);
   return res;
}

static unsigned int AOTOMdev_poll(struct file *filp, poll_table *wait)
{
  unsigned int mask = 0;

  poll_wait(filp, &wq, wait);

  if(receiveCount > 0)
  {
    mask = POLLIN | POLLRDNORM;
  }

  return mask;
}

static struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = AOTOMdev_ioctl,
	.write = AOTOMdev_write,
	.read  = AOTOMdev_read,
  	.poll  = (void*) AOTOMdev_poll,
	.open  = AOTOMdev_open,
	.release  = AOTOMdev_close
};

/*----- Button driver -------*/

static char *button_driver_name = "Edision vip2 frontpanel buttons";
static struct input_dev *button_dev;
static int button_value = -1;
static int bad_polling = 1;
static struct workqueue_struct *fpwq;

void button_bad_polling(void)
{
	int btn_pressed = 0;

	while(bad_polling == 1)
	{
		msleep(50);
		button_value = AOTOMfp_Get_Key_Value();
		if (button_value != INVALID_KEY) {
		    dprintk(5, "got button: %X\n", button_value);
            VFD_Show_Ico(DOT2,LOG_ON);
            btn_pressed = 1;
			switch(button_value) {
				case KEY_LEFT: {
					input_report_key(button_dev, KEY_LEFT, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_RIGHT: {
					input_report_key(button_dev, KEY_RIGHT, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_UP: {
					input_report_key(button_dev, KEY_UP, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_DOWN: {
					input_report_key(button_dev, KEY_DOWN, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_OK: {
					input_report_key(button_dev, KEY_OK, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_MENU: {
					input_report_key(button_dev, KEY_MENU, 1);
					input_sync(button_dev);
					break;
				}
				case KEY_POWER: {
					input_report_key(button_dev, KEY_POWER, 1);
					input_sync(button_dev);
					break;
				}
				default:
					dprintk(5, "[BTN] unknown button_value?\n");
			}
		}
		else {
			if(btn_pressed) {
				btn_pressed = 0;
				msleep(50);
				VFD_Show_Ico(DOT2,LOG_OFF);
			}
			input_report_key(button_dev, KEY_UP, 	0);
			input_report_key(button_dev, KEY_DOWN, 	0);
			input_report_key(button_dev, KEY_LEFT, 	0);
			input_report_key(button_dev, KEY_RIGHT, 0);
			input_report_key(button_dev, KEY_OK,	0);
			input_report_key(button_dev, KEY_MENU, 	0);
			input_report_key(button_dev, KEY_POWER, 0);
			input_sync(button_dev);
		}
	}
}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
static DECLARE_WORK(button_obj, button_bad_polling);
#else
static DECLARE_WORK(button_obj, button_bad_polling, NULL);
#endif
static int button_input_open(struct input_dev *dev)
{
	fpwq = create_workqueue("button");
	if(queue_work(fpwq, &button_obj))
	{
		dprintk(5, "[BTN] queue_work successful ...\n");
	}
	else
	{
		dprintk(5, "[BTN] queue_work not successful, exiting ...\n");
		return 1;
	}

	return 0;
}

static void button_input_close(struct input_dev *dev)
{
	bad_polling = 0;
	msleep(55);
	bad_polling = 1;

	if (fpwq)
	{
		destroy_workqueue(fpwq);
		dprintk(5, "[BTN] workqueue destroyed\n");
	}
}

int button_dev_init(void)
{
	int error;

	dprintk(5, "[BTN] allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name = button_driver_name;
	button_dev->open = button_input_open;
	button_dev->close= button_input_close;


	set_bit(EV_KEY		, button_dev->evbit );
	set_bit(KEY_UP		, button_dev->keybit);
	set_bit(KEY_DOWN	, button_dev->keybit);
	set_bit(KEY_LEFT	, button_dev->keybit);
	set_bit(KEY_RIGHT	, button_dev->keybit);
	set_bit(KEY_POWER	, button_dev->keybit);
	set_bit(KEY_MENU	, button_dev->keybit);
	set_bit(KEY_OK		, button_dev->keybit);

	error = input_register_device(button_dev);
	if (error) {
		input_free_device(button_dev);
		return error;
	}

	return 0;
}

void button_dev_exit(void)
{
	dprintk(5, "[BTN] unregistering button device\n");
	input_unregister_device(button_dev);
}

static int __init aotom_init_module(void)
{
	int i;

	dprintk(5, "%s >\n", __func__);

	if(vfd_init_func()) {
		printk("unable to init module\n");
		return -1;
	}

	if(button_dev_init() != 0)
		return -1;

	if (register_chrdev(VFD_MAJOR,"VFD",&vfd_fops))
		printk("unable to get major %d for VFD\n",VFD_MAJOR);

	sema_init(&write_sem, 1);
	sema_init(&key_mutex, 1);

	for (i = 0; i < LASTMINOR; i++)
	    sema_init(&FrontPanelOpen[i].sem, 1);

	dprintk(5, "%s <\n", __func__);
	return 0;
}

static void __exit aotom_cleanup_module(void)
{

    if(cfg.data != NULL)
      stpio_free_pin (cfg.data);
    if(cfg.clk != NULL)
      stpio_free_pin (cfg.clk);
    if(cfg.cs != NULL)
      stpio_free_pin (cfg.cs);

	dprintk(5, "[BTN] unloading ...\n");
	button_dev_exit();

	unregister_chrdev(VFD_MAJOR,"VFD");
	printk("VIP2 FrontPanel module unloading\n");
}


module_init(aotom_init_module);
module_exit(aotom_cleanup_module);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION("VFD module for Edision vip2");
MODULE_AUTHOR("Spider-Team");
MODULE_LICENSE("GPL");
