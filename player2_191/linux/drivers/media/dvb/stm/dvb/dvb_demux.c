/************************************************************************
Copyright (C) 2006 STMicroelectronics. All Rights Reserved.

This file is part of the Player2 Library.

Player2 is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by the
Free Software Foundation.

Player2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with player2; see the file COPYING.  If not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

The Player2 Library may alternatively be licensed under a proprietary
license from ST.

Source file name : dvb_demux.c
Author :           Julian

Implementation of linux dvb demux hooks

Date        Modification                                    Name
----        ------------                                    --------
01-Nov-06   Created                                         Julian

************************************************************************/

#include <linux/module.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#ifdef __TDT__
#include <linux/dvb/version.h>
#include <linux/dvb/ca.h>
#include "dvb_ca_en50221.h"
#endif

#include "dvb_demux.h"          /* provides kernel demux types */

#include "dvb_module.h"
#include "dvb_audio.h"
#include "dvb_video.h"
#include "dvb_dmux.h"
#include "backend.h"

#ifdef __TDT__
extern int AudioIoctlSetAvSync (struct DeviceContext_s* Context, unsigned int State);
extern int AudioIoctlStop (struct DeviceContext_s* Context);

extern int stpti_start_feed (struct dvb_demux_feed *dvbdmxfeed,
                             struct DeviceContext_s *DeviceContext);
extern int stpti_stop_feed (struct dvb_demux_feed *dvbdmxfeed,
                     struct DeviceContext_s *pContext);
#endif

/********************************************************************************
 *  This file contains the hook functions which allow the player to use the built-in
 *  kernel demux device so that in-mux non audio/video streams can be read out of
 *  the demux device.
 ********************************************************************************/

/*{{{  COMMENT DmxWrite*/
#if 0
/********************************************************************************
 *  \brief      Write user data into player and kernel filter engine
 *              DmxWrite is called by the dvr device write function.  It allows us
 *              to intercept data writes from the user and de blue ray them.
 *              Data is injected into the kernel first to preserve user context.
 ********************************************************************************/
int DmxWrite (struct dmx_demux* Demux, const char* Buffer, size_t Count)
{
    size_t                      DataLeft        = Count;
    int                         Result          = 0;
    unsigned int                Offset          = 0;
    unsigned int                Written         = 0;
    struct dvb_demux*           DvbDemux        = (struct dvb_demux*)Demux->priv;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;

    if (((Count % TRANSPORT_PACKET_SIZE) == 0) || ((Count % BLUERAY_PACKET_SIZE) != 0))
        Context->DmxWrite (Demux, Buffer, Count);
    else
    {
        Offset      = sizeof(unsigned int);
        while (DataLeft > 0)
        {
            Result          = Context->DmxWrite (Demux, Buffer+Offset, TRANSPORT_PACKET_SIZE);
            Offset         += BLUERAY_PACKET_SIZE;
            DataLeft       -= BLUERAY_PACKET_SIZE;
            if (Result < 0)
                return Result;
            else if (Result != TRANSPORT_PACKET_SIZE)
                return Written + Result;
            else
                Written    += BLUERAY_PACKET_SIZE;
        }
    }

    return DemuxInjectFromUser (Context->DemuxStream, Buffer, Count);  /* Pass data to player before putting into the demux */

}
#endif
/*}}}*/
#if 0
/*{{{  StartFeed*/
/********************************************************************************
 *  \brief      Set up player to receive transport stream
 *              StartFeed is called by the demux device immediately before starting
 *              to demux data.
 ********************************************************************************/
static const unsigned int AudioId[DVB_MAX_DEVICES_PER_ADAPTER]  = {DMX_TS_PES_AUDIO0, DMX_TS_PES_AUDIO1, DMX_TS_PES_AUDIO2, DMX_TS_PES_AUDIO3};
static const unsigned int VideoId[DVB_MAX_DEVICES_PER_ADAPTER]  = {DMX_TS_PES_VIDEO0, DMX_TS_PES_VIDEO1, DMX_TS_PES_VIDEO2, DMX_TS_PES_VIDEO3};
int StartFeed (struct dvb_demux_feed* Feed)
{
    struct dvb_demux*           DvbDemux        = Feed->demux;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;
    struct DvbContext_s*        DvbContext      = Context->DvbContext;
    int                         Result          = 0;
    int                         i;
    unsigned int                Video           = false;
    unsigned int                Audio           = false;

    if (Feed->pes_type > DMX_TS_PES_OTHER)
        return -EINVAL;

    switch (Feed->type)
    {
        case DMX_TYPE_TS:
            for (i = 0; i < DVB_MAX_DEVICES_PER_ADAPTER; i++)
            {
                if (Feed->pes_type == AudioId[i])
                {
                    Audio       = true;
                    break;
                }
                if (Feed->pes_type == VideoId[i])
                {
                    Video       = true;
                    break;
                }
            }
            if (!Audio && !Video)
                return 0;

            mutex_lock (&(DvbContext->Lock));
            if ((Context->Playback == NULL) && (Context->DemuxContext->Playback == NULL))
            {
                Result      = DvbPlaybackCreate (&Context->Playback);
                if (Result < 0)
                    return Result;
                if (Context != Context->DemuxContext)
                    Context->DemuxContext->Playback    = Context->Playback;
            }
            if ((Context->DemuxStream == NULL) && (Context->DemuxContext->DemuxStream == NULL))
            {
                Result      = DvbPlaybackAddDemux (Context->Playback, Context->DemuxContext->Id, &Context->DemuxStream);
                if (Result < 0)
                {
                    mutex_unlock (&(DvbContext->Lock));
                    return Result;
                }
                if (Context != Context->DemuxContext)
                    Context->DemuxContext->DemuxStream  = Context->DemuxStream;
            }

            if (Video)
            {
                struct DeviceContext_s* VideoContext    = &Context->DvbContext->DeviceContext[i];

                VideoContext->DemuxContext      = Context;
                VideoIoctlSetId (VideoContext, Feed->pid);
                VideoIoctlPlay (VideoContext);
            }
            else
            {
                struct DeviceContext_s* AudioContext    = &Context->DvbContext->DeviceContext[i];

                AudioContext->DemuxContext      = Context;
                AudioIoctlSetId (AudioContext, Feed->pid);
                AudioIoctlPlay (AudioContext);
            }
            mutex_unlock (&(DvbContext->Lock));

            break;
        case DMX_TYPE_SEC:
            break;
        default:
            return -EINVAL;
    }

    return 0;
}
/*}}}*/
#else
/*{{{  StartFeed*/
/********************************************************************************
 *  \brief      Set up player to receive transport stream
 *              StartFeed is called by the demux device immediately before starting
 *              to demux data.
 ********************************************************************************/
static const unsigned int AudioId[DVB_MAX_DEVICES_PER_ADAPTER]  = {DMX_TS_PES_AUDIO0, DMX_TS_PES_AUDIO1, DMX_TS_PES_AUDIO2, DMX_TS_PES_AUDIO3};
static const unsigned int VideoId[DVB_MAX_DEVICES_PER_ADAPTER]  = {DMX_TS_PES_VIDEO0, DMX_TS_PES_VIDEO1, DMX_TS_PES_VIDEO2, DMX_TS_PES_VIDEO3};
int StartFeed (struct dvb_demux_feed* Feed)
{
    struct dvb_demux*                   DvbDemux        = Feed->demux;
    struct dmxdev_filter*               Filter          = (struct dmxdev_filter*)Feed->feed.ts.priv;
    struct dmx_pes_filter_params*       Params          = &Filter->params.pes;
    struct DeviceContext_s*             Context         = (struct DeviceContext_s*)DvbDemux->priv;
    struct DvbContext_s*                DvbContext      = Context->DvbContext;
    int                                 Result          = 0;
    int                                 i;
    unsigned int                        Video           = false;
    unsigned int                        Audio           = false;
#ifdef __TDT__
    struct DeviceContext_s *AvContext = NULL;
#endif

    DVB_DEBUG ("(demux%d)\n", Context->Id);

#ifdef __TDT__
#ifdef no_subtitles
  if ((Feed->type == DMX_TYPE_TS) && (Feed->pes_type > DMX_TS_PES_OTHER))
    {
      DVB_DEBUG ("pes_type %d > %d (OTHER)>\n", Feed->pes_type,
                 DMX_TS_PES_OTHER);
      return -EINVAL;
    }
#endif
  DVB_DEBUG("t = %d, pt = %d, pid = %d\n", Feed->type, Feed->pes_type, Feed->pid);
#endif

    switch (Feed->type)
    {
        case DMX_TYPE_TS:
            if (Feed->pes_type > DMX_TS_PES_OTHER)
                return -EINVAL;

            for (i = 0; i < DVB_MAX_DEVICES_PER_ADAPTER; i++)
            {
                if (Feed->pes_type == AudioId[i])
                {
                    Audio       = true;
                    break;
                }
                if (Feed->pes_type == VideoId[i])
                {
                    Video       = true;
                    break;
                }
            }
#ifdef __TDT__
          AvContext = &Context->DvbContext->DeviceContext[i];
          //fix freeze if record starts in background
          //AvContext->DemuxContext      = Context;

          //videotext & subtitles (other)
          if ((Feed->pes_type == DMX_TS_PES_TELETEXT) ||
              (Feed->pes_type == DMX_TS_PES_OTHER))
            {
              mutex_lock (&(DvbContext->Lock));
              stpti_start_feed (Feed, Context);
              mutex_unlock (&(DvbContext->Lock));

              break;
            }
#endif

            if (!Audio && !Video)
            {
#ifdef __TDT__
		DVB_DEBUG ("pes_type = %d\n<\n", Feed->pes_type);
#endif
                /*mutex_unlock (&(DvbContext->Lock));  This doesn't look right we haven't taken it yet*/
                return 0;
            }

            mutex_lock (&(DvbContext->Lock));
#ifndef __TDT__
            if ((Video && !Context->VideoOpenWrite) || (Audio && !Context->AudioOpenWrite))
            {
                mutex_unlock (&(DvbContext->Lock));
                return -EBADF;
            }
#endif
            if ((Context->Playback == NULL) && (Context->SyncContext->Playback == NULL))
            {
                Result      = DvbPlaybackCreate (&Context->Playback);
                if (Result < 0)
                {
                    mutex_unlock (&(DvbContext->Lock));
                    return Result;
                }
                Context->SyncContext->Playback      = Context->Playback;
                if (Context->PlaySpeed != DVB_SPEED_NORMAL_PLAY)
                {
                    Result      = VideoIoctlSetSpeed (Context, Context->PlaySpeed);
                    if (Result < 0)
#ifdef __TDT__
                    {
                        mutex_unlock (&(DvbContext->Lock));
                        return Result;
                    }
#else
                        return Result;
#endif
                }
#ifdef __TDT__
		        if ((Context->VideoPlayInterval.start != DVB_TIME_NOT_BOUNDED) ||
                            (Context->VideoPlayInterval.end   != DVB_TIME_NOT_BOUNDED))
		        {
                    Result = VideoIoctlSetPlayInterval (Context, &Context->AudioPlayInterval);
		            if (Result < 0)
                            {
                  	        mutex_unlock (&(DvbContext->Lock));
                  	        return Result;
                            }
		        }
#endif
            }
            else if (Context->Playback == NULL)
                Context->Playback               = Context->SyncContext->Playback;
            else if (Context->SyncContext->Playback == NULL)
                Context->SyncContext->Playback  = Context->Playback;
            else if (Context->Playback != Context->SyncContext->Playback)
                DVB_ERROR ("Context playback not equal to sync context playback\n");

            if (Context->DemuxStream == NULL)
            {
                Result      = DvbPlaybackAddDemux (Context->Playback, Context->DemuxContext->Id, &Context->DemuxStream);
                if (Result < 0)
                {
                    mutex_unlock (&(DvbContext->Lock));
                    return Result;
                }
            }

#ifdef __TDT__
	        if (Video)
	        {
		       stpti_start_feed (Feed, Context);

		       if(Feed->ts_type & TS_DECODER)
		          VideoIoctlSetId (AvContext, Feed->pid);
	        }
	        else if (Audio)
	        {
		       stpti_start_feed (Feed, Context);

		       if(Feed->ts_type & TS_DECODER)
		          AudioIoctlSetId (AvContext, Feed->pid);
	        }
#else
            if (Video)
            {
                struct DeviceContext_s* VideoContext    = &Context->DvbContext->DeviceContext[i];

                VideoContext->DemuxContext      = Context;
                VideoIoctlSetId (VideoContext, Feed->pid | (Params->flags & DMX_FILTER_BY_PRIORITY_MASK));
                VideoIoctlPlay (VideoContext);
                if ((Context->VideoPlayInterval.start != DVB_TIME_NOT_BOUNDED) ||
                    (Context->VideoPlayInterval.end   != DVB_TIME_NOT_BOUNDED))
                    VideoIoctlSetPlayInterval (Context, &Context->AudioPlayInterval);
            }
            else
            {
                struct DeviceContext_s*         AudioContext    = &Context->DvbContext->DeviceContext[i];

                AudioContext->DemuxContext      = Context;
                AudioIoctlSetId (AudioContext, Feed->pid | (Params->flags & DMX_FILTER_BY_PRIORITY_MASK));
                AudioIoctlPlay (AudioContext);
                if ((Context->AudioPlayInterval.start != DVB_TIME_NOT_BOUNDED) ||
                    (Context->AudioPlayInterval.end   != DVB_TIME_NOT_BOUNDED))
                    AudioIoctlSetPlayInterval (Context, &Context->AudioPlayInterval);
            }
#endif
            mutex_unlock (&(DvbContext->Lock));

            break;
        case DMX_TYPE_SEC:
#ifdef __TDT__
            //DVB_DEBUG ("feed type = SEC\n");

            mutex_lock (&(DvbContext->Lock));
            stpti_start_feed (Feed, Context);
            mutex_unlock (&(DvbContext->Lock));
#endif
            break;
        default:
#ifdef __TDT
	        DVB_DEBUG ("< (type = %d unknown\n", Feed->type);
#endif
            return -EINVAL;
    }

    return 0;
}
/*}}}*/
#endif
/*{{{  StopFeed*/
/********************************************************************************
 *  \brief      Shut down this feed
 *              StopFeed is called by the demux device immediately after finishing
 *              demuxing data.
 ********************************************************************************/
int StopFeed (struct dvb_demux_feed* Feed)
{
    struct dvb_demux*           DvbDemux        = Feed->demux;
    struct DeviceContext_s*     Context         = (struct DeviceContext_s*)DvbDemux->priv;
    struct DvbContext_s*        DvbContext      = Context->DvbContext;
    /*int                         Result          = 0;*/
#ifdef __TDT__
    int i                                       = 0;
#endif

    switch (Feed->type)
    {
        case DMX_TYPE_TS:
#ifdef __TDT__
            for (i = 0; i < DVB_MAX_DEVICES_PER_ADAPTER; i++)
            {
                if (Feed->pes_type == AudioId[i])
                {
                    mutex_lock (&(DvbContext->Lock));
                    /*AvContext = &Context->DvbContext->DeviceContext[i];
                    if(Feed->ts_type & TS_DECODER)
                    {
                      AudioIoctlSetAvSync (AvContext, 0);
                      AudioIoctlStop (AvContext);
                    }*/
                    stpti_stop_feed(Feed, Context);
                    mutex_unlock (&(DvbContext->Lock));
                    break;
                }
                if (Feed->pes_type == VideoId[i])
                {
                    mutex_lock (&(DvbContext->Lock));
                    /*AvContext = &Context->DvbContext->DeviceContext[i];
                    if(Feed->ts_type & TS_DECODER)
                      VideoIoctlStop(AvContext, AvContext->VideoState.video_blank);*/
                    stpti_stop_feed(Feed, Context);
                    mutex_unlock (&(DvbContext->Lock));
                    break;
                }
                //videotext & subtitles (other)
                // FIXME: TTX1, TTX2, TTX3, PCR1 etc.
                if ((Feed->pes_type == DMX_TS_PES_TELETEXT) ||
                    (Feed->pes_type == DMX_TS_PES_OTHER))
                {
                    mutex_lock (&(DvbContext->Lock));
                    stpti_stop_feed(Feed, Context);
                    mutex_unlock (&(DvbContext->Lock));
                    break;
                }
                else if (Feed->pes_type == DMX_TS_PES_PCR)
                        break;
            }

            if (i >= DVB_MAX_DEVICES_PER_ADAPTER)
            {
                printk("%s(): INVALID PES TYPE (%d, %d)\n", __func__, Feed->pid, Feed->pes_type);
                return -EINVAL;
            }
            break;
#else
            mutex_lock (&(DvbContext->Lock));

            if (((Feed->pes_type == DMX_TS_PES_VIDEO) && !Context->VideoOpenWrite) ||
                ((Feed->pes_type == DMX_TS_PES_AUDIO) && !Context->AudioOpenWrite))
            {
                mutex_unlock (&(DvbContext->Lock));
                return -EBADF;
            }

            switch (Feed->pes_type)
            {
                case DMX_TS_PES_VIDEO:
                    VideoIoctlStop (Context, Context->VideoState.video_blank);
                    break;
                case DMX_TS_PES_AUDIO:
                    AudioIoctlStop (Context);
                    break;
                case DMX_TS_PES_TELETEXT:
                case DMX_TS_PES_PCR:
                case DMX_TS_PES_OTHER:
                    break;
                default:
                    mutex_unlock (&(DvbContext->Lock));
                    return -EINVAL;
            }
            mutex_unlock (&(DvbContext->Lock));
            /*
            if ((Context->AudioId == DEMUX_INVALID_ID) && (Context->VideoId == DEMUX_INVALID_ID) &&
                (Context->DemuxStream != NULL))
            {
                Result      = DvbPlaybackRemoveDemux (Context->Playback, Context->DemuxStream);
                Context->DemuxContext->DemuxStream      = NULL;
                if (Context != Context->DemuxContext)
                    Context->DemuxContext->DemuxStream  = NULL;
            }
            */
            break;
#endif
        case DMX_TYPE_SEC:
#ifdef __TDT__
            mutex_lock (&(DvbContext->Lock));
            stpti_stop_feed(Feed, Context);
            mutex_unlock (&(DvbContext->Lock));
#endif
            break;
        default:
#ifdef __TDT
	    printk("%s(): INVALID FEED TYPE (%d)\n", __func__, Feed->type);
#endif
            return -EINVAL;
    }
    return 0;
}
/*}}}*/

#ifdef __TDT__
/* Uncomment the define to enable player decoupling from the DVB API.
   With this workaround packets sent to the player do not block the DVB API
   and do not cause the scheduling bug (waiting on buffers during spin_lock).
   However, there is a side effect - playback may disturb recordings. */
#define DECOUPLE_PLAYER_FROM_DVBAPI
#ifndef DECOUPLE_PLAYER_FROM_DVBAPI

/*{{{  WriteToDecoder*/
int WriteToDecoder (struct dvb_demux_feed *Feed, const u8 *buf, size_t count)
{
  struct dvb_demux* demux = Feed->demux;
  struct DeviceContext_s* Context = (struct DeviceContext_s*)demux->priv;
  int j = 0;
  int audio = 0;

  if(Feed->type != DMX_TYPE_TS)
    return 0;

  /* select the context */
  /* no more than two output devices supported */
  switch (Feed->pes_type)
  {
    case DMX_PES_AUDIO0:
      audio = 1;
    case DMX_PES_VIDEO0:
      Context = &Context->DvbContext->DeviceContext[0];
      break;
    case DMX_PES_AUDIO1:
      audio = 1;
    case DMX_PES_VIDEO1:
      Context = &Context->DvbContext->DeviceContext[1];
      break;
    default:
      return 0;
  }
  /* injecting scrambled data crashes the player */
  while (j < count)
  {
      if ((buf[j+3] & 0xc0) > 0)
          return count;
      j+=188;
  }

  /* don't inject if playback is stopped */
  if (audio == 1)
  {
    if (Context->AudioState.play_state == AUDIO_STOPPED)
            return count;
  }
  else if (Context->VideoState.play_state == VIDEO_STOPPED)
            return count;

  return DvbStreamInject(Context->DemuxContext->DemuxStream, buf, count);
}

void demultiplexDvbPackets(struct dvb_demux* demux, const u8 *buf, int count)
{
      dvb_dmx_swfilter_packets(demux, buf, count);
}

#else

/*{{{  WriteToDecoder*/
int WriteToDecoder (struct dvb_demux_feed *Feed, const u8 *buf, size_t count)
{
  struct dvb_demux* demux = Feed->demux;
  struct DeviceContext_s* Context = (struct DeviceContext_s*)demux->priv;

  /* The decoder needs only the video and audio PES.
     For whatever reason the demux provides the video packets twice
     (once as PES_VIDEO and then as PES_PCR). Therefore it is IMPORTANT
     not to overwrite the flag or the PES type. */
  if((Feed->type == DMX_TYPE_TS) &&
     ((Feed->pes_type == DMX_PES_AUDIO0) ||
      (Feed->pes_type == DMX_PES_VIDEO0) ||
      (Feed->pes_type == DMX_PES_AUDIO1) ||
      (Feed->pes_type == DMX_PES_VIDEO1)))
  {
    Context->provideToDecoder = 1;
    Context->feedPesType = Feed->pes_type;
  }

  return 0;
}
/*}}}  */

int writeToDecoder (struct dvb_demux *demux, int pes_type, const u8 *buf, size_t count)
{
  struct DeviceContext_s* Context = (struct DeviceContext_s*)demux->priv;
  int j = 3;

  /* select the context */
  /* no more than two output devices supported */
  /* don't inject if playback is stopped */
  switch (pes_type)
  {
    case DMX_PES_AUDIO0:
      Context = &Context->DvbContext->DeviceContext[0];
      if (Context->AudioState.play_state == AUDIO_STOPPED)
              return count;
      break;
    case DMX_PES_VIDEO0:
      Context = &Context->DvbContext->DeviceContext[0];
      if (Context->VideoState.play_state == VIDEO_STOPPED)
              return count;
      break;
    case DMX_PES_AUDIO1:
      Context = &Context->DvbContext->DeviceContext[1];
      if (Context->AudioState.play_state == AUDIO_STOPPED)
              return count;
      break;
    case DMX_PES_VIDEO1:
      Context = &Context->DvbContext->DeviceContext[1];
      if (Context->VideoState.play_state == VIDEO_STOPPED)
              return count;
      break;
    default:
      return 0;
  }

  /* injecting scrambled data crashes the player */
  while (j < count)
  {
      if ((buf[j] & 0xc0) > 0)
          return count;
      j+=188;
  }

  return DvbStreamInject(Context->DemuxContext->DemuxStream, buf, count);
}

static inline u16 ts_pid(const u8 *buf)
{
        return ((buf[1] & 0x1f) << 8) + buf[2];
}

void demultiplexDvbPackets(struct dvb_demux* demux, const u8 *buf, int count)
{
  int first = 0;
  int next = 0;
  int cnt = 0;
  int diff_count;
  const u8 *first_buf;
  u16 pid, firstPid;

  struct DeviceContext_s* Context = (struct DeviceContext_s*)demux->priv;

  /* Group the packets by the PIDs and feed them into the kernel demuxer.
     If there is data for the decoder we will be informed via the callback.
     After the demuxer finished its work on the packet block that block is
     fed into the decoder if required.
     This workaround eliminates the scheduling bug caused by waiting while
     the demux spin is locked. */

#if DVB_API_VERSION > 3

  while (count > 0)
  {
    first = next;
    cnt = 0;
    firstPid = ts_pid(&buf[first]);
    while(count > 0)
    {
      count--;
      next += 188;
      cnt++;
      pid = ts_pid(&buf[next]);
      if((pid != firstPid) || (cnt > 8))
          break;
    }
    if((next - first) > 0)
    {
      mutex_lock_interruptible(&Context->injectMutex);

      /* reset the flag (to be set by the callback */
      Context->provideToDecoder = 0;
      dvb_dmx_swfilter_packets(demux, buf + first, cnt);
      if(Context->provideToDecoder)
      {
        /* the demuxer indicated that the packets are for the decoder */
        writeToDecoder(demux, Context->feedPesType, buf + first, next - first);
      }
      mutex_unlock(&Context->injectMutex);
    }
  }
#else

  firstPid = ts_pid(&buf[first]);
  while(count)
  {
    count--;
    next += 188;
    cnt++;
    if(cnt > 8 || ts_pid(&buf[next]) != firstPid || !count || buf[next] != 0x47)
    {
      diff_count = next - first;
      first_buf = buf + first;

      mutex_lock_interruptible(&Context->injectMutex);

      // reset the flag (to be set by the callback //
      Context->provideToDecoder = 0;

      spin_lock(&demux->lock);

      dvb_dmx_swfilter_packet(demux, first_buf, diff_count);
      spin_unlock(&demux->lock);

      // the demuxer indicated that the packets are for the decoder //
      if(Context->provideToDecoder)
        writeToDecoder(demux, Context->feedPesType, first_buf, diff_count);

      mutex_unlock(&Context->injectMutex);

      first = next;
      cnt = 0;
      firstPid = ts_pid(&buf[first]);
    }
  }

#endif
}
#endif
#endif

