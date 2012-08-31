/*
 * aotom.c
 *
 * (c) 2010 Spider-Team
 * (c) 2011 oSaoYa
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
 * fulan front panel driver.
 *
 * Devices:
 *	- /dev/vfd (vfd ioctls and read/write function)
 *
 */


#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/kthread.h>
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

#include "aotom_main.h"

static short paramDebug = 0;
#define TAGDEBUG "[aotom] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
} while (0)

#define INVALID_KEY    	-1
#define LOG_OFF     	0
#define LOG_ON      	1



static char *gmt = "+0000";

typedef struct
{
	struct file*      fp;
	int               read;
	struct semaphore  sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC             1
#define LASTMINOR                 	    2

static tFrontPanelOpen FrontPanelOpen [LASTMINOR];

#define BUFFERSIZE                256     //must be 2 ^ n

struct receive_s
{
   int           len;
   unsigned char buffer[BUFFERSIZE];
};

#define cMaxReceiveQueue	100
static wait_queue_head_t   wq;

struct receive_s receive[cMaxReceiveQueue];
static int receiveCount = 0;

struct semaphore 	   write_sem;
struct semaphore 	   rx_int_sem; /* unused until the irq works */
struct semaphore 	   transmit_sem;
struct semaphore 	   receive_sem;
struct semaphore 	   key_mutex;

static struct semaphore  display_sem;

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


static int VFD_Show_Time(u8 hh, u8 mm)
{
    if( (hh > 24) || (mm > 59))
    {
    	dprintk(2, "%s bad parameter!\n", __func__);
    	return -1;
    }

    return YWPANEL_FP_SetTime(hh*3600 + mm*60);
}

static int VFD_Show_Ico(LogNum_T log_num, int log_stat)
{
	return YWPANEL_VFD_ShowIco(log_num, log_stat);
}

static struct task_struct *thread;
static int thread_stop  = 1;
int aotomSetIcon(int which, int on);

void clear_display(void)
{
	YWPANEL_VFD_ShowString("        ");
}

static void VFD_clr(void)
{
	int i;

	YWPANEL_VFD_ShowTimeOff();
	clear_display();
	for(i=1; i < 45; i++)
		aotomSetIcon(i, 0);
}

void draw_thread(void *arg)
{
  struct vfd_ioctl_data *data;
  struct vfd_ioctl_data draw_data;
  unsigned char buf[17];
  int count = 0;
  int pos = 0;


  data = (struct vfd_ioctl_data *)arg;

  draw_data.length = data->length;
  memset(draw_data.data, 0, sizeof(draw_data.data));
  memcpy(draw_data.data,data->data,data->length);

  thread_stop = 0;

  count = utf8_len(&draw_data.data[0], draw_data.length);

#if defined(SPARK)
  if(count > 4)
#else
  if(count > 8)
#endif
  {
    while(pos < draw_data.length)
    {
       if(kthread_should_stop())
       {
    	   thread_stop = 1;
    	   return;
       }

       clear_display();
       memset(buf,0, sizeof(buf));
       int countb = utf8_count(&draw_data.data[pos], draw_data.length - pos, 8);
       memcpy(buf, &draw_data.data[pos], countb);
       YWPANEL_VFD_ShowString(buf);
       msleep(200);
       if (draw_data.data[pos] > 128)
       {
           pos += 2;
       } else {
           pos += 1;
       }
// #if defined(SPARK)
//        if((count - pos) < 4)
// #else
//        if((count - pos) < 8)
// #endif
//     	   break;
    }
  }

  if(count > 0)
  {
      clear_display();
      memset(buf,0, sizeof(buf));
      int countb = utf8_count(&draw_data.data[0], draw_data.length, 8);
      memcpy(buf, draw_data.data, countb);
      YWPANEL_VFD_ShowString(buf);
  }

  thread_stop = 1;
}

int run_draw_thread(struct vfd_ioctl_data *draw_data)
{
    if(!thread_stop)
      kthread_stop(thread);

    //wait thread stop
    while(!thread_stop)
    {msleep(1);}


    thread_stop = 2;
    thread=kthread_run(draw_thread,draw_data,"draw thread",NULL,true);

    //wait thread run
    while(thread_stop == 2)
    {msleep(1);}

    return 0;
}

static int AOTOMfp_Get_Key_Value(void)
{
	int ret, key_val = INVALID_KEY;

	ret =  YWPANEL_VFD_GetKeyValue();

	switch(ret)
	{
        case 105:
            key_val = KEY_LEFT;
            break;
        case 103:
            key_val = KEY_UP;
            break;
        case 28:
            key_val = KEY_OK;
            break;
        case 106:
            key_val = KEY_RIGHT;
            break;
        case 108:
            key_val = KEY_DOWN;
            break;
        case 88:
            key_val = KEY_POWER;
            break;
        case 102:
            key_val = KEY_MENU;
            break;
        case 48:
            key_val = KEY_EXIT;
            break;
        default:
            key_val = INVALID_KEY;
            break;
    }

	return key_val;
}

int aotomSetTime(char* time)
{
   int res = 0;

	dprintk(5, "%s >\n", __func__);
	dprintk(5, "%s time: %02d:%02d\n", __func__, time[2], time[3]);
	res = VFD_Show_Time(time[2], time[3]);
#if defined(SPARK) || defined(SPARK7162)
   YWPANEL_FP_ControlTimer(true);
#endif
   dprintk(5, "%s <\n", __func__);
   return res;
}

int vfd_init_func(void)
{
	dprintk(5, "%s >\n", __func__);
	printk("Fulan VFD module initializing\n");
	return 0;
}

int aotomSetIcon(int which, int on)
{
	int  res = 0;

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

static ssize_t AOTOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char* kernel_buf;
	int minor, vLoop, res = 0;

	struct vfd_ioctl_data data;

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

	data.length = len;
	if (data.length > VFD_DATA_LEN)
		data.length = VFD_DATA_LEN;

	while ((data.length > 0) && (kernel_buf[data.length - 1 ] == '\n'))
	  data.length--;

	if(data.length <0)
	{
	  res = -1;
	  dprintk(2, "empty string\n");
	}
	else
	{
	  memcpy(data.data,kernel_buf,data.length);
	  res=run_draw_thread(&data);
	}

	kfree(kernel_buf);

	up(&write_sem);

	dprintk(10, "%s < res %d len %d\n", __func__, res, data.length);

	if (res < 0)
	   return res;
	else
	   return len;
}

static ssize_t AOTOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
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
	{
#if defined(SPARK) || defined(SPARK7162)
		res = YWPANEL_VFD_SetLed(aotom->u.led.led_nr, aotom->u.led.on);
		//printk("res = %d\n", res);
#endif
		break;
	}
	case VFDBRIGHTNESS:
		break;
	case VFDICONDISPLAYONOFF:
	{
	 	//struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
		//res = aotomSetIcon(aotom->u.icon.icon_nr, aotom->u.icon.on);
#if defined(SPARK) || defined(SPARK7162)
		switch (aotom->u.icon.icon_nr)
		{
			case 0:
			{
   				struct vfd_ioctl_data * vfd = (struct vfd_ioctl_data *)arg;
				if (5 == vfd->length)
				{
					if ((0x1e & 0xf) == vfd->data[0])
					{
						res = YWPANEL_VFD_SetLed(0, vfd->data[4]);
					}
				}
				break;
			}
			case 35:
				res = YWPANEL_VFD_SetLed(1, aotom->u.led.on);
				break;
			default:
				break;
		}
#endif

		mode = 0;
		break;
	}
	case VFDSTANDBY:
	{
#if defined(SPARK) || defined(SPARK7162)
		u32 uTime = 0;
		u32 uStandByKey = 0;
		u32 uPowerOnTime = 0;
		get_user(uTime, (int *) arg);
		//printk("uTime = %d\n", uTime);

		uPowerOnTime = YWPANEL_FP_GetPowerOnTime();
		//printk("1uPowerOnTime = %d\n", uPowerOnTime);

		YWPANEL_FP_SetPowerOnTime(uTime);

		uPowerOnTime = YWPANEL_FP_GetPowerOnTime();
		//printk("2uPowerOnTime = %d\n", uPowerOnTime);
		#if 0
		uStandByKey = YWPANEL_FP_GetStandByKey(0);
		printk("uStandByKey = %d\n", uStandByKey);
		uStandByKey = YWPANEL_FP_GetStandByKey(1);
		printk("uStandByKey = %d\n", uStandByKey);
		uStandByKey = YWPANEL_FP_GetStandByKey(2);
		printk("uStandByKey = %d\n", uStandByKey);
		uStandByKey = YWPANEL_FP_GetStandByKey(3);
		printk("uStandByKey = %d\n", uStandByKey);
		uStandByKey = YWPANEL_FP_GetStandByKey(4);
		printk("uStandByKey = %d\n", uStandByKey);
		#endif
		YWPANEL_FP_ControlTimer(true);
		YWPANEL_FP_SetCpuStatus(0x02);

#endif
	   break;
	}
	case VFDSETTIME:
		//struct set_time_s *data2 = (struct set_time_s *) arg;
		res = aotomSetTime((char *)arg);
		break;
	case VFDGETTIME:
	{
#if defined(SPARK) || defined(SPARK7162)
		u32 uTime = 0;
		char cTime[5];
		uTime = YWPANEL_FP_GetTime();
		//printk("uTime = %d\n", uTime);
		put_user(uTime, (int *) arg);
#endif
		break;
	}
	case VFDGETWAKEUPMODE:
		break;
	case VFDDISPLAYCHARS:
		if (mode == 0)
		{
	 	  struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
		  if(data->length <0)
	            {
	              res = -1;
	              dprintk(2, "empty string\n");
	            }
		    else
		     res = run_draw_thread(data);
		} else
		{
			//not supported
		}
		mode = 0;
		break;
	case VFDDISPLAYWRITEONOFF:
		break;
	case VFDDISPLAYCLR:
		if(!thread_stop)
		  kthread_stop(thread);
		//wait thread stop
		while(!thread_stop)
		  {msleep(1);}
		VFD_clr();
		break;
#if defined(SPARK)
	case 0x5305:
		break;
#endif
	case 0x5401:
		break;
	default:
		printk("VFD/AOTOM: unknown IOCTL 0x%x\n", cmd);
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

static char *button_driver_name = "fulan front panel buttons";
static struct input_dev *button_dev;
static int button_value = -1;
static int bad_polling = 0;
static struct workqueue_struct *fpwq;

struct semaphore button_sem;

void button_bad_polling(void)
{
	int btn_pressed = 0;

	int report_key = 0;

	while(bad_polling == 1)
	{
		msleep(50);
		button_value = AOTOMfp_Get_Key_Value();
		if (button_value != INVALID_KEY) {
			dprintk(5, "got button: %X\n", button_value);
	        VFD_Show_Ico(DOT2,LOG_ON);
			YWPANEL_VFD_SetLed(1, LOG_ON);
			if (1 == btn_pressed)
			{
				if (report_key != button_value)
				{
					input_report_key(button_dev, report_key, 0);
					input_sync(button_dev);
				}
				else
				{
				    continue;
				}
			}
			report_key = button_value;
	        btn_pressed = 1;
			switch(button_value) {
				case KEY_LEFT:
				case KEY_RIGHT:
				case KEY_UP:
				case KEY_DOWN:
				case KEY_OK:
				case KEY_MENU:
				case KEY_EXIT:
				case KEY_POWER:
					input_report_key(button_dev, button_value, 1);
					input_sync(button_dev);
					break;
				default:
					dprintk(5, "[BTN] unknown button_value?\n");
			}
		}
		else {
			if(btn_pressed) {
				btn_pressed = 0;
				msleep(80);
				VFD_Show_Ico(DOT2,LOG_OFF);
				YWPANEL_VFD_SetLed(1, LOG_OFF);
				input_report_key(button_dev, report_key, 0);
			input_sync(button_dev);
			}
		}
	}
	up(&button_sem);

}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
static DECLARE_WORK(button_obj, button_bad_polling);
#else
static DECLARE_WORK(button_obj, button_bad_polling, NULL);
#endif
static int button_input_open(struct input_dev *dev)
{
	if (down_interruptible(&button_sem))
	{
		printk("[BTN] ERROR workqueue already running\n");
		return 1;
	}

	bad_polling = 1;
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
	down(&button_sem);

	if (fpwq)
	{
		destroy_workqueue(fpwq);
		dprintk(5, "[BTN] workqueue destroyed\n");
	}
	up(&button_sem);
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
	set_bit(KEY_EXIT	, button_dev->keybit);

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

	printk("Fulan front panel driver\n");

	sema_init(&display_sem,1);
	sema_init(&button_sem, 1);

	if(YWPANEL_VFD_Init()) {
		printk("unable to init module\n");
		return -1;
	}

	VFD_clr();
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
	dprintk(5, "[BTN] unloading ...\n");
	button_dev_exit();

	//kthread_stop(time_thread);

	unregister_chrdev(VFD_MAJOR,"VFD");
	printk("Fulan front panel module unloading\n");
}

module_init(aotom_init_module);
module_exit(aotom_cleanup_module);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(gmt,charp,0);
MODULE_PARM_DESC(gmt, "gmt offset (default +0000");

MODULE_DESCRIPTION("VFD module for fulan boxes");
MODULE_AUTHOR("Spider-Team, oSaoYa");
MODULE_LICENSE("GPL");
