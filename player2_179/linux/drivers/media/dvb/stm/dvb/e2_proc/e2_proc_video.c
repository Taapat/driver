/*
 * e2_proc_video.c
 */

#include <linux/version.h>
#include <linux/proc_fs.h>  	/* proc fs */
#include <asm/uaccess.h>    	/* copy_from_user */

#include <linux/dvb/video.h>	/* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/fb.h>

#define STMFBIO_VAR_CAPS_OPACITY        (1L<<4)

struct stmfbio_var_screeninfo_ex
{
  /*
   * Display layer to operate on, 0 is always the layer showing the framebuffer.
   * No other layers have to be defined and the IOCTL will return ENODEV
   * if given an invalid layerid.
   */
  __u32 layerid;
  __u32 caps;                    /* Valid entries in structure               */
  __u32 failed;                  /* Indicates entries that failed during set */
  __u32 activate;

  /*
   * STMFBIO_VAR_CAPS_COLOURKEY
   */
  __u32 min_colour_key;
  __u32 max_colour_key;
  __u32 colourKeyFlags;

  __u32 ff_state;          /* STMFBIO_VAR_CAPS_FLICKER_FILTER      */

  __u8 premultiplied_alpha;           /* STMFBIO_VAR_CAPS_PREMULTIPLIED       */
  __u8 rescale_colour_to_video_range; /* STBFBIO_VAR_CAPS_RESCALE_COLOUR_...  */

  __u8 opacity;                       /* STMFBIO_VAR_CAPS_OPACITY             */
  __u8 gain;                          /* STMFBIO_VAR_CAPS_GAIN                */
  __u8 brightness;                    /* STMFBIO_VAR_CAPS_BRIGHTNESS          */
  __u8 saturation;                    /* STMFBIO_VAR_CAPS_SATURATION          */
  __u8 contrast;                      /* STMFBIO_VAR_CAPS_CONTRAST            */
  __u8 tint;                          /* STMFBIO_VAR_CAPS_HUE                 */
  __u8 alpha_ramp[2];                 /* STMFBIO_CAR_CAPS_ALPHA_RAMP          */

  __u32 z_position;                   /* STMFBIO_VAR_CAPS_ZPOSITION           */
};

struct stmfb_info;

extern int stmfb_set_var_ex   (struct stmfbio_var_screeninfo_ex *v, struct stmfb_info *i);
extern int isDisplayCreated (char*           Media,
                      unsigned int    SurfaceId);
extern struct stmfb_info* stmfb_get_fbinfo_ptr(void);
extern int avs_command_kernel(unsigned int cmd, void *arg);

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

			/* fixme:
			 * brauchen wir für die stmfb aufrufe:
			 * void* fb =  stmfb_get_fbinfo_ptr();
			 * stmfb_ioctl((void*) fb , YourRequest, &screen_info);
			 */


extern int DisplayCreate              (char*                   Media,
                                unsigned int            SurfaceId);
extern int DisplayDelete              (char*                   Media,
                                unsigned int            SurfaceId);

/* FIXME Header ? */
#define SAAIOGREG               1 /* read registers                             */
#define SAAIOSINP               2 /* input control                              */
#define SAAIOSOUT               3 /* output control                     */
#define SAAIOSENC               4 /* set encoder (pal/ntsc)             */
#define SAAIOSMODE              5 /* set mode (rgb/fbas/svideo/component) */
#define SAAIOSWSS              10 /* set wide screen signaling data */

#define SAA_MODE_RGB    0
#define SAA_MODE_FBAS   1
#define SAA_MODE_SVIDEO 2
#define SAA_MODE_COMPONENT 3

#define SAA_NTSC                0
#define SAA_PAL                 1
#define SAA_PAL_M               2

#define SAA_INP_MP1             1
#define SAA_INP_MP2             2
#define SAA_INP_CSYNC   4
#define SAA_INP_DEMOFF  6
#define SAA_INP_SYMP    8
#define SAA_INP_CBENB   128

#define SAA_WSS_43F     0
#define SAA_WSS_169F    7
#define SAA_WSS_OFF     8

extern struct DeviceContext_s* ProcDeviceContext;


static struct Modes {
    int mode;
    char * name;
    const int xres;
    const int yres;
    const int vxres;
    const int vyres;
    const int bit;

    const int pixclock;
    const int left;
    const int right;
    const int upper;
    const int lower;
    const int hslen;
    const int vslen;

    const int sync;
    const int vmode;

} Options[] = {

    { 25, "1024x768_60", 1024, 768, 1024, 768, 8, 15385, 160, 24, 29, 3, 136, 6,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 24, "1024x768_70", 1024, 768, 1024, 768, 8, 13334, 144, 24, 29, 3, 136, 6,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 23, "1024x768_75", 1024, 768, 1024, 768, 8, 12699, 176, 16, 28, 1, 96, 3,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 22, "1024x768_90", 1024, 768, 1024, 768, 8, 10000, 192, 0, 41, 21, 96, 15,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 21, "1024x768_100", 1024, 768, 1024, 768, 8, 9091, 280, 0, 16, 0, 88, 8,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},

    { 20, "1280x1024_60", 1280, 1024, 1280, 1024, 8, 9260, 248, 48, 38, 1, 112, 3,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 19, "1280x1024_70", 1280, 1024, 1280, 1024, 8, 7937, 216, 80, 36, 1, 112, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 18, "1280x1024_75", 1280, 1024, 1280, 3264, 8, 7414, 232, 64, 38, 1, 112, 3,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},

    { 17, "1600x1200_60", 1600, 1200, 1600, 1200, 8, 6411, 256, 32, 52, 10, 160, 8,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},

    { 16, "1080p60", 1920, 1080, 1920, 1080, 16, 6734, 148,  88, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    { 15, "1080p59", 1920, 1080, 1920, 1080, 16, 6741, 148,  88, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    { 14, "1080p50", 1920, 1080, 1920, 1080, 16, 6734, 148,  528, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},

    { 13, "1080p30", 1920, 1080, 1920, 1080, 16, 13468, 148,  88, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    { 12, "1080p29", 1920, 1080, 1920, 1080, 16, 13481, 148,  88, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    { 11, "1080p25", 1920, 1080, 1920, 1080, 16, 13468, 148, 528, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    { 10, "1080p24", 1920, 1080, 1920, 1080, 16, 13468, 148, 638, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},
    {  9, "1080p23", 1920, 1080, 1920, 1080, 16, 13481, 148, 638, 36, 4, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_CONUPDATE},

    { 8, "1080i60", 1920, 1080, 1920, 1080, 16, 13468, 148,  88, 35, 5, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},
    { 7, "1080i59", 1920, 1080, 1920, 1080, 16, 13481, 148,  88, 35, 5, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},
    { 6, "1080i50", 1920, 1080, 1920, 1080, 16, 13468, 148, 528, 35, 5, 44, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},

    { 5, "720p60", 1280, 720, 1280, 720, 16, 13468, 220, 110, 20, 5, 40, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
    { 4, "720p50", 1280, 720, 1280, 720, 16, 13468, 220, 440, 20, 5, 40, 5,
	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},

/* @donald: where are these values taken from? I'm not sure if these are device dependant?!?!
 * these one are taken from ufs922
 */
#ifdef alter_tobak
    { 3, "576p50", 720, 576, 720, 576, 16, 37037, 68, 12, 34, 10, 64, 5, 0,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
#else
    { 3, "576p50", 720, 576, 720, 576, 16, 37037, 68, 12, 39, 5, 64, 5, 0,
	FB_VMODE_NONINTERLACED | FB_VMODE_CONUPDATE},
#endif

#ifdef alter_tobak
    { 2, "576i50", 720, 576, 720, 576, 16, 74074, 68, 12, 38, 5, 64, 6, 0,
	FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},
#else
    { 2, "576i50", 720, 576, 720, 576, 16, 74074, 69, 12, 41, 5, 63, 3, 0,
	FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},
#endif
    { 1, "pal"   , 720, 576, 720, 576, 16, 74074, 68, 12, 38, 5, 64, 6, 0,
	FB_VMODE_INTERLACED | FB_VMODE_CONUPDATE},
    { 0, NULL,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define VIDEO_POL_LETTER_BOX 	0
#define VIDEO_POL_PAN_SCAN 	1
#define VIDEO_POL_BEST_FIT 	2
#define VIDEO_POL_NON_LINEAR 	3

static int aspect_e2  = (video_format_t) VIDEO_FORMAT_16_9;
static int aspect_ply = (video_format_t) VIDEO_FORMAT_16_9;
static int policy_e2  = VIDEO_POL_LETTER_BOX;
static int policy_ply = (video_displayformat_t) VIDEO_LETTER_BOX;

int proc_video_policy_get(void) {

/* Dagobert: You cannot use this semaphore here because this will be called from
 * VideoIoctl which holds this semaphore too. This means deadlock.
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));
*/
	if (ProcDeviceContext->VideoStream != NULL)
	{
	   int cur_video_aspect = VIDEO_FORMAT_16_9;
	   if ( ProcDeviceContext->VideoSize.aspect_ratio ==  VIDEO_FORMAT_16_9 || (ProcDeviceContext->VideoSize.h > 576 && ProcDeviceContext->VideoSize.w > 720)) {
		cur_video_aspect = VIDEO_FORMAT_16_9;
	   } else
		cur_video_aspect = VIDEO_FORMAT_4_3;

	   if (cur_video_aspect == VIDEO_FORMAT_16_9) {	//16:9 movie
		if (aspect_e2 == VIDEO_FORMAT_16_9) {
                        policy_ply = VIDEO_LETTER_BOX; //on an 16:9 screen always show an 16:9 movie as fullscreen
			aspect_ply = VIDEO_FORMAT_16_9;
		} else { //4:3
			//TODO: set the correct modes
			if (policy_e2 == VIDEO_POL_LETTER_BOX) {
		               	policy_ply = VIDEO_LETTER_BOX;
				aspect_ply = VIDEO_FORMAT_4_3;

		  	} else if (policy_e2 == VIDEO_POL_PAN_SCAN) {
		               	policy_ply = VIDEO_PAN_SCAN;
				aspect_ply = VIDEO_FORMAT_4_3;

		   	} else if (policy_e2 == VIDEO_POL_BEST_FIT) {
		               	policy_ply = VIDEO_LETTER_BOX;
				aspect_ply = VIDEO_FORMAT_16_9;
		   	}
		}
	   } else {	//4:3movie
		if (aspect_e2 == VIDEO_FORMAT_16_9) {
			if (policy_e2 == VIDEO_POL_LETTER_BOX) {
	                	policy_ply = VIDEO_LETTER_BOX;
				aspect_ply = VIDEO_FORMAT_16_9;

		 	} else if (policy_e2 == VIDEO_POL_PAN_SCAN) {
	                	policy_ply = VIDEO_PAN_SCAN;
				aspect_ply = VIDEO_FORMAT_16_9;

		   	} else if (policy_e2 == VIDEO_POL_BEST_FIT) {
	                	policy_ply = VIDEO_LETTER_BOX;
				aspect_ply = VIDEO_FORMAT_4_3;

		   	} else if (policy_e2 == VIDEO_POL_NON_LINEAR) {
	                	policy_ply = VIDEO_CENTER_CUT_OUT;
				aspect_ply = VIDEO_FORMAT_16_9;
		   	}
		} else { //4:3, no need for scaling
			//TODO: set the correct modes
			policy_ply = VIDEO_LETTER_BOX;
			aspect_ply = VIDEO_FORMAT_4_3;
		}
	   }

	   //SCART
	   avs_command_kernel(SAAIOSWSS, (void*) (aspect_e2==VIDEO_FORMAT_16_9?SAA_WSS_169F:SAA_WSS_43F));

	}
/* Dagobert: You cannot use this semaphore here because this will be called from
 * VideoIoctl which holds this semaphore too. This means deadlock.
	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
*/
	return policy_ply;
}

int proc_video_aspect_get(void) {
//just return the type, check was already done by policy_get
	return aspect_ply;
}



//TODO: say avs to send the widescreenflag if needed
int proc_video_aspect_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */

	printk("%s %ld - ", __FUNCTION__, count);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	printk("%p, %p, %d, %p", ProcDeviceContext, ProcDeviceContext->DvbContext, ProcDeviceContext->DvbContext->Lock, ProcDeviceContext->VideoStream);
#else
	printk("%p, %p, %d, %p", ProcDeviceContext, ProcDeviceContext->DvbContext, ProcDeviceContext->DvbContext->Lock.count.counter, ProcDeviceContext->VideoStream);
#endif

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
		//printk("%s\n", myString);


		aspect_e2 = VIDEO_FORMAT_16_9;

		printk("%s\n", myString);

		if (strncmp("4:3", myString, count - 1) == 0)
		{
			aspect_e2 = VIDEO_FORMAT_4_3;
		}

		if (strncmp("16:9", myString, count - 1) == 0)
		{
			aspect_e2 = VIDEO_FORMAT_16_9;
		}

		/*if (strncmp("221:1", myString, count - 1) == 0)
		{
			aspect_e2 = VIDEO_FORMAT_221_1;
		}*/

		//we dont support any, whatever this is
		if (strncmp("any", myString, count - 1) == 0)
		{
			aspect_e2 = VIDEO_FORMAT_4_3;
		}

		//printk("Video Format = %d\n", format);

		/* always return count to avoid endless loop */
		ret = count;
		kfree(myString);
	}

out:
	free_page((unsigned long)page);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_video_aspect_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	if (aspect_e2 == VIDEO_FORMAT_4_3) {
		len = sprintf(page, "4:3\n");
	} else if (aspect_e2 == VIDEO_FORMAT_16_9) {
		len = sprintf(page, "16:9\n");
	/*} else if (aspect_e2 == VIDEO_FORMAT_221_1) {
		len = sprintf(page, "221:1\n");*/
	} else { //should never occur
		len = sprintf(page, "any\n");
	}

        return len;
}

int proc_video_aspect_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	//we dont support any, whatever this is
	//len = sprintf(page, "4:3 16:9 any\n");
	len = sprintf(page, "4:3 16:9\n");

        return len;
}


int proc_video_policy_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s\n", myString);

		if (strncmp("letterbox", myString, count - 1) == 0) {
			policy_e2 = VIDEO_POL_LETTER_BOX;
		} else if (strncmp("panscan", myString, count - 1) == 0) {
		        policy_e2 = VIDEO_POL_PAN_SCAN;
		} else if (strncmp("bestfit", myString, count - 1) == 0) {
		        policy_e2 = VIDEO_POL_BEST_FIT;
		} else if (strncmp("non", myString, count - 1) == 0) {
		        policy_e2 = VIDEO_POL_NON_LINEAR;
		} else if (strncmp("nonlinear", myString, count - 1) == 0) { // e2 writes nonlinear
		        policy_e2 = VIDEO_POL_NON_LINEAR;
		}

		kfree(myString);

		if (ProcDeviceContext->VideoStream != NULL)
		{
		   policy_ply = proc_video_policy_get();
		   aspect_ply = proc_video_aspect_get();
		    //printk("Calling StreamSetOption ->PLAY_OPTION_VIDEO_ASPECT_RATIO\n");

		   result  = StreamSetOption (ProcDeviceContext->VideoStream, PLAY_OPTION_VIDEO_ASPECT_RATIO, aspect_ply);
	    	   	if (result != 0)
				printk("Error setting stream option %d\n", result);
			else
	    			ProcDeviceContext->VideoState.video_format = (video_format_t)aspect_ply;


			result  = StreamSetOption (ProcDeviceContext->VideoStream, PLAY_OPTION_VIDEO_DISPLAY_FORMAT, policy_ply);

	    		if (result != 0)
				printk("Failed to set option %d\n", result);
			else
	    			ProcDeviceContext->VideoState.display_format = (video_displayformat_t)policy_ply;

		} else
		   printk("Can't set policy, VideoStream NULL\n");
	}

	ret = count;
out:
	free_page((unsigned long)page);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_video_policy_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	if (policy_e2 == VIDEO_POL_LETTER_BOX) {
		len = sprintf(page, "letterbox\n");
	} else if (policy_e2 == VIDEO_POL_PAN_SCAN) {
		len = sprintf(page, "panscan\n");
	} else if (policy_e2 == VIDEO_POL_NON_LINEAR) {
		len = sprintf(page, "non\n");
	} else if (policy_e2 == VIDEO_POL_BEST_FIT) {
		len = sprintf(page, "bestfit\n");
	}

        return len;
}

int proc_video_policy_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "letterbox panscan non bestfit\n");

        return len;
}

/* hack hack ;-) */
int proc_video_videomode_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */
	int		vLoop;
        int             new_count;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	void* fb = stmfb_get_fbinfo_ptr();
	struct fb_info *info = (struct fb_info*) fb;

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		int modeToSet = -1;
		int aktmode = -1;
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

        	/* Dagobert: echo add a \n which will be counted as a char
		 */
		if (page[count - 1] == '\n')
		   new_count = count - 1;
                else
		   new_count = count;

		strncpy(myString, page, new_count);
		myString[new_count] = '\0';

		printk("%s\n", myString);

		//whithout -1 write a unsupportet string hangs the driver
		for (vLoop = 0; vLoop < (sizeof(Options) / sizeof(struct Modes)) - 1; vLoop++)
		{

			if (Options[vLoop].xres     == info->var.xres &&
					Options[vLoop].yres     == info->var.yres &&
					Options[vLoop].vxres    == info->var.xres_virtual &&
					Options[vLoop].vyres    == info->var.yres_virtual &&
					Options[vLoop].pixclock == info->var.pixclock &&
					Options[vLoop].left     == info->var.left_margin &&
					Options[vLoop].right    == info->var.right_margin &&
					Options[vLoop].upper    == info->var.upper_margin &&
					Options[vLoop].lower    == info->var.lower_margin &&
					Options[vLoop].hslen    == info->var.hsync_len &&
					Options[vLoop].vslen    == info->var.vsync_len &&
					Options[vLoop].sync     == info->var.sync/* &&
					Options[vLoop].vmode    == info->var.vmode*/ )
			{
				aktmode = vLoop;
			}

			if (strncmp(myString, Options[vLoop].name, new_count) == 0)
			{
				printk("Found mode to set %s at %d\n",  Options[vLoop].name, vLoop);
				modeToSet = vLoop;
			}
		}

		if(aktmode == modeToSet)
			modeToSet = -1;

		if (modeToSet != -1)
		{
			struct fb_var_screeninfo screen_info;
			int createNew = 0;

			memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));

			if (fb != NULL)
			{
				int err;

				/* otherwise we got EBUSY from stmfb device */
				/* Dagobert: Bugfix: "demux stop" bug; ticket #10 */
				if (ProcDeviceContext != NULL)
				{
					if (ProcDeviceContext->VideoState.play_state != VIDEO_STOPPED)
					   VideoIoctlStop (ProcDeviceContext, 1);

			       		if (isDisplayCreated(BACKEND_VIDEO_ID, ProcDeviceContext->Id))
					{
						createNew = 1;
						printk("delete display\n");
						DisplayDelete (BACKEND_VIDEO_ID, ProcDeviceContext->Id);
					}
				}

				info->flags |= FBINFO_MISC_USEREVENT;

				screen_info.xres = Options[modeToSet].xres;			/* visible resolution		*/
				screen_info.yres = Options[modeToSet].yres;
				screen_info.xres_virtual = Options[modeToSet].vxres;		/* virtual resolution		*/
				screen_info.yres_virtual = Options[modeToSet].vyres;

				screen_info.pixclock = Options[modeToSet].pixclock;
				screen_info.left_margin = Options[modeToSet].left;
				screen_info.right_margin = Options[modeToSet].right;
				screen_info.upper_margin = Options[modeToSet].upper;
				screen_info.lower_margin = Options[modeToSet].lower;
				screen_info.hsync_len = Options[modeToSet].hslen;
				screen_info.vsync_len = Options[modeToSet].vslen;
				screen_info.sync = Options[modeToSet].sync;
				screen_info.vmode = Options[modeToSet].vmode;
				screen_info.activate = FB_ACTIVATE_FORCE;

				err = fb_set_var(fb, &screen_info);

				if (err != 0)
					printk("error setting new resolution %d\n", err);

				if ((ProcDeviceContext != NULL) && (createNew == 1))
				{
					printk("create new display\n");
					DisplayCreate (BACKEND_VIDEO_ID, ProcDeviceContext->Id);

					VideoIoctlPlay(ProcDeviceContext);

				    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream,
				    		0,0, Options[modeToSet].xres, Options[modeToSet].yres);

				    	if (err != 0)
				    	{
				    		printk("failed to set output window %d, %d, %d\n",  Options[modeToSet].xres,  Options[modeToSet].yres, err);
				    	} else
				    		printk("set output window to %d, %d ok\n",  Options[modeToSet].xres,  Options[modeToSet].yres);

				}
			} else
			{
				printk("Cannot get stmfb_info struct\n");
			}
		}

		/* always return count to avoid endless loop */
		ret = count;
	}

out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_video_videomode_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int                      len = 0;
	int                      vLoop = 0;
	void                     *fb = NULL;
	struct fb_info           *info;

	printk("%s\n", __FUNCTION__);

   fb = stmfb_get_fbinfo_ptr(); /* cannot return a NULL pointer */

	/* Dagobert:
	 * dirty hack, does only work because fb->info is first param of struct stmfb_info
	 * we should include a header here instead ...
	 */
	info = (struct fb_info*) fb;

	/* default */
	len = sprintf(page, "pal\n");
	//whithout -1 if resolution not found hangs the driver
	for (vLoop = 0; vLoop < (sizeof(Options) / sizeof(struct Modes)) - 1; vLoop++)
	{
/*
printk("%d\n", info->var.xres);
printk("%d\n", info->var.yres);
printk("%d\n", info->var.xres_virtual);
printk("%d\n", info->var.yres_virtual);
printk("%d\n", info->var.pixclock);
printk("%d\n", info->var.left_margin);
printk("%d\n", info->var.right_margin);
printk("%d\n", info->var.upper_margin);
printk("%d\n", info->var.lower_margin);
printk("%d\n", info->var.hsync_len);
printk("%d\n", info->var.vsync_len);
printk("%d\n", info->var.sync);
*/
		if (Options[vLoop].xres     == info->var.xres &&
			 Options[vLoop].yres     == info->var.yres &&
			 Options[vLoop].vxres    == info->var.xres_virtual &&
			 Options[vLoop].vyres    == info->var.yres_virtual &&
			 Options[vLoop].pixclock == info->var.pixclock &&
			 Options[vLoop].left     == info->var.left_margin &&
			 Options[vLoop].right    == info->var.right_margin &&
			 Options[vLoop].upper    == info->var.upper_margin &&
			 Options[vLoop].lower    == info->var.lower_margin &&
			 Options[vLoop].hslen    == info->var.hsync_len &&
			 Options[vLoop].vslen    == info->var.vsync_len &&
			 Options[vLoop].sync     == info->var.sync/* &&
			 Options[vLoop].vmode    == info->var.vmode*/ )
		{
			printk("Found mode to set %s at %d\n",  Options[vLoop].name, vLoop);
		   len = sprintf(page, "%s\n", Options[vLoop].name);
			break;
		}
	}

   return len;
}

int proc_video_pal_h_start_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */
	int		value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		void* fb;
		struct fb_info *info;
		struct fb_var_screeninfo screen_info;
		int createNew = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%x", &value);

		fb =  stmfb_get_fbinfo_ptr();

		info = (struct fb_info*) fb;

		memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));

		if (fb != NULL)
		{
			int err;

			/* otherwise we got EBUSY from stmfb device */
			/* Dagobert: Bugfix: "demux stop" bug; ticket #10 */
			if (ProcDeviceContext != NULL)
			{
				if (ProcDeviceContext->VideoState.play_state != VIDEO_STOPPED)
				   VideoIoctlStop (ProcDeviceContext, 1);

		       		if (isDisplayCreated(BACKEND_VIDEO_ID, ProcDeviceContext->Id))
				{
					createNew = 1;
					printk("delete display\n");
					DisplayDelete (BACKEND_VIDEO_ID, ProcDeviceContext->Id);
				}
			}

			info->flags |= FBINFO_MISC_USEREVENT;

			screen_info.left_margin = value;

			err = fb_set_var(fb, &screen_info);

			if (err != 0)
				printk("error setting new resolution %d\n", err);

			if ((ProcDeviceContext != NULL) && (createNew == 1))
			{
				printk("create new display\n");
				DisplayCreate (BACKEND_VIDEO_ID, ProcDeviceContext->Id);

				VideoIoctlPlay(ProcDeviceContext);

			    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream,
			    		0,0, screen_info.xres, screen_info.yres);

			    	if (err != 0)
			    	{
			    		printk("failed to set output window %d, %d, %d\n",  screen_info.xres,  screen_info.yres, err);
			    	} else
			    		printk("set output window to %d, %d ok\n",  screen_info.xres,  screen_info.yres);

			}
		} else
		{
			printk("Cannot get stmfb_info struct\n");
		}

		/* always return count to avoid endless loop */
		ret = count;
	}

out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_video_pal_h_end_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */
	int		value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		void* fb;
		struct fb_info *info;
		struct fb_var_screeninfo screen_info;
		int createNew = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%x", &value);

		fb =  stmfb_get_fbinfo_ptr();

		info = (struct fb_info*) fb;

		memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));

		if (fb != NULL)
		{
			int err;

			/* otherwise we got EBUSY from stmfb device */
			/* Dagobert: Bugfix: "demux stop" bug; ticket #10 */
			if (ProcDeviceContext != NULL)
			{
				if (ProcDeviceContext->VideoState.play_state != VIDEO_STOPPED)
				   VideoIoctlStop (ProcDeviceContext, 1);

		       		if (isDisplayCreated(BACKEND_VIDEO_ID, ProcDeviceContext->Id))
				{
					createNew = 1;
					printk("delete display\n");
					DisplayDelete (BACKEND_VIDEO_ID, ProcDeviceContext->Id);
				}
			}

			info->flags |= FBINFO_MISC_USEREVENT;

			screen_info.right_margin = value;

			err = fb_set_var(fb, &screen_info);

			if (err != 0)
				printk("error setting new resolution %d\n", err);

			if ((ProcDeviceContext != NULL) && (createNew == 1))
			{
				printk("create new display\n");
				DisplayCreate (BACKEND_VIDEO_ID, ProcDeviceContext->Id);

				VideoIoctlPlay(ProcDeviceContext);

			    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream,
			    		0,0, screen_info.xres, screen_info.yres);

			    	if (err != 0)
			    	{
			    		printk("failed to set output window %d, %d, %d\n",  screen_info.xres,  screen_info.yres, err);
			    	} else
			    		printk("set output window to %d, %d ok\n",  screen_info.xres,  screen_info.yres);

			}
		} else
		{
			printk("Cannot get stmfb_info struct\n");
		}

		/* always return count to avoid endless loop */
		ret = count;
	}

out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_video_pal_v_start_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */
	int		value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		void *fb;
		struct fb_info *info;
		struct fb_var_screeninfo screen_info;
		int createNew = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%x", &value);

		fb =  stmfb_get_fbinfo_ptr();

		info = (struct fb_info*) fb;

		memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));

		if (fb != NULL)
		{
			int err;

			/* otherwise we got EBUSY from stmfb device */
			/* Dagobert: Bugfix: "demux stop" bug; ticket #10 */
			if (ProcDeviceContext != NULL)
			{
				if (ProcDeviceContext->VideoState.play_state != VIDEO_STOPPED)
				   VideoIoctlStop (ProcDeviceContext, 1);

		       		if (isDisplayCreated(BACKEND_VIDEO_ID, ProcDeviceContext->Id))
				{
					createNew = 1;
					printk("delete display\n");
					DisplayDelete (BACKEND_VIDEO_ID, ProcDeviceContext->Id);
				}
			}

			info->flags |= FBINFO_MISC_USEREVENT;

			screen_info.upper_margin = value;

			err = fb_set_var(fb, &screen_info);

			if (err != 0)
				printk("error setting new resolution %d\n", err);

			if ((ProcDeviceContext != NULL) && (createNew == 1))
			{
				printk("create new display\n");
				DisplayCreate (BACKEND_VIDEO_ID, ProcDeviceContext->Id);

				VideoIoctlPlay(ProcDeviceContext);

			    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream,
			    		0,0, screen_info.xres, screen_info.yres);

			    	if (err != 0)
			    	{
			    		printk("failed to set output window %d, %d, %d\n",  screen_info.xres,  screen_info.yres, err);
			    	} else
			    		printk("set output window to %d, %d ok\n",  screen_info.xres,  screen_info.yres);

			}
		} else
		{
			printk("Cannot get stmfb_info struct\n");
		}

		/* always return count to avoid endless loop */
		ret = count;
	}

out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_video_pal_v_end_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */
	int		value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		void *fb;
		struct fb_info *info;
		struct fb_var_screeninfo screen_info;
		int createNew = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%x", &value);

		fb =  stmfb_get_fbinfo_ptr();

		info = (struct fb_info*) fb;

		memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));

		if (fb != NULL)
		{
			int err;

			/* otherwise we got EBUSY from stmfb device */
			/* Dagobert: Bugfix: "demux stop" bug; ticket #10 */
			if (ProcDeviceContext != NULL)
			{
				if (ProcDeviceContext->VideoState.play_state != VIDEO_STOPPED)
				   VideoIoctlStop (ProcDeviceContext, 1);

		       		if (isDisplayCreated(BACKEND_VIDEO_ID, ProcDeviceContext->Id))
				{
					createNew = 1;
					printk("delete display\n");
					DisplayDelete (BACKEND_VIDEO_ID, ProcDeviceContext->Id);
				}
			}

			info->flags |= FBINFO_MISC_USEREVENT;

			screen_info.lower_margin = value;

			err = fb_set_var(fb, &screen_info);

			if (err != 0)
				printk("error setting new resolution %d\n", err);

			if ((ProcDeviceContext != NULL) && (createNew == 1))
			{
				printk("create new display\n");
				DisplayCreate (BACKEND_VIDEO_ID, ProcDeviceContext->Id);

				VideoIoctlPlay(ProcDeviceContext);

			    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream,
			    		0,0, screen_info.xres, screen_info.yres);

			    	if (err != 0)
			    	{
			    		printk("failed to set output window %d, %d, %d\n",  screen_info.xres,  screen_info.yres, err);
			    	} else
			    		printk("set output window to %d, %d ok\n",  screen_info.xres,  screen_info.yres);

			}
		} else
		{
			printk("Cannot get stmfb_info struct\n");
		}

		/* always return count to avoid endless loop */
		ret = count;
	}

out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_video_pal_h_start_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int                      len = 0;
	void                     *fb = NULL;
	struct fb_info           *info;

	printk("%s\n", __FUNCTION__);

   	fb = stmfb_get_fbinfo_ptr(); /* cannot return a NULL pointer */

	/*
	 * dirty hack, does only work because fb->info is first param of struct stmfb_info
	 * we should include a header here instead ...
	 */
	info = (struct fb_info*) fb;

	len = sprintf(page, "%X\n", info->var.left_margin);

	return len;
}

int proc_video_pal_h_end_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int                      len = 0;
	void                     *fb = NULL;
	struct fb_info           *info;

	printk("%s\n", __FUNCTION__);

   	fb = stmfb_get_fbinfo_ptr(); /* cannot return a NULL pointer */

	/*
	 * dirty hack, does only work because fb->info is first param of struct stmfb_info
	 * we should include a header here instead ...
	 */
	info = (struct fb_info*) fb;

	len = sprintf(page, "%X\n", info->var.right_margin);

	return len;
}

int proc_video_pal_v_start_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int                      len = 0;
	void                     *fb = NULL;
	struct fb_info           *info;

	printk("%s\n", __FUNCTION__);

   	fb = stmfb_get_fbinfo_ptr(); /* cannot return a NULL pointer */

	/*
	 * dirty hack, does only work because fb->info is first param of struct stmfb_info
	 * we should include a header here instead ...
	 */
	info = (struct fb_info*) fb;

	len = sprintf(page, "%X\n", info->var.upper_margin);

	return len;
}

int proc_video_pal_v_end_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int                      len = 0;
	void                     *fb = NULL;
	struct fb_info           *info;

	printk("%s\n", __FUNCTION__);

   	fb = stmfb_get_fbinfo_ptr(); /* cannot return a NULL pointer */

	/*
	 * dirty hack, does only work because fb->info is first param of struct stmfb_info
	 * we should include a header here instead ...
	 */
	info = (struct fb_info*) fb;

	len = sprintf(page, "%X\n", info->var.lower_margin);

	return len;
}

int proc_video_videomode_choices_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		/* always return count to avoid endless loop */
		ret = count;
	}

	free_page((unsigned long)page);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_video_videomode_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s %d\n", __FUNCTION__, count);
#if defined(UFS912) || defined(SPARK) || defined (SPARK7162) || defined(ATEVIO7500) || defined(HS7810A) || defined(HS7110)
	len = sprintf(page, "pal 1080i50 720p50 576p50 576i50 1080i60 720p60 1080p24 1080p25 1080p30 1080p50 1080p59 1080p60 PC\n");
#else
	len = sprintf(page, "pal 1080i50 720p50 576p50 576i50 1080i60 720p60 1080p24 1080p25 1080p30 PC\n");
#endif

        return len;
}

int proc_video_videomode_preferred_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		kfree(myString);
	}

out:
	free_page((unsigned long)page);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_video_videomode_preferred_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "HDMI\n");

        return len;
}

int proc_video_alpha_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	/* int		result; */

	printk("%s %ld - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		struct stmfb_info *info = stmfb_get_fbinfo_ptr();

		struct stmfbio_var_screeninfo_ex varEx;
		int err = 0;
		int alpha = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%d", &alpha);



		varEx.layerid  = 0;
		varEx.caps     = 0;
		varEx.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;
		varEx.caps |= STMFBIO_VAR_CAPS_OPACITY;
		varEx.opacity = alpha;

		err = stmfb_set_var_ex(&varEx, info);

		kfree(myString);
	}

	ret = count;
out:

	free_page((unsigned long)page);
	return ret;
}


int proc_video_alpha_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	/*void* fb =  stmfb_get_fbinfo_ptr();

	struct stmfb_info *info = (struct stmfb_info*) fb;

	struct stmfbio_var_screeninfo_ex screen_info;
	memcpy(&screen_info, &info->current_var_ex, sizeof(struct stmfbio_var_screeninfo_ex));

	if (fb != NULL)
	{
		len = sprintf(page, "%d\n", screen_info.opacity);
	} else*/
		len = sprintf(page, "0\n");

        return len;
}
