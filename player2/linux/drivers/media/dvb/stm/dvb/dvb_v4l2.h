/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : dvb_v4l2.h
Author :           Pete

Describes internal v4l interfaces

Date        Modification                                    Name
----        ------------                                    --------
09-Dec-07   Created                                         Pete

************************************************************************/

#ifndef __DVB_V4L2
#define __DVB_V4L2


/******************************
 * DEFINES
 ******************************/
#define MAX_BUFFERS 10


/******************************
 * LOCAL VARIABLES
 ******************************/
// Input surfaces to user

struct ldvb_v4l2_capture {
	struct task_struct	  	                 *thread;
	int                                      running;
	unsigned long                            physical_address;
	unsigned long                            size;
	unsigned long                            stride;
	unsigned long                            buffer_format;
	int                                      width;
	int                                      height;
	int                                      complete;
};

struct ldvb_v4l2_description {
	char                       name[32];
	//int audioId (when implemented the id associated with audio description)
	int                        deviceId;
	int                        virtualId;
	int                        inuse;
	struct linuxdvb_v4l2      *priv;
};

static struct ldvb_v4l2_description g_ldvb_v4l2_device[] = {
	{"LINUXDVB_0",0,0,0,NULL},
	{"LINUXDVB_1",0,0,0,NULL},
	{"LINUXDVB_2",0,0,0,NULL}
};

static int g_ldvb_v4l2Count = sizeof(g_ldvb_v4l2_device)/sizeof(struct ldvb_v4l2_description);

/******************************
 * FUNCTION PROTOTYPES
 ******************************/
void* stm_v4l2_findbuffer(unsigned long userptr, unsigned int size,int device);

#endif
