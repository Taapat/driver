/*
 * nuvoton.c
 * 
 * (c) 2009 teamducktales
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
 *	- /dev/vfd (vfd ioctls and read/write function)
 *	- /dev/rc  (reading of key events)
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
#include "../vfd/utf.h"

static short paramDebug = 0;
#define TAGDEBUG "[nuvoton] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
} while (0)

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

struct receive_s receive[cMaxReceiveQueue];

static int receiveCount = 0;

static wait_queue_head_t   wq;
static wait_queue_head_t   task_wq;
static wait_queue_head_t   ioctl_wq;

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

typedef struct
{
	struct file*      fp;
	int               read;
	struct semaphore  sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC             1
#define LASTMINOR                 	    2

static tFrontPanelOpen FrontPanelOpen [LASTMINOR];

enum {
	cStateIdle,
	cStateTransmission,
	cStateWaitAck,
	cStateWaitDataAck,
	cStateWaitEvent,
	cStateWaitStartOfAnswer
};

static int state = cStateIdle;

enum {
		ICON_MIN = 0x0,
		ICON_USB = 0x1,
		ICON_HD,
		ICON_HDD,
		ICON_SCRAMBLED,
		ICON_BLUETOOTH,
		ICON_MP3,
		ICON_RADIO,
		ICON_DOLBY,
		ICON_EMAIL,
		ICON_MUTE,
		ICON_PLAY,
		ICON_PAUSE,
		ICON_FF,
		ICON_REW,
		ICON_REC,
		ICON_TIMER,
/* hdbox only: must be implemented in SystemPlugins/VFD-Icons/plugin.py */
		ICON_TIMESHIFT,
		ICON_TUNER1,
		ICON_MAX
};


struct iconToInternal {
   char  *name;
	u16	icon;
	u8		internalCode1;
	u8		internalCode2;
} nuvotonIcons[] =
{ 
#ifdef icons
0xff means not known or not available
0x38 = Ähm ja ein Icon, nur was stellt es da :D
#endif
   { "ICON_USB"      , ICON_USB       , 0x35, 0xff},
   { "ICON_HD"       , ICON_HD        , 0x34, 0xff},
   { "ICON_HDD"      , ICON_HDD       , 0xff, 0xff},
   { "ICON_SCRAMBLED", ICON_SCRAMBLED , 0x36, 0xff},
   { "ICON_BLUETOOTH", ICON_BLUETOOTH , 0xff, 0xff},
   { "ICON_MP3"      , ICON_MP3       , 0xff, 0xff},
   { "ICON_RADIO"    , ICON_RADIO     , 0xff, 0xff},
   { "ICON_DOLBY"    , ICON_DOLBY     , 0x37, 0xff},
   { "ICON_EMAIL"    , ICON_EMAIL     , 0xff, 0xff},
   { "ICON_MUTE"     , ICON_MUTE      , 0xff, 0xff},
   { "ICON_PLAY"     , ICON_PLAY      , 0xff, 0xff},
   { "ICON_PAUSE"    , ICON_PAUSE     , 0xff, 0xff},
   { "ICON_FF"       , ICON_FF        , 0xff, 0xff},
   { "ICON_REW"      , ICON_REW       , 0xff, 0xff},
   { "ICON_REC"      , ICON_REC       , 0x30, 0xff},
   { "ICON_TIMER"    , ICON_TIMER     , 0x33, 0xff},
   { "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x31, 0x32},
   { "ICON_TUNER1"   , ICON_TUNER1    , 0x39, 0xff},
};

#define cLenHeader            3
#define cLenRCUCommand        7
#define cLenFPCommand         6
#define cLenStandByCommand    5

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
#define cCommandSetIcon          0xc4 //0xc2
#else
#define cCommandSetIcon          0xc2
#endif

#ifdef OCTAGON1008
#define cCommandSetVFD           0xc4
#else
#define cCommandSetVFD           0xce /* 0xc0 */
#endif

#define cCommandSetVFDBrightness 0xd2

#define cCommandGetFrontInfo     0xe0

#define cCommandSetPwrLed        0x93 /* added by zeroone, only used in this file; set PowerLed Brightness on HDBOX */

/* from the fp */
#define cEventFP           0x51 /* fp buttons */
#define cEventRCU	   0x63 /* rcu keys */
#define cEventStandby	   0x80 /* power off selected; disabled in our case */

/* from the fp */
#define cAnswerGetMJD        0x15
#define cAnswerGetPowerOnSrc 0x81
#define cAnswerGetFrontInfo  0xe4
#define cAnswerGetGetIrCode  0xa5
#define cAnswerGetPort       0xb3

/* general */
#define SOP 2
#define EOP 3

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
   {0x00, 0x00, ' '},
};

#endif

int nuvotonWriteString(unsigned char* aBuf, int len);


/* ************************************************** */
/* Access ASC3; from u-boot; copied from TF7700 ;-)   */
/* ************************************************** */
#define ASC0BaseAddress 0xb8030000
#define ASC1BaseAddress 0xb8031000
#define ASC2BaseAddress 0xb8032000
#define ASC3BaseAddress 0xb8033000
#define ASC_BAUDRATE    0x000
#define ASC_TX_BUFF     0x004
#define ASC_RX_BUFF     0x008
#define ASC_CTRL        0x00c
#define ASC_INT_EN      0x010
#define ASC_INT_STA     0x014
#define ASC_GUARDTIME   0x018
#define ASC_TIMEOUT     0x01c
#define ASC_TX_RST      0x020
#define ASC_RX_RST      0x024
#define ASC_RETRIES     0x028

#define ASC_INT_STA_RBF   0x01
#define ASC_INT_STA_TE    0x02
#define ASC_INT_STA_THE   0x04
#define ASC_INT_STA_PE    0x08
#define ASC_INT_STA_FE    0x10
#define ASC_INT_STA_OE    0x20
#define ASC_INT_STA_TONE  0x40
#define ASC_INT_STA_TOE   0x80
#define ASC_INT_STA_RHF   0x100
#define ASC_INT_STA_TF    0x200
#define ASC_INT_STA_NKD   0x400

#define ASC_CTRL_FIFO_EN  0x400

/*  GPIO Pins  */

#define PIO0BaseAddress   0xb8020000
#define PIO1BaseAddress   0xb8021000
#define PIO2BaseAddress   0xb8022000
#define PIO3BaseAddress   0xb8023000
#define PIO4BaseAddress   0xb8024000
#define PIO5BaseAddress   0xb8025000

#define PIO_CLR_PnC0      0x28
#define PIO_CLR_PnC1      0x38
#define PIO_CLR_PnC2      0x48
#define PIO_CLR_PnCOMP    0x58
#define PIO_CLR_PnMASK    0x68
#define PIO_CLR_PnOUT     0x08
#define PIO_PnC0          0x20
#define PIO_PnC1          0x30
#define PIO_PnC2          0x40
#define PIO_PnCOMP        0x50
#define PIO_PnIN          0x10
#define PIO_PnMASK        0x60
#define PIO_PnOUT         0x00
#define PIO_SET_PnC0      0x24
#define PIO_SET_PnC1      0x34
#define PIO_SET_PnC2      0x44
#define PIO_SET_PnCOMP    0x54
#define PIO_SET_PnMASK    0x64
#define PIO_SET_PnOUT     0x04

static int serial2_putc (char Data)
{
  char                  *ASC_2_TX_BUFF = (char*)(ASC2BaseAddress + ASC_TX_BUFF);
  unsigned int          *ASC_2_INT_STA = (unsigned int*)(ASC2BaseAddress + ASC_INT_STA);
  unsigned long         Counter = 200000;

  while (((*ASC_2_INT_STA & ASC_INT_STA_THE) == 0) && --Counter);
  
  if (Counter == 0)
  {
  	dprintk(1, "Error writing char (%c) \n", Data);
//  	*(unsigned int*)(ASC2BaseAddress + ASC_TX_RST)   = 1;
//	return 0;
  }
  
  *ASC_2_TX_BUFF = Data;
  return 1;
}

static u16 serial2_getc(void)
{
  unsigned char         *ASC_2_RX_BUFF = (unsigned char*)(ASC2BaseAddress + ASC_RX_BUFF);
  unsigned int          *ASC_2_INT_STA = (unsigned int*)(ASC2BaseAddress + ASC_INT_STA);
  unsigned long         Counter = 10000;
  u16                   result;
  
  while (((*ASC_2_INT_STA & ASC_INT_STA_RBF) == 0) && --Counter);
  
  /* return an error code for timeout in the MSB */
  if (Counter == 0)
      result = 0x01 << 8;
  else
      result = 0x00 << 8 | *ASC_2_RX_BUFF;
		
  dprintk(100, "0x%04x\n", result);		
  return result;
}


static void serial2_init (void)
{
  /* Configure the PIO pins */
  stpio_request_pin(4, 3,  "ASC_TX", STPIO_ALT_OUT); /* Tx */
  stpio_request_pin(4, 2,  "ASC_RX", STPIO_IN);      /* Rx */

#if 0
  *(unsigned int*)(PIO4BaseAddress + PIO_CLR_PnC0) = 0x03/*0x07*/;
  *(unsigned int*)(PIO4BaseAddress + PIO_CLR_PnC1) = 0x38 /*0x06*/;
  *(unsigned int*)(PIO4BaseAddress + PIO_SET_PnC1) = 0x38 /*0x01*/;
  *(unsigned int*)(PIO4BaseAddress + PIO_SET_PnC2) = 0xac /*0x07*/;
#endif

  *(unsigned int*)(ASC2BaseAddress + ASC_INT_EN)   = 0x00000000;
  *(unsigned int*)(ASC2BaseAddress + ASC_CTRL)     = 0x00001589;
  *(unsigned int*)(ASC2BaseAddress + ASC_TIMEOUT)  = 0x00000010;
  *(unsigned int*)(ASC2BaseAddress + ASC_BAUDRATE) = 0x000000c9;
  *(unsigned int*)(ASC2BaseAddress + ASC_TX_RST)   = 0;
  *(unsigned int*)(ASC2BaseAddress + ASC_RX_RST)   = 0;
}

/* process commands where we expect data from frontcontroller
 * e.g. getTime and getWakeUpMode
 */
int processDataAck(char* data, int count)
{

   if (count == transmit[0].ack_len - transmit[0].ack_len_header)
   {
	    /* 0. claim semaphore */
	    down_interruptible(&receive_sem);

	    /* 1. copy data */
	    memcpy(ioctl_data, data, transmit[0].ack_len);

	    /* 2. free semaphore */
	    up(&receive_sem);

	    /* 3. wake up thread */
	    dataReady = 1;
	    wake_up_interruptible(&ioctl_wq);

	    return 1;
   }

   return 0;
}

/* process event data from frontcontroller. An event is
 * defined as something which is _not_ initiated
 * by a command _to_ the frontcontroller.
 */
int processEvent(unsigned char* data, int count)
{
   int i;
	
	dprintk(100, "data[0] = 0x%02x, c = %d, maxLen %d\n", data[0], count, cLenRCUCommand - cLenHeader + 1);
	
	switch (data[0])
	{
		case cEventRCU:  /* rcu */

         if ((data[count - 1] == EOP) && (count - 1 != 5) && (count != cLenRCUCommand - cLenHeader + 1))
         {
		      /* there seems to be sometimes an EOP in the middle, which is 
				 * really ment as stopping the reception. but an EOP (0x03) is
				 * also the key left code. :-/
				 */
		      return 2;
			}

			if (count != cLenRCUCommand - cLenHeader + 1)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(20, "command RCU complete (count = %d)\n", count);

         for (i = 0; i < count; i++)
			   dprintk(100, "0x%02x ", data[i]);
		   dprintk(100, "\n");
 
			/* 1. copy data */
			
			if (receiveCount == cMaxReceiveQueue)
			{
			 	printk("receive queue full!\n");
				/* 2. free semaphore */
				up(&receive_sem);
				return 1;
			}	
			
			memcpy(receive[receiveCount].buffer, data, cLenRCUCommand - cLenHeader);
			receive[receiveCount].len = cLenRCUCommand - cLenHeader;

			receiveCount++;
			
			/* 2. free semaphore */
			up(&receive_sem);
			
			/* 3. wake up threads */
			wake_up_interruptible(&wq);
			return 1;
		break;
		case cEventFP:  /* fp button */

/* fixme: also respect the EOP here */
             
			if (count != cLenFPCommand - cLenHeader + 1)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(20, "command FP Button complete\n");

			/* 1. copy data */
			
			if (receiveCount == cMaxReceiveQueue)
			{
			 	printk("receive queue full!\n");
				/* 2. free semaphore */
				up(&receive_sem);
				return 1;
			}	
			
			memcpy(receive[receiveCount].buffer, data, cLenFPCommand - cLenHeader);
			receive[receiveCount].len = cLenFPCommand - cLenHeader;
			
			receiveCount++;
			
			/* 2. free semaphore */
			up(&receive_sem);
			
			/* 3. wake up threads */
			wake_up_interruptible(&wq);
			return 1;
		break;
		case cEventStandby:  /* rcu & fp */

			if (count != cLenStandByCommand - cLenHeader + 1)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(20, "command standby complete\n");

			/* 1. copy data */
			
			if (receiveCount == cMaxReceiveQueue)
			{
			 	printk("receive queue full!\n");
				/* 2. free semaphore */
				up(&receive_sem);
				return 1;
			}	
			
			memcpy(receive[receiveCount].buffer, data, cLenStandByCommand - cLenHeader);
			receive[receiveCount].len = cLenStandByCommand - cLenHeader;
			
			receiveCount++;
			
			/* 2. free semaphore */
			up(&receive_sem);
			
			/* 3. wake up threads */
			wake_up_interruptible(&wq);
			return 1;
	      break;
	}
	
	return 0;
}

/* detect the start of an event from the frontcontroller */
int detectEvent(unsigned char c[], int count)
{
   int vLoop;
	
   for (vLoop = 0; vLoop < count - 1; vLoop++)
   {
       if (c[vLoop] == SOP)
       {
          switch (c[vLoop + 1])
          {
              case cEventRCU:
              case cEventFP:
              case cEventStandby:
                 dprintk(10, "detect start\n");
                 return 1;
              break;				
          }
       }
   }
			
   return 0;
}


/* search for the start of an answer of an command
 * we have send to the frontcontroller where we
 * expect data from the controller.
 */
int searchAnswerStart(unsigned char c[], int count)
{
   int vLoop, found;
	
   found = 1;
   for (vLoop = 0; vLoop < count; vLoop++)
   {
       if (c[vLoop] != transmit[0].ack_buffer[vLoop])
       {
           found = 0;
           break;
       }
   }
			
   return found;
}

/* if the frontcontroller has returned an error code
 * or if the trial counter overflows then we requeue
 * the data for currently 5 times. the command will
 * be re-send automatically to the controller.
 */
void requeueData(void)
{
      dprintk(10, "requeue data %d\n", state);
      transmit[0].requeueCount++;

      if (transmit[0].requeueCount == cMaxQueueCount)
      {
	      printk("max requeueCount reached aborting transmission %d\n", state);

	      transmitCount--;

	      memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	      if (transmitCount != 0)		  
     	         dprintk(10, "next command will be 0x%x\n", transmit[0].buffer[1]);

	      dataReady = 0;
	      timeoutOccured = 1;
	      
			/* 3. wake up thread */
	      wake_up_interruptible(&ioctl_wq);
      
      }
}

int fpReceiverTask(void* dummy)
{
  unsigned int         *ASC_2_INT_EN = (unsigned int*)(ASC2BaseAddress + ASC_INT_EN);
  unsigned char c;
  int count = 0;
  unsigned char receivedData[BUFFERSIZE];
  int timeout = 0;
  u16 res;
  
  daemonize("fp_rcv");

  allow_signal(SIGTERM);

  //we are on so enable the irq  
  *ASC_2_INT_EN = *ASC_2_INT_EN | 0x00000001;

  while(1)
  {
	    dprintk(200, "w ");
	  
       down(&transmit_sem);

	    res = serial2_getc();
		  
   	 /* timeout ? */
   	 if ((res & 0xff00) != 0)
		 {
           timeout = 1;
		 } else
           timeout = 0;

		 c = res & 0x00ff; 
		  
	    if ((timeout) && ((state == cStateIdle) || (state == cStateWaitEvent)))
		 {
		    dprintk(150, "t ");
          up(&transmit_sem);
			 
			 msleep(150);
		    continue;
		 }
		  
	    dprintk(100, "(0x%x) ", c);  
	
	    /* process our state-machine */
	    switch (state)
	    {
     	    case cStateIdle: /* nothing do to, search for command start */

             dprintk(20, "cStateIdle: count %d, 0x%02x\n", count, c);
        		 receivedData[count++] = c;
     		    
				 if (c == EOP)
				 {
                   printk("receiving unexpected EOP - resetting\n");
						 count = 0;
                   state = cStateIdle;
				 }
				 else
				 if (detectEvent(receivedData, count))
     		    {
				    count = 0;
					 
					 dprintk(20, "receive[%d]: 0x%02x\n", count, c);
        		    receivedData[count++] = c;
			       state = cStateWaitEvent;
     		    } else
				 if (count == BUFFERSIZE-1)
				    count = 0;
	       break;
     	    case cStateWaitDataAck: /* each getter */
	       {
		       int err;

				 if (timeout)
				   err = 0;
			    else
				 {
            	 receivedData[count++] = c;
		      	 err = processDataAck(receivedData, count);
             }
				 
		       if (err == 1)
		       {
	    	       /* data is processed remove it from queue */
		          transmitCount--;

	             memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	             if (transmitCount != 0)		  
     	  		       dprintk(10, "next command will be 0x%x\n", transmit[0].buffer[1]);

	 	          waitAckCounter = 0;
     	 	       dprintk(20, "detect ACK %d\n", state);
		          state = cStateIdle;
		          count = 0;
		       }
	       }
	       break;
     	    case cStateWaitEvent: /* rcu, button or other event */
		   	if (receiveCount < cMaxReceiveQueue)
		   	{
				   int err;
					
					dprintk(20, "receive[%d]: 0x%02x\n", count, c);

        	   	receivedData[count++] = c;
                
					err = processEvent(receivedData, count);

					if (err == 1)
			   	{
			   	   	/* command completed */
                  	count = 0;
                  	state = cStateIdle;
			   	} else
					if (err == 2)
					{
                   printk("receiving unexpected EOP - resetting\n");
						 count = 0;
                   state = cStateIdle;
					}
					
               if (count >= cLenRCUCommand)
					{
                     /* something went wrong ! */
                  	count = 0;
                  	state = cStateIdle;
/* fixme: we must change in a new state and wait
 * until we've got the EOP, otherwise we are not
 * clear!
 */							
							printk("event overflow - resetting\n");
					}

		   	} else
		   	{
			   	/* noop: wait that someone reads data */
            	dprintk(1, "overflow, wait for readers\n");
		   	}
	       break;
     	    case cStateWaitStartOfAnswer: /* each getter */
	       {
             int err;
				 
				 if (timeout)
				    err = 0;
			    else
             {
            	 dprintk(100, "count %d, 0x%02x\n", count, c);
        			 receivedData[count++] = c;

					 if (count < transmit[0].ack_len_header)
					 {
	                up(&transmit_sem);
				   	 continue;
                }
		      	 err = searchAnswerStart(receivedData, count);
             }


		       if (err == 1)
		       {
			        /* answer start detected now process data */
        		     count = 0;
			        state = cStateWaitDataAck;
		       } else
		       {
		   	    /* no answer start detected */
	 	          udelay(1);
	             waitAckCounter--;

		          dprintk(10, "2. %d\n", waitAckCounter);

			       if (waitAckCounter <= 0)
			       {
			          dprintk(10, "missing ACK from micom ->requeue data %d\n", state);
			          requeueData();

			          count = 0;
			          waitAckCounter = 0;
			          state = cStateIdle;
			       }
					 
				    if (count == BUFFERSIZE-1)
				       count = 0;
					 
		       }
	       }
	       break;
	       case cStateTransmission:
		   	/* we currently transmit data so ignore all 
				 * ->should not happen because we use semaphore!
				 */
	       break;

	    }
	    up(&transmit_sem);
  }

  printk("fpReceiverTask died !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

  return 0;
}

int fpTransmitterTask(void* dummy)
{
  char 	nuvoton_cmd[128];
  int  	vLoop;	

  daemonize("fp_transmit");

  allow_signal(SIGTERM);

  while(1)
  {
     //wait_event_interruptible(task_wq, (waitAck == 0) && (transmitCount != 0));
     
     /* only send new command if we dont wait already for acknowledge
      * or we dont wait for a command/answer completion and we have
      * something to transmit
      */
     if ((state == cStateIdle) && (transmitCount > 0))
     {
     	  int sendFailed = 0;
	  
     	  /* 0. claim sema */
	     down_interruptible(&transmit_sem);
     
	     /* fixme: the state may have changed while we sleep ...
		   * I'm not sure if this is a proper solution?!?!
			*/
	     if (state != cStateIdle)
		  {
		      up(&transmit_sem);
				continue;
		  }
	  
     	  dprintk(1, "send data to fp (0x%x)\n", transmit[0].buffer[0]);

		  /* 1. send it to the frontpanel */
		  memset(nuvoton_cmd, 0, 128);

		  memcpy(nuvoton_cmd, transmit[0].buffer, transmit[0].len);

		  state = cStateTransmission;

		  for(vLoop = 0 ; vLoop < transmit[0].len; vLoop++)
		  {	
	        udelay(100);
		     if (serial2_putc((nuvoton_cmd[vLoop])) == 0)
		     {
			     printk("%s failed < char = %c \n", __func__, nuvoton_cmd[vLoop]);
			     state = cStateIdle;
			     sendFailed = 1;
			     break;
		     } else
		   	  dprintk(100, "<0x%x> ", nuvoton_cmd[vLoop]);
		  }

		  if (sendFailed == 0)
		  {
	   	  if (transmit[0].needAck)
	   	  {
	           waitAckCounter = cMaxAckAttempts;

		        timeoutOccured = 0;
	  	        state = cStateWaitStartOfAnswer;
           } else
	   	  {
	  	        /* no acknowledge needed so remove it direct */
		        transmitCount--;

	           memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	           if (transmitCount != 0)		  
				  {
     	  	        
#ifdef OCTAGON1008
                 if (transmit[0].buffer[1] == 0xc4)
				        dprintk(1, "%s: next command will be 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__, transmit[0].buffer[0], 
						                       transmit[0].buffer[1], transmit[0].buffer[2],
													  transmit[0].buffer[3], transmit[0].buffer[4],
													  transmit[0].buffer[5], transmit[0].buffer[6]);
                 else
#endif
				        dprintk(1, "%s: next command will be 0x%x\n", __func__, transmit[0].buffer[1]);
             					                      
				  }
				  
		        state = cStateIdle;
	   	  }
		  }

		  /* 4. free sem */
		  up(&transmit_sem);
     } else
     {
     	  dprintk(100, "%d, %d\n", state, transmitCount);
        msleep(100);
     }
  }

  return 0;
}


/* End ASC2 */

int nuvotonWriteCommand(char* buffer, int len, int needAck, char* ack_buffer, int ack_len_header, int ack_len)
{

	dprintk(5, "%s >\n", __func__);

	/* 0. claim semaphore */
	if (down_interruptible(&transmit_sem))
	   return -ERESTARTSYS;;
	
	if (transmitCount < cMaxTransQueue)
	{
	   /* 1. setup new entry */
	   memset(transmit[transmitCount].buffer, 0, 128);
	   memcpy(transmit[transmitCount].buffer, buffer, len);

	   transmit[transmitCount].len = len;
	   transmit[transmitCount].needAck = needAck;
	   transmit[transmitCount].requeueCount = 0;

	   memset(transmit[transmitCount].ack_buffer, 0, 128);
	   memcpy(transmit[transmitCount].ack_buffer, ack_buffer, ack_len_header);
	   transmit[transmitCount].ack_len = ack_len;
	   transmit[transmitCount].ack_len_header = ack_len_header;

	   transmitCount++;

	} else
	{
		printk("error transmit queue overflow");
	}

	/* 2. free semaphore */
	up(&transmit_sem);

	dprintk(5, "%s < \n", __func__);
	
	return 0;
}

#ifdef OCTAGON1008
int nuvotonSetIcon(int which, int on)
{
	char buffer[128];
   int  res = 0, i;
		
	dprintk(5, "%s > %d, %d\n", __func__, which, on);

	if (which < 0 || which >= 8)
	{
		printk("VFD/Nuvoton icon number out of range %d\n", which);
		return -EINVAL;
		//Trick: symbole left
/*		memset(buffer, 0, 128);

		buffer[0] = SOP;
		buffer[1] = cCommandSetIcon;
		buffer[2] = 8;
		if (on)
			buffer[3] = 0x01;
		else	
			buffer[3] = 0x00;

		buffer[4] = which;//0x00;
		buffer[5] = 0x00;
		buffer[6] = EOP;

		res = nuvotonWriteCommand(buffer, 7, 0, NULL, 0, 0);
		
		return res;*/
	}
	printk("VFD/Nuvoton icon number %d\n", which);
	memset(buffer, 0, 128);

	buffer[0] = SOP;
	buffer[1] = cCommandSetIcon;

	if (on){
		buffer[3] = 0x01;
		vfdbuf[0].aktiv = 1;
	}else
	{
		buffer[3] = 0x00;
		vfdbuf[0].aktiv = 0;
	}
//TRICK: FIXME
	switch (which){
		case 0x00:
			vfdbuf[0].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[0].buf1;
			buffer[5] = vfdbuf[0].buf2;
			break;
		case 0x01:
			vfdbuf[1].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[1].buf1;
			buffer[5] = vfdbuf[1].buf2;
			break;
		case 0x02:
			vfdbuf[2].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[2].buf1;
			buffer[5] = vfdbuf[2].buf2;
			break;
		case 0x03:
			vfdbuf[3].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[3].buf1;
			buffer[5] = vfdbuf[3].buf2;
			break;
		case 0x04:
			vfdbuf[4].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[4].buf1;
			buffer[5] = vfdbuf[4].buf2;
			break;
		case 0x05:
			vfdbuf[5].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[5].buf1;
			buffer[5] = vfdbuf[5].buf2;
			break;
		case 0x06:
			vfdbuf[6].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[6].buf1;
			buffer[5] = vfdbuf[6].buf2;
			break;
		case 0x07:
			vfdbuf[7].which = which;
			buffer[2] = which;
			buffer[4] = vfdbuf[7].buf1;
			buffer[5] = vfdbuf[7].buf2;
			break;
	}
	buffer[6] = EOP;
	//printk("Icon 0x%02x 0x%02x 0x%02x \n",buffer[2],buffer[4],buffer[5]);
	res = nuvotonWriteCommand(buffer, 7, 0, NULL, 0, 0);

	dprintk(5, "%s <\n", __func__);

   return res;
}
#else
int nuvotonSetIcon(int which, int on)
{
	char buffer[128];
	u8   internalCode1, internalCode2;
   int  vLoop, res = 0;
		
	dprintk(5, "%s > %d, %d\n", __func__, which, on);

//fixme
	if (which < 1 || which >= ICON_MAX)
	{
		printk("VFD/Nuvoton icon number out of range %d\n", which);
		return -EINVAL;
	}

   internalCode1 = 0xff;
   internalCode2 = 0xff;
   for (vLoop = 0; vLoop < ARRAY_SIZE(nuvotonIcons); vLoop++)
   {
       if ((which & 0xff) == nuvotonIcons[vLoop].icon)
		 {
		     internalCode1 = nuvotonIcons[vLoop].internalCode1;
		     internalCode2 = nuvotonIcons[vLoop].internalCode2;
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
	buffer[1] = cCommandSetIcon;
	buffer[2] = internalCode1;
   if (on)
	  buffer[3] = 0x02;
   else	
	  buffer[3] = 0x00;

	buffer[4] = EOP;

	res = nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);

   /* e.g. timeshift is splitted in two parts */
   if (internalCode2 != 0xff)
	{
	  memset(buffer, 0, 128);

	  buffer[0] = SOP;
	  buffer[1] = cCommandSetIcon;
	  buffer[2] = internalCode2;
     if (on)
		 buffer[3] = 0x02;
     else	
		 buffer[3] = 0x00;

	  buffer[4] = EOP;

	  res = nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);
	}
	
	dprintk(5, "%s <\n", __func__);

   return res;
}
#endif

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetIcon);

int nuvotonSetLED(int which, int on)
{
	char buffer[8];
	int res = 0;
	
	dprintk(5, "%s > %d, %d\n", __func__, which, on);

//FIXME
	if (which < 1 || which > 6)
	{
		printk("VFD/Nuvoton led number out of range %d\n", which);
		return -EINVAL;
	}

	memset(buffer, 0, 8);

//fixme
	
	dprintk(5, "%s <\n", __func__);

   return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetLED);

int nuvotonSetBrightness(int level)
{
	char buffer[8];
	int res = 0;
	
	dprintk(5, "%s > %d\n", __func__, level);

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
	
	res = nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);

	dprintk(5, "%s <\n", __func__);

   return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetBrightness);

// BEGIN SetPwrLed
int nuvotonSetPwrLed(int level)
{
	char buffer[8];
	int res = 0;
	
	dprintk(5, "%s > %d\n", __func__, level);

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
	
	res = nuvotonWriteCommand(buffer, 6, 0, NULL, 0, 0);

	dprintk(5, "%s <\n", __func__);

   return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetPwrLed);
// END SetPwrLed
int nuvotonSetStandby(char* time)
{
	char     buffer[8];
   char     power_off[] = {SOP, cCommandPowerOffReplay, 0x01, EOP};
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

  	res = nuvotonWriteString("Bye bye ...", strlen("Bye bye ..."));

   /* set wakeup time */
	memset(buffer, 0, 8);

	buffer[0] = SOP;
	buffer[1] = cCommandSetWakeupMJD;

	memcpy(buffer + 2, time, 4); /* only 4 because we do not have seconds here */
	buffer[6] = EOP;
	
	res = nuvotonWriteCommand(buffer, 7, 0, NULL, 0, 0);

   /* now power off */
   res = nuvotonWriteCommand(power_off, sizeof(power_off), 0, NULL, 0, 0);

	dprintk(5, "%s <\n", __func__);

   return res;
}

int nuvotonSetTime(char* time)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	buffer[0] = SOP;
	buffer[1] = cCommandSetMJD;
	
	memcpy(buffer + 2, time, 5);
	buffer[7] = EOP;
	
	res = nuvotonWriteCommand(buffer, 8, 0, NULL, 0, 0);
	
	dprintk(5, "%s <\n", __func__);

   return res;
}
 
int nuvotonGetTime(void)
{
	char 	   buffer[3];
	char 	   ack_buffer[3];
   int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 3);
	
	buffer[0] = SOP;
	buffer[1] = cCommandGetMJD;
	buffer[2] = EOP;

   /* the replay sequence */
	ack_buffer[0] = 0x00;
	ack_buffer[1] = SOP;
	ack_buffer[2] = cAnswerGetMJD;
	
	dataReady = 0;
	res = nuvotonWriteCommand(buffer, 3, 1, ack_buffer, 3, 8);
	
	wait_event_interruptible(ioctl_wq, dataReady || timeoutOccured);
	
	if (timeoutOccured == 1)
	{
		/* timeout */
		memset(ioctl_data, 0, 8);
		printk("timeout\n");
	
	   res = -ETIMEDOUT;
	} else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
		dprintk(20, "myTime= 0x%02x - 0x%02x - 0x%02x - 0x%02x - 0x%02x\n", ioctl_data[0], ioctl_data[1]
		         , ioctl_data[2], ioctl_data[3], ioctl_data[4]);
	}
	 
	dprintk(5, "%s <\n", __func__);

   return res;
}

int nuvotonGetWakeUpMode(void)
{
	char 	   buffer[8];
	char 	   ack_buffer[3];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	
	buffer[0] = SOP;
	buffer[1] = cCommandGetPowerOnSrc;
	buffer[2] = EOP;

   /* the replay sequence */
	ack_buffer[0] = 0x00;
	ack_buffer[1] = SOP;
	ack_buffer[2] = cAnswerGetPowerOnSrc;

	dataReady = 0;
	res = nuvotonWriteCommand(buffer, 3, 1, ack_buffer, 3, 5);
	
	wait_event_interruptible(ioctl_wq, dataReady || timeoutOccured);
	
	if (timeoutOccured == 1)
	{
		/* timeout */
		memset(ioctl_data, 0, 8);
		printk("timeout\n");

	   res = -ETIMEDOUT;
	} else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}
	 
	dprintk(5, "%s <\n", __func__);

   return res;
}

#ifndef OCTAGON1008
int nuvotonWriteString(unsigned char* aBuf, int len)
{
	unsigned char bBuf[128];
	int i =0;
	int j =0;
   int res = 0;
	
	dprintk(5, "%s >\n", __func__);
	
	memset(bBuf, ' ', 128);
	
	/* start of command write */
	bBuf[0] = SOP;
	bBuf[1] = cCommandSetVFD;
	bBuf[2] = 0x11;
	
	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;
	
	dprintk(10, "len %d\n", len);
	
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
	
	res = nuvotonWriteCommand(bBuf, 17, 0, NULL, 0, 0);

	dprintk(5, "%s <\n", __func__);

   return res;
}
#else

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

	dprintk(5, "%s > %d\n", __func__, len);
	
	memset(bBuf, ' ', 128);

	max = (len > 8) ? 8 : len;
	printk("max = %d\n", max);

	//clean display text
	for(i=0;i<8;i++)
	{
		bBuf[0] = SOP;
	   	bBuf[2] = 7 - i; /* position: 0x00 = right */
		bBuf[1] = cCommandSetIcon;
		bBuf[3] = 0x00;
		bBuf[4] = 0x00;
		bBuf[5] = 0x00;
		bBuf[6] = EOP;
		nuvotonWriteCommand(bBuf, 8, 0, NULL, 0, 0);
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
	           res |= nuvotonWriteCommand(bBuf, 8, 0, NULL, 0, 0);
			     break;
			 }
		}
		//printk(" 0x%02x,0x%02x,0x%02x\n",vfdbuf[7 - i].pos,vfdbuf[7 - i].buf1,vfdbuf[7 - i].buf2);
	}
	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;
	
	dprintk(10, "len %d\n", len);

	dprintk(5, "%s <\n", __func__);

   return res;
}

#endif

int nuvoton_init_func(void)
{
   char standby_disable[] = {SOP, cCommandPowerOffReplay, 0x02, EOP};
   char init1[] = {SOP, cCommandSetBootOn, EOP};
   char init2[] = {SOP, cCommandSetTimeFormat, 0x81, EOP};
   char init3[] = {SOP, cCommandSetWakeupTime, 0xff, 0xff, EOP}; /* delete/invalidate wakeup time ? */
   char init4[] = {SOP, 0x93, 0x01, 0x00, 0x08, EOP};
#ifdef OCTAGON1008
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

	dprintk(5, "%s >\n", __func__);

	printk("Fortis HDBOX VFD/Nuvoton module initializing\n");

   /* must be called before standby_disable */
	res = nuvotonWriteCommand(init1, sizeof(init1), 0, NULL, 0, 0);

	/* setup: frontpanel should not power down the receiver if standby is selected */
	res = nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0, NULL, 0, 0);

	res |= nuvotonWriteCommand(init2, sizeof(init2), 0, NULL, 0, 0);
	res |= nuvotonWriteCommand(init3, sizeof(init3), 0, NULL, 0, 0);
	res |= nuvotonWriteCommand(init4, sizeof(init4), 0, NULL, 0, 0);
	res |= nuvotonWriteCommand(init5, sizeof(init5), 0, NULL, 0, 0);

	res |= nuvotonSetBrightness(1);

  	res |= nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));
	
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++) 
		res |= nuvotonSetIcon(vLoop, 0);
		
	dprintk(5, "%s <\n", __func__);

	return 0;
}

static ssize_t NUVOTONdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
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

   if(down_interruptible (&write_sem))
      return -ERESTARTSYS;

   /* Dagobert: echo add a \n which will be counted as a char
	 */ 
	if (kernel_buf[len - 1] == '\n')
	   res = nuvotonWriteString(kernel_buf, len - 1);
   else
	   res = nuvotonWriteString(kernel_buf, len);

	kfree(kernel_buf);
	
	up(&write_sem);

	dprintk(10, "%s < res %d len %d\n", __func__, res, len);
	
	if (res < 0)
	   return res;
	else
	   return len;
}

static ssize_t NUVOTONdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;

	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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

int NUVOTONdev_open(struct inode *inode, struct file *filp)
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

int NUVOTONdev_close(struct inode *inode, struct file *filp)
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

static int NUVOTONdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct nuvoton_ioctl_data * nuvoton = (struct nuvoton_ioctl_data *)arg;
   int res = 0;

	dprintk(5, "%s > 0x%.8x\n", __func__, cmd);

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

	dprintk(5, "%s <\n", __func__);
	return res;
}

static unsigned int NUVOTONdev_poll(struct file *filp, poll_table *wait)
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
	.ioctl = NUVOTONdev_ioctl,
	.write = NUVOTONdev_write,
	.read  = NUVOTONdev_read,
  	.poll  = (void*) NUVOTONdev_poll,
	.open  = NUVOTONdev_open,
	.release  = NUVOTONdev_close
};

static int __init nuvoton_init_module(void)
{	
  unsigned int         *ASC_2_INT_EN = (unsigned int*)(ASC2BaseAddress + ASC_INT_EN);
  unsigned int         *ASC_2_CTRL   = (unsigned int*)(ASC2BaseAddress + ASC_CTRL);
  int i;
  
  dprintk(5, "%s >\n", __func__);

  //Disable all ASC 2 interrupts
  *ASC_2_INT_EN = *ASC_2_INT_EN & ~0x000001ff;

  serial2_init();

  sema_init(&rx_int_sem, 0);
  sema_init(&write_sem, 1);
  sema_init(&receive_sem, 1);
  sema_init(&transmit_sem, 1);

  for (i = 0; i < LASTMINOR; i++)
    sema_init(&FrontPanelOpen[i].sem, 1);

  init_waitqueue_head(&wq);
  init_waitqueue_head(&task_wq);
  init_waitqueue_head(&ioctl_wq);

  //Enable the FIFO
  *ASC_2_CTRL = *ASC_2_CTRL | ASC_CTRL_FIFO_EN;

  kernel_thread(fpReceiverTask, NULL, 0);
  kernel_thread(fpTransmitterTask, NULL, 0);
 
  msleep(1000);
  nuvoton_init_func();

  if (register_chrdev(VFD_MAJOR,"VFD",&vfd_fops))
	 printk("unable to get major %d for VFD/Nuvoton\n",VFD_MAJOR);

  dprintk(5, "%s <\n", __func__);
  return 0;
}

static void __exit nuvoton_cleanup_module(void)
{
	unregister_chrdev(VFD_MAJOR,"VFD");
	printk("Fortis HDBOX VFD/Nuvoton module unloading\n");
}


module_init(nuvoton_init_module);
module_exit(nuvoton_cleanup_module);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION("VFD/Nuvoton module for Fortis HDBOX");
MODULE_AUTHOR("TDT");
MODULE_LICENSE("GPL");
