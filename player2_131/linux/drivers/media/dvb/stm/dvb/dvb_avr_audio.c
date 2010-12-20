/***********************************************************************
 *
 * File: media/dvb/stm/dvb/dvb_avr_audio.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * V4L2 dvp output device driver for ST SoC display subsystems.
 *
\***********************************************************************/

#include <asm/page.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/videodev.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include "dvb_module.h"
#include "stm_v4l2.h"
#include "backend.h"
#include "osdev_device.h"
#include "ksound.h"
#include "linux/dvb/stm_ioctls.h"
#include "pseudo_mixer.h"
#include "monitor_inline.h"

#include "dvb_avr.h"
#include "dvb_avr_audio.h"

/******************************
 * FUNCTION PROTOTYPES
 ******************************/

extern struct class_device* player_sysfs_get_class_device(void *playback, void* stream);
extern int player_sysfs_get_stream_id(struct class_device* stream_dev);
extern void player_sysfs_new_attribute_notification(struct class_device *stream_dev);

/******************************
 * Private CONSTANTS and MACROS
 ******************************/

#define AUDIO_CAPTURE_THREAD_PRIORITY	40

static snd_pcm_uframes_t AUDIO_PERIOD_FRAMES	= 1024;
static snd_pcm_uframes_t AUDIO_BUFFER_FRAMES	= 20480; /*10240*/

#define AUDIO_DEFAULT_SAMPLE_RATE 	44100
#define AUDIO_CHANNELS 			2
#define AUDIO_SAMPLE_DEPTH 		32
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#	define AUDIO_CAPTURE_CARD 0
#	if defined(CONFIG_CPU_SUBTYPE_STX7200)
#		define AUDIO_CAPTURE_PCM 7
#	elif defined(CONFIG_CPU_SUBTYPE_STX7111)
#		define AUDIO_CAPTURE_PCM 3
#               warning Need to check this value
#	elif defined(CONFIG_CPU_SUBTYPE_STX7141)
#		define AUDIO_CAPTURE_PCM 3
#               warning Need to check this value
#	elif defined(CONFIG_CPU_SUBTYPE_STX7105)
#		define AUDIO_CAPTURE_PCM 3
#               warning Need to check this value
#	elif defined(CONFIG_CPU_SUBTYPE_STB7100) || defined (CONFIG_CPU_SUBTYPE_STX7100)
#		define AUDIO_CAPTURE_PCM 3
#	else
#		error Unsupported platform!
#	endif
#else /* STLinux 2.2 kernel */
#	define AUDIO_CAPTURE_CARD 6
#	define AUDIO_CAPTURE_PCM 0
#endif

#define MID(x,y) ((x+y)/2)

/******************************
 * GLOBAL VARIABLES
 ******************************/


/**
 * Lookup table to help the sysfs callbacks locate the context structure.
 *
 * It really is rather difficult to get hold of a context variable from a sysfs callback. The sysfs module
 * does however provide a means for us to discover the stream id. Since the sysfs attribute is unique to
 * each stream we can use the id to index into a table and thus grab hold of a context variable.
 *
 * Thus we are pretty much forced to use a global variable but at least it doesn't stop us being
 * multi-instance.
 */
static avr_v4l2_audio_handle_t *AudioContextLookupTable[16];


/******************************
 * PES HEADER UTILS
 ******************************/


// PES header creation utilities
#define INVALID_PTS_VALUE               0x200000000ull
#define FIRST_PES_START_CODE            0xba
#define LAST_PES_START_CODE             0xbf
#define VIDEO_PES_START_CODE(C)         ((C&0xf0)==0xe0)
#define AUDIO_PES_START_CODE(C)         ((C&0xc0)==0xc0)
#define PES_START_CODE(C)               (VIDEO_PES_START_CODE(C) || AUDIO_PES_START_CODE(C) || \
                                        ((C>=FIRST_PES_START_CODE) && (C<=LAST_PES_START_CODE)))
#define PES_HEADER_SIZE 32 //bytes
#define MAX_PES_PACKET_SIZE                     65400

#define PES_AUDIO_PRIVATE_HEADER_SIZE   4//16                                // consider maximum private header size.
#define PES_AUDIO_HEADER_SIZE           (32 + PES_AUDIO_PRIVATE_HEADER_SIZE)

#define PES_PRIVATE_STREAM1 0xbd
#define PES_PADDING_STREAM  0xbe
#define PES_PRIVATE_STREAM2 0xbf


typedef struct BitPacker_s
{
    unsigned char*      Ptr;                                    /* write pointer */
    unsigned int        BitBuffer;                              /* bitreader shifter */
    int                 Remaining;                              /* number of remaining in the shifter */
} BitPacker_t;


static void PutBits(BitPacker_t *ld, unsigned int code, unsigned int length)
{
    unsigned int bit_buf;
    int bit_left;

    bit_buf = ld->BitBuffer;
    bit_left = ld->Remaining;

    if (length < bit_left)
    {
        /* fits into current buffer */
        bit_buf = (bit_buf << length) | code;
        bit_left -= length;
    }
    else
    {
        /* doesn't fit */
        bit_buf <<= bit_left;
        bit_buf |= code >> (length - bit_left);
        ld->Ptr[0] = (char)(bit_buf >> 24);
        ld->Ptr[1] = (char)(bit_buf >> 16);
        ld->Ptr[2] = (char)(bit_buf >> 8);
        ld->Ptr[3] = (char)bit_buf;
        ld->Ptr   += 4;
        length    -= bit_left;
        bit_buf    = code & ((1 << length) - 1);
        bit_left   = 32 - length;
    }

    /* writeback */
    ld->BitBuffer = bit_buf;
    ld->Remaining = bit_left;
}


static void FlushBits(BitPacker_t * ld)
{
    ld->BitBuffer <<= ld->Remaining;
    while (ld->Remaining < 32)
    {
        *ld->Ptr++ = ld->BitBuffer >> 24;
        ld->BitBuffer <<= 8;
        ld->Remaining += 8;
    }
    ld->Remaining = 32;
    ld->BitBuffer = 0;
}


static int InsertPrivateDataHeader(unsigned char *data, int payload_size, int sampling_frequency)
{
    BitPacker_t ld2 = {data, 0, 32};
    //int HeaderLength = PES_AUDIO_PRIVATE_HEADER_SIZE;

    PutBits(&ld2, payload_size, 16); // PayloadSize
    PutBits(&ld2, 2, 4);             // ChannelAssignment  :: { 2 = Stereo}
    PutBits(&ld2, sampling_frequency, 4);   // SamplingFrequency  :: { 0 = 44 ; 1 = BD_48k ; 2 = 88k; 3 = 176k; 4 = BD_96k; 5 = BD=192k; 6 = 32k; 7 = 16k; 8 = 22k; 9 = 24k}
    PutBits(&ld2, 0, 2);             // BitsPerSample      :: { 0 = BD_reserved/32bit  ; 1 = BD_16bit ; 2 = BD_20bit; 3 = BD_24bits }
    PutBits(&ld2, 0, 1);             // StartFlag --> EmphasisFlag :: FALSE;
    PutBits(&ld2, 0, 5);             // reserved.

	FlushBits(&ld2);

	return (ld2.Ptr - data);
}


static int InsertPesHeader (unsigned char *data, int size, unsigned char stream_id, unsigned long long int pts, int pic_start_code)
{
    BitPacker_t ld2 = {data, 0, 32};

    if (size>MAX_PES_PACKET_SIZE)
        DVB_DEBUG("Packet bigger than 63.9K eeeekkkkk\n");

    PutBits(&ld2,0x0  ,8);
    PutBits(&ld2,0x0  ,8);
    PutBits(&ld2,0x1  ,8);  // Start Code
    PutBits(&ld2,stream_id ,8);  // Stream_id = Audio Stream
    //4
    PutBits(&ld2,size + 3 + (pts != INVALID_PTS_VALUE ? 5:0) + (pic_start_code ? (5/*+1*/) : 0),16); // PES_packet_length
    //6 = 4+2
    PutBits(&ld2,0x2  ,2);  // 10
    PutBits(&ld2,0x0  ,2);  // PES_Scrambling_control
    PutBits(&ld2,0x0  ,1);  // PES_Priority
    PutBits(&ld2,0x1  ,1);  // data_alignment_indicator
    PutBits(&ld2,0x0  ,1);  // Copyright
    PutBits(&ld2,0x0  ,1);  // Original or Copy
    //7 = 6+1

    if (pts!=INVALID_PTS_VALUE)
        PutBits(&ld2,0x2 ,2);
    else
        PutBits(&ld2,0x0 ,2);  // PTS_DTS flag

    PutBits(&ld2,0x0 ,1);  // ESCR_flag
    PutBits(&ld2,0x0 ,1);  // ES_rate_flag
    PutBits(&ld2,0x0 ,1);  // DSM_trick_mode_flag
    PutBits(&ld2,0x0 ,1);  // additional_copy_ingo_flag
    PutBits(&ld2,0x0 ,1);  // PES_CRC_flag
    PutBits(&ld2,0x0 ,1);  // PES_extension_flag
    //8 = 7+1

    if (pts!=INVALID_PTS_VALUE)
        PutBits(&ld2,0x5,8);
    else
        PutBits(&ld2,0x0 ,8);  // PES_header_data_length
    //9 = 8+1

    if (pts!=INVALID_PTS_VALUE)
    {
        PutBits(&ld2,0x2,4);
        PutBits(&ld2,(pts>>30) & 0x7,3);
        PutBits(&ld2,0x1,1);
        PutBits(&ld2,(pts>>15) & 0x7fff,15);
        PutBits(&ld2,0x1,1);
        PutBits(&ld2,pts & 0x7fff,15);
        PutBits(&ld2,0x1,1);
    }
    //14 = 9+5

    if (pic_start_code)
    {
        PutBits(&ld2,0x0 ,8);
        PutBits(&ld2,0x0 ,8);
        PutBits(&ld2,0x1 ,8);  // Start Code
        PutBits(&ld2,pic_start_code & 0xff ,8);  // 00, for picture start
        PutBits(&ld2,(pic_start_code >> 8 )&0xff,8);  // For any extra information (like in mpeg4p2, the pic_start_code)
        //14 + 4 = 18
    }

    FlushBits(&ld2);

    return (ld2.Ptr - data);
}


/******************************
 * SYSFS TOOLS
 ******************************/


static ssize_t AvrAudioSysfsShowEmergencyMute (struct class_device *class_dev, char *buf)
{
    int streamid = player_sysfs_get_stream_id(class_dev);
    avr_v4l2_audio_handle_t *AudioContext;
    const char *value;

    BUG_ON(streamid < 0 || streamid >= ARRAY_SIZE(AudioContextLookupTable));
    AudioContext = AudioContextLookupTable[streamid];

    switch (AudioContext->EmergencyMuteReason)
    {
        case 0:
    	value = "None"; break;
        case 1:
    	value = "User"; break;
        case 2:
    	value = "Accelerated"; break;
        case 3:
    	value = "Sample Rate Change"; break;
        case 4:
    	value = "Error"; break;
        default:
    	return sprintf(buf, "This status does not exist\n"); break;
    }
    return sprintf(buf, "%s\n", value);
}


CLASS_DEVICE_ATTR(emergency_mute, S_IRUGO, AvrAudioSysfsShowEmergencyMute, NULL);


static int AvrAudioSysfsCreateEmergencyMute (avr_v4l2_audio_handle_t *AudioContext)
{
    int Result 						= 0;
    const struct class_device_attribute* attribute 	= NULL;
    playback_handle_t playerplayback 			= NULL;
    stream_handle_t playerstream 			= NULL;
    int streamid;

    attribute = &class_device_attr_emergency_mute;

    Result = StreamGetPlayerEnvironment (AudioContext->DeviceContext->AudioStream, &playerplayback, &playerstream);
    if (Result < 0) {
	DVB_ERROR("StreamGetPlayerEnvironment failed\n");
	return -1;
    }

    AudioContext->EmergencyMuteClassDevice = player_sysfs_get_class_device(playerplayback, playerstream);
    if (AudioContext->EmergencyMuteClassDevice == NULL) {
    	DVB_ERROR("get_class_device failed -> cannot create attribute \n");
        return -1;
    }

    streamid = player_sysfs_get_stream_id(AudioContext->EmergencyMuteClassDevice);
    if (streamid < 0 || streamid >= ARRAY_SIZE(AudioContextLookupTable)) {
	DVB_ERROR("streamid out of range -> cannot create attribute\n");
	return -1;
    }

    AudioContextLookupTable[streamid] = AudioContext;

    Result  = class_device_create_file(AudioContext->EmergencyMuteClassDevice, attribute);
    if (Result) {
	printk("Error in %s: class_device_create_file failed (%d)\n", __FUNCTION__, Result);
        return -1;
    }

    player_sysfs_new_attribute_notification(AudioContext->EmergencyMuteClassDevice);

    return 0;
}


/******************************
 * INJECTOR THREAD AND RELATED TOOLS
 ******************************/


static int AvrAudioXrunRecovery(ksnd_pcm_t *handle, int err)
{
    if (err == -EPIPE) {	/* under-run */

	/*err = snd_pcm_prepare(handle); */
	err = ksnd_pcm_prepare(handle);
	if (err < 0)
		DVB_ERROR("Can't recovery from underrun, prepare failed: %d\n", err);
	return 0;
    }

    else if (err == -ESTRPIPE) {
#if 0
	while ((err = snd_pcm_resume(handle)) == -EAGAIN) sleep(1);	/* wait until the suspend flag is released */

	if (err < 0) {
	    err = snd_pcm_prepare(handle);
	    if (err < 0)
		DVB_ERROR("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
	}
#endif
	BUG();
	return -1;
    }
    else
    {
	DVB_ERROR("This error code (err = %d) is not handled\n", err);
	return -1;
    }

    return err;
}


static int AvrAudioCalculateSampleRate (avr_v4l2_audio_handle_t *AudioContext,
	                     snd_pcm_sframes_t initial_delay, snd_pcm_sframes_t final_delay,
	    		     unsigned long long initial_time, unsigned long long final_time)
{
    int ret = 0;

    unsigned int SampleRate;
    unsigned int DiscreteSampleRate;

//    if (initial_time == final_time)
//    {
//	DVB_ERROR("Initial and final time are equal\n");
//	return -EINVAL;
//    }

    SampleRate = (((unsigned long long)(AUDIO_PERIOD_FRAMES - initial_delay + final_delay)) * 1000000ull) / (final_time - initial_time);
    //DVB_DEBUG("Calculated SampleRate = %llu\n", SampleRate);


    if (SampleRate <= MID(16000,  22050))  {
	SampleRate = 16000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_16000;
    }
    else if (SampleRate <= MID(22050,  24000))  {
	SampleRate = 22050;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_22050;
    }
    else if (SampleRate <= MID(24000,  32000))  {
	SampleRate = 24000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_24000;
    }
    else if (SampleRate <= MID(32000,  44100))  {
	SampleRate = 32000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_32000;
    }
    else if (SampleRate <= MID(44100,  48000))  {
	SampleRate = 44100;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_44100;
    }
    else if (SampleRate <= MID(48000,  64000))  {
	SampleRate = 48000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_48000;
    }
    else if (SampleRate <= MID(64000,  88200))  {
	SampleRate = 64000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_64000;
    }
    else if (SampleRate <= MID(88200,  96000))  {
	SampleRate = 88200;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_88200;
    }
    else if (SampleRate <= MID(96000,  128000)) {
	SampleRate = 96000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_96000;
    }
    else if (SampleRate <= MID(128000,  176400)) {
	SampleRate = 128000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_128000;
    }
    else if (SampleRate <= MID(176400, 192000)) {
	SampleRate = 176000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_176400;
    }
    else {
	SampleRate = 192000;
	DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_192000;
    }

    AudioContext->SampleRate = SampleRate;
    AudioContext->DiscreteSampleRate = DiscreteSampleRate;

    if (AudioContext->PreviousSampleRate != SampleRate)
    {
	// ok, if overrrun or underrun, then I have to keep the same frequency I had before, as
	// the new calculated one is not going to be reliable
	if (AudioContext->EmergencyMuteReason == EMERGENCY_MUTE_REASON_ERROR)
		return 0;

        ret = AvrAudioSetEmergencyMuteReason(AudioContext, EMERGENCY_MUTE_REASON_SAMPLE_RATE_CHANGE);
        if (ret < 0) {
		DVB_ERROR("dvp_deploy_emergency_mute failed\n");
		return -EINVAL;
        }

        DVB_DEBUG("New SampleRate = %u\n", SampleRate);
        AudioContext->PreviousSampleRate = SampleRate;

	if ((SampleRate == 64000) || (SampleRate == 128000))
	{
	    DVB_ERROR("SampleRate not supported \n");
	    return 1;
	}
        {
           unsigned int Parameters[MONITOR_PARAMETER_COUNT];
           memset (Parameters, 0, sizeof (Parameters));
           Parameters[0]        = SampleRate;
           MonitorSignalEvent (MONITOR_EVENT_AUDIO_SAMPLING_RATE_CHANGE, Parameters, "New sample rate detected");
        }
    }

    if ((SampleRate == 64000) || (SampleRate == 128000))
	    return 1;

    return 0;
}


static int AvrAudioSendPacketToPlayer(avr_v4l2_audio_handle_t *AudioContext,
		                      snd_pcm_uframes_t capture_frames, snd_pcm_uframes_t capture_offset,
		                      const snd_pcm_channel_area_t *capture_areas,
		                      snd_pcm_sframes_t DelayInSamples, unsigned long long time_average)
{
    unsigned char PesHeader[PES_AUDIO_HEADER_SIZE];
    int Result	 				= 0;
    unsigned int HeaderLength 			= 0;
    unsigned long long audioPts			= 0;
    unsigned long long DelayInMicroSeconds 	= 0;
    unsigned long long time_absolute 		= 0;
    unsigned long long NativeTime 		= 0;
    size_t AudioDataSize 			= 0;
    void* AudioData 				= NULL;

    // Build audio data

    AudioData 		= capture_areas[0].addr + capture_areas[0].first / 8 + capture_offset * capture_areas[0].step / 8;
    AudioDataSize 	= capture_frames * (AUDIO_SAMPLE_DEPTH * AUDIO_CHANNELS / 8);

    // Calculate audioPts

    DelayInMicroSeconds	= (DelayInSamples * 1000000ull) / AudioContext->SampleRate;
    time_absolute	= time_average - DelayInMicroSeconds;
    audioPts 		= (time_absolute * 90ull) / 1000ull;

    // Enable external time mapping

    NativeTime	= audioPts & 0x1ffffffffULL;
    Result = avr_set_external_time_mapping(
		    AudioContext->SharedContext, AudioContext->DeviceContext->AudioStream,
		    NativeTime, time_absolute);
    if (Result < 0) {
	DVB_ERROR("dvp_enable_external_time_mapping failed\n");
    	return -EINVAL;
    }

    // Build and inject audio PES header into player

    memset (PesHeader, '0', PES_AUDIO_HEADER_SIZE);
    HeaderLength 	= InsertPesHeader (PesHeader, AudioDataSize + 4, PES_PRIVATE_STREAM1, audioPts, 0);
    HeaderLength 	+= InsertPrivateDataHeader(&PesHeader[HeaderLength], AudioDataSize, AudioContext->DiscreteSampleRate);
    Result 		= StreamInject (AudioContext->DeviceContext->AudioStream, PesHeader, HeaderLength);
    if (Result < 0) {
	DVB_ERROR("StreamInject failed with PES header\n");
	return -EINVAL;
    }

    // Inject audio data into player

    Result 		= StreamInject (AudioContext->DeviceContext->AudioStream, AudioData, AudioDataSize);
    if (Result < 0) {
	DVB_ERROR("StreamInject failed on Audio data\n");
        return -EINVAL;
    }

    return Result;
}


static int AvrAudioInjectorThreadCleanup(avr_v4l2_audio_handle_t *AudioContext)
{
    int ret = 0;

    AudioContext->ThreadRunning = false;

    if (AudioContext->DeviceContext->AudioStream != NULL)
    {
	ret = StreamDrain(AudioContext->DeviceContext->AudioStream, true);
	if (ret != 0)
	{
	    DVB_ERROR("StreamDrain failed\n" );
	    return -EINVAL;
	}

	ret = PlaybackRemoveStream(AudioContext->DeviceContext->Playback,
			           AudioContext->DeviceContext->AudioStream);
	if (ret != 0)
	{
	    DVB_ERROR("PlaybackRemoveStream failed\n" );
	    return -EINVAL;
	}
	AudioContext->DeviceContext->AudioStream = NULL;
    }

    AudioContext->ThreadShouldStop = 0;

    return 0;
}


static int AvrAudioInjectorThread (void *data)
{
    avr_v4l2_audio_handle_t *AudioContext = (avr_v4l2_audio_handle_t *)data;
    struct DeviceContext_s *Context     = AudioContext->DeviceContext;
    struct timespec time_stamp_as_timespec;
    ksnd_pcm_t* capture_handle 			 = NULL;
    const snd_pcm_channel_area_t *capture_areas  = NULL;
    const snd_pcm_channel_area_t *capture_areas0 = NULL;
    snd_pcm_uframes_t capture_offset 	= 0;
    snd_pcm_uframes_t capture_frames 	= 0;
    snd_pcm_uframes_t capture_offset0 	= 0;
    snd_pcm_uframes_t capture_frames0 	= 0;
    snd_pcm_sframes_t commited_frames 	= 0;
    snd_pcm_sframes_t DelayInSamples 	= 0;
    snd_pcm_sframes_t DelayInSamples0 	= 0;
    snd_pcm_sframes_t DelayInSamples1 	= 0;
    unsigned long long time_average 	= 0;
    unsigned long long t0 		= 0;
    unsigned long long t1 		= 0;
    unsigned long SamplesAvailable	= 0;
    unsigned int packet_number 		= 0;
    int Result 				= 0;
    int ret 				= 0;

    // Add the Stream

    Result = PlaybackAddStream (Context->Playback,
				    BACKEND_AUDIO_ID,
				    BACKEND_PES_ID,
				    BACKEND_SPDIFIN_ID,
				    DEMUX_INVALID_ID,
				    Context->Id,
				    &Context->AudioStream);
    if (Result < 0) {
	DVB_ERROR("PlaybackAddStream failed with %d\n" , Result);
        goto err2;
    }
    else {
	Context->VideoState.play_state = AUDIO_PLAYING;
    }

    // SYSFS - now that the audio stream has been created, let's create the Emergency Mute attribute

    Result = AvrAudioSysfsCreateEmergencyMute(AudioContext);
    if (Result) {
	printk("Error in %s %d: create_emergency_mute_attribute failed\n",__FUNCTION__, __LINE__);
	goto err2;
    }

    // Set AVD sync - Audio Stream

    Result  = StreamSetOption (Context->AudioStream, PLAY_OPTION_AV_SYNC, 1);
    if (Result < 0) {
	DVB_ERROR("PLAY_OPTION_AV_SYNC set failed\n" );
	goto err2;
    }

    AudioContext->ThreadRunning = true;

    // Audio capture init

    Result = ksnd_pcm_open(&capture_handle, AUDIO_CAPTURE_CARD, AUDIO_CAPTURE_PCM, SND_PCM_STREAM_CAPTURE);
    if (Result < 0) {
	DVB_ERROR("Cannot open ALSA %d,%d\n" , AUDIO_CAPTURE_CARD, AUDIO_CAPTURE_PCM);
	goto err2;
    }

    Result = ksnd_pcm_set_params(capture_handle, AUDIO_CHANNELS, AUDIO_SAMPLE_DEPTH, AUDIO_DEFAULT_SAMPLE_RATE,
	    			 AUDIO_PERIOD_FRAMES, AUDIO_BUFFER_FRAMES);
    if (Result < 0) {
	DVB_ERROR("Cannot initialize ALSA parameters\n" );
	ksnd_pcm_close(capture_handle);
	goto err2;
    }

    // Audio capture start

    ksnd_pcm_start(capture_handle);

    while(AudioContext->ThreadRunning && AudioContext->ThreadShouldStop == 0)
    {
	// Retrieve Audio Data

	capture_frames = ksnd_pcm_avail_update(capture_handle);
	while (capture_frames < AUDIO_PERIOD_FRAMES)
	{
            // timeout if we wait for more than 100ms (selected because this is the time it takes to capture three frames at 32KHz)
	    Result = ksnd_pcm_wait(capture_handle, 100);
	    if (Result <= 0)
	    {
		DVB_ERROR("Failed to wait for capture period expiry: %d)\n", Result );

		ret = AvrAudioSetEmergencyMuteReason(AudioContext, EMERGENCY_MUTE_REASON_ERROR);
	        if (ret < 0)
	            DVB_ERROR("dvp_deploy_emergency_mute failed\n" );

                if (Result < 0)
                {
	            Result = AvrAudioXrunRecovery(capture_handle, Result);
		    if (Result < 0) {
		        DVB_ERROR("Cannot recovery from xrun after waiting for period expiry\n" );
		        break;
		    }
		    ksnd_pcm_start(capture_handle);
                }
	    }

	    if (kthread_should_stop()) {
		Result = -1;
		break;
	    }
	    capture_frames = ksnd_pcm_avail_update(capture_handle);
	}

	if (Result < 0)
	    break;

	//DVB_DEBUG("Captured %lu frames...\n" , capture_frames);

#if 0  // old implementation to get Delay In Samples and time average
	unsigned long long time_early	= 0;
	unsigned long long time_late 	= 0;
	ktime_t Ktime;
	struct timeval Now;
	unsigned int AntiLockCounter 	= 0;

	AntiLockCounter = 0;
        do
        {
            Ktime = ktime_get();
            Now = ktime_to_timeval(Ktime);
	    time_early = ((Now.tv_sec * 1000000ull) + (Now.tv_usec));

	    Result = ksnd_pcm_delay(capture_handle, &DelayInSamples);
	    if (Result < 0)
		DVB_ERROR("Cannot read the sound card latency (delay)\n" );

            Ktime = ktime_get();
            Now = ktime_to_timeval(Ktime);
            time_late = ((Now.tv_sec * 1000000ull) + (Now.tv_usec));

	    if (AntiLockCounter++ > 10) {
		DVB_DEBUG("Cannot accurately determine PCM delay (this will knacker the audio sync)\n" );
		break;
	    }

        } while ( (time_late - time_early) > 500);

	// HACK HACK HACK - I think these samples should already be included in the 'delay' but
	//                  observation suggests otherwise.
	//DelayInSamples += capture_frames;

        time_average = (time_early + time_late) / 2ull;
#endif

        Result = ksnd_pcm_mtimestamp(capture_handle, &SamplesAvailable, &time_stamp_as_timespec);
        if (Result < 0) {
            DVB_ERROR("Cannot read the sound card timestamp\n" );

            /* error recovery */
            ktime_get_ts(&time_stamp_as_timespec);
            SamplesAvailable = 0;
        }

        time_average = time_stamp_as_timespec.tv_sec * 1000000ll + time_stamp_as_timespec.tv_nsec / 1000;
        DelayInSamples = SamplesAvailable;

        capture_frames = AUDIO_PERIOD_FRAMES;
        Result = ksnd_pcm_mmap_begin(capture_handle, &capture_areas, &capture_offset, &capture_frames);
        if (Result < 0) {
            DVB_ERROR("Failed to mmap capture buffer\n" );
            break;
        }

	// DVB_DEBUG("capture_areas[0].addr=%p, capture_areas[0].first=%d, capture_areas[0].step=%d, capture_offset=%lu, capture_frames=%lu\n",
	//		__FUNCTION__, capture_areas[0].addr, capture_areas[0].first, capture_areas[0].step, capture_offset, capture_frames);

	// First packet: we don't know yet the Sample Rate, so we need to cycle another time before being able to inject it
	// into the player -> for now, let's just store the values

	if (packet_number == 0)
	{
	    t0 = time_average;
	    DelayInSamples0 = DelayInSamples;
	    capture_frames0 = capture_frames;
	    capture_areas0 = capture_areas;
	    capture_offset0 = capture_offset;
	}

	// Second Packet: we can calculate now the Sample Rate and use that to get the remaining values for the first packet
	// and inject it into the Player. After, we can inject the second packet.

	if (packet_number == 1)
	{
	    t1 = time_average;
	    DelayInSamples1 = DelayInSamples;

	    // Calculate sample rate and sample frequency

	    Result = AvrAudioCalculateSampleRate(AudioContext, DelayInSamples0, DelayInSamples1, t0, t1);
	    if (Result == 0)
	    {
		// Build and send first audio packet to player

		Result = AvrAudioSendPacketToPlayer(AudioContext, capture_frames0, capture_offset0, capture_areas0, DelayInSamples0, t0);
		if (Result < 0)
		{
		    DVB_ERROR("dvp_send_audio_packet_to_player failed \n" );
		    goto err2;
		}

		// Build and send second audio packet to player

		Result = AvrAudioSendPacketToPlayer(AudioContext, capture_frames, capture_offset, capture_areas, DelayInSamples1, t1);
		if (Result < 0)
		{
		    DVB_ERROR("dvp_send_audio_packet_to_player failed \n" );
		    goto err2;
		}
	    }
	}

	if (packet_number > 1)
	{
	    t0 = t1;
	    t1 = time_average;
	    DelayInSamples0 = DelayInSamples1;
	    DelayInSamples1 = DelayInSamples;

	    // Calculate sample rate and sample frequency

	    Result = AvrAudioCalculateSampleRate(AudioContext, DelayInSamples0, DelayInSamples1, t0, t1);
	    if (Result == 0)
	    {
		Result = AvrAudioSendPacketToPlayer(AudioContext, capture_frames, capture_offset, capture_areas, DelayInSamples, time_average);
		if (Result < 0)
		{
		    DVB_ERROR("dvp_send_audio_packet_to_player failed \n" );
		    goto err2;
		}
	    }
	}

	commited_frames = ksnd_pcm_mmap_commit(capture_handle, capture_offset, capture_frames);
	if (commited_frames < 0 || (snd_pcm_uframes_t) commited_frames != capture_frames) {
	    //DVB_DEBUG("Capture XRUN! (commit %ld captured %ld)\n" , commited_frames,capture_frames);
	    Result = AvrAudioXrunRecovery(capture_handle, commited_frames >= 0 ? -EPIPE : commited_frames);
	    if (Result < 0) {
		DVB_ERROR("MMAP commit error\n" );
		break;
	    }
	    ksnd_pcm_start(capture_handle);
	}

	packet_number++;

    } // end while

    // Audio capture close

    ksnd_pcm_close(capture_handle);
    AvrAudioInjectorThreadCleanup(AudioContext);

    return 0;

err2:
    AvrAudioInjectorThreadCleanup(AudioContext);
    return -EINVAL;

}


/******************************
 * EXTERNAL INTERFACE
 ******************************/


avr_v4l2_audio_handle_t *AvrAudioNew(avr_v4l2_shared_handle_t *SharedContext)
{
	avr_v4l2_audio_handle_t *AudioContext;

	AudioContext = kzalloc(sizeof(avr_v4l2_audio_handle_t), GFP_KERNEL);

	if (AudioContext) {
		AudioContext->SharedContext = SharedContext;
		AudioContext->DeviceContext = &SharedContext->avr_device_context;

		AudioContext->AacDecodeEnable = true;
		AudioContext->EmergencyMuteReason = EMERGENCY_MUTE_REASON_NONE;
		AudioContext->FormatRecogniserEnable = true;

		AudioContext->EmergencyMuteClassDevice = NULL;

		AudioContext->SampleRate = 48000;
		AudioContext->PreviousSampleRate = 0;
		AudioContext->DiscreteSampleRate = AVR_AUDIO_DISCRETE_SAMPLE_RATE_48000;

		AudioContext->ThreadHandle = NULL;
		AudioContext->ThreadRunning = false;
		AudioContext->ThreadShouldStop = false;
	}

	return AudioContext;
}


int AvrAudioDelete(avr_v4l2_audio_handle_t *AudioContext)
{
	if (AudioContext->ThreadRunning) {
		DVB_ERROR("Audio thread still running. Nothing to do.");
		return -EBUSY;
	}

	kfree(AudioContext);

	return 0;
}


bool AvrAudioGetAacDecodeEnable(avr_v4l2_audio_handle_t *AudioContext)
{
	return AudioContext->AacDecodeEnable;
}


void AvrAudioSetAacDecodeEnable(avr_v4l2_audio_handle_t *AudioContext, bool Enabled)
{
	AudioContext->AacDecodeEnable = Enabled;
}


avr_audio_emergency_mute_reason_t AvrAudioGetEmergencyMuteReason(avr_v4l2_audio_handle_t *AudioContext)
{
	return AudioContext->EmergencyMuteReason;
}


int AvrAudioSetEmergencyMuteReason(avr_v4l2_audio_handle_t *AudioContext, avr_audio_emergency_mute_reason_t reason)
{
    int ret = 0;

    if (!AudioContext->DeviceContext->AudioStream) return -EINVAL;

    if (AudioContext->EmergencyMuteReason != EMERGENCY_MUTE_REASON_NONE)
    {
        ret = StreamEnable (AudioContext->DeviceContext->AudioStream, reason);
        if (ret < 0) {
            DVB_ERROR("StreamEnable failed\n");
            return -EINVAL;
        }
    }
    AudioContext->EmergencyMuteReason = reason;

    // SYSFS - emergency mute attribute change notification
    sysfs_notify (&((*(AudioContext->EmergencyMuteClassDevice)).kobj), NULL, "emergency_mute");

    return ret;
}


bool AvrAudioGetFormatRecogniserEnable(avr_v4l2_audio_handle_t *AudioContext)
{
	return AudioContext->FormatRecogniserEnable;
}


void AvrAudioSetFormatRecogniserEnable(avr_v4l2_audio_handle_t *AudioContext, bool Enabled)
{
	AudioContext->FormatRecogniserEnable = Enabled;
}


int AvrAudioIoctlOverlayStart(avr_v4l2_audio_handle_t *AudioContext)
{
	struct sched_param Param_audio;

	if (AudioContext->ThreadRunning) {
		DVB_DEBUG("Capture Thread already started\n" );
		return 0; //return -EBUSY;
	}

	AudioContext->ThreadHandle = kthread_create(AvrAudioInjectorThread, AudioContext, "AvrAudioInjectorThread");
	if (IS_ERR(AudioContext->ThreadHandle))
	{
	    DVB_ERROR("Unable to start audio thread\n" );
	    AudioContext->ThreadHandle = NULL;
	    return -EINVAL;
	}

	// Switch to real time scheduling
	Param_audio.sched_priority = AUDIO_CAPTURE_THREAD_PRIORITY;
	if (sched_setscheduler(AudioContext->ThreadHandle, SCHED_RR, &Param_audio))
	{
	    DVB_ERROR("FAILED to set scheduling parameters to priority %d (SCHED_RR)\n", Param_audio.sched_priority);
	    return -EINVAL;
	}

	wake_up_process(AudioContext->ThreadHandle);

	return 0;
}


int AvrAudioIoctlOverlayStop(avr_v4l2_audio_handle_t *AudioContext)
{
	if (AudioContext->ThreadHandle) {
		AudioContext->ThreadShouldStop = 1;

		while (readl(&AudioContext->ThreadShouldStop) == 1)
			msleep(10);
	}
	AudioContext->ThreadHandle = NULL;

	return 0;
}
