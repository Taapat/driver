/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : dvb_module.c
Author :           Julian

Implementation of the LinuxDVB interface to the DVB streamer

Date        Modification                                    Name
----        ------------                                    --------
24-Mar-05   Created                                         Julian

************************************************************************/

#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/ioport.h>
#ifdef __TDT__
#include <linux/dvb/version.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/bpa2.h>
#else
#include <linux/bigphysarea.h>
#endif
#endif
#include <linux/bpa2.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/autoconf.h>
#include <asm/uaccess.h>

#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>

#include "dvb_demux.h"          /* provides kernel demux types */

#define USE_KERNEL_DEMUX

#include "dvb_module.h"
#include "dvb_audio.h"
#include "dvb_video.h"
#include "dvb_dmux.h"
#include "dvb_dvr.h"
#include "dvb_ca.h"
#include "backend.h"

extern int __init avr_init(void);
extern int __init cap_init(void);
extern void linuxdvb_v4l2_init(void);

#ifdef __TDT__
extern void init_e2_proc(struct DeviceContext_s* DC);
extern void ptiInit ( struct DeviceContext_s *pContext);
extern int SetSource (struct dmx_demux* demux, const dmx_source_t *src);
#endif

/*static*/ int  __init      StmLoadModule (void);
static void __exit      StmUnloadModule (void);

module_init             (StmLoadModule);
module_exit             (StmUnloadModule);

MODULE_DESCRIPTION      ("Linux DVB video and audio driver for STM streaming architecture.");
MODULE_AUTHOR           ("Julian Wilson");
MODULE_LICENSE          ("GPL");

#define MODULE_NAME     "STM Streamer"

#ifdef __TDT__
int highSR = 0;
int swts = 0;

module_param(highSR, int, 0444);
MODULE_PARM_DESC(highSR, "Start Driver with support for Symbol Rates 30000.\nIf 1 then some CAMS will not work.");

module_param(swts, int, 0444);
MODULE_PARM_DESC(swts, "Do not route injected data through the tsm/pti\n");
#endif

struct DvbContext_s*     DvbContext;

long DvbGenericUnlockedIoctl(struct file *file, unsigned int foo, unsigned long bar)
{
    return dvb_generic_ioctl(NULL, file, foo, bar);
}

/*static*/ int __init StmLoadModule (void)
{
    int                         Result;
    int                         i;
    short int                   AdapterNumbers[] = { -1 };

    DvbContext  = kzalloc (sizeof (struct DvbContext_s),  GFP_KERNEL);
#ifdef __TDT__
    memset(DvbContext, 0, sizeof*DvbContext);
#endif

    if (DvbContext == NULL)
    {
	DVB_ERROR("Unable to allocate device memory\n");
	return -ENOMEM;
    }

#ifdef __TDT__
    if (swts)
      printk("swts ->routing streams from dvr0 to tsm to pti to player\n");
    else
      printk("no swts ->routing streams from dvr0 direct to the player\n");
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
  #if (DVB_API_VERSION > 3)
#ifdef __TDT__
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE,NULL, AdapterNumbers);
#else
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE,NULL, adapter_num);
#endif
  #else
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE,NULL);
  #endif
    #else /* STLinux 2.2 kernel */
#ifdef __TDT__
    #if (DVB_API_VERSION > 3)
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE,NULL, AdapterNumbers);
    #else
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE,NULL);
    #endif
#else
    Result      = dvb_register_adapter (&DvbContext->DvbAdapter, MODULE_NAME, THIS_MODULE);
#endif
    #endif
    if (Result < 0)
    {
	DVB_ERROR("Failed to register adapter (%d)\n", Result);
	kfree(DvbContext);
	DvbContext      = NULL;
	return -ENOMEM;
    }

    mutex_init (&(DvbContext->Lock));
    mutex_lock (&(DvbContext->Lock));
    /*{{{  Register devices*/
    for (i = 0; i < DVB_MAX_DEVICES_PER_ADAPTER; i++)
    {
	struct DeviceContext_s* DeviceContext   = &DvbContext->DeviceContext[i];
	struct dvb_demux*       DvbDemux        = &DeviceContext->DvbDemux;
	struct dmxdev*          DmxDevice       = &DeviceContext->DmxDevice;
	struct dvb_device*      DvrDevice;

#ifdef __TDT__
               //sylvester: wenn der stream vom user kommt soll WriteToDecoder nix
                //tun, da das ja hier schon passiert. keine ahnung wie man das ansonsten
                //verhindern soll;-)
                DeviceContext->dvr_write = 0;
#endif

	DeviceContext->DvbContext               = DvbContext;
    #if defined (USE_KERNEL_DEMUX)
	memset (DvbDemux, 0, sizeof (struct dvb_demux));
#ifdef __TDT__
        DvbDemux->dmx.capabilities              = DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_MEMORY_BASED_FILTERING | DMX_TS_DESCRAMBLING;
        /* currently only dummy to avoid EINVAL error. Later we need it for second frontend ?! */
        DvbDemux->dmx.set_source                   = SetSource;
#else
	DvbDemux->dmx.capabilities              = DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_MEMORY_BASED_FILTERING;
#endif
	DvbDemux->priv                          = DeviceContext;
	DvbDemux->filternum                     = 40;
	DvbDemux->feednum                       = 40;
	DvbDemux->start_feed                    = StartFeed;
	DvbDemux->stop_feed                     = StopFeed;
#ifndef __TDT__
	DvbDemux->write_to_decoder              = NULL;
#else
    DvbDemux->write_to_decoder              = WriteToDecoder;
#endif
	Result                                  = dvb_dmx_init (DvbDemux);
	if (Result < 0)
	{
	    DVB_ERROR ("dvb_dmx_init failed (errno = %d)\n", Result);
	    return Result;
	}

	memset (DmxDevice, 0, sizeof (struct dmxdev));
	DmxDevice->filternum                    = DvbDemux->filternum;
	DmxDevice->demux                        = &DvbDemux->dmx;
	DmxDevice->capabilities                 = 0;
	Result                                  = dvb_dmxdev_init (DmxDevice, &DvbContext->DvbAdapter);
	if (Result < 0)
	{
	    DVB_ERROR("dvb_dmxdev_init failed (errno = %d)\n", Result);
	    dvb_dmx_release (DvbDemux);
	    return Result;
	}
	DvrDevice                               = DvrInit (DmxDevice->dvr_dvbdev->fops);
	/* Unregister the built-in dvr device and replace it with our own version */
#ifdef __TDT__
        printk("%d: DeviceContext %p, DvbDemux %p, DmxDevice %p\n", i, DeviceContext, DvbDemux, DmxDevice);
#endif
	dvb_unregister_device  (DmxDevice->dvr_dvbdev);
	dvb_register_device (&DvbContext->DvbAdapter,
			     &DmxDevice->dvr_dvbdev,
			     DvrDevice,
			     DmxDevice,
			     DVB_DEVICE_DVR);


	DeviceContext->MemoryFrontend.source    = DMX_MEMORY_FE;
	Result                                  = DvbDemux->dmx.add_frontend (&DvbDemux->dmx, &DeviceContext->MemoryFrontend);
	if (Result < 0)
	{
	    DVB_ERROR ("add_frontend failed (errno = %d)\n", Result);
	    dvb_dmxdev_release (DmxDevice);
	    dvb_dmx_release    (DvbDemux);
	    return Result;
	}
    #else
	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->DemuxDevice,
			     DemuxInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_DEMUX);

	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->DvrDevice,
			     DvrInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_DVR);
	#endif

	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->AudioDevice,
			     AudioInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_AUDIO);

#ifdef __TDT__
        /* register the CA device (e.g. CIMAX) */
        if(i < 2)
#ifndef VIP2_V1
	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->CaDevice,
			     CaInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_CA);
#endif
#else
	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->CaDevice,
			     CaInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_CA);
#endif

	dvb_register_device (&DvbContext->DvbAdapter,
			     &DeviceContext->VideoDevice,
			     VideoInit (DeviceContext),
			     DeviceContext,
			     DVB_DEVICE_VIDEO);                             

	DeviceContext->Id                       = i;
	DeviceContext->DemuxContext             = DeviceContext;        /* wire directly to own demux by default */
	DeviceContext->SyncContext              = DeviceContext;        /* we are our own sync group by default */
	DeviceContext->Playback                 = NULL;
	DeviceContext->StreamType               = STREAM_TYPE_TRANSPORT;
	DeviceContext->DvbContext               = DvbContext;
	DeviceContext->DemuxStream              = NULL;
	DeviceContext->VideoStream              = NULL;
	DeviceContext->AudioStream              = NULL;
	DeviceContext->PlaySpeed                = DVB_SPEED_NORMAL_PLAY;
	DeviceContext->dvr_in                   = kmalloc(65536,GFP_KERNEL); // 128Kbytes is quite a lot per device.
	DeviceContext->dvr_out                  = kmalloc(65536,GFP_KERNEL); // However allocating on each write is expensive.
	DeviceContext->EncryptionOn             = 0;
#ifdef __TDT__
        DeviceContext->VideoPlaySpeed           = DVB_SPEED_NORMAL_PLAY;
        DeviceContext->provideToDecoder = 0;
        DeviceContext->feedPesType = 0;
        mutex_init(&DeviceContext->injectMutex);

        if(i < 3)
        {
          ptiInit(DeviceContext);
        }

        if(i < 1)
        {
          init_e2_proc(DeviceContext);
        }
#endif
    }

    mutex_unlock (&(DvbContext->Lock));

    BackendInit ();

#ifndef __TDT__
    dvp_init();
#endif

    /*}}}  */
#if defined (CONFIG_CPU_SUBTYPE_STX7200)
    avr_init();
#endif 

#ifndef __TDT__

#if defined (CONFIG_CPU_SUBTYPE_STX7105) // || defined (CONFIG_CPU_SUBTYPE_STX7200)
    cap_init();
#endif  

#endif

    linuxdvb_v4l2_init();

    DVB_DEBUG("STM stream device loaded\n");

    return 0;
}

static void __exit StmUnloadModule (void)
{
    int i;

    BackendDelete ();

    for (i = 0; i < DVB_MAX_DEVICES_PER_ADAPTER; i++)
    {
	struct DeviceContext_s* DeviceContext   = &DvbContext->DeviceContext[i];
	struct dvb_demux*       DvbDemux        = &DeviceContext->DvbDemux;
	struct dmxdev*          DmxDevice       = &DeviceContext->DmxDevice;

#if defined (USE_KERNEL_DEMUX)
	if (DmxDevice != NULL)
	{
	    /* We don't need to unregister DmxDevice->dvr_dvbdev as this will be done by dvb_dmxdev_release */
	    dvb_dmxdev_release (DmxDevice);
	}
	if (DvbDemux != NULL)
	{
	    DvbDemux->dmx.remove_frontend (&DvbDemux->dmx, &DeviceContext->MemoryFrontend);
	    dvb_dmx_release    (DvbDemux);
	}
#else
	dvb_unregister_device  (DeviceContext->DemuxDevice);
	dvb_unregister_device  (DeviceContext->DvrDevice);
#endif
	if (DeviceContext->AudioDevice != NULL)
	    dvb_unregister_device  (DeviceContext->AudioDevice);
	if (DeviceContext->VideoDevice != NULL)
	    dvb_unregister_device  (DeviceContext->VideoDevice);

	PlaybackDelete (DeviceContext->Playback);
	DeviceContext->AudioStream              = NULL;
	DeviceContext->VideoStream              = NULL;
	DeviceContext->Playback                 = NULL;
	kfree(DeviceContext->dvr_in);
	kfree(DeviceContext->dvr_out);
    }


    if (DvbContext != NULL)
    {
	dvb_unregister_adapter (&DvbContext->DvbAdapter);
	kfree (DvbContext);
    }
    DvbContext  = NULL;

    DVB_DEBUG("STM stream device unloaded\n");

    return;
}

