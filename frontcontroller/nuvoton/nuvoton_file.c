/*
 * nuvoton_file.c
 *
 * (c) 2009 Dagobert@teamducktales
 * (c) 2010 Schischu & konfetti: Add irq handling
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

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

#include "nuvoton.h"
#include "nuvoton_asc.h"
#include "nuvoton_utf.h"

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
int nuvotonWriteString(unsigned char* aBuf, int len);

struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[8];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
    int   length;
    char  data[128];
};

#ifdef OCTAGON1008
//Trick: text + icon
struct vfd_buffer vfdbuf[8];

/* Dagobert: On octagon the "normal" fp setting does not work.
 * it seems that with this command we set the segment elements
 * direct (not sure here). I dont know how this work but ...
 */
struct chars {
u8 s1;
u8 s2;
u8 c;
} octagon_chars[] =
{
   {0x07, 0xf1, 'A'},
   {0x52, 0xd5, 'B'},
   {0x44, 0x21, 'C'},
   {0x52, 0x95, 'D'},
   {0x45, 0xe1, 'E'},
   {0x05, 0xe1, 'F'},
   {0x46, 0xe1, 'G'},
   {0x07, 0xf0, 'H'},
   {0x10, 0x84, 'I'},
   {0x46, 0x10, 'J'},
   {0x0d, 0xA2, 'K'},
   {0x44, 0x20, 'L'},
   {0x06, 0xba, 'M'},
   {0x0e, 0xb8, 'N'},
   {0x46, 0x31, 'O'},
   {0x05, 0xf1, 'P'},
   {0x4e, 0x31, 'Q'},
   {0x0d, 0xf1, 'R'},
   {0x43, 0xe1, 'S'},
   {0x10, 0x85, 'T'},
   {0x46, 0x30, 'U'},
   {0x24, 0xa2, 'V'},
   {0x2e, 0xb0, 'W'},
   {0x28, 0x8a, 'X'},
   {0x10, 0x8a, 'Y'},
   {0x60, 0x83, 'Z'},
   {0x02, 0x10, '1'},
   {0x45, 0xd1, '2'},
   {0x43, 0xd1, '3'},
   {0x03, 0xf0, '4'},
   {0x43, 0xe1, '5'},
   {0x47, 0xe1, '6'},
   {0x02, 0x11, '7'},
   {0x47, 0xf1, '8'},
   {0x03, 0xf1, '9'},
   {0x46, 0x31, '0'},
   {0x38, 0x8e, '!'},
   {0x20, 0x82, '/'},
   {0x20, 0x00, '.'},
   {0x20, 0x00, ','},
   {0x11, 0xc4, '+'},
   {0x01, 0xc0, '-'},
   {0x40, 0x00, '_'},
   {0x08, 0x82, '<'},
   {0x20, 0x88, '<'},
   {0x00, 0x00, ' '}
};

#endif

#ifdef HS7810A
typedef struct
{
	char ch;
	long font;
} _VFD_FONT_;


static _VFD_FONT_ _7seg_fonts[] =
{
	{'0', 0x3f},
	{'1', 0x06},
	{'2', 0x5b},
	{'3', 0x4f},
	{'4', 0x66},
	{'5', 0x6d},
	{'6', 0x7c},
	{'7', 0x27},
	{'8', 0x7f},
	{'9', 0x67},
	{'-', 0x40},
	{'A', 0x77},
	{'a', 0x77},
	{'B', 0x7c},
	{'b', 0x7c},
	{'C', 0x39},
	{'c', 0x39},
	{'D', 0x5e},
	{'d', 0x5e},
	{'E', 0x79},
	{'e', 0x79},
	{'F', 0x71},
	{'f', 0x71},
	{'G', 0x7d},
	{'g', 0x7d},
	{'H', 0x76},
	{'h', 0x76},
	{'I', 0x06},
	{'i', 0x06},
	{'J', 0x0e},
	{'j', 0x0e},
	{'K', 0x7a},
	{'k', 0x7a},
	{'M', 0x55},
	{'m', 0x55},
	{'L', 0x38},
	{'l', 0x38},
	{'N', 0x37},
	{'n', 0x37},
	{'O', 0x3f},
	{'o', 0x3f},
	{'P', 0x73},
	{'p', 0x73},
	{'Q', 0x67},
	{'q', 0x67},
	{'R', 0x55},
	{'r', 0x50},
	{'s', 0x6d},
	{'S', 0x6d},
	{'T', 0x78},
	{'U', 0x3e},
	{'u', 0x3e},
	{'y', 0x66},
	{'z', 0x5b},
	{' ', 0x00},
//	{' ', 0x01},
};
#endif

enum {
	ICON_MIN,
	ICON_STANDBY,
	ICON_SAT,
	ICON_REC,
	ICON_TIMESHIFT,
	ICON_TIMER,
	ICON_HD,
	ICON_USB,
	ICON_SCRAMBLED,
	ICON_DOLBY,
	ICON_MUTE,
	ICON_TUNER1,
	ICON_TUNER2,
	ICON_MP3,
	ICON_REPEAT,
	ICON_Play,
	ICON_TER,
	ICON_FILE,
	ICON_480i,
	ICON_480p,
	ICON_576i,
	ICON_576p,
	ICON_720p,
	ICON_1080i,
	ICON_1080p,
	ICON_Play_1,
	ICON_MAX
};

struct iconToInternal {
	char *name;
	u16 icon;
	u8 internalCode1;
	u8 SymbolData1;
	u8 internalCode2;
	u8 SymbolData2;
} nuvotonIcons[] ={
/*--------------------- SetIcon ------- code1 data1 code2 data2 -----*/
	{ "ICON_STANDBY"  , ICON_STANDBY   , 0x20, 0x08, 0xff, 0x00}, // ok
	{ "ICON_SAT"      , ICON_SAT       , 0x20, 0x04, 0xff, 0x00}, // ok
	{ "ICON_REC"      , ICON_REC       , 0x30, 0x03, 0xff, 0x00}, // ok
	{ "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x31, 0x03, 0x32, 0x03}, // ok
	{ "ICON_TIMER"    , ICON_TIMER     , 0x33, 0x02, 0xff, 0x00}, // ok
	{ "ICON_HD"       , ICON_HD        , 0x34, 0x02, 0xff, 0x00}, // ok
	{ "ICON_USB"      , ICON_USB       , 0x35, 0x02, 0xff, 0x00}, // ok
	{ "ICON_SCRAMBLED", ICON_SCRAMBLED , 0x36, 0x02, 0xff, 0x00}, // ok
	{ "ICON_DOLBY"    , ICON_DOLBY     , 0x37, 0x02, 0xff, 0x00}, // ok
	{ "ICON_MUTE"     , ICON_MUTE      , 0x38, 0x02, 0xff, 0x00}, // ok
	{ "ICON_TUNER1"   , ICON_TUNER1    , 0x39, 0x03, 0xff, 0x00}, // ok
	{ "ICON_TUNER2"   , ICON_TUNER2    , 0x3a, 0x03, 0xff, 0x00}, // ok
	{ "ICON_MP3"      , ICON_MP3       , 0x3b, 0x03, 0xff, 0x00}, // ok
	{ "ICON_REPEAT"   , ICON_REPEAT    , 0x3c, 0x03, 0xff, 0x00}, // ok
	{ "ICON_Play"     , ICON_Play      , 0x20, 0x02, 0xff, 0x00}, // ok
	{ "ICON_Play_1"   , ICON_Play_1    , 0x23, 0x04, 0xff, 0x00}, // ok
/*------------------------ SetIcon noch offen --------------------------*/
	{ "ICON_TER"      , ICON_TER       , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_FILE"     , ICON_FILE      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_480i"     , ICON_480i      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_480p"     , ICON_480p      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_576i"     , ICON_576i      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_576p"     , ICON_576p      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_720p"     , ICON_720p      , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_1080i"    , ICON_1080i     , 0xff, 0x00, 0xff, 0x00},
	{ "ICON_1080p"    , ICON_1080p     , 0xff, 0x00, 0xff, 0x00}
};

/* to the fp */
#define cCommandGetMJD           0x10
#define cCommandSetTimeFormat    0x11
#define cCommandSetMJD           0x15

#define cCommandSetPowerOff      0x20

#define cCommandPowerOffReplay   0x31
#define cCommandSetBootOn        0x40
#define cCommandSetBootOnExt     0x41

#define cCommandSetWakeupTime    0x72
#define cCommandSetWakeupMJD     0x74

#define cCommandGetPowerOnSrc    0x80

#define cCommandGetIrCode        0xa0
#define cCommandGetIrCodeExt     0xa1
#define cCommandSetIrCode        0xa5

#define cCommandGetPort          0xb2
#define cCommandSetPort          0xb3

#ifdef OCTAGON1008
#define cCommandSetIcon          0xc4
#else
#define cCommandSetIconI         0xc2 /* 0xc2 0xc7 0xcb 0xcc */ /* display cgram */
#define cCommandSetIconII        0xc7
#endif

#ifdef OCTAGON1008
#define cCommandSetVFD           0xc4
#elif HS7810A
#define cCommandSetVFD           0x24
#else
#define cCommandSetVFD           0xce /* 0xc0 */
#endif


#define cCommandSetVFDBrightness 0xd2

#if defined(ATEVIO7500) || defined(HS7810A)
#define cCommandGetFrontInfo     0xd0
#else
#define cCommandGetFrontInfo     0xe0
#endif

#define cCommandSetPwrLed        0x93 /* added by zeroone, only used in this file; set PowerLed Brightness on HDBOX */

static struct saved_data_s lastdata;

void write_sem_up(void)
{
    up(&write_sem);
}

int write_sem_down(void)
{
    return down_interruptible (&write_sem);
}

void copyData(unsigned char* data, int len)
{
    printk("%s len %d\n", __func__, len);
    memcpy(ioctl_data, data, len);
}

int nuvotonWriteCommand(char* buffer, int len, int needAck)
{
    int i;

    dprintk(150, "%s >\n", __func__);

    for (i = 0; i < len; i++)
    {
        serial_putc (buffer[i]);
    }

    if (needAck)
        if (ack_sem_down())
            return -ERESTARTSYS;

    dprintk(150, "%s < \n", __func__);

    return 0;
}

#ifdef OCTAGON1008
int nuvotonSetIcon(int which, int on)
{
    char buffer[128];
    int  res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);

    if (which < 0 || which >= 8)
    {
        printk("VFD/Nuvoton icon number out of range %d\n", which);
        return -EINVAL;
    }

    printk("VFD/Nuvoton icon number %d\n", which);
    memset(buffer, 0, 128);

    buffer[0] = SOP;
    buffer[1] = cCommandSetIcon;

    if (on) {
        buffer[3] = 0x01;
    } else
    {
        buffer[3] = 0x00;
    }

    buffer[2] = which;
    buffer[4] = vfdbuf[which].buf1;
    buffer[5] = vfdbuf[which].buf2;
    buffer[6] = EOP;

    res = nuvotonWriteCommand(buffer, 7, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

#elif defined(ATEVIO7500) || defined(FORTIS_HDBOX)
int nuvotonSetIcon(int which, int on)
{
    char buffer[128];
    u8 internalCode1, SymbolData1, internalCode2, SymbolData2;
    int  vLoop, res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);

    if (which < 1 || which >= ICON_MAX)
    {
        printk("VFD/Nuvoton icon number out of range %d\n", which);
        return -EINVAL;
    }

    internalCode1 = 0xff;
    internalCode2 = 0xff;
    SymbolData1 = 0x00;
    SymbolData2 = 0x00;
    for (vLoop = 0; vLoop < ARRAY_SIZE(nuvotonIcons); vLoop++)
    {
        if ((which & 0xff) == nuvotonIcons[vLoop].icon)
        {
            internalCode1 = nuvotonIcons[vLoop].internalCode1;
            internalCode2 = nuvotonIcons[vLoop].internalCode2;
            SymbolData1 = nuvotonIcons[vLoop].SymbolData1;
            SymbolData2 = nuvotonIcons[vLoop].SymbolData2;
            if (internalCode1 == 0xff)
            {
                printk("%s: not known or not supported icon %d ->%s\n", __func__, which, nuvotonIcons[vLoop].name);
                return -EINVAL;
            }

            break;
        }
    }

    if (internalCode1 == 0xff)
    {
        printk("%s: not known or not supported icon %d ->%s\n", __func__, which, nuvotonIcons[vLoop].name);
        return -EINVAL;
    }

    memset(buffer, 0, 128);

    buffer[0] = SOP;
    buffer[1] = cCommandSetIconI;
    buffer[2] = internalCode1;
    if (on)
        buffer[3] = SymbolData1;
    else
        buffer[3] = 0x00;

    buffer[4] = EOP;

    res = nuvotonWriteCommand(buffer, 5, 0);

    /* e.g. timeshift is splitted in two parts */
    if (internalCode2 != 0xff)
    {
        memset(buffer, 0, 128);

        buffer[0] = SOP;
        buffer[1] = cCommandSetIconI;
        buffer[2] = internalCode2;
        if (on)
            buffer[3] = SymbolData2;
        else
            buffer[3] = 0x00;

        buffer[4] = EOP;

        res = nuvotonWriteCommand(buffer, 5, 0);
    }

    dprintk(100, "%s <\n", __func__);

    return res;
}
#else // HS7810A 7seg_vfd
int nuvotonSetIcon(int which, int on)
{
	return 0;
}
#endif

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetIcon);

int nuvotonSetLED(int which, int on)
{
    char buffer[8];
    int res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);

//FIXME
    if (which < 1 || which > 6)
    {
        printk("VFD/Nuvoton led number out of range %d\n", which);
        return -EINVAL;
    }

    memset(buffer, 0, 8);

//fixme

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetLED);

int nuvotonSetBrightness(int level)
{
    char buffer[8];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, level);

#ifndef HS7810A
    if (level < 0 || level > 7)
    {
        printk("VFD/Nuvoton brightness out of range %d\n", level);
        return -EINVAL;
    }

    memset(buffer, 0, 8);

    buffer[0] = SOP;
    buffer[1] = cCommandSetVFDBrightness;
    buffer[2] = 0x00;

    buffer[3] = level;
    buffer[4] = EOP;

    res = nuvotonWriteCommand(buffer, 5, 0);
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetBrightness);

int nuvotonSetPwrLed(int level)
{
    char buffer[8];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, level);

#ifndef HS7810A
    if (level < 0 || level > 15)
    {
        printk("VFD/Nuvoton PwrLed out of range %d\n", level);
        return -EINVAL;
    }

    memset(buffer, 0, 8);

    buffer[0] = SOP;
    buffer[1] = cCommandSetPwrLed;
    buffer[2] = 0xf2;
    buffer[3] = level;
    buffer[4] = 0x00;
    buffer[5] = EOP;

    res = nuvotonWriteCommand(buffer, 6, 0);
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetPwrLed);

int nuvotonSetStandby(char* time)
{
    char     buffer[8];
    char     power_off[] = {SOP, cCommandPowerOffReplay, 0x01, EOP};
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    res = nuvotonWriteString("Bye bye ...", strlen("Bye bye ..."));

#ifndef HS7810A
    /* set wakeup time */
    memset(buffer, 0, 8);

    buffer[0] = SOP;
    buffer[1] = cCommandSetWakeupMJD;

    memcpy(buffer + 2, time, 4); /* only 4 because we do not have seconds here */
    buffer[6] = EOP;

    res = nuvotonWriteCommand(buffer, 7, 0);

    /* now power off */
    res = nuvotonWriteCommand(power_off, sizeof(power_off), 0);
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}

int nuvotonSetTime(char* time)
{
    char       buffer[8];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

#ifndef HS7810A
    memset(buffer, 0, 8);

    buffer[0] = SOP;
    buffer[1] = cCommandSetMJD;

    memcpy(buffer + 2, time, 5);
    buffer[7] = EOP;

    res = nuvotonWriteCommand(buffer, 8, 0);
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}

int nuvotonGetTime(void)
{
    char       buffer[3];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

#ifndef HS7810A
    memset(buffer, 0, 3);

    buffer[0] = SOP;
    buffer[1] = cCommandGetMJD;
    buffer[2] = EOP;

    errorOccured = 0;
    res = nuvotonWriteCommand(buffer, 3, 1);

    if (errorOccured == 1)
    {
        /* error */
        memset(ioctl_data, 0, 8);
        printk("error\n");
        res = -ETIMEDOUT;
    } else
    {
        /* time received ->noop here */
        dprintk(1, "time received\n");
        dprintk(20, "myTime= 0x%02x - 0x%02x - 0x%02x - 0x%02x - 0x%02x\n", ioctl_data[0], ioctl_data[1]
                , ioctl_data[2], ioctl_data[3], ioctl_data[4]);
    }
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}

int nuvotonGetWakeUpMode(void)
{
    char       buffer[8];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

#ifndef HS7810A
    memset(buffer, 0, 8);

    buffer[0] = SOP;
    buffer[1] = cCommandGetPowerOnSrc;
    buffer[2] = EOP;

    errorOccured = 0;
    res = nuvotonWriteCommand(buffer, 3, 1);

    if (errorOccured == 1)
    {
        /* error */
        memset(ioctl_data, 0, 8);
        printk("error\n");

        res = -ETIMEDOUT;
    } else
    {
        /* time received ->noop here */
        dprintk(1, "time received\n");
    }
#endif

    dprintk(100, "%s <\n", __func__);

    return res;
}

#if defined(HS7810A)
int nuvotonWriteString(unsigned char* aBuf, int len)
{
	int i,j,k, res;
	int buflen;
	int dot_count=0;
	unsigned char cmd_buf[8];

    dprintk(100, "%s > %d\n", __func__, len);

	memset(cmd_buf,0,8);
	buflen = len;

	if(buflen > 8)
		buflen = 8;

	for(i=0;i<buflen;i++)
		if(aBuf[i]== '.')
			dot_count++;

	k=2;
	for(i=0;i<4+dot_count;i++)
	{
		for(j=0; j < (sizeof(_7seg_fonts)/sizeof(_7seg_fonts[0])); j++)
			if(_7seg_fonts[j].ch == aBuf[i])
				cmd_buf[k]=_7seg_fonts[j].font ;

		if( aBuf[i+1] == '.')
		{
			cmd_buf[k]+=0x80;
			k++;
		}
		else
		{
			if( aBuf[i] == '.')
				continue;
			else
				k++;
		}
	}

	cmd_buf[0]=SOP;
	cmd_buf[1]=cCommandSetVFD;
	cmd_buf[6]=EOP;
	res = nuvotonWriteCommand(cmd_buf,7,0);

    dprintk(100, "%s <\n", __func__);
	return res;
}
#elif defined(OCTAGON1008)

inline char toupper(const char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (c - 0x20);
    return c;
}

int nuvotonWriteString(unsigned char* aBuf, int len)
{
    unsigned char bBuf[128];
    int i =0, max = 0;
    int j =0;
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, len);

    memset(bBuf, ' ', 128);

    max = (len > 8) ? 8 : len;
    printk("max = %d\n", max);

    //clean display text
    for(i=0; i<8; i++)
    {
        bBuf[0] = SOP;
        bBuf[2] = 7 - i; /* position: 0x00 = right */
        bBuf[1] = cCommandSetIcon;
        bBuf[3] = 0x00;
        bBuf[4] = 0x00;
        bBuf[5] = 0x00;
        bBuf[6] = EOP;
        nuvotonWriteCommand(bBuf, 8, 0);
    }

    for (i = 0; i < max ; i++)
    {
        bBuf[0] = SOP;
        bBuf[2] = 7 - i; /* position: 0x00 = right */
        bBuf[1] = cCommandSetIcon;
        bBuf[3] = 0x00;
        bBuf[6] = EOP;

        for (j = 0; j < ARRAY_SIZE(octagon_chars); j++)
        {
            if (octagon_chars[j].c == toupper(aBuf[i]))
            {
                bBuf[4] = octagon_chars[j].s1;
                vfdbuf[7 - i].buf1 = bBuf[4];
                bBuf[5] = octagon_chars[j].s2;
                vfdbuf[7 - i].buf2 = bBuf[5];
                res |= nuvotonWriteCommand(bBuf, 8, 0);
                break;
            }
        }
        //printk(" 0x%02x,0x%02x,0x%02x\n",vfdbuf[7 - i].pos,vfdbuf[7 - i].buf1,vfdbuf[7 - i].buf2);
    }

    /* save last string written to fp */
    memcpy(&lastdata.data, aBuf, 128);
    lastdata.length = len;

    dprintk(70, "len %d\n", len);

    dprintk(100, "%s <\n", __func__);

    return res;
}
#else
int nuvotonWriteString(unsigned char* aBuf, int len)
{
    unsigned char bBuf[128];
    int i =0;
    int j =0;
    int res = 0;

    dprintk(100, "%s >\n", __func__);

    memset(bBuf, ' ', 128);

    /* start of command write */
    bBuf[0] = SOP;
    bBuf[1] = cCommandSetVFD;
    bBuf[2] = 0x11;

    /* save last string written to fp */
    memcpy(&lastdata.data, aBuf, 128);
    lastdata.length = len;

    dprintk(70, "len %d\n", len);

    while ((i < len) && (j < 12))
    {
        if (aBuf[i] < 0x80)
            bBuf[j + 3] = aBuf[i];
        else if (aBuf[i] < 0xE0)
        {
            switch (aBuf[i])
            {
            case 0xc2:
                UTF_Char_Table = UTF_C2;
                break;
            case 0xc3:
                UTF_Char_Table = UTF_C3;
                break;
            case 0xc4:
                UTF_Char_Table = UTF_C4;
                break;
            case 0xc5:
                UTF_Char_Table = UTF_C5;
                break;
            case 0xd0:
                UTF_Char_Table = UTF_D0;
                break;
            case 0xd1:
                UTF_Char_Table = UTF_D1;
                break;
            default:
                UTF_Char_Table = NULL;
            }
            i++;
            if (UTF_Char_Table)
                bBuf[j + 3] = UTF_Char_Table[aBuf[i] & 0x3f];
            else
            {
                sprintf(&bBuf[j + 3],"%02x",aBuf[i-1]);
                j += 2;
                bBuf[j + 3] = (aBuf[i] & 0x3f) | 0x40;
            }
        }
        else
        {
            if (aBuf[i] < 0xF0)
                i += 2;
            else if (aBuf[i] < 0xF8)
                i += 3;
            else if (aBuf[i] < 0xFC)
                i += 4;
            else
                i += 5;
            bBuf[j + 3] = 0x20;
        }
        i++;
        j++;
    }
    /* end of command write, string must be filled with spaces */
    bBuf[15] = 0x20;
    bBuf[16] = EOP;

    res = nuvotonWriteCommand(bBuf, 17, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}
#endif

#ifndef ATEVIO7500
int nuvoton_init_func(void)
{
    char standby_disable[] = {SOP, cCommandPowerOffReplay, 0x02, EOP};
    char init1[] = {SOP, cCommandSetBootOn, EOP};
    char init2[] = {SOP, cCommandSetTimeFormat, 0x81, EOP};
    char init3[] = {SOP, cCommandSetWakeupTime, 0xff, 0xff, EOP}; /* delete/invalidate wakeup time ? */
    char init4[] = {SOP, 0x93, 0x01, 0x00, 0x08, EOP};
#if defined(OCTAGON1008)
    char init5[] = {SOP, 0x93, 0xf2, 0x08, 0x00, EOP};
#else
    char init5[] = {SOP, 0x93, 0xf2, 0x0a, 0x00, EOP};
#endif
    int  vLoop;
    int  res = 0;
    /*
    write (count=5, fd = 28): 0x02 0xc2 0x22 0x20 0x03
    write (count=5, fd = 28): 0x02 0xc2 0x23 0x04 0x03
    write (count=5, fd = 28): 0x02 0xc2 0x20 0x04 0x03
    */

    dprintk(100, "%s >\n", __func__);

    sema_init(&write_sem, 1);

    printk("Fortis HDBOX VFD/Nuvoton module initializing\n");

    /* must be called before standby_disable */
    res = nuvotonWriteCommand(init1, sizeof(init1), 0);

    /* setup: frontpanel should not power down the receiver if standby is selected */
    res = nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0);

    res |= nuvotonWriteCommand(init2, sizeof(init2), 0);
    res |= nuvotonWriteCommand(init3, sizeof(init3), 0);
    res |= nuvotonWriteCommand(init4, sizeof(init4), 0);
    res |= nuvotonWriteCommand(init5, sizeof(init5), 0);

#ifndef HS7810A
    res |= nuvotonSetBrightness(1);

    res |= nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));

    for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
        res |= nuvotonSetIcon(vLoop, 0);
#else
    res |= nuvotonWriteString("----", 4);
#endif

    dprintk(100, "%s <\n", __func__);

    return 0;
}
#else
int nuvoton_init_func(void)
{
    char standby_disable[] = {SOP, cCommandPowerOffReplay, 0x02, EOP};

    char init1[] = {SOP, 0xc2, 0x10, 0x00, EOP}; /* display cgram */
    char init2[] = {SOP, cCommandSetIrCode, 0x01, 0x02, 0xf9, 0x10, 0x0b, EOP};

    char init3[] = {SOP, cCommandSetTimeFormat, 0x81, EOP};
    char init4[] = {SOP, cCommandSetWakeupTime, 0xff, 0xff, EOP}; /* delete/invalidate wakeup time ? */

    int  vLoop;
    int  res = 0;

    dprintk(100, "%s >\n", __func__);

    sema_init(&write_sem, 1);

    printk("ATEVIO7500 VFD/Nuvoton module initializing\n");

    /* must be called before standby_disable */
    res = nuvotonWriteCommand(init1, sizeof(init1), 0);

    res = nuvotonWriteCommand(init2, sizeof(init2), 0);

    /* setup: frontpanel should not power down the receiver if standby is selected */
    res = nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0);

    res |= nuvotonWriteCommand(init3, sizeof(init3), 0);
    res |= nuvotonWriteCommand(init4, sizeof(init4), 0);
    res |= nuvotonSetBrightness(1);

    res |= nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));

    dprintk(100, "%s <\n", __func__);

    return 0;
}
#endif

static ssize_t NUVOTONdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
    char* kernel_buf;
    int minor, vLoop, res = 0;

    dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

    if (len == 0)
        return len;

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

    dprintk(70, "minor = %d\n", minor);

    /* dont write to the remote control */
    if (minor == FRONTPANEL_MINOR_RC)
        return -EOPNOTSUPP;

    kernel_buf = kmalloc(len, GFP_KERNEL);

    if (kernel_buf == NULL)
    {
        printk("%s return no mem<\n", __func__);
        return -ENOMEM;
    }

    copy_from_user(kernel_buf, buff, len);

    if (write_sem_down())
        return -ERESTARTSYS;

    /* Dagobert: echo add a \n which will be counted as a char
     */
    if (kernel_buf[len - 1] == '\n')
        res = nuvotonWriteString(kernel_buf, len - 1);
    else
        res = nuvotonWriteString(kernel_buf, len);

    kfree(kernel_buf);

    write_sem_up();

    dprintk(70, "%s < res %d len %d\n", __func__, res, len);

    if (res < 0)
        return res;
    else
        return len;
}

static ssize_t NUVOTONdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
    int minor, vLoop;

    dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

    if (len == 0)
        return len;

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

    dprintk(100, "minor = %d\n", minor);

    if (minor == FRONTPANEL_MINOR_RC)
    {
        int           size = 0;
        unsigned char data[20];

        memset(data, 0, 20);

        getRCData(data, &size);

        if (size > 0)
        {
            if (down_interruptible(&FrontPanelOpen[minor].sem))
                return -ERESTARTSYS;

            copy_to_user(buff, data, size);

            up(&FrontPanelOpen[minor].sem);

            dprintk(100, "%s < %d\n", __func__, size);
            return size;
        }

        dumpValues();

        return 0;
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

    dprintk(100, "%s < (len %d)\n", __func__, len);
    return len;
}

int NUVOTONdev_open(struct inode *inode, struct file *filp)
{
    int minor;

    dprintk(100, "%s >\n", __func__);

    /* needed! otherwise a racecondition can occur */
    if(down_interruptible (&write_sem))
        return -ERESTARTSYS;

    minor = MINOR(inode->i_rdev);

    dprintk(70, "open minor %d\n", minor);

    if (FrontPanelOpen[minor].fp != NULL)
    {
        printk("EUSER\n");
        up(&write_sem);
        return -EUSERS;
    }
    
    FrontPanelOpen[minor].fp = filp;
    FrontPanelOpen[minor].read = 0;

    up(&write_sem);

    dprintk(100, "%s <\n", __func__);

    return 0;

}

int NUVOTONdev_close(struct inode *inode, struct file *filp)
{
    int minor;

    dprintk(100, "%s >\n", __func__);

    minor = MINOR(inode->i_rdev);

    dprintk(20, "close minor %d\n", minor);

    if (FrontPanelOpen[minor].fp == NULL)
    {
        printk("EUSER\n");
        return -EUSERS;
    }
    FrontPanelOpen[minor].fp = NULL;
    FrontPanelOpen[minor].read = 0;

    dprintk(100, "%s <\n", __func__);
    return 0;
}

static int NUVOTONdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
    static int mode = 0;
    struct nuvoton_ioctl_data * nuvoton = (struct nuvoton_ioctl_data *)arg;
    int res = 0;
    
    dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

    if(down_interruptible (&write_sem))
        return -ERESTARTSYS;

    switch(cmd) {
    case VFDSETMODE:
        mode = nuvoton->u.mode.compat;
        break;
    case VFDSETLED:
        res = nuvotonSetLED(nuvoton->u.led.led_nr, nuvoton->u.led.on);
        break;
    case VFDBRIGHTNESS:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            res = nuvotonSetBrightness(data->start_address);
        } else
        {
            res = nuvotonSetBrightness(nuvoton->u.brightness.level);
        }
        mode = 0;
        break;
    case VFDPWRLED:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            res = nuvotonSetPwrLed(data->start_address);
        } else
        {
            res = nuvotonSetPwrLed(nuvoton->u.pwrled.level);
        }
        mode = 0;
        break;
    case VFDDRIVERINIT:
        res = nuvoton_init_func();
        mode = 0;
        break;
    case VFDICONDISPLAYONOFF:
#ifndef ATEVIO7500
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            int icon_nr = (data->data[0] & 0xf) + 1;
            int on = data->data[4];
            res = nuvotonSetIcon(icon_nr, on);
        } else
        {
            res = nuvotonSetIcon(nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
        }
#else
        res = 0;
#endif
        mode = 0;
        break;
    case VFDSTANDBY:
        res = nuvotonSetStandby(nuvoton->u.standby.time);
        break;
    case VFDSETTIME:
        if (nuvoton->u.time.time != 0)
            res = nuvotonSetTime(nuvoton->u.time.time);
        break;
    case VFDGETTIME:
        res = nuvotonGetTime();
        copy_to_user(arg, &ioctl_data, 5);
        break;
    case VFDGETWAKEUPMODE:
        res = nuvotonGetWakeUpMode();
        copy_to_user(arg, &ioctl_data, 1);
        break;
    case VFDDISPLAYCHARS:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            res = nuvotonWriteString(data->data, data->length);
        } else
        {
            //not suppoerted
        }
        mode = 0;
        break;
    case VFDDISPLAYWRITEONOFF:
        /* ->alles abschalten ? VFD_Display_Write_On_Off */
        printk("VFDDISPLAYWRITEONOFF ->not yet implemented\n");
        break;
    default:
        printk("VFD/Nuvoton: unknown IOCTL 0x%x\n", cmd);
        mode = 0;
        break;
    }
    up(&write_sem);
    dprintk(100, "%s <\n", __func__);
    return res;
}

struct file_operations vfd_fops =
{
    .owner = THIS_MODULE,
    .ioctl = NUVOTONdev_ioctl,
    .write = NUVOTONdev_write,
    .read  = NUVOTONdev_read,
    .open  = NUVOTONdev_open,
    .release  = NUVOTONdev_close
};
