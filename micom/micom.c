/*
 * micom.c
 * 
 * (c) 2009 Dagobert@teamducktales
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
 * Description:
 *
 * Kathrein UFS922 MICOM Kernelmodule ported from MARUSYS uboot source,
 * from vfd driver and from tf7700 frontpanel handling.
 * 
 * Devices:
 *	- /dev/vfd (vfd ioctls and read/write function)
 *	- /dev/rc  (reading of key events)
 *
 * TODO:
 * - implement a real led and button driver?!
 * - implement a real event driver?!
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
#include "../vfd/utf.h"

static short paramDebug = 0;
#define TAGDEBUG "[micom] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
} while (0)

/* structure to queue transmit data is necessary because
 * after most transmissions we need to wait for an acknowledge
 */
 
struct transmit_s
{
	char 		command;
	unsigned char 	buffer[20];
	int		len;
	int  		needAck; /* should we increase ackCounter? */
	int		requeueCount;
	int		isGetter;
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
	unsigned char buffer[20];
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
	char  data[20];
};

static struct saved_data_s lastdata;

/* last received ioctl command. we dont queue answers
 * from "getter" requests to the fp. they are protected
 * by a semaphore and the threads goes to sleep until
 * the answer has been received or a timeout occurs.
 */
static char ioctl_data[8];

#define BUFFERSIZE                256     //must be 2 ^ n

int writePosition = 0;
int readPosition = 0;
unsigned char receivedData[BUFFERSIZE];

/* konfetti: quick and dirty open handling */
typedef struct
{
	struct file* 		fp;
	int			read;
	struct semaphore 	sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC             1
#define LASTMINOR                 	2

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
		LED_AUX = 0x1,
		LED_LIST,
		LED_POWER,
		LED_TV_R,
		LED_VOL,
		LED_WHEEL,
};

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
		ICON_MAX
};

#define cMaxCommandLen 	8

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


//#define IRQ_WORKS
#ifdef IRQ_WORKS
static irqreturn_t FP_interrupt(int irq, void *dev_id)
{
  unsigned int         *ASC_3_INT_STA = (unsigned int*)(ASC3BaseAddress + ASC_INT_STA);
  char                 *ASC_3_RX_BUFF = (char*)(ASC3BaseAddress + ASC_RX_BUFF);
  int 			dataArrived = 0;

  while (*ASC_3_INT_STA & ASC_INT_STA_RBF)
  {
     dataArrived = 1;
     
     dprintk(100, "arrived: status 0x%x\n", *ASC_3_INT_STA);
     receivedData[writePosition] = *ASC_3_RX_BUFF;
     dprintk(50, "(0x%x) ", receivedData[writePosition]);  
  
     writePosition = (writePosition + 1) % BUFFERSIZE;

     if (writePosition == readPosition)
     	printk("overflow\n");	
  }

  // signal the data arrival
  if (dataArrived == 1)
  {
    up(&rx_int_sem);
  }
  return IRQ_HANDLED;
}
#endif

static int serial3_putc (char Data)
{
  char                  *ASC_3_TX_BUFF = (char*)(ASC3BaseAddress + ASC_TX_BUFF);
  unsigned int          *ASC_3_INT_STA = (unsigned int*)(ASC3BaseAddress + ASC_INT_STA);
  unsigned long         Counter = 200000;

  while (((*ASC_3_INT_STA & ASC_INT_STA_THE) == 0) && --Counter);
  
  if (Counter == 0)
  {
  	dprintk(1, "Error writing char (%c) \n", Data);
//  	*(unsigned int*)(ASC3BaseAddress + ASC_TX_RST)   = 1;
//	return 0;
  }
  
  *ASC_3_TX_BUFF = Data;
  return 1;
}

static u16 serial3_getc(void)
{
  unsigned char         *ASC_3_RX_BUFF = (unsigned char*)(ASC3BaseAddress + ASC_RX_BUFF);
  unsigned int          *ASC_3_INT_STA = (unsigned int*)(ASC3BaseAddress + ASC_INT_STA);
  unsigned long         Counter = 100000;
  u16                   result;

  while (((*ASC_3_INT_STA & ASC_INT_STA_RBF) == 0) && --Counter);

  /* return an error code for timeout in the MSB */
  if (Counter == 0)
      result = 0x01 << 8;
  else
      result = 0x00 << 8 | *ASC_3_RX_BUFF;
		
  dprintk(100, "0x%04x\n", result);		
  return result;
}


#define BAUDRATE_VAL_M1(bps, clk)	( ((bps * (1 << 14)) / ((clk) / (1 << 6)) ) + 1 )

static void serial3_init (void)
{
  /* Configure the PIO pins */
  stpio_request_pin(5, 0,  "ASC_TX", STPIO_ALT_OUT); /* Tx */
  stpio_request_pin(5, 1,  "ASC_RX", STPIO_IN);      /* Rx */

  *(unsigned int*)(PIO5BaseAddress + PIO_CLR_PnC0) = 0x07;
  *(unsigned int*)(PIO5BaseAddress + PIO_CLR_PnC1) = 0x06;
  *(unsigned int*)(PIO5BaseAddress + PIO_SET_PnC1) = 0x01;
  *(unsigned int*)(PIO5BaseAddress + PIO_SET_PnC2) = 0x07;

  *(unsigned int*)(ASC3BaseAddress + ASC_INT_EN)   = 0x00000000;
  *(unsigned int*)(ASC3BaseAddress + ASC_CTRL)     = 0x00001589;
  *(unsigned int*)(ASC3BaseAddress + ASC_TIMEOUT)  = 0x00000010;
  *(unsigned int*)(ASC3BaseAddress + ASC_BAUDRATE) = 0x000000c9;
  *(unsigned int*)(ASC3BaseAddress + ASC_TX_RST)   = 0;
  *(unsigned int*)(ASC3BaseAddress + ASC_RX_RST)   = 0;
}

/* process commands where we expect data from frontcontroller
 * e.g. getTime and getWakeUpMode
 */
int processDataAck(unsigned char* data, int count)
{
   int i;
	
	switch (data[0])
	{
		case 0xB9: /* timeval ->len = 6 */
			if (count != 6)
			    return 0;

         for (i = 0; i < 6; i++)
			{
			   dprintk(20, "0x%02x ", data[i] & 0xff);
         }
			
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(1, "command getTime complete\n");

			/* 1. copy data */
			
			memcpy(ioctl_data, data, 8);
			
			/* 2. free semaphore */
			up(&receive_sem);
			
			/* 3. wake up thread */
	  		dataReady = 1;
			wake_up_interruptible(&ioctl_wq);
			return 1;
		break;
		case 0x77: /* wakeup ->len = 8 */
/* fixme: determine the real len here
 * I think it will be two but I'm not sure
 * ->this will be block until forever if
 * the number does not match!
 */
			if (count != 8)
			    return 0;

			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(1, "command getWakeupMode complete\n");

			/* 1. copy data */
			
			memcpy(ioctl_data, data, 8);
			
			/* 2. free semaphore */
			up(&receive_sem);
			
			/* 3. wake up thread */
	  		dataReady = 1;
			wake_up_interruptible(&ioctl_wq);
			return 1;
		break;				
	}
	
	return 0;
}

/* process acknowledges from frontcontroller. this is for
 * one shot commands which we have send to the controller.
 * all setXY Functions
 */
int processAck(unsigned char c)
{
	switch (c)
	{
		case 0xF5: /* cmd_error ->len = 1 */
                   dprintk(1, "receive error from fp\n");
                   return 2;
		break;
		case 0xFA: /* cmd_ok ->len = 1*/
		case 0xF1:
                    return 1;
		break;				
		case 0xd: /* this is not a real response */
                    return 0;
		break;				
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
		case 0xD2:  /* rcu ->len = 8 */
		case 0xD1:  /* button ->len = 8 */
			if (count != 8)
			    return 0;
			    
			/* 0. claim semaphore */
			down_interruptible(&receive_sem);
			
			dprintk(1, "command RCU complete\n");

			/* 1. copy data */
			
			if (receiveCount == cMaxReceiveQueue)
			{
			 	printk("receive queue full!\n");
				/* 2. free semaphore */
				up(&receive_sem);
				return 1;
			}	
			
			memcpy(receive[receiveCount].buffer, data, 8);
			
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
int detectEvent(unsigned char c)
{
	switch (c)
	{
		case 0xD2:  /* rcu ->len = 8 */
		case 0xD1:  /* button ->len = 8 */
			return 1;
		break;				
	}
	
	return 0;
}


/* search for the start of an answer of an command
 * we have send to the frontcontroller where we
 * expect data from the controller.
 */
int searchAnswerStart(unsigned char c)
{
	switch (c)
	{
		case 0xF5: /* cmd_error ->len = 1 */
			dprintk(1, "receive error from fp\n");
			return 2;
		break;
		case 0xFA: /* cmd_ok ->len = 1*/
		case 0xB9: /* timeval ->len = 8 */
		case 0x77: /* wakeup ->len = 8 */
		case 0xC1: /* wakeup from rcu ->len = 8 */
		case 0xC2: /* wakeup from front ->len = 8 */
		case 0xC3: /* wakeup from time ->len = 8 */
		case 0xC4: /* wakeup from ac ->len = 8 */
			return 1;
		break;				
	}
	
	return 0;
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
     	     dprintk(1, "next command will be 0x%x\n", transmit[0].command);

        dataReady = 0;
        timeoutOccured = 1;
        
		  /* 3. wake up thread */
        wake_up_interruptible(&ioctl_wq);
      
      }
}

int fpReceiverTask(void* dummy)
{
  unsigned int         *ASC_3_INT_EN = (unsigned int*)(ASC3BaseAddress + ASC_INT_EN);
  unsigned char c;
  int count = 0;
  unsigned char receivedData[16];
  int timeout = 0;
  u16 res;
  
  daemonize("fp_rcv");

  allow_signal(SIGTERM);

  //we are on so enable the irq  
  *ASC_3_INT_EN = *ASC_3_INT_EN | 0x00000001;

  while(1)
  {
#ifdef IRQ_WORKS  
     if(down_interruptible (&rx_int_sem))
       break;
#endif  
     
	  dprintk(200, "w ");
	  
     down(&transmit_sem);

     res = serial3_getc();

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
		  msleep(100);
		  continue;
	  }

	  dprintk(100, "(0x%x) ", c);  

	  /* process our state-machine */
	  switch (state)
	  {
     	   case cStateIdle: /* nothing do to, search for command start */
     	      if (detectEvent(c))
     	      {
		         dprintk(20, "receive[%d]: 0x%02x\n", count, c);
     		      count = 0;
        	      receivedData[count++] = c;
		         state = cStateWaitEvent;
     	      }
	      break;
     	   case cStateWaitAck: /* each setter */
     	   {
			   int err;
				
				if (timeout)
				   err = 0;
			   else
	            err = processAck(c);    

	         if (err == 2)
	         {
	    	      /* an error is detected from fp requeue data ...
		          */
			      requeueData();

			      state = cStateIdle;	  
			      waitAckCounter = 0;
			      count = 0;
	         }
	         else
	         if (err == 1)
	         {
	    	       /* data is processed remove it from queue */

		          transmitCount--;

	             memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

	             if (transmitCount != 0)		  
     	  		       dprintk(1, "next command will be 0x%x\n", transmit[0].command);

	 	          waitAckCounter = 0;
     	 	       dprintk(1, "detect ACK %d\n", state);
		          state = cStateIdle;
		          count = 0;
	          }
	          else
	          if (err == 0)
	          {
	 	           udelay(1);
	              waitAckCounter--;

		           dprintk(10, "1. %d\n", waitAckCounter);

		           if (waitAckCounter <= 0)
		           {
			           dprintk(1, "missing ACK from micom ->requeue data %d\n", state);

			           requeueData();

			           count = 0;
			           waitAckCounter = 0;
			           state = cStateIdle;
		           }
     	       }
	      }
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
     	  		   	dprintk(1, "next command will be 0x%x\n", transmit[0].command);

	 	      	waitAckCounter = 0;
            	dprintk(1, "detect ACK %d\n", state);
		      	state = cStateIdle;
		      	count = 0;
		   	}
	   	}
	   	break;
     		case cStateWaitEvent: /* key or button */
		   	if (receiveCount < cMaxReceiveQueue)
		   	{
					dprintk(20, "receive[%d]: 0x%02x\n", count, c);
        	   	receivedData[count++] = c;

					if (processEvent(receivedData, count))
			   	{
			   	   	/* command completed */
                  	count = 0;
                  	state = cStateIdle;
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
		   	   err = searchAnswerStart(c);

		   	if (err == 2) 
		   	{
					/* error detected ->requeue data ... */

					requeueData();

					state = cStateIdle;	  
					waitAckCounter = 0;
					count = 0;
		   	}
		   	else
		   	if (err == 1)
		   	{
					/* answer start detected now process data */
        			count = 0;
					receivedData[count++] = c;
					state = cStateWaitDataAck;
		   	} else
		   	{
		   	   	/* no answer start detected */
	 	         	udelay(1);
               	waitAckCounter--;

		         	dprintk(10, "2. %d\n", waitAckCounter);

						if (waitAckCounter <= 0)
						{
			   			 dprintk(1, "missing ACK from micom ->requeue data %d\n", state);
			   			 requeueData();

			   			 count = 0;
			   			 waitAckCounter = 0;
			   			 state = cStateIdle;
						}
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
  char 	micom_cmd[20];
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

     	  dprintk(1, "send data to fp (0x%x)\n", transmit[0].command);
     
	     /* 1. send it to the frontpanel */
	     memset(micom_cmd, 0, 20);

        micom_cmd[0] = transmit[0].command;
	
	     memcpy(micom_cmd + 1, transmit[0].buffer, transmit[0].len);
	
        state = cStateTransmission;
	
	     for(vLoop = 0 ; vLoop < transmit[0].len + 1; vLoop++)
	     {	
	        udelay(100);
		     if (serial3_putc((micom_cmd[vLoop])) == 0)
		     {
			     printk("%s failed < char = %c \n", __func__, micom_cmd[vLoop]);
			     state = cStateIdle;
			     sendFailed = 1;
			     break;
		     } else
		        dprintk(100, "<0x%x> ", micom_cmd[vLoop]);
	      }

	      if (sendFailed == 0)
	      {
	         if (transmit[0].needAck)
	         {
	         	 waitAckCounter = cMaxAckAttempts;

	  	   		 if (transmit[0].isGetter)
		   		 {
		      		 timeoutOccured = 0;
	  	      		 state = cStateWaitStartOfAnswer;
		   		 }
		   		 else
	  	         	 state = cStateWaitAck;
             } else
	          {
	  	   		 /* no acknowledge needed so remove it direct */
		   		 transmitCount--;

                memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));	

                if (transmitCount != 0)		  
                   dprintk(1, "%s: next command will be 0x%x\n", __func__, transmit[0].command);

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


/* End ASC3 */

int micomWriteCommand(char command, char* buffer, int len, int needAck, int isGetter)
{

	dprintk(5, "%s >\n", __func__);

	/* 0. claim semaphore */
	if (down_interruptible(&transmit_sem))
	   return -ERESTARTSYS;;
	
	if (transmitCount < cMaxTransQueue)
	{
	
	   /* 1. setup new entry */
	   memset(transmit[transmitCount].buffer, 0, 20);
	   memcpy(transmit[transmitCount].buffer, buffer, len);

	   transmit[transmitCount].len = len;
	   transmit[transmitCount].command = command;
	   transmit[transmitCount].needAck = needAck;
	   transmit[transmitCount].requeueCount = 0;
	   transmit[transmitCount].isGetter = isGetter;

	   transmitCount++;

	} else
	{
		printk("error transmit queue overflow");
	}

	/* 2. free semaphore */
	up(&transmit_sem);

	dprintk(10, "%s < \n", __func__);

   return 0;
}

int micomSetIcon(int which, int on)
{
	char buffer[8];
	int  res = 0;
	
	dprintk(5, "%s > %d, %d\n", __func__, which, on);
	if (which < 1 || which > 16)
	{
		printk("VFD/MICOM icon number out of range %d\n", which);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = which;
	
	if (on == 1)
	   res = micomWriteCommand(0x11, buffer, 7, 1 ,0);
	else
	   res = micomWriteCommand(0x12, buffer, 7, 1 ,0);
	
	dprintk(10, "%s <\n", __func__);

   return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetIcon);

int micomSetLED(int which, int on)
{
	char buffer[8];
	int  res = 0;
	
	dprintk(5, "%s > %d, %d\n", __func__, which, on);
	if (which < 1 || which > 6)
	{
		printk("VFD/MICOM led number out of range %d\n", which);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = which;
	
	if (on == 1)
	   res = micomWriteCommand(0x06, buffer, 7, 1 ,0);
	else
	   res = micomWriteCommand(0x22, buffer, 7, 1 ,0);
	
	dprintk(10, "%s <\n", __func__);

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
	
	res = micomWriteCommand(0x25, buffer, 7, 1 ,0);

	dprintk(10, "%s <\n", __func__);

   return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetModel(void)
{
	char buffer[8];
	int  res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	buffer[0] = 0x1;
	
	res = micomWriteCommand(0x3, buffer, 7, 0 ,0);

	dprintk(10, "%s <\n", __func__);

   return res;
}

int micomSetStandby(char* time)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

   if (time[0] == '\0')
	{
	   /* clear wakeup time */
	   res = micomWriteCommand(0x33, buffer, 7, 1 ,0);
	} else
	{
	   /* set wakeup time */

	   memcpy(buffer, time, 5);
	   res = micomWriteCommand(0x32, buffer, 7, 1 ,0);
	}
	
	if (res < 0)
	{
	   printk("%s <res %d \n", __func__, res);
		return res;
	}
	
	memset(buffer, 0, 8);
	/* enter standby */
	res = micomWriteCommand(0x41, buffer, 7, 0 ,0);
	
	dprintk(10, "%s <\n", __func__);

   return res;
}

int micomSetTime(char* time)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	memcpy(buffer, time, 5);
	res = micomWriteCommand(0x31, buffer, 7, 1 ,0);
	
	dprintk(10, "%s <\n", __func__);

   return res;
}

int micomGetTime(void)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	
   timeoutOccured = 0;
	dataReady = 0;
	res = micomWriteCommand(0x39, buffer, 7, 1 ,1); 
	
	if (res < 0)
	{
	   printk("%s < res %d\n", __func__, res);
	   return res;
	}
	
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
	 
	dprintk(10, "%s <\n", __func__);

   return res;
}

int micomGetWakeUpMode(void)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	
   timeoutOccured = 0;
	dataReady = 0;
	res = micomWriteCommand(0x43, buffer, 7, 1 ,1);

	if (res < 0)
	{
	   printk("%s < res %d\n", __func__, res);
	   return res;
	}

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
	 
	dprintk(10, "%s <\n", __func__);

   return res;
}

int micomReboot(void)
{
	char 	   buffer[8];
   int      res = 0;
	
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	res = micomWriteCommand(0x46, buffer, 7, 0 ,0);
	
	dprintk(10, "%s <\n", __func__);

   return res;
}


int micomWriteString(unsigned char* aBuf, int len)
{
	unsigned char bBuf[20];
	int i =0;
	int j =0;
   int res = 0;
	
	dprintk(5, "%s >\n", __func__);
	
//utf8:	if (len > 16 || len < 0)
//	{
//		printk("VFD String Length value is over! %d\n", len);
//		len = 16;
//	}

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
	res = micomWriteCommand(0x21, bBuf, 16, 1 ,0);

	dprintk(10, "%s <\n", __func__);

   return res;
}

int micom_init_func(void)
{
	int vLoop;
	
	dprintk(5, "%s >\n", __func__);

	printk("Kathrein UFS922 VFD/MICOM module initializing\n");

	micomSetModel();
	
	micomSetBrightness(1);

  	micomWriteString(" Team Ducktales ", strlen(" Team Ducktales "));
	
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++) 
		micomSetIcon(vLoop, 0);
		
	dprintk(10, "%s <\n", __func__);

	return 0;
}

static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
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
	   res = micomWriteString(kernel_buf, len - 1);
	else
	   res = micomWriteString(kernel_buf, len);
	
	kfree(kernel_buf);
	
	up(&write_sem);

	dprintk(10, "%s < res %d len %d\n", __func__, res, len);
	
	if (res < 0)
	   return res;
	else
	   return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
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
     copy_to_user(buff, receive[0].buffer, 8);

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

int MICOMdev_open(struct inode *inode, struct file *filp)
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

	dprintk(10, "%s <\n", __func__);
	return 0;

}

int MICOMdev_close(struct inode *inode, struct file *filp)
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

	dprintk(10, "%s <\n", __func__);
	return 0;
}

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct micom_ioctl_data * micom = (struct micom_ioctl_data *)arg;
   int res = 0;
	
	dprintk(5, "%s > 0x%.8x\n", __func__, cmd);

   if(down_interruptible (&write_sem))
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
		break;		
	default:
		printk("VFD/MICOM: unknown IOCTL 0x%x\n", cmd);

		mode = 0;
		break;
	}

   up(&write_sem);

	dprintk(10, "%s <\n", __func__);
	return res;
}

static unsigned int MICOMdev_poll(struct file *filp, poll_table *wait)
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
	.ioctl = MICOMdev_ioctl,
	.write = MICOMdev_write,
	.read  = MICOMdev_read,
  	.poll  = (void*) MICOMdev_poll,
	.open  = MICOMdev_open,
	.release  = MICOMdev_close
};

static int __init micom_init_module(void)
{	
  unsigned int         *ASC_3_INT_EN = (unsigned int*)(ASC3BaseAddress + ASC_INT_EN);
  unsigned int         *ASC_3_CTRL   = (unsigned int*)(ASC3BaseAddress + ASC_CTRL);
  int i;
  
  dprintk(5, "%s >\n", __func__);

  //Disable all ASC 3 interrupts
  *ASC_3_INT_EN = *ASC_3_INT_EN & ~0x000001ff;

  serial3_init();

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
  *ASC_3_CTRL = *ASC_3_CTRL | ASC_CTRL_FIFO_EN;

#ifdef IRQ_WORKS
  i = request_irq(120, (void*)FP_interrupt, SA_INTERRUPT, "FP_serial", NULL);
  if (!i)
  {
    *ASC_3_INT_EN = *ASC_3_INT_EN | 0x00000001;

  }
  else printk("FP: Can't get irq\n");
#endif
  kernel_thread(fpReceiverTask, NULL, 0);
  kernel_thread(fpTransmitterTask, NULL, 0);
 
  msleep(1000);
  micom_init_func();

  if (register_chrdev(VFD_MAJOR,"VFD",&vfd_fops))
	printk("unable to get major %d for VFD/MICOM\n",VFD_MAJOR);

  dprintk(10, "%s <\n", __func__);
  return 0;
}

static void __exit micom_cleanup_module(void)
{
	unregister_chrdev(VFD_MAJOR,"VFD");
	printk("Kathrein UFS922 VFD/MICOM module unloading\n");
}


module_init(micom_init_module);
module_exit(micom_cleanup_module);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION("VFD/MICOM module for Kathrein UFS922");
MODULE_AUTHOR("Dagobert");
MODULE_LICENSE("GPL");
