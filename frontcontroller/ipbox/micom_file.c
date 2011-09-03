/*
 * micom_file.c
 *
 * (c) 2011 konfetti 
 * partly copied from user space implementation from M.Majoor
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

#include "micom.h"
#include "micom_asc.h"
#include "micom_utf.h"

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
int micomWriteString(unsigned char* aBuf, int len);

struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[20];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
    int   length;
    char  data[128];
};

/* version date of fp */
int micom_day, micom_month, micom_year;
int oldFP = 0;

/* to the fp */
#define VFD_GETA0                0xA0
#define VFD_GETWAKEUPSTATUS      0xA1
#define VFD_GETA2                0xA2
#define VFD_GETRAM               0xA3
#define VFD_GETDATETIME          0xA4
#define VFD_GETMICOM             0xA5
#define VFD_GETWAKEUP            0xA6
#define VFD_SETWAKEUPDATE        0xC0
#define VFD_SETWAKEUPTIME        0xC1
#define VFD_SETDATETIME          0xC2
#define VFD_SETBRIGHTNESS        0xC3
#define VFD_SETVFDTIME           0xC4
#define VFD_SETLED               0xC5
#define VFD_SETLEDSLOW           0xC6
#define VFD_SETLEDFAST           0xC7
#define VFD_SETSHUTDOWN          0xC8
#define VFD_SETRAM               0xC9
#define VFD_SETTIME              0xCA
#define VFD_SETCB                0xCB
#define VFD_SETFANON             0xCC
#define VFD_SETFANOFF            0xCD
#define VFD_SETRFMODULATORON     0xCE
#define VFD_SETRFMODULATOROFF    0xCF
#define VFD_SETCHAR              0xD0
#define VFD_SETDISPLAYTEXT       0xD1
#define VFD_SETD2                0xD2
#define VFD_SETD3                0xD3
#define VFD_SETD4                0xD4
#define VFD_SETCLEARTEXT         0xD5
#define VFD_SETD6                0xD6
#define VFD_SETMODETIME          0xD7
#define VFD_SETSEGMENTI          0xD8
#define VFD_SETSEGMENTII         0xD9
#define VFD_SETCLEARSEGMENTS     0xDA

//character table from puppy fp.c
//does not work for me and my logs shows other things (see below)
char iso8859_1[256] = 
{
  /* 0x00 */0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  /* 0x10 */0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  /*                !    "      #     $     %     &     '    (     )     *     +     ,     -     .     /    */
  /* 0x20 */0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  /*          0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >    ?    */
  /* 0x30 */0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  /*          @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O   */
  /* 0x40 */0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  /*          P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _   */
  /* 0x50 */0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x90, 0x4D, 0x4E, 0x4F, 
  /*          `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o   */
  /* 0x60 */0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  /*          p     q     r     s     t     u     v     w     x     z     y     {     |     }     ~         */
  /* 0x70 */0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x10,
  /* 0x80 */0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  /* 0x90 */0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  /* 0xa0 */0x59, 0x10, 0x10, 0x80, 0x8A, 0x4C, 0xE5, 0x81, 0x10, 0x10, 0x10, 0xDC, 0x10, 0x10, 0x10, 0x10,
  /* 0xb0 */0x10, 0x8E, 0x88, 0x89, 0x10, 0x78, 0x10, 0x10, 0x10, 0x10, 0x10, 0xDD, 0x10, 0x8B, 0x10, 0x10,
  /* 0xc0 */0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x10, 0x33, 0x35, 0x35, 0x35, 0x35, 0x39, 0x39, 0x39, 0x39,
  /* 0xd0 */0x34, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x48, 0x20, 0x45, 0x45, 0x45, 0x45, 0x49, 0x10, 0x71,
  /* 0xe0 */0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x10, 0x53, 0x55, 0x55, 0x55, 0x55, 0x59, 0x59, 0x59, 0x59,
  /* 0xf0 */0x7B, 0x5E, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F, 0x10, 0x20, 0x65, 0x65, 0x65, 0x65, 0x69, 0x10, 0x69,
};

struct chars {
u8 s1;
u8 s2;
u8 c;
} micom_chars[] =
{
   {0xe3, 0x11, 'A'},
   {0xcb, 0x25, 'B'},
   {0x21, 0x30, 'C'},
   {0x0b, 0x25, 'D'},
   {0xe1, 0x30, 'E'},
   {0xe1, 0x10, 'F'},
   {0xa1, 0x31, 'G'},
   {0xe2, 0x11, 'H'},
   {0x09, 0x24, 'I'},
   {0x09, 0x08, 'J'},
   {0x64, 0x12, 'K'},
   {0x20, 0x30, 'L'},
   {0x36, 0x11, 'M'},
   {0x32, 0x13, 'N'},
   {0x23, 0x31, 'O'},
   {0xe3, 0x10, 'P'},
   {0x23, 0x33, 'Q'},
   {0xe3, 0x12, 'R'},
   {0xe1, 0x21, 'S'},
   {0x09, 0x04, 'T'},
   {0x22, 0x31, 'U'},
   {0x24, 0x18, 'V'},
   {0x22, 0x1b, 'W'},
   {0x14, 0x0a, 'X'},
   {0xe2, 0x04, 'Y'},
   {0x05, 0x28, 'Z'},
   {0x08, 0x04, '1'},
   {0xc3, 0x30, '2'},
   {0xc3, 0x21, '3'},
   {0xe2, 0x01, '4'},
   {0xe1, 0x21, '5'},
   {0xe1, 0x31, '6'},
   {0x23, 0x01, '7'},
   {0xe3, 0x31, '8'},
   {0xe3, 0x21, '9'},
   {0x23, 0x31, '0'},
   {0x38, 0x8e, '!'}, //fixme
   {0x20, 0x82, '/'}, //fixme
   {0x00, 0x40, '.'},
   {0x20, 0x00, ','}, //fixme
   {0x11, 0xc4, '+'}, //fixme
   {0xc0, 0x00, '-'},
   {0x40, 0x00, '_'}, //fixme
   {0x08, 0x82, '<'}, //fixme
   {0x20, 0x88, '<'}, //fixme
   {0x00, 0x00, ' '}
};

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
	u8 codemsb;
	u8 codelsb;
	u8 segment;
} micomIcons[] ={
/*--------------------- SetIcon -------  msb   lsb   segment -----*/
	{ "ICON_STANDBY"  , ICON_STANDBY   , 0x03, 0x00, 1},
	{ "ICON_SAT"      , ICON_SAT       , 0x02, 0x00, 1},
	{ "ICON_REC"      , ICON_REC       , 0x00, 0x00, 0},
	{ "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x01, 0x01, 0},
	{ "ICON_TIMER"    , ICON_TIMER     , 0x01, 0x03, 0},
	{ "ICON_HD"       , ICON_HD        , 0x01, 0x04, 0},
	{ "ICON_USB"      , ICON_USB       , 0x01, 0x05, 0},
	{ "ICON_SCRAMBLED", ICON_SCRAMBLED , 0x01, 0x06, 0}, //locked not scrambled
	{ "ICON_DOLBY"    , ICON_DOLBY     , 0x01, 0x07, 0},
	{ "ICON_MUTE"     , ICON_MUTE      , 0x01, 0x08, 0},
	{ "ICON_TUNER1"   , ICON_TUNER1    , 0x01, 0x09, 0},
	{ "ICON_TUNER2"   , ICON_TUNER2    , 0x01, 0x0a, 0},
	{ "ICON_MP3"      , ICON_MP3       , 0x01, 0x0b, 0},
	{ "ICON_REPEAT"   , ICON_REPEAT    , 0x01, 0x0c, 0},
	{ "ICON_Play"     , ICON_Play      , 0x01, 0x00, 1},
	{ "ICON_Play_1"   , ICON_Play_1    , 0x01, 0x04, 1},
	{ "ICON_TER"      , ICON_TER       , 0x02, 0x01, 1},
	{ "ICON_FILE"     , ICON_FILE      , 0x02, 0x02, 1},
	{ "ICON_480i"     , ICON_480i      , 0x06, 0x03, 1},
	{ "ICON_480p"     , ICON_480p      , 0x06, 0x02, 1},
	{ "ICON_576i"     , ICON_576i      , 0x06, 0x00, 1},
	{ "ICON_576p"     , ICON_576p      , 0x05, 0x04, 1},
	{ "ICON_720p"     , ICON_720p      , 0x05, 0x03, 1},
	{ "ICON_1080i"    , ICON_1080i     , 0x05, 0x02, 1},
	{ "ICON_1080p"    , ICON_1080p     , 0x05, 0x01, 1}
};

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

int micomWriteCommand(char* buffer, int len, int needAck)
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

int micomSetLED(int on)
{
    char buffer[5];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, on);

    memset(buffer, 0, 5);

    /* on:
     * 0 = off
     * 1 = on
     * 2 = slow
     * 3 = fast
     */
    switch (on)
    {
        case 0:
            buffer[0] = VFD_SETLED;
            buffer[1] = 0x00;
        break;
        case 1:
            buffer[0] = VFD_SETLED;
            buffer[1] = 0x01;
        break;
        case 2:
            buffer[0] = VFD_SETLEDSLOW;
            buffer[1] = 0x80;
        break;
        case 3:
            buffer[0] = VFD_SETLEDFAST;
            buffer[1] = 0x80;
        break;
    } 

    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s (%d) <\n", __func__, res);

    return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetLED);

int micomSetFan(int on)
{
    char buffer[5];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, on);

    memset(buffer, 0, 5);

    /* on:
     * 0 = off
     * 1 = on
     */
    if (on == 1)
    { 
        buffer[0] = VFD_SETFANON;
    } else
    { 
        buffer[0] = VFD_SETFANOFF;
    }
     
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s (%d) <\n", __func__, res);

    return res;
}

int micomSetRF(int on)
{
    char buffer[5];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, on);

    memset(buffer, 0, 5);

    /* on:
     * 0 = off
     * 1 = on
     */
    if (on == 1)
    { 
        buffer[0] = VFD_SETRFMODULATORON;
    } else
    { 
        buffer[0] = VFD_SETRFMODULATOROFF;
    }
     
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s (%d) <\n", __func__, res);

    return res;
}

int micomSetBrightness(int level)
{
    char buffer[5];
    int res = 0;

    dprintk(100, "%s > %d\n", __func__, level);

    if (level < 0 || level > 7)
    {
        printk("VFD/Micom brightness out of range %d\n", level);
        return -EINVAL;
    }

    memset(buffer, 0, 5);

    buffer[0] = VFD_SETBRIGHTNESS;
    buffer[1] = level &0x07;

    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetIcon(int which, int on)
{
    char buffer[5];
    int  vLoop, res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);

    if (which < 1 || which >= ICON_MAX)
    {
        printk("VFD/Nuvoton icon number out of range %d\n", which);
        return -EINVAL;
    }
    memset(buffer, 0, 5);

    for (vLoop = 0; vLoop < ARRAY_SIZE(micomIcons); vLoop++)
    {
        if ((which & 0xff) == micomIcons[vLoop].icon)
        {
            buffer[0] = VFD_SETSEGMENTI + micomIcons[vLoop].segment;
            buffer[1] = on;
            buffer[2] = micomIcons[vLoop].codelsb;
            buffer[3] = micomIcons[vLoop].codemsb;
            res = micomWriteCommand(buffer, 5, 0);

            break;
        }
    }

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* expected format: YYMMDDhhmm */
int micomSetStandby(char* time)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    res = micomWriteString("Bye bye ...", strlen("Bye bye ..."));

    /* set wakeup time */
    memset(buffer, 0, 5);

    buffer[0] = VFD_SETWAKEUPDATE;
    buffer[1] = ((time[0] - '0') << 4) | (time[1] - '0'); //year 
    buffer[2] = ((time[2] - '0') << 4) | (time[3] - '0'); //month
    buffer[3] = ((time[4] - '0') << 4) | (time[5] - '0'); //day

    res = micomWriteCommand(buffer, 5, 0);

    memset(buffer, 0, 5);

    buffer[0] = VFD_SETWAKEUPTIME;
    buffer[1] = ((time[6] - '0') << 4) | (time[7] - '0'); //hour
    buffer[2] = ((time[8] - '0') << 4) | (time[9] - '0'); //minute
    buffer[3] = 1; /* on/off */

    res = micomWriteCommand(buffer, 5, 0);

    /* now power off */
    memset(buffer, 0, 5);

    buffer[0] = VFD_SETSHUTDOWN;
    /* 0 = shutdown
     * 1 = reboot
     * 2 = warmoff
     * 3 = warmon
     */
    buffer[1] = 0;
 
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micomReboot(void)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    res = micomWriteString("Bye bye ...", strlen("Bye bye ..."));

    memset(buffer, 0, 5);

    buffer[0] = VFD_SETSHUTDOWN;
    /* 0 = shutdown
     * 1 = reboot
     * 2 = warmoff
     * 3 = warmon
     */
    buffer[1] = 1;
 
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* expected format: YYMMDDhhmmss */
int micomSetTime(char* time)
{
    char       buffer[5];
    int        res = 0;

    dprintk(100, "%s >\n", __func__);

    /* set wakeup time */
    memset(buffer, 0, 5);

    buffer[0] = VFD_SETDATETIME;
    buffer[1] = ((time[0] - '0') << 4) | (time[1] - '0'); //year
    buffer[2] = ((time[2] - '0') << 4) | (time[3] - '0'); //month
    buffer[3] = ((time[4] - '0') << 4) | (time[5] - '0'); //day

    res = micomWriteCommand(buffer, 5, 0);

    memset(buffer, 0, 5);

    buffer[0] = VFD_SETTIME;
    buffer[1] = ((time[6] - '0') << 4) | (time[7] - '0'); //hour
    buffer[2] = ((time[8] - '0') << 4) | (time[9] - '0'); //minute
    buffer[3] = ((time[10] - '0') << 4) | (time[11] - '0'); //second

    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micomClearIcons(void)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    memset(buffer, 0, 5);

    buffer[0] = VFD_SETCLEARSEGMENTS;
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micomGetTime(void)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    memset(buffer, 0, 5);

//fixme

    buffer[0] = VFD_GETDATETIME;

    errorOccured = 0;
    res = micomWriteCommand(buffer, 5, 1);

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

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* get date (version) of micom controller */
int micomGetMicom(void)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    memset(buffer, 0, 5);

    buffer[0] = VFD_GETMICOM;

    errorOccured = 0;
    res = micomWriteCommand(buffer, 5, 1);

    if (errorOccured == 1)
    {
        /* error */
        memset(ioctl_data, 0, 20);
        printk("error\n");
        res = -ETIMEDOUT;
    } else
    {
        dprintk(100, "0x%02x 0x%02x 0x%02x\n", ioctl_data[0], ioctl_data[1], ioctl_data[2]);
        
        micom_year  = (ioctl_data[0] & 0xff) + 2000;
        micom_month = (ioctl_data[1] & 0xff);
        micom_day   = (ioctl_data[2] & 0xff);

        if (micom_year > 2007)
           oldFP = 0;
        else
           oldFP = 1;

        dprintk(1, "time received\n");
        dprintk(1, "myTime= %02d.%02d.%04d\n", micom_day, micom_month, micom_year);
    }

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micomGetWakeUpMode(void)
{
    char     buffer[5];
    int      res = 0;

    dprintk(100, "%s >\n", __func__);

    memset(buffer, 0, 5);

//fixme

    dprintk(100, "%s <\n", __func__);

    return res;
}

inline char toupper(const char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (c - 0x20);
    return c;
}

int micomWriteString(unsigned char* aBuf, int len)
{
    char       buffer[5];
    char       bBuf[11];
    int        res = 0, i, j;

    dprintk(100, "%s >\n", __func__);

//fixme: utf not implemented
 
    /* always write all characters and fill unused with 0x10 */
    memset(bBuf, 0x10, 11);
    memcpy(bBuf, aBuf, len);
    len = 11;

    memset(buffer, 0, 5);
    buffer[0] = VFD_SETCLEARTEXT;
    res = micomWriteCommand(buffer, 5, 0);

    /* set text char by char */
    for (i = 0; i < len; i++)
    {
        memset(buffer, 0, 5);

        buffer[0] = VFD_SETCHAR;
        buffer[1] = i & 0xff; /* position */

        if (oldFP)
        {
            for (j = 0; j < ARRAY_SIZE(micom_chars); j++)
            {
                if (micom_chars[j].c == toupper(bBuf[i]))
                {
                    buffer[2] = micom_chars[j].s1;
                    buffer[3] = micom_chars[j].s2;
                    break;
                }
            }
        } else
        {
            buffer[2] = iso8859_1[(unsigned char) bBuf[i] & 0xff];
//        buffer[2] = bBuf[i] & 0xff;
        }
        res = micomWriteCommand(buffer, 5, 0);
    }

    memset(buffer, 0, 5);
    buffer[0] = VFD_SETDISPLAYTEXT;
    res = micomWriteCommand(buffer, 5, 0);

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micom_init_func(void)
{
    int  res = 0;

    dprintk(100, "%s >\n", __func__);

    sema_init(&write_sem, 1);

    printk("Cuberevo 9000 HD VFD/Micom module initializing\n");

    micomGetMicom();
    res |= micomSetFan(0);
    res |= micomSetBrightness(7);
    res |= micomWriteString("T.Ducktales", strlen("T.Ducktales"));

    dprintk(100, "%s <\n", __func__);

    return 0;
}

static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
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
        res = micomWriteString(kernel_buf, len - 1);
    else
        res = micomWriteString(kernel_buf, len);

    kfree(kernel_buf);

    write_sem_up();

    dprintk(70, "%s < res %d len %d\n", __func__, res, len);

    if (res < 0)
        return res;
    else
        return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
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

int MICOMdev_open(struct inode *inode, struct file *filp)
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

int MICOMdev_close(struct inode *inode, struct file *filp)
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

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
    static int mode = 0;
    struct micom_ioctl_data * micom = (struct micom_ioctl_data *)arg;
    int res = 0;
    
    dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

    if(down_interruptible (&write_sem))
        return -ERESTARTSYS;

    switch(cmd) {
    case VFDSETMODE:
        mode = micom->u.mode.compat;
        break;
    case VFDSETLED:
        res = micomSetLED(micom->u.led.on);
        break;
    case VFDBRIGHTNESS:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            res = micomSetBrightness(data->start);
        } else
        {
            res = micomSetBrightness(micom->u.brightness.level);
        }
        mode = 0;
        break;
    case VFDDRIVERINIT:
        res = micom_init_func();
        mode = 0;
        break;
    case VFDICONDISPLAYONOFF:
        if (!oldFP)
        {
            if (mode == 0)
            {
                struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
                int icon_nr = (data->data[0] & 0xf) + 1;
                int on = data->data[4];
                res = micomSetIcon(icon_nr, on);
            } else
            {
                res = micomSetIcon(micom->u.icon.icon_nr, micom->u.icon.on);
            }
        } else
           res = 0;
        mode = 0;
        break;
    case VFDSTANDBY:
        res = micomSetStandby(micom->u.standby.time);
        break;
    case VFDREBOOT:
        res = micomReboot();
        break;
    case VFDSETTIME:
        if (micom->u.time.time != 0)
            res = micomSetTime(micom->u.time.time);
        break;
    case VFDSETFAN:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            int on = data->start;
            res = micomSetFan(on);
        } else
        {
            res = micomSetFan(micom->u.fan.on);
        }
        break;
    case VFDSETRF:
        res = micomSetRF(micom->u.rf.on);
        break;
    case VFDGETTIME:
        res = micomGetTime();
        copy_to_user((void*) arg, &ioctl_data, 5);
        break;
    case VFDGETWAKEUPMODE:
        res = micomGetWakeUpMode();
        copy_to_user((void*) arg, &ioctl_data, 1);
        break;
    case VFDDISPLAYCHARS:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            res = micomWriteString(data->data, data->length);
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
    case VFDCLEARICONS:
        res = 0;
        if (!oldFP)
           res = micomClearIcons();
    break;
    default:
        printk("VFD/Micom: unknown IOCTL 0x%x\n", cmd);
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
    .ioctl = MICOMdev_ioctl,
    .write = MICOMdev_write,
    .read  = MICOMdev_read,
    .open  = MICOMdev_open,
    .release  = MICOMdev_close
};
