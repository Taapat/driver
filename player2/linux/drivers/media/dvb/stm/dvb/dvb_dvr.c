/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : dvb_dvr.c
Author :           Julian

Implementation of linux dvb dvr input device

Date        Modification                                    Name
----        ------------                                    --------
01-Nov-06   Created                                         Julian

************************************************************************/

#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <linux/dvb/dmx.h>

#include "dvb_module.h"
#include "dvb_dvr.h"
#include "backend.h"

#include "dvb_v4l2.h"
#ifdef CONFIG_STM_TKDMA
#include <linux/stm/tkdma.h>
#endif

#ifdef __TDT__
#include <linux/delay.h>

extern void demultiplexDvbPackets(struct dvb_demux* demux, const u8 *buf, int count);
extern int stm_tsm_inject_user_data(const char __user *data, off_t size);
#endif

static int      DvrOpen        (struct inode*           Inode,
                                struct file*            File);
static int      DvrRelease     (struct inode*           Inode,
                                struct file*            File);
static ssize_t  DvrWrite       (struct file*            File,
                                const char __user*      Buffer,
                                size_t                  Count,
                                loff_t*                 ppos);

static struct file_operations   OriginalDvrFops;
static struct file_operations   DvrFops;

static struct dvb_device        DvrDevice =
{
        priv:            NULL,
        users:           1,
        readers:         1,
        writers:         1,
        fops:            &DvrFops,
};

#ifdef __TDT__
extern int swts;
#endif

struct dvb_device* DvrInit (struct file_operations* KernelDvrFops)
{
    /*
     * Copy the fops table from the standard dvr fops.  We use our own versions of
     * write, open and release so we can convert blueray content and detect when
     * the device is opened and closed allowing us to create/destroy the player demux.
     */
    memcpy (&OriginalDvrFops, KernelDvrFops, sizeof (struct file_operations));
    memcpy (&DvrFops, KernelDvrFops, sizeof (struct file_operations));

    DvrFops.owner                      = THIS_MODULE;
    DvrFops.open                       = DvrOpen;
    DvrFops.release                    = DvrRelease;
    DvrFops.write                      = DvrWrite;

    return &DvrDevice;
}

static int DvrOpen     (struct inode*     Inode,
                        struct file*      File)
{
#ifdef __TDT__
    struct dvb_device*          DvbDevice       = (struct dvb_device*)File->private_data;
    struct dmxdev*              DmxDevice       = (struct dmxdev*)DvbDevice->priv;
    struct dvb_demux*           DvbDemux        = (struct dvb_demux*)DmxDevice->demux->priv;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;
#endif
    DVB_DEBUG("%p, %x\n", DvbDemux, File->f_flags & O_ACCMODE);

#if 0
    if ((Context->Playback == NULL) && (Context->DemuxContext->Playback == NULL))
    {
        Result      = PlaybackCreate (&Context->Playback);
        if (Result < 0)
            return Result;
        Result      = PlaybackSetSpeed (Context->Playback, Context->PlaySpeed);
        if (Result < 0)
            return Result;
        if (Context != Context->DemuxContext)
            Context->DemuxContext->Playback    = Context->Playback;
    }
    if ((Context->DemuxStream == NULL) && (Context->DemuxContext->DemuxStream == NULL))
    {
        Result      = PlaybackAddDemux (Context->Playback, Context->DemuxContext->Id, &Context->DemuxStream);
        if (Result < 0)
            return Result;
        if (Context != Context->DemuxContext)
            Context->DemuxContext->DemuxStream  = Context->DemuxStream;
    }
#endif
	//sylvester: wenn der stream vom user kommt soll WriteToDecoder nix
	//tun, da das ja hier schon passiert. keine ahnung wie man das ansonsten
	//verhindern soll;-)
	Context->dvr_write = 0;

    return OriginalDvrFops.open (Inode, File);

}

static int DvrRelease  (struct inode*  Inode,
                        struct file*   File)
{
    struct dvb_device*          DvbDevice       = (struct dvb_device*)File->private_data;
    struct dmxdev*              DmxDevice       = (struct dmxdev*)DvbDevice->priv;
    struct dvb_demux*           DvbDemux        = (struct dvb_demux*)DmxDevice->demux->priv;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;
    int                         Result          = 0;


#ifdef __TDT__
    DVB_DEBUG("%p, %x\n", DvbDemux, File->f_flags & O_ACCMODE);

//Dagobert: This is also responsible for the crash when ending timeshift.
//Not sure if this is a good idea, must be tested
#if 0
    //dagobert: the dvr device can be opened for
    //recording. so for this case it is a bad idea
    //to destroy the live stream playback ;-)
    if (Context->dvr_write == 1)
    {

       if (Context->DemuxStream != NULL)
       {
           Result      = PlaybackRemoveDemux (Context->Playback, Context->DemuxStream);
           Context->DemuxStream                    = NULL;
           if (Context != Context->DemuxContext)
               Context->DemuxContext->DemuxStream  = NULL;
       }

       /* Check to see if audio and video have also finished so we can release the playback */
       if ((Context->AudioStream == NULL) && (Context->VideoStream == NULL) && (Context->Playback != NULL))
       {
           /* Try and delete playback then set our demux to Null if succesful or not.  If we fail someone else
              is still using it but we are done. */
           if (PlaybackDelete (Context->Playback) == 0)
               DVB_TRACE("Playback deleted successfully\n");

           Context->Playback               = NULL;
           Context->StreamType             = STREAM_TYPE_TRANSPORT;
           Context->PlaySpeed              = DVB_SPEED_NORMAL_PLAY;
           Context->PlayInterval.start     = DVB_TIME_NOT_BOUNDED;
           Context->PlayInterval.end       = DVB_TIME_NOT_BOUNDED;
           Context->SyncContext            = Context;
       }

       Context->StreamType         = STREAM_TYPE_TRANSPORT;
    }	
#endif
    Result = OriginalDvrFops.release (Inode, File);

    //sylvester: wenn der stream vom user kommt soll WriteToDecoder nix
    //tun, da das ja hier schon passiert. keine ahnung wie man das ansonsten
    //verhindern soll;-)
    Context->dvr_write = 0;

    return Result;
#else
    DVB_DEBUG("\n");

    if (Context->DemuxStream != NULL)
    {
        Result      = PlaybackRemoveDemux (Context->Playback, Context->DemuxStream);
        Context->DemuxStream                    = NULL;
        if (Context != Context->DemuxContext)
            Context->DemuxContext->DemuxStream  = NULL;
    }

    /* Check to see if audio and video have also finished so we can release the playback */
    if ((Context->AudioStream == NULL) && (Context->VideoStream == NULL) && (Context->Playback != NULL))
    {
        /* Try and delete playback then set our demux to Null if succesful or not.  If we fail someone else
           is still using it but we are done. */
        if (PlaybackDelete (Context->Playback) == 0)
            DVB_TRACE("Playback deleted successfully\n");

        Context->Playback               = NULL;
        Context->StreamType             = STREAM_TYPE_TRANSPORT;
        Context->PlaySpeed              = DVB_SPEED_NORMAL_PLAY;
        Context->PlayInterval.start     = DVB_TIME_NOT_BOUNDED;
        Context->PlayInterval.end       = DVB_TIME_NOT_BOUNDED;
        Context->SyncContext            = Context;
    }

    Context->StreamType         = STREAM_TYPE_TRANSPORT;

    return OriginalDvrFops.release (Inode, File);
#endif
}

static ssize_t DvrWrite (struct file *File, const char __user* Buffer, size_t Count, loff_t* ppos)
{
    struct dvb_device*          DvbDevice       = (struct dvb_device*)File->private_data;
    struct dmxdev*              DmxDevice       = (struct dmxdev*)DvbDevice->priv;
    struct dvb_demux*           DvbDemux        = (struct dvb_demux*)DmxDevice->demux->priv;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;
    int                         Result          = 0;
#ifdef __TDT__
    // attach to the video stream context
    struct DeviceContext_s* Context0 = &Context->DvbContext->DeviceContext[0];
#endif

    if (!DmxDevice->demux->write)
        return -EOPNOTSUPP;

    if ((File->f_flags & O_ACCMODE) != O_WRONLY)
        return -EINVAL;

#ifdef __TDT__
    while(1)
    {
      // Check whether a video stream is available and in FREEZED state.
      // If the video stream is freezed and the number of buffers with
      // decoded video frames exceeds a limit then sleep until the video
      // stream is playing or the write operation is cancelled.
      // It solves the issue with the first timeshift start after reboot.
      // It also allows e2 to interrupt the video stream while it is
      // paused (until now e2 kept sending SIGUSR1 to the file push thread).
      if((Context0->VideoStream != NULL) &&
         (Context0->VideoState.play_state == VIDEO_FREEZED))
      {
         int NumberOfBuffers = 0;
         int BuffersInUse = 0;

         StreamGetDecodeBufferPoolStatus(Context0->VideoStream,
			  &NumberOfBuffers,&BuffersInUse);
         
			if(BuffersInUse > 5)
         {
           // 40 milliseconds corresponds to one full frame
			  if(msleep_interruptible(40) != 0)
           {
             DVB_DEBUG("interrupted\n");
             return 0;
           }
         }
         else
         {
            break;
         }
      }
      else
        break;
    }
#endif

    if (mutex_lock_interruptible (&DmxDevice->mutex))
        return -ERESTARTSYS;

#if 0 // We should enable this for security reasons, just want to check the impact because it can be performed inline.
    if (!access_ok(VERIFY_READ, Buffer, Count))
        return -EINVAL;
#endif

#ifdef __TDT__
     //sylvester: wenn der stream vom user kommt soll WriteToDecoder nix
     //tun, da das ja hier schon passiert. keine ahnung wie man das ansonsten
     //verhindern soll;-)
     Context->dvr_write = 1;
#endif

    /*
     * Assume that we have a blueray packet if content size is divisible by 192 but not by 188
     * If ordinary ts call demux write function on whole lot.  If bdts give data to write function
     * 188 bytes at a time.
     */
    if ((((Count % TRANSPORT_PACKET_SIZE) == 0) || ((Count % BLUERAY_PACKET_SIZE) != 0)) && !Context->EncryptionOn)
    {
#ifdef __TDT__
        if (swts)
	{
	   /* inject data through tsm */
           stm_tsm_inject_user_data(Buffer, Count);
        }
        else
           demultiplexDvbPackets (DvbDemux, Buffer, Count/188);
	
        mutex_unlock (&DmxDevice->mutex);

        //printk("Context %p, DvbDemux %p, DmxDevice %p\n",
        //	Context, DvbDemux, DmxDevice);

        //Dagobert: dvbtest does not care the return value but e2 does.
        //StreamInject seems to return zero if it has injected all
        //(must be investigate for further versions and maybe for this).
        if (Result == 0) 
           Result = Count;
#else
        Result = DmxDevice->demux->write (DmxDevice->demux, Buffer, Count);
        mutex_unlock (&DmxDevice->mutex);

        if (Context->DemuxStream)
        {
           mutex_lock (&(Context->VideoWriteLock));
           Result  = StreamInject (Context->DemuxStream, Buffer, Count);
           mutex_unlock (&(Context->VideoWriteLock)); 
        }
#endif
        return Result;

    }
    else if ( (Count % (192*32)) || (!Context->EncryptionOn))
    {
       int n;

       for (n=0;n<Count;n+=192)
          Result += DmxDevice->demux->write (DmxDevice->demux, &Context->dvr_out[n+4], TRANSPORT_PACKET_SIZE) + 4;
       mutex_unlock (&DmxDevice->mutex);

       if (Context->DemuxStream)
       {
          mutex_lock (&(Context->VideoWriteLock));
          Result  = StreamInject (Context->DemuxStream, Buffer, Count);
          mutex_unlock (&(Context->VideoWriteLock));
       }
       return Result;
    }
#ifdef CONFIG_STM_TKDMA
    else
    {
        int n;
        unsigned long ptr = (unsigned long)stm_v4l2_findbuffer((unsigned long)Buffer,Count,0);

        if (ptr && (ptr & ~31))
        {
          tkdma_bluray_decrypt_data(&Context->dvr_out[0],(void*)ptr,Count / (32*192),0); // Need to do an interruptibility test
        } else {
          copy_from_user(Context->dvr_in,&Buffer[0],Count);
          //copy_from_user(&Context->dvr_out[16],Buffer,16);
          tkdma_bluray_decrypt_data(&Context->dvr_out[0],Context->dvr_in,Count / (32*192),0); //Need to do an interruptibility test       
        }

#if 1
        for (n=0;n<Count;n+=192)
             Result += DmxDevice->demux->write (DmxDevice->demux, &Context->dvr_out[n+4], TRANSPORT_PACKET_SIZE) + 4;
#endif

        mutex_unlock (&DmxDevice->mutex);

	if (Context->DemuxStream)
	{
		mutex_lock (&(Context->VideoWriteLock));
		Result  = StreamInject (Context->DemuxStream, &Context->dvr_out[0], Count);
		mutex_unlock (&(Context->VideoWriteLock));
	}
        return Result;
    }
#endif    

    return Result;
}

