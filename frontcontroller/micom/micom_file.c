/*
 * micom_ioctl.c
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

#include "micom.h"
#include "micom_asc.h"
#include "../vfd/utf.h"

extern void ack_sem_up(void);
extern int  ack_sem_down(void);

struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[8];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int   length;
	char  data[20];
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

int micomWriteCommand(char command, char* buffer, int len, int needAck)
{
    int i;

    dprintk(100, "%s >\n", __func__);

    serial_putc(command);

    for (i = 0; i < len; i++)
    {
	dprintk(201, "Put: %c\n", buffer[i]);
        serial_putc (buffer[i]);
    }

    if (needAck)
        if (ack_sem_down())
           return -ERESTARTSYS;

    dprintk(100, "%s < \n", __func__);

    return 0;
}

int micomSetIcon(int which, int on)
{
    char buffer[8];
    int  res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);
    if (which < 1 || which > 16)
    {
        printk("VFD/MICOM icon number out of range %d\n", which);
        return -EINVAL;
    }

    memset(buffer, 0, 8);
    buffer[0] = which;

    if (on == 1)
        res = micomWriteCommand(0x11, buffer, 7, 1);
    else
        res = micomWriteCommand(0x12, buffer, 7, 1);

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetIcon);

int micomSetLED(int which, int on)
{
    char buffer[8];
    int  res = 0;

    dprintk(100, "%s > %d, %d\n", __func__, which, on);

#ifndef UFS912
    if (which < 1 || which > 6)
    {
        printk("VFD/MICOM led number out of range %d\n", which);
        return -EINVAL;
    }
#else
    /* 0x02 = green
     * 0x03 = red
     */
    if (which < 0x02 || which > 0x03)
    {
        printk("VFD/MICOM led number out of range %d\n", which);
        return -EINVAL;
    }
#endif

    memset(buffer, 0, 8);
    buffer[0] = which;

    if (on == 1)
        res = micomWriteCommand(0x06, buffer, 7, 1);
    else
        res = micomWriteCommand(0x22, buffer, 7, 1);

    dprintk(100, "%s <\n", __func__);

    return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetLED);


int micomSetBrightness(int level)
{
    char buffer[8];
    int  res = 0;

    dprintk(5, "%s > %d\n", __func__, level);
    if (level < 1 || level > 5)
    {
        printk("VFD/MICOM brightness out of range %d\n", level);
        return -EINVAL;
    }

    memset(buffer, 0, 8);
    buffer[0] = level;

    res = micomWriteCommand(0x25, buffer, 7, 1);

    dprintk(10, "%s <\n", __func__);

    return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetLedBrightness(int level)
{
    char buffer[8];
    int  res = 0;

    dprintk(5, "%s > %d\n", __func__, level);
    if (level < 0 || level > 0xff)
    {
        printk("VFD/MICOM button brightness out of range %d\n", level);
        return -EINVAL;
    }

    memset(buffer, 0, 8);
    buffer[0] = level;

    res = micomWriteCommand(0x07, buffer, 7, 1);

    dprintk(10, "%s <\n", __func__);

    return res;
}

EXPORT_SYMBOL(micomSetLedBrightness);

#ifdef UFS922
int micomSetModel(void)
{
    char buffer[8];
    int  res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);
    buffer[0] = 0x1;

    res = micomWriteCommand(0x3, buffer, 7, 0);

    dprintk(10, "%s <\n", __func__);

    return res;
}
#else
int micomInitialize(void)
{
    char buffer[8];
    int  res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);
    buffer[0] = 0x1;

    res = micomWriteCommand(0x3, buffer, 7, 0);

    memset(buffer, 0, 8);

    /* unknown command:
     * modifications of most of the values leads to a
     * not working fp which can only be fixed by switching
     * off receive. value 0x46 can be modified and resetted.
     * changing the values behind 0x46 has no effect for me.
     */
    buffer[0] = 0x02;
    buffer[1] = 0xff;
    buffer[2] = 0x80;
    buffer[3] = 0x46;
    buffer[4] = 0x01;

    res = micomWriteCommand(0x55, buffer, 7, 0);

    dprintk(10, "%s <\n", __func__);

    return res;
}
#endif


int micomSetStandby(char* time)
{
    char       buffer[8];
    int      res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    if (time[0] == '\0')
    {
        /* clear wakeup time */
        res = micomWriteCommand(0x33, buffer, 7, 1);
    } else
    {
        /* set wakeup time */

        memcpy(buffer, time, 5);
        res = micomWriteCommand(0x32, buffer, 7, 1);
    }

    if (res < 0)
    {
        printk("%s <res %d \n", __func__, res);
        return res;
    }

    memset(buffer, 0, 8);
    /* enter standby */
    res = micomWriteCommand(0x41, buffer, 7, 0);

    dprintk(10, "%s <\n", __func__);

    return res;
}

int micomSetTime(char* time)
{
    char       buffer[8];
    int      res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    memcpy(buffer, time, 5);
    res = micomWriteCommand(0x31, buffer, 7, 1);

    dprintk(10, "%s <\n", __func__);

    return res;
}

int micomGetVersion(void)
{
    char       buffer[8];
    int        res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    errorOccured   = 0;
    res = micomWriteCommand(0x05, buffer, 7, 1);

    if (res < 0)
    {
        printk("%s < res %d\n", __func__, res);
        return res;
    }

    if (errorOccured)
    {
        memset(ioctl_data, 0, 8);
        printk("error\n");

        res = -ETIMEDOUT;
    } else
    {
        /* version received ->noop here */
        dprintk(1, "version received\n");
    }

    dprintk(10, "%s <\n", __func__);

    return res;
}

int micomGetTime(void)
{
    char       buffer[8];
    int        res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    errorOccured   = 0;
    res = micomWriteCommand(0x39, buffer, 7, 1);

    if (res < 0)
    {
        printk("%s < res %d\n", __func__, res);
        return res;
    }

    if (errorOccured)
    {
        memset(ioctl_data, 0, 8);
        printk("error\n");

        res = -ETIMEDOUT;
    } else
    {
        /* time received ->noop here */
        dprintk(1, "time received\n");
    }

    dprintk(10, "%s <\n", __func__);

    return res;
}

/* 0xc1 = rcu
 * 0xc2 = front
 * 0xc3 = time
 * 0xc4 = ac ???
 */
int micomGetWakeUpMode(void)
{
    char       buffer[8];
    int      res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    errorOccured   = 0;
    res = micomWriteCommand(0x43, buffer, 7, 1);

    if (res < 0)
    {
        printk("%s < res %d\n", __func__, res);
        return res;
    }

    if (errorOccured)
    {
        memset(ioctl_data, 0, 8);
        printk("error\n");

        res = -ETIMEDOUT;
    } else
    {
        /* time received ->noop here */
        dprintk(1, "time received\n");
    }

    dprintk(10, "%s <\n", __func__);

    return res;
}

int micomReboot(void)
{
    char       buffer[8];
    int      res = 0;

    dprintk(5, "%s >\n", __func__);

    memset(buffer, 0, 8);

    res = micomWriteCommand(0x46, buffer, 7, 0);

    dprintk(10, "%s <\n", __func__);

    return res;
}


int micomWriteString(unsigned char* aBuf, int len)
{
    unsigned char bBuf[20];
    int i =0;
    int j =0;
    int res = 0;

    dprintk(100, "%s >\n", __func__);

//utf8: if (len > 16 || len < 0)
//  {
//      printk("VFD String Length value is over! %d\n", len);
//      len = 16;
//  }

    memset(bBuf, ' ', 20);

    /* save last string written to fp */
    memcpy(&lastdata.data, aBuf, 20);
    lastdata.length = len;

    while ((i< len) && (j < 16))
    {
        if (aBuf[i] < 0x80)
            bBuf[j] = aBuf[i];
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
                bBuf[j] = UTF_Char_Table[aBuf[i] & 0x3f];
            else
            {
                sprintf(&bBuf[j],"%02x",aBuf[i-1]);
                j+=2;
                bBuf[j] = (aBuf[i] & 0x3f) | 0x40;
            }
        }
        else
        {
            if (aBuf[i] < 0xF0)
                i+=2;
            else if (aBuf[i] < 0xF8)
                i+=3;
            else if (aBuf[i] < 0xFC)
                i+=4;
            else
                i+=5;
            bBuf[j] = 0x20;
        }
        i++;
        j++;
    }
    res = micomWriteCommand(0x21, bBuf, 16, 1);

    dprintk(100, "%s <\n", __func__);

    return res;
}

int micom_init_func(void)
{
#ifdef UFS922
    int vLoop;
#endif
    dprintk(5, "%s >\n", __func__);

    sema_init(&write_sem, 1);

#ifdef UFS922
    printk("Kathrein UFS922 VFD/MICOM module initializing\n");
    micomSetModel();
#else
    printk("Kathrein UFS912 VFD/MICOM module initializing\n");
    micomInitialize();
#endif
    micomSetBrightness(1);

    micomSetLedBrightness(0x50);

    micomWriteString(" Team Ducktales ", strlen(" Team Ducktales "));

#ifndef UFS912
    for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
        micomSetIcon(vLoop, 0);
#endif
    dprintk(10, "%s <\n", __func__);

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

    dprintk(100, "%s < res %d len %d\n", __func__, res, len);

    if (res < 0)
        return res;
    else
        return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
    int minor, vLoop;
    dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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

            dprintk(150, "%s < %d\n", __func__, size);
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

    dprintk(70, "close minor %d\n", minor);

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

    if (write_sem_down())
        return -ERESTARTSYS;

    switch(cmd) {
    case VFDSETMODE:
        mode = micom->u.mode.compat;
        break;
    case VFDSETLED:
        res = micomSetLED(micom->u.led.led_nr, micom->u.led.on);
        break;
    case VFDBRIGHTNESS:
        if (mode == 0)
        {
            struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
            int level = data->start_address;

            /* scale level from 0 - 7 to a range from 1 - 5 where 5 is off */
            level = 7 - level;

            level =  ((level * 100) / 7 * 5) / 100 + 1;

            res = micomSetBrightness(level);
        } else
        {
            res = micomSetBrightness(micom->u.brightness.level);
        }
        mode = 0;
        break;
    case VFDLEDBRIGHTNESS:
        if (mode == 0)
        {
        } else
        {
            res = micomSetLedBrightness(micom->u.brightness.level);
        }
        mode = 0;
        break;
    case VFDDRIVERINIT:
        res = micom_init_func();
        mode = 0;
        break;
    case VFDICONDISPLAYONOFF:
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
    case VFDGETTIME:
        res = micomGetTime();
        copy_to_user(arg, &ioctl_data, 8);
        break;
    case VFDGETVERSION:
        res = micomGetVersion();
        copy_to_user(arg, &ioctl_data, 8);
        break;
    case VFDGETWAKEUPMODE:
        res = micomGetWakeUpMode();
        copy_to_user(arg, &ioctl_data, 8);
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
#ifdef UFS912
        micomInitialize();
#endif
        break;
    default:
        printk("VFD/MICOM: unknown IOCTL 0x%x\n", cmd);

        mode = 0;
        break;
    }

    write_sem_up();

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
