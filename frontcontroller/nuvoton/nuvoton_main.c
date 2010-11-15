/*
 * nuvoton_main.c
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

/*
 * Fortis HDBOX 9200 Frontpanel driver.
 *
 * Devices:
 *  - /dev/vfd (vfd ioctls and read/write function)
 *  - /dev/rc  (reading of key events)
 *
 * BUGS:
 * - exiting the rcS leads to no data receiving from ttyAS0 ?!?!?
 * - some Icons are missing
 * - buttons (see evremote)
 * - GetWakeUpMode not tested (dont know the meaning of the mode ;) )
 *
 * Unknown Commands:
 * 0x02 0x40 0x03
 * 0x02 0xce 0x10
 * 0x02 0xce 0x30
 * 0x02 0x72
 * 0x02 0x93
 * The next two are written by the app every keypress. At first I think
 * this is the visual feedback but doing this manual have no effect.
 * 0x02, 0x93, 0x01, 0x00, 0x08, 0x03
 * 0x02, 0x93, 0xf2, 0x0a, 0x00, 0x03
 *
 * New commands from octagon1008:
 * 0x02 0xd0 x03
 *
 * 0x02 0xc4 0x20 0x00 0x00 0x00 0x03
 *
 * fixme icons: must be mapped somewhere!
 * 0x00 dolby
 * 0x01 dts
 * 0x02 video
 * 0x03 audio
 * 0x04 link
 * 0x05 hdd
 * 0x06 disk
 * 0x07 DVB
 * 0x08 DVD
 * fixme: usb icon at the side and some other?
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

//----------------------------------------------

#define EVENT_BTN                  0x51
#define EVENT_RC                   0x63
#define EVENT_STANDBY              0x80

#define EVENT_ANSWER_GETTIME       0x15
#define EVENT_ANSWER_WAKEUP_REASON 0x81
#define EVENT_ANSWER_FRONTINFO     0xe4 /* unsused */
#define EVENT_ANSWER_GETIRCODE     0xa5 /* unsused */
#define EVENT_ANSWER_GETPORT       0xb3 /* unsused */

#define DATA_BTN_EVENT   2

//----------------------------------------------
short paramDebug = 101;

static unsigned char expectEventData = 0;
static unsigned char expectEventId = 1;

struct semaphore     waitForAck;

#define cPackageSizeRC       7
#define cPackageSizeFP       5
#define cPackageSizeStandby  5

#define cGetTimeSize         9
#define cGetWakeupReasonSize 5

#define cMinimumSize         5

#define BUFFERSIZE   256     //must be 2 ^ n
static unsigned char RCVBuffer [BUFFERSIZE];
static int           RCVBufferStart = 0, RCVBufferEnd = 0;

static wait_queue_head_t   wq;

//----------------------------------------------

void ack_sem_up(void)
{
    up(&waitForAck);
}

int ack_sem_down(void)
{
    /* fixme: we should supervise this in the irq (time based)
     * to avoid blocking for ever for the abnormal case when
     * no answer is reached from the rc ->see processResponse
     */
    return down_interruptible (&waitForAck);
}

EXPORT_SYMBOL(ack_sem_down);

//------------------------------------------------------------------
int getLen(int expectedLen)
{
    int i,j, len;

    i = 0;
    j = RCVBufferEnd;

    while (1)
    {
        if (RCVBuffer[j] == EOP)
        {
            if ((expectedLen == -1) || (i == expectedLen - 1))
                break;
        }
        
        j++;
        i++;

        if (j >= BUFFERSIZE)
        {
            j = 0;
        }
        
        if (j == RCVBufferStart)
        {
            i = -1;
            break;
        }
    }

    len = i + 1;

    return len;
}

void getRCData(unsigned char* data, int* len)
{
    int i, j;

    *len = 0;
    while(RCVBufferStart == RCVBufferEnd)
    {
        dprintk(200, "%s %d - %d\n", __func__, RCVBufferStart, RCVBufferEnd);

        if (wait_event_interruptible(wq, (RCVBufferStart != RCVBufferEnd) && (getLen(-1) != 0)))
        {
            printk("wait_event_interruptible failed\n");
            return;
        }
    }

    *len = getLen(-1); /* use -1 here because of differen len from RC and Buttons */

    if (*len <= 0)
    {
        *len = 0;
        return;
    }
    
    i = 0;
    j = RCVBufferEnd;

    while (i != *len)
    {
        data[i] = RCVBuffer[j];
        j++;
        i++;

        if (j >= BUFFERSIZE)
            j = 0;

        if (j == RCVBufferStart)
        {
            break;
        }
    }

    RCVBufferEnd = (RCVBufferEnd + *len) % BUFFERSIZE;

    dprintk(150, " <len %d, End %d\n", *len, RCVBufferEnd);
}

void handleCopyData(int len)
{
    int i,j;
    
    unsigned char* data = kmalloc(len - 4, GFP_KERNEL);
    
    i = 0;

    /* filter 0x00 0x02 cmd */
    j = (RCVBufferEnd + 3) % BUFFERSIZE;
    
    while (i != len - 4)
    {
        data[i] = RCVBuffer[j];
        j++;
        i++;

        if (j >= BUFFERSIZE)
            j = 0;

        if (j == RCVBufferStart)
        {
            break;
        }
    }

    copyData(data, len - 4);
    
    kfree(data);
}

void dumpValues(void)
{
    dprintk(150, "BuffersStart %d, BufferEnd %d, len %d\n", RCVBufferStart, RCVBufferEnd, getLen(-1));
}

void dumpData(void)
{
    int i, j, len;
    
    if (paramDebug < 150)
        return;
    
    len = getLen(-1);
    
    i = RCVBufferEnd;
    for (j = 0; j < len; j++)
    {
        printk("0x%02x ", RCVBuffer[i]);

        i++;

        if (i >= BUFFERSIZE)
        {
            i = 0;
        }

        if (i == RCVBufferStart)
        {
            i = -1;
            break;
        }
    }
    printk("\n");
}

static void processResponse(void)
{
    int len;

    dumpData();
    len = getLen(-1);

    if (len < cMinimumSize)
        return;

    dumpData();

    if (expectEventId)
    {
        /* DATA_BTN_EVENT can be wrapped to start */
        int index = (RCVBufferEnd + DATA_BTN_EVENT) % BUFFERSIZE;
        
        expectEventData = RCVBuffer[index];
        expectEventId = 0;
    }

    dprintk(100, "event 0x%02x %d %d\n", expectEventData, RCVBufferStart, RCVBufferEnd);

    if (expectEventData)
    {
        switch (expectEventData)
        {
        case EVENT_BTN:
        {
            /* no longkeypress for frontpanel buttons! */
            len = getLen(cPackageSizeFP);

            if (len == 0)
                goto out_switch;

            if (len < cPackageSizeFP)
                goto out_switch;

            wake_up_interruptible(&wq);
        }
        break;
        case EVENT_RC:
        {
            len = getLen(cPackageSizeRC);

            if (len == 0)
                goto out_switch;

            if (len < cPackageSizeRC)
                goto out_switch;

            wake_up_interruptible(&wq);
        }
        break;
        case EVENT_ANSWER_GETTIME:

            len = getLen(cGetTimeSize);

            if (len == 0)
                goto out_switch;

            if (len < cGetTimeSize)
                goto out_switch;

            handleCopyData(len);

            /* if there is a waiter for an acknowledge ... */
            if(atomic_read(&waitForAck.count) < 0)
            {
                dprintk(20, "Pos. response received\n");
                errorOccured = 0;
                ack_sem_up();
            } else
            {
                printk("[nuvoton] receive answer not waiting for\n");
            }

            RCVBufferEnd = (RCVBufferEnd + cGetTimeSize) % BUFFERSIZE;
            break;
        case EVENT_ANSWER_WAKEUP_REASON:

            len = getLen(cGetWakeupReasonSize);

            if (len == 0)
                goto out_switch;

            if (len < cGetWakeupReasonSize)
                goto out_switch;

            handleCopyData(len);

            /* if there is a waiter for an acknowledge ... */
            if(atomic_read(&waitForAck.count) < 0)
            {
                dprintk(1, "Pos. response received\n");
                errorOccured = 0;
                ack_sem_up();
            } else
            {
                printk("[nuvoton] receive answer not waiting for\n");
            }

            RCVBufferEnd = (RCVBufferEnd + cGetWakeupReasonSize) % BUFFERSIZE;

            break;
        case EVENT_ANSWER_FRONTINFO:
        case EVENT_ANSWER_GETIRCODE:
        case EVENT_ANSWER_GETPORT:
        default: // Ignore Response
            dprintk(1, "Invalid Response %02x:\n", expectEventData);
            dumpData();

            /* increase until we found a new valid answer */
            RCVBufferEnd = (RCVBufferEnd + 1) % BUFFERSIZE;
            break;
        }
    }
out_switch:
        expectEventId = 1;
        expectEventData = 0;
}

static irqreturn_t FP_interrupt(int irq, void *dev_id)
{
    unsigned int *ASC_X_INT_STA = (unsigned int*)(ASCXBaseAddress + ASC_INT_STA);
    char         *ASC_X_RX_BUFF = (char*)        (ASCXBaseAddress + ASC_RX_BUFF);
    int          dataArrived = 0;

    if (paramDebug > 100)
        printk("i - ");

    while (*ASC_X_INT_STA & ASC_INT_STA_RBF)
    {
        RCVBuffer [RCVBufferStart] = *ASC_X_RX_BUFF;
        RCVBufferStart = (RCVBufferStart + 1) % BUFFERSIZE;

        dataArrived = 1;

        if (RCVBufferStart == RCVBufferEnd)
        {
            printk ("FP: RCV buffer overflow!!!\n");
        }
    }

    if (dataArrived)
        processResponse();

    /* konfetti comment: in normal case we would also
     * check the transmission state and send queued
     * data, but I think this is not necessary in this case
     */

    return IRQ_HANDLED;
}

//----------------------------------------------

static int __init nuvoton_init_module(void)
{
    int i = 0;

    // Address for Interrupt enable/disable
    unsigned int         *ASC_X_INT_EN     = (unsigned int*)(ASCXBaseAddress + ASC_INT_EN);
    // Address for FiFo enable/disable
    unsigned int         *ASC_X_CTRL       = (unsigned int*)(ASCXBaseAddress + ASC_CTRL);

    dprintk(5, "%s >\n", __func__);

    //Disable all ASC 2 interrupts
    *ASC_X_INT_EN = *ASC_X_INT_EN & ~0x000001ff;

    serial_init();

    init_waitqueue_head(&wq);

    sema_init(&waitForAck, 0);

    for (i = 0; i < LASTMINOR; i++)
        sema_init(&FrontPanelOpen[i].sem, 1);

    //Enable the FIFO
    *ASC_X_CTRL = *ASC_X_CTRL | ASC_CTRL_FIFO_EN;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
    i = request_irq(InterruptLine, (void*)FP_interrupt, IRQF_DISABLED, "FP_serial", NULL);
#else
    i = request_irq(InterruptLine, (void*)FP_interrupt, SA_INTERRUPT, "FP_serial", NULL);
#endif

    if (!i)
        *ASC_X_INT_EN = *ASC_X_INT_EN | 0x00000001;
    else printk("FP: Can't get irq\n");

    msleep(1000);
    nuvoton_init_func();

    if (register_chrdev(VFD_MAJOR, "VFD", &vfd_fops))
        printk("unable to get major %d for VFD/NUVOTON\n",VFD_MAJOR);

    dprintk(10, "%s <\n", __func__);

    return 0;
}


static void __exit nuvoton_cleanup_module(void)
{
    printk("NUVOTON frontcontroller module unloading\n");

    unregister_chrdev(VFD_MAJOR,"VFD");

    free_irq(InterruptLine, NULL);
}


//----------------------------------------------

module_init(nuvoton_init_module);
module_exit(nuvoton_cleanup_module);

MODULE_DESCRIPTION("NUVOTON frontcontroller module");
MODULE_AUTHOR("Dagobert & Schischu & Konfetti");
MODULE_LICENSE("GPL");

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

