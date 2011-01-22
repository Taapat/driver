/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : dvb_ca.c
Author :           Pete

Implementation of linux dvb dvr input device

Date        Modification                                    Name
----        ------------                                    --------
01-Nov-06   Created                                         Pete

************************************************************************/

#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/ca.h>

#include "dvb_module.h"
#include "backend.h"
#include "dvb_ca.h"

static int CaOpen                    (struct inode           *Inode,
					 struct file            *File);
static int CaRelease                 (struct inode           *Inode,
					 struct file            *File);
static int CaIoctl                   (struct inode           *Inode,
					 struct file            *File,
					 unsigned int            IoctlCode,
					 void                   *ParamAddress);

static struct file_operations CaFops =
{
	owner:          THIS_MODULE,
	ioctl:          dvb_generic_ioctl,
	open:           CaOpen,
	release:        CaRelease,
};

static struct dvb_device CaDevice =
{
	priv:            NULL,
	users:           1,
	readers:         1,
	writers:         1,
	fops:            &CaFops,
	kernel_ioctl:    CaIoctl,
};

struct dvb_device* CaInit (struct DeviceContext_s*        DeviceContext)
{
    return &CaDevice;
}

static int CaOpen (struct inode*     Inode,
		   struct file*      File)
{
    struct dvb_device* DvbDevice = (struct dvb_device*)File->private_data;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDevice->priv;
    int                         Error;

    Error       = dvb_generic_open (Inode, File);
    if (Error < 0)
	return Error;

    Context->EncryptionOn = 0;

    return 0;
}

static int CaRelease (struct inode*  Inode,
			 struct file*   File)
{
    struct dvb_device* DvbDevice = (struct dvb_device*)File->private_data;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDevice->priv;

    Context->EncryptionOn = 0;

    return dvb_generic_release (Inode, File);
}

static int CaIoctl (struct inode*    Inode,
		       struct file*     File,
		       unsigned int     IoctlCode,
		       void*            Parameter)
{
    struct dvb_device*  DvbDevice    = (struct dvb_device*)File->private_data;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDevice->priv;

    //print("CaIoctl : Ioctl %08x\n", IoctlCode);
    switch (IoctlCode)
    {
    case    CA_SEND_MSG:
      {
	struct ca_msg *msg;

	msg = (struct ca_msg*)Parameter;

//if (msg->type==1)
//	  tkdma_set_iv(msg->msg);
//	else if (msg->type==2)
//	  tkdma_set_key(msg->msg,0);
	if (msg->type==3)
		Context->EncryptionOn = 1;
	else if (msg->type==4)
		Context->EncryptionOn = 0;
	else if (msg->type==7) 
		memcpy(&Context->StartOffset,msg->msg,sizeof(Context->StartOffset));
	else if (msg->type==8) 
		memcpy(&Context->EndOffset,msg->msg,sizeof(Context->EndOffset));
	  return -ENOIOCTLCMD;

	return 0;

      }

    default:
      printk ("%s: Error - invalid ioctl %08x\n", __FUNCTION__, IoctlCode);
    }

    return -ENOIOCTLCMD;
}
