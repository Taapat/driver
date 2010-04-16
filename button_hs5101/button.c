/*
 * Homecast 5101 frontpanel buttons driver - 01. Jan 2010 - corev
 * based on ufs910 button driver from captaintrip
 *
 * Buttons are polled through a 74hc164 8-bit shift register
 *
 * concept is to clock in a inverted one-hot (one-cold) bit and
 * read out data in which is high by default and low if button
 * is pressed.
 *
 * the frontpanel has 7 buttons in polling order:
 * - bit 0 = Up/Channel Up
 * - bit 1 = Down/Channel Down
 * - bit 2 = Left/Volume Down
 * - bit 3 = Right/Volume Up
 * - bit 4 = Power
 * - bit 5 = Menu/Ok
 * - bit 6 = Exit/VFormat
 *
 * the 3 pio pins are:
 * - PIO1.5 = Data In
 * - PIO1.6 = Clock
 * - PIO1.7 = Data Out
 *
 */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/stpio.h>

#include "button.h"
#include "pio.h"

#define BUTTON_POLL_MSLEEP 50
#define BUTTON_POLL_DELAY  10

#define BUTTON_CLK_PORT 1
#define BUTTON_CLK_PIN  6
#define BUTTON_DO_PORT  1
#define BUTTON_DO_PIN   5
#define BUTTON_DI_PORT  1
#define BUTTON_DI_PIN   7

#define HS5101_BUTTON_UP    0x01
#define HS5101_BUTTON_DOWN  0x02
#define HS5101_BUTTON_LEFT  0x04
#define HS5101_BUTTON_RIGHT 0x08
#define HS5101_BUTTON_POWER 0x10
#define HS5101_BUTTON_MENU  0x20
#define HS5101_BUTTON_EXIT  0x40

static char                    *button_driver_name = "Homecast 5101 frontpanel buttons";
static struct workqueue_struct *button_wq;
static struct input_dev        *button_dev;
static struct stpio_pin        *button_clk = NULL;
static struct stpio_pin        *button_do = NULL;
static struct stpio_pin        *button_di = NULL;
static int                      button_polling = 1;
static unsigned char            button_value = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
void button_poll(void)
#else
void button_poll(struct work_struct *ignored)
#endif
{
  unsigned char value = 0, change;
  int           i;

  while(button_polling == 1)
  {
    msleep( BUTTON_POLL_MSLEEP );

    stpio_set_pin( button_do,  0 );
    stpio_set_pin( button_clk, 0 );
    udelay( BUTTON_POLL_DELAY );
    stpio_set_pin( button_clk, 1 );
    udelay( BUTTON_POLL_DELAY );

    stpio_set_pin( button_do,  1 );
    for( i=0; i<7; i++ )
    {
      if ( stpio_get_pin( button_di ) == 0 ) value |= 0x80;
      value >>= 1;
      stpio_set_pin( button_clk, 0 );
      udelay( BUTTON_POLL_DELAY );
      stpio_set_pin( button_clk, 1 );
      udelay( BUTTON_POLL_DELAY );
    }

    change = button_value ^ value;
    if ( change & HS5101_BUTTON_UP    ) input_report_key( button_dev, KEY_UP,    ( value & HS5101_BUTTON_UP    ) );
    if ( change & HS5101_BUTTON_DOWN  ) input_report_key( button_dev, KEY_DOWN,  ( value & HS5101_BUTTON_DOWN  ) );
    if ( change & HS5101_BUTTON_LEFT  ) input_report_key( button_dev, KEY_LEFT,  ( value & HS5101_BUTTON_LEFT  ) );
    if ( change & HS5101_BUTTON_RIGHT ) input_report_key( button_dev, KEY_RIGHT, ( value & HS5101_BUTTON_RIGHT ) );
    if ( change & HS5101_BUTTON_POWER ) input_report_key( button_dev, KEY_POWER, ( value & HS5101_BUTTON_POWER ) );
    if ( change & HS5101_BUTTON_MENU  ) input_report_key( button_dev, KEY_MENU,  ( value & HS5101_BUTTON_MENU  ) );
    if ( change & HS5101_BUTTON_EXIT  ) input_report_key( button_dev, KEY_EXIT,  ( value & HS5101_BUTTON_EXIT  ) );
    if ( change ) input_sync( button_dev );

    button_value = value;
  }
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static DECLARE_WORK(button_obj, button_poll, NULL); 
#else
static DECLARE_WORK(button_obj, button_poll); 
#endif
static int button_input_open(struct input_dev *dev)
{
	button_wq = create_workqueue("button");                   
	if(queue_work(button_wq, &button_obj))
	{                    
		dprintk("[BTN] queue_work successful ...\n");
	}
	else
	{
		dprintk("[BTN] queue_work not successful, exiting ...\n");
		return 1;
	}

	return 0;
}

static void button_input_close(struct input_dev *dev)
{
	button_polling = 0;
	msleep(55);
	button_polling = 1;

	if (button_wq)
	{
		destroy_workqueue(button_wq);
		dprintk("[BTN] workqueue destroyed\n");
	}
}

int button_dev_init(void)
{
	int error;

	dprintk("[BTN] allocating and button pio pins\n");
	button_clk = stpio_request_pin( BUTTON_CLK_PORT, BUTTON_CLK_PIN, "btn_clk", STPIO_OUT );
	if ( button_clk == NULL )
	{
	  printk("[BTN] unable to allocate stpio(%d,%d) for btn_clk\n", BUTTON_CLK_PORT, BUTTON_CLK_PIN );
	  return -EIO;
	}

	button_do = stpio_request_pin( BUTTON_DO_PORT, BUTTON_DO_PIN, "btn_do", STPIO_OUT );
	if ( button_do == NULL )
	{
	  printk("[BTN] unable to allocate stpio(%d,%d) for btn_do\n", BUTTON_DO_PORT, BUTTON_DO_PIN );
	  stpio_free_pin( button_clk );
	  return -EIO;
	}

	button_di = stpio_request_pin( BUTTON_DI_PORT, BUTTON_DI_PIN, "btn_di", STPIO_IN );
	if ( button_di == NULL )
	{
	  printk("[BTN] unable to allocate stpio(%d,%d) for btn_di\n", BUTTON_DI_PORT, BUTTON_DI_PIN );
	  stpio_free_pin( button_do  );
	  stpio_free_pin( button_clk );
	  return -EIO;
	}

	dprintk("[BTN] allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name  = button_driver_name;
	button_dev->open  = button_input_open;
	button_dev->close = button_input_close;
	button_value      = 0 ;

	set_bit(EV_KEY,    button_dev->evbit);
	set_bit(KEY_UP,    button_dev->keybit); 
	set_bit(KEY_DOWN,  button_dev->keybit); 
	set_bit(KEY_LEFT,  button_dev->keybit); 
	set_bit(KEY_RIGHT, button_dev->keybit); 
	set_bit(KEY_POWER, button_dev->keybit); 
	set_bit(KEY_MENU,  button_dev->keybit); 
	set_bit(KEY_EXIT,  button_dev->keybit); 

	error = input_register_device(button_dev);
	if (error) {
		input_free_device(button_dev);
		return error;
	}

	return 0;
}

void button_dev_exit(void)
{
	dprintk("[BTN] unregistering button device\n");
	input_unregister_device(button_dev);
	if ( button_clk ) stpio_free_pin( button_clk );
	if ( button_do  ) stpio_free_pin( button_do  );
	if ( button_di  ) stpio_free_pin( button_di  );
}

int __init button_init(void)
{
	dprintk("[BTN] initializing ...\n");

	if(button_dev_init() != 0)
		return 1;

	return 0;
}

void __exit button_exit(void)
{
	dprintk("[BTN] unloading ...\n");

	button_dev_exit();
}

module_init(button_init);
module_exit(button_exit);

MODULE_DESCRIPTION("Homecast 5101 frontpanel buttons driver");
MODULE_AUTHOR("corev");
MODULE_LICENSE("GPL");
