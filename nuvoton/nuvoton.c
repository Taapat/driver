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
 * - exiting the rcS leads to no data receiving in from ttyAS0 ?!?!?
 * - visual feedback of keypress does not work. I think it must work
 * automatically. One I have a state where the fp stays on while the
 * receiver was off and on every keypress the visual feedback comes up.
 * So I think it is an intialization issue. Ok we could fake some other
 * thinks for visual feedback.
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
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#if defined (CONFIG_KERNELVERSION) /* ST Linux 2.3 */
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

#include "nuvoton.h"
#include "../vfd/utf.h"
int debug = 1;
#define dprintk(x...) do { if (debug) printk(KERN_WARNING x); } while (0)

#define BUFFERSIZE                256     //must be 2 ^ n

/* structure to queue transmit data is necessary because
 * after most transmissions we need to wait for an acknowledge
 */
 
struct transmit_s
{
	unsigned char 	buffer[BUFFERSIZE];
	int		      len;
	int  		      needAck; /* should we increase ackCounter? */

   int            ack_len; /* len of complete acknowledge sequence */
   int            ack_len_header; /* len of ack header ->contained in ack_buffer */
	unsigned char 	ack_buffer[BUFFERSIZE]; /* the ack sequence we wait for */

	int		      requeueCount;
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

#define cCommandRCU	   0x63
#define cCommandFP      0x51
#define cCommandStandby	0x80

void nuvotonWriteString(unsigned char* aBuf, int len);


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
  	dprintk("Error writing char (%c) \n", Data);
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
  unsigned long         Counter = 100000;
  u16                   result;
  
  while (((*ASC_2_INT_STA & ASC_INT_STA_RBF) == 0) && --Counter);
  
  if (Counter == 0)
      result = 0x01 << 8;
  else
      result = 0x00 << 8 | *ASC_2_RX_BUFF;
		
  //printk("0x%04x\n", result);		
  return result;
}


static void serial2_init (void)
{
  /* Configure the PIO pins */
#ifdef ohne_ttyas0
  stpio_request_pin(4, 3,  "ASC_TX", STPIO_ALT_OUT); /* Tx */
  stpio_request_pin(4, 2,  "ASC_RX", STPIO_IN);      /* Rx */

  *(unsigned int*)(PIO4BaseAddress + PIO_CLR_PnC0) = 0x07;
  *(unsigned int*)(PIO4BaseAddress + PIO_CLR_PnC1) = 0x06;
  *(unsigned int*)(PIO4BaseAddress + PIO_SET_PnC1) = 0x01;
  *(unsigned int*)(PIO4BaseAddress + PIO_SET_PnC2) = 0x07;
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
	switch (data[0])
	{
		case cCommandRCU:  /* rcu */
			if (count != cLenRCUCommand - cLenHeader)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk("command RCU complete (count = %d)\n", count);

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
		case cCommandFP:  /* fp button */
			if (count != cLenFPCommand - cLenHeader)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk("command FP Button complete\n");

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
		case cCommandStandby:  /* rcu & fp */

			if (count != cLenStandByCommand - cLenHeader)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk("command standby complete\n");

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
	
   for (vLoop = 0; vLoop < count; vLoop++)
	{
   	if ((c[vLoop] == 0x00) && (c[vLoop + 1] == 0x02))
		{
			switch (c[vLoop + 2])
			{
      		case cCommandRCU:
				case cCommandFP:
				case cCommandStandby:
				   dprintk("detect start\n");
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
      dprintk("requeue data %d\n", state);
      transmit[0].requeueCount++;

      if (transmit[0].requeueCount == cMaxQueueCount)
      {
	      printk("max requeueCount reached aborting transmission %d\n", state);
	      down_interruptible(&transmit_sem);

	      transmitCount--;

	      memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	      if (transmitCount != 0)		  
     	     dprintk("next command will be 0x%x\n", transmit[0].buffer[1]);

	      up(&transmit_sem);

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
  
  daemonize("fp_rcv");

  allow_signal(SIGTERM);

  //we are on so enable the irq  
  *ASC_2_INT_EN = *ASC_2_INT_EN | 0x00000001;

  while(1)
  {
     if (state != cStateTransmission)
     {
	     u16 res = serial2_getc();
		  
		  //printk("res 0x%04x\n", res);
		  
       if ((res & 0xff00) != 0)
		 {
			 msleep(100);
		    continue;
		 }
		  
		 c = res & 0x00ff; 
	
	    dprintk("(0x%x) ", c);  
	
	    /* process our state-machine */
	    switch (state)
	    {
     	    case cStateIdle: /* nothing do to, search for command start */

             dprintk("cStateIdle: count %d, 0x%02x\n", count, c);
        		 receivedData[count++] = c;
     		    
				 if (count < cLenHeader)
				    continue;

				 if (detectEvent(receivedData, count))
     		    {
				    count = 0;
        		    receivedData[count++] = c;
			       state = cStateWaitEvent;
     		    } else
				 if (count == BUFFERSIZE-1)
				    count = 0;
	       break;
     	    case cStateWaitDataAck: /* each getter */
	       {
		       int err;

             receivedData[count++] = c;
		       err = processDataAck(receivedData, count);

		       if (err == 1)
		       {
	    	       /* data is processed remove it from queue */

		          down_interruptible(&transmit_sem);

		          transmitCount--;

	             memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	             if (transmitCount != 0)		  
     	  		       dprintk("next command will be 0x%x\n", transmit[0].buffer[0]);

		          up(&transmit_sem);

	 	          waitAckCounter = 0;
     	 	       dprintk("detect ACK %d\n", state);
		          state = cStateIdle;
		          count = 0;
		       }
	       }
	       break;
     	    case cStateWaitEvent: /* rcu, button or other event */
         	 dprintk("cStateWaitEvent: count %d, 0x%02x\n", count, c);

		   	 if (receiveCount < cMaxReceiveQueue)
		   	 {
			   	 if (processEvent(receivedData, count))
			   	 {
			   		 /* command completed */
     	   			 count = 0;
				   	 state = cStateIdle;
			   	 } else
			   	 {
			   		 /* command not completed */
        				 receivedData[count++] = c;
			   	 }

		   	 } else
		   	 {
			   	 /* noop: wait that someone reads data */
		   		 dprintk("overflow, wait for readers\n");
		   	 }
	       break;
     	    case cStateWaitStartOfAnswer: /* each getter */
	       {
             int err;
				 
             //printk("count %d, 0x%02x\n", count, c);
        		 receivedData[count++] = c;

				 if (count < transmit[0].ack_len_header)
				    continue;

		       err = searchAnswerStart(receivedData, count);

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

		          dprintk("2. %d\n", waitAckCounter);

			       if (waitAckCounter <= 0)
			       {
			          dprintk("missing ACK from micom ->requeue data %d\n", state);
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
		   	/* we currently transmit data so ignore all */
	       break;

	    }
     }
  }

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
     
     	  dprintk("send data to fp (0x%x)\n", transmit[0].buffer[0]);

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
		   	  dprintk("<0x%x> ", nuvoton_cmd[vLoop]);
		  }

		  state = cStateIdle;

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
     	  	        dprintk("%s: next command will be 0x%x\n", __func__, transmit[0].buffer[0]);

	   	  }
		  }

		  /* 4. free sem */
		  up(&transmit_sem);
     } else
     {
     	  //dprintk("%d, %d\n", state, transmitCount);
        msleep(100);
     }
  }

  return 0;
}


/* End ASC2 */

void nuvotonWriteCommand(char* buffer, int len, int needAck, char* ack_buffer, int ack_len_header, int ack_len)
{

	dprintk("%s >\n", __func__);

	/* 0. claim semaphore */
	down_interruptible(&transmit_sem);
	
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

	dprintk("%s < \n", __func__);
}

void nuvotonSetIcon(int which, int on)
{
	char buffer[128];
	u8   internalCode1, internalCode2;
   int  vLoop;
		
	dprintk("%s > %d, %d\n", __func__, which, on);
//fixme
	if (which < 1 || which > 16)
	{
		printk("VFD/Nuvoton icon number out of range %d\n", which);
		return;
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
               return;
			  }
			  
			  break;
		 }
   }

   if (internalCode1 == 0xff)
	{
		 printk("%s: not known or not supported icon %d ->%s\n", __func__, which, nuvotonIcons[vLoop].name);
       return;
	}

	memset(buffer, 0, 128);

	buffer[0] = 0x02;
	buffer[1] = 0xc2;
	buffer[2] = internalCode1;
   if (on)
	  buffer[3] = 0x02;
   else	
	  buffer[3] = 0x00;

	buffer[4] = 0x03;

	nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);

   /* e.g. timeshift is splitted in two parts */
   if (internalCode2 != 0xff)
	{
	  memset(buffer, 0, 128);

	  buffer[0] = 0x02;
	  buffer[1] = 0xc2;
	  buffer[2] = internalCode2;
     if (on)
		 buffer[3] = 0x02;
     else	
		 buffer[3] = 0x00;

	  buffer[4] = 0x03;

	  nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);
	}
	
	dprintk("%s <\n", __func__);
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetIcon);

void nuvotonSetLED(int which, int on)
{
	char buffer[8];
	
	dprintk("%s > %d, %d\n", __func__, which, on);

//FIXME
	if (which < 1 || which > 6)
	{
		printk("VFD/Nuvoton led number out of range %d\n", which);
		return;
	}

	memset(buffer, 0, 8);
	
	dprintk("%s <\n", __func__);
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetLED);

void nuvotonSetBrightness(int level)
{
	char buffer[8];
	
	dprintk("%s > %d\n", __func__, level);

	if (level < 0 || level > 7)
	{
		printk("VFD/Nuvoton brightness out of range %d\n", level);
		return;
	}

	memset(buffer, 0, 8);
	
	buffer[0] = 0x02;
	buffer[1] = 0xd2;
	buffer[2] = 0x00;
	
	buffer[3] = level;
	buffer[4] = 0x03;
	
	nuvotonWriteCommand(buffer, 5, 0, NULL, 0, 0);

	dprintk("%s <\n", __func__);
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetBrightness);

void nuvotonSetStandby(char* time)
{
	char     buffer[8];
   char     power_off[] = {0x02, 0x31, 0x01, 0x03};

	dprintk("%s >\n", __func__);

  	nuvotonWriteString("Bye bye ...", strlen("Bye bye ..."));

   /* set wakeup time */
	memset(buffer, 0, 8);

	buffer[0] = 0x02;
	buffer[1] = 0x74;

	memcpy(buffer + 2, time, 4); /* only 4 because we do not have seconds here */
	buffer[6] = 0x03;
	
	nuvotonWriteCommand(buffer, 7, 0, NULL, 0, 0);

   /* now power off */
   nuvotonWriteCommand(power_off, sizeof(power_off), 0, NULL, 0, 0);

	dprintk("%s <\n", __func__);
}

void nuvotonSetTime(char* time)
{
	char 	   buffer[8];

	dprintk("%s >\n", __func__);

	memset(buffer, 0, 8);

	buffer[0] = 0x02;
	buffer[1] = 0x15;
	
	memcpy(buffer + 2, time, 5);
	buffer[7] = 0x03;
	
	nuvotonWriteCommand(buffer, 8, 0, NULL, 0, 0);
	
	dprintk("%s <\n", __func__);
}
 
void nuvotonGetTime(void)
{
	char 	   buffer[3];
	char 	   ack_buffer[3];

	dprintk("%s >\n", __func__);

	memset(buffer, 0, 3);
	
	buffer[0] = 0x02;
	buffer[1] = 0x10;
	buffer[2] = 0x03;

   /* the replay sequence */
	ack_buffer[0] = 0x00;
	ack_buffer[1] = 0x02;
	ack_buffer[2] = 0x15;
	
	dataReady = 0;
	nuvotonWriteCommand(buffer, 3, 1, ack_buffer, 3, 8);
	wait_event_interruptible(ioctl_wq, dataReady || timeoutOccured);
	
	if (timeoutOccured == 1)
	{
		/* timeout */
		memset(ioctl_data, 0, 8);
		printk("timeout\n");
	} else
	{
		/* time received ->noop here */
		dprintk("time received\n");
		printk("myTime= 0x%02x - 0x%02x - 0x%02x - 0x%02x - 0x%02x\n", ioctl_data[0], ioctl_data[1]
		         , ioctl_data[2], ioctl_data[3], ioctl_data[4]);
	}
	 
	dprintk("%s <\n", __func__);
}

void nuvotonGetWakeUpMode(void)
{
	char 	   buffer[8];
	char 	   ack_buffer[3];

	dprintk("%s >\n", __func__);

	memset(buffer, 0, 8);
	
	buffer[0] = 0x02;
	buffer[1] = 0x80;
	buffer[2] = 0x03;

   /* the replay sequence */
	ack_buffer[0] = 0x00;
	ack_buffer[1] = 0x02;
	ack_buffer[2] = 0x81;

	dataReady = 0;
	nuvotonWriteCommand(buffer, 3, 1, ack_buffer, 3, 5);
	wait_event_interruptible(ioctl_wq, dataReady || timeoutOccured);
	
	if (timeoutOccured == 1)
	{
		/* timeout */
		memset(ioctl_data, 0, 8);
		printk("timeout\n");
	} else
	{
		/* time received ->noop here */
		dprintk("time received\n");
	}
	 
	dprintk("%s <\n", __func__);
}

void nuvotonWriteString(unsigned char* aBuf, int len)
{
	unsigned char bBuf[128];
	int i =0;
	int j =0;

	dprintk("%s >\n", __func__);
	
	memset(bBuf, ' ', 128);
	
	/* start of command write */
	bBuf[0] = 0x02;
	bBuf[1] = 0xce;
	bBuf[2] = 0x11;
	
	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;
	
	dprintk("len %d\n", len);
	
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
	bBuf[16] = 0x03;
	
	nuvotonWriteCommand(bBuf, 17, 0, NULL, 0, 0);

	dprintk("%s <\n", __func__);
}

int nuvoton_init_func(void)
{
   char standby_disable[] = {0x02, 0x31, 0x02, 0x03};
   char init1[] = {0x02, 0x40, 0x03};
   char init2[] = {0x02, 0x11, 0x81, 0x03};
   char init3[] = {0x02, 0x72, 0xff, 0xff, 0x03};
   char init4[] = {0x02, 0x93, 0x01, 0x00, 0x08, 0x03};
   char init5[] = {0x02, 0x93, 0xf2, 0x0a, 0x00, 0x03};
	int  vLoop;
/*
write (count=5, fd = 28): 0x02 0xc2 0x22 0x20 0x03
write (count=5, fd = 28): 0x02 0xc2 0x23 0x04 0x03
write (count=5, fd = 28): 0x02 0xc2 0x20 0x04 0x03
*/

	dprintk("%s >\n", __func__);

	printk("Fortis HDBOX VFD/Nuvoton module initializing\n");

   /* must be called before standby_disable */
	nuvotonWriteCommand(init1, sizeof(init1), 0, NULL, 0, 0);

	/* setup: frontpanel should not power down the receiver if standby is selected */
	nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0, NULL, 0, 0);

	nuvotonWriteCommand(init2, sizeof(init2), 0, NULL, 0, 0);
	nuvotonWriteCommand(init3, sizeof(init3), 0, NULL, 0, 0);
	nuvotonWriteCommand(init4, sizeof(init4), 0, NULL, 0, 0);
	nuvotonWriteCommand(init5, sizeof(init5), 0, NULL, 0, 0);

	nuvotonSetBrightness(1);

  	nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));
	
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++) 
		nuvotonSetIcon(vLoop, 0);
		
	dprintk("%s <\n", __func__);

	return 0;
}

static ssize_t NUVOTONdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char* kernel_buf;
	int minor, vLoop;
	dprintk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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

	dprintk("minor = %d\n", minor);

	/* dont write to the remote control */
	if (minor == FRONTPANEL_MINOR_RC)
		return -EOPNOTSUPP;

	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
	   dprintk("%s return no mem<\n", __func__);
	   return -ENOMEM;
	}
	copy_from_user(kernel_buf, buff, len); 

        if(down_interruptible (&write_sem))
            return -ERESTARTSYS;

        /* Dagobert: echo add a \n which will be counted as a char
	 */ 
	if (kernel_buf[len - 1] == '\n')
	   nuvotonWriteString(kernel_buf, len - 1);
        else
	   nuvotonWriteString(kernel_buf, len);

	kfree(kernel_buf);
	
	up(&write_sem);

	dprintk("%s <\n", __func__);
	return len;
}

static ssize_t NUVOTONdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;
	dprintk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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

	dprintk("minor = %d\n", minor);

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
	   dprintk("%s return erestartsys<\n", __func__);
   	   return -ERESTARTSYS;
	}

	if (FrontPanelOpen[minor].read == lastdata.length)
	{
	    FrontPanelOpen[minor].read = 0;
	    
	    up (&FrontPanelOpen[minor].sem);
	    dprintk("%s return 0<\n", __func__);
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

	dprintk("%s < (len %d)\n", __func__, len);
	return len;
}

int NUVOTONdev_open(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk("%s >\n", __func__);

   minor = MINOR(inode->i_rdev);

	dprintk("open minor %d\n", minor);

  	if (FrontPanelOpen[minor].fp != NULL)
  	{
		printk("EUSER\n");
    		return -EUSERS;
  	}
  	FrontPanelOpen[minor].fp = filp;
  	FrontPanelOpen[minor].read = 0;

	dprintk("%s <\n", __func__);
	return 0;

}

int NUVOTONdev_close(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk("%s >\n", __func__);

  	minor = MINOR(inode->i_rdev);

	dprintk("close minor %d\n", minor);

  	if (FrontPanelOpen[minor].fp == NULL) 
	{
		printk("EUSER\n");
		return -EUSERS;
  	}
	FrontPanelOpen[minor].fp = NULL;
  	FrontPanelOpen[minor].read = 0;

	dprintk("%s <\n", __func__);
	return 0;
}

static int NUVOTONdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct nuvoton_ioctl_data * nuvoton = (struct nuvoton_ioctl_data *)arg;

	dprintk("%s > 0x%.8x\n", __func__, cmd);

        if(down_interruptible (&write_sem))
            return -ERESTARTSYS;

	switch(cmd) {
	case VFDSETMODE:
		mode = nuvoton->u.mode.compat;
		break;
	case VFDSETLED:
		nuvotonSetLED(nuvoton->u.led.led_nr, nuvoton->u.led.on);
		break;
	case VFDBRIGHTNESS:
		if (mode == 0)
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg; 

			nuvotonSetBrightness(data->start_address);
		} else
		{
			nuvotonSetBrightness(nuvoton->u.brightness.level);
		}
		mode = 0;
		break;
	case VFDDRIVERINIT:
		nuvoton_init_func();
		mode = 0;
		break;
	case VFDICONDISPLAYONOFF:
		if (mode == 0)
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg; 
			int icon_nr = (data->data[0] & 0xf) + 1;
			int on = data->data[4];
						
			nuvotonSetIcon(icon_nr, on);
		} else
		{
			nuvotonSetIcon(nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
		}
		
		mode = 0;
		break;	
	case VFDSTANDBY:
		   nuvotonSetStandby(nuvoton->u.standby.time);
		break;	
	case VFDSETTIME:
		if (nuvoton->u.time.time != 0)
		   nuvotonSetTime(nuvoton->u.time.time);
		break;	
	case VFDGETTIME:
	   nuvotonGetTime();
      copy_to_user(arg, &ioctl_data, 5);
		break;	
	case VFDGETWAKEUPMODE:
		nuvotonGetWakeUpMode();
      copy_to_user(arg, &ioctl_data, 1);
		break;	
	case VFDDISPLAYCHARS:
		if (mode == 0)
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg; 
						
			nuvotonWriteString(data->data, data->length);
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

	dprintk("%s <\n", __func__);
	return 0;
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
  
  dprintk("%s >\n", __func__);

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

  dprintk("%s <\n", __func__);
  return 0;
}

static void __exit nuvoton_cleanup_module(void)
{
	unregister_chrdev(VFD_MAJOR,"VFD");
	printk("Fortis HDBOX VFD/Nuvoton module unloading\n");
}


module_init(nuvoton_init_module);
module_exit(nuvoton_cleanup_module);

MODULE_DESCRIPTION("VFD/Nuvoton module for Fortis HDBOX");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");
