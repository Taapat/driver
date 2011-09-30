/* *********************************************************************
* CA Device
*  Association:
*    ca0 -> cimax
*    ca1 .. caX -> hardware descrambler via PTI session
***********************************************************************/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/ca.h>

#include "dvb_module.h"
#include "backend.h"
#include "dvb_ca.h"

//__TDT__ ->file moved in this directory

#include "st-common.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include "../../../../../../../pti/pti_hal.h"
#endif

static int ca_open (struct inode*     Inode,
		   struct file*      File)
{
    //struct dvb_device* DvbDevice = (struct dvb_device*)File->private_data;

    return dvb_generic_open (Inode, File);
}

static int ca_release (struct inode*  Inode,
			 struct file*   File)
{
    //struct dvb_device* DvbDevice = (struct dvb_device*)File->private_data;

    return dvb_generic_release (Inode, File);
}

static int ca_ioctl(struct inode *inode, struct file *file,
		 unsigned int cmd, void *parg)
{
    struct dvb_device* DvbDevice = (struct dvb_device*)file->private_data;
    struct DeviceContext_s* pContext = (struct DeviceContext_s*)DvbDevice->priv;
    struct PtiSession *pSession = pContext->pPtiSession;

    if(pSession == NULL)
    {
      printk("CA is not associated with a session\n");
      return -EINVAL;
    }

    dprintk("TEST %s : Ioctl %08x\n", __func__, cmd);
    switch (cmd)
    {
	case CA_SET_PID: // currently this is useless but prevents from softcams errors
	{
		ca_pid_t *service_pid = (ca_pid_t*) parg;
		int vLoop;
		unsigned short pid = service_pid->pid;
		int descramble_index = service_pid->index;
		printk("CA_SET_PID index = %d pid %d\n",descramble_index,pid);
		if(descramble_index >=0){
			if( descramble_index >= NUMBER_OF_DESCRAMBLERS){
				printk("Error only descramblers 0 - %d supportet\n",NUMBER_OF_DESCRAMBLERS-1);
				return -1;
			}
			pSession->descramblerForPid[pid]=descramble_index;
			for ( vLoop = 0; vLoop < pSession->num_pids; vLoop++ )
  			{
    				if (( ( unsigned short ) pSession->pidtable[vLoop] ==
	 			( unsigned short ) pid ))
    				{
					if ( pSession->type[vLoop] == DMX_TYPE_TS )
  					{
    						/* link audio/video slot to the descrambler */
						if ((pSession->pes_type[vLoop] == DMX_TS_PES_VIDEO) ||
        					(pSession->pes_type[vLoop] == DMX_TS_PES_AUDIO) ||
						(pid>50)) /*dirty hack because for some reason the pes_type is changed to DMX_TS_PES_OTHER*/
    						{
							int err;
							if(pSession->descramblerindex[vLoop]!=descramble_index){
								pSession->descramblerindex[vLoop]=descramble_index;
								if ((err = pti_hal_descrambler_link(pSession->session,
									pSession->descramblers[pSession->descramblerindex[vLoop]],
									pSession->slots[vLoop])) != 0)
								printk("Error linking slot %d to descrambler %d, err = %d\n",
									pSession->slots[vLoop],
									pSession->descramblers[pSession->descramblerindex[vLoop]],
									err);
								else dprintk("linking pid %d slot %d to descrambler %d, session = %d pSession %p\n",
									pid,pSession->slots[vLoop],
									pSession->descramblers[pSession->descramblerindex[vLoop]],
									pSession->session, pSession);
								return 0;
							}else { printk("pid %x is already linked to descrambler %d\n",pid,descramble_index);return 0;}
						}else{ printk("pid %x no audio or video pid! type=%d slot=%d not linking to descrambler\n",pid,pSession->pes_type[vLoop],pSession->slots[vLoop]);return -1;}
					}else{ printk("pid %x type is not DMX_TYPE_TS! not linking to descrambler\n",pid);return -1;}
				}
			}
			printk("pid %x not found in pidtable, it might be inactive\n",pid);
		}
		return 0;
	break;
	}
	case CA_SET_DESCR:
	{
		ca_descr_t *descr = (ca_descr_t*) parg;

		dprintk("CA_SET_DESCR\n");

		if (descr->index >= 16)
			return -EINVAL;
		if (descr->parity > 1)
			return -EINVAL;

		if (&pContext->DvbContext->Lock != NULL)
                   mutex_lock (&pContext->DvbContext->Lock);

		dprintk("index = %d\n", descr->index);
		dprintk("parity = %d\n", descr->parity);
		dprintk("cw[0] = %d\n", descr->cw[0]);
		dprintk("cw[1] = %d\n", descr->cw[1]);
		dprintk("cw[2] = %d\n", descr->cw[2]);
		dprintk("cw[3] = %d\n", descr->cw[3]);
		dprintk("cw[4] = %d\n", descr->cw[4]);
		dprintk("cw[5] = %d\n", descr->cw[5]);
		dprintk("cw[6] = %d\n", descr->cw[6]);
		dprintk("cw[7] = %d\n", descr->cw[7]);
		if(descr->index < 0 || descr->index >= NUMBER_OF_DESCRAMBLERS){
			printk("Error descrambler %d not supported! needs to be in range 0 - %d\n", descr->index, NUMBER_OF_DESCRAMBLERS-1);
			return -1;
		}
		if (pti_hal_descrambler_set(pSession->session, pSession->descramblers[descr->index], descr->cw, descr->parity) != 0)
			printk("Error while setting descrambler keys\n");

		if (&pContext->DvbContext->Lock != NULL)
                   mutex_unlock (&pContext->DvbContext->Lock);


		return 0;
	break;
	}

    default:
      printk ("%s: Error - invalid ioctl %08x\n", __FUNCTION__, cmd);
    }

    return -ENOIOCTLCMD;
}


static struct file_operations ca_fops =
{
	owner:          THIS_MODULE,
	ioctl:          dvb_generic_ioctl, /* fixme: kann das nicht wech; sollte doch nur stoeren hier?! */
	open:           ca_open,
	release:        ca_release,
};

static struct dvb_device ca_device =
{
	priv:            NULL,
	users:           1,
	readers:         1,
	writers:         1,
	fops:            &ca_fops,
	kernel_ioctl:    ca_ioctl,
};

static int caInitialized = 0;

#if !defined(VIP2_V1) && !defined (SPARK) && !defined (SPARK7162) && !defined (IPBOX99) && !defined (IPBOX55) && !defined (ADB_BOX)
extern int init_ci_controller(struct dvb_adapter* dvb_adap);
#endif

struct dvb_device *CaInit(struct DeviceContext_s *DeviceContext)
{
  printk("CaInit()\n");
  if(!caInitialized)
  {
    /* the following call creates ca0 associated with the cimax hardware */
    printk("Initializing CI Controller\n");

#if !defined(VIP2_V1) && !defined (SPARK) && !defined (SPARK7162) && !defined (IPBOX99) && !defined (IPBOX55) && !defined(ADB_BOX) 
    init_ci_controller(&DeviceContext->DvbContext->DvbAdapter);
#endif

    caInitialized = 1;
  }

  /* returning the device structure creates further ca devices which
     are associated with the hardware descrambler */
  return &ca_device;
}

