/* 
 * e2_proc_hdmi.c
 */
 
#include <linux/proc_fs.h>  	/* proc fs */ 
#include <asm/uaccess.h>    	/* copy_from_user */

#include <linux/dvb/video.h>	/* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/fb.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

#define STMHDMIIO_AUDIO_SOURCE_2CH_I2S (0)
#define STMHDMIIO_AUDIO_SOURCE_PCM     STMHDMIIO_AUDIO_SOURCE_2CH_I2S
#define STMHDMIIO_AUDIO_SOURCE_SPDIF   (1)
#define STMHDMIIO_AUDIO_SOURCE_8CH_I2S (2)
#define STMHDMIIO_AUDIO_SOURCE_NONE    (0xffffffff)

#define STMHDMIIO_EDID_STRICT_MODE_HANDLING 0
#define STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING 1

extern long stmhdmiio_set_audio_source(unsigned int arg);
extern long stmhdmiio_get_audio_source(unsigned int * arg);

extern long stmhdmiio_set_edid_handling(unsigned int arg);
extern long stmhdmiio_get_edid_handling(unsigned int * arg);

extern struct DeviceContext_s* ProcDeviceContext;



int proc_hdmi_audio_source_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	unsigned int 	value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));


	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		
		if (strncmp("spdif", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_SPDIF;
		}		
		else if (strncmp("pcm", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_PCM;
		}
		else if (strncmp("8ch", myString, count - 1) == 0)
		{
			value = STMHDMIIO_AUDIO_SOURCE_8CH_I2S;
		}
		else 
		{
			value = STMHDMIIO_AUDIO_SOURCE_NONE;
		}		

		stmhdmiio_set_audio_source(value);

		
		/* always return count to avoid endless loop */
		ret = count;	
	}
	
out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}


int proc_hdmi_audio_source_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value = 0;


	printk("%s\n", __FUNCTION__);
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	stmhdmiio_get_audio_source(&value);
	printk("%s - %u\n", __FUNCTION__, value);
	switch (value)
	{
	case STMHDMIIO_AUDIO_SOURCE_2CH_I2S:
		len = sprintf(page, "pcm\n");
		break;
	case STMHDMIIO_AUDIO_SOURCE_SPDIF:
		len = sprintf(page, "spdif\n");
		break;
	case STMHDMIIO_AUDIO_SOURCE_8CH_I2S:
		len = sprintf(page, "8ch\n");
		break;
	case STMHDMIIO_AUDIO_SOURCE_NONE:
	default:
		len = sprintf(page, "none\n");
		break;
	}

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}

int proc_hdmi_audio_source_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;

	printk("%s\n", __FUNCTION__);
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	len = sprintf(page, "pcm spdif 8ch none\n");

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}

int proc_hdmi_edid_handling_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	unsigned int 	value;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %ld - ", __FUNCTION__, count);

	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));


	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		
		if (strncmp("strict", myString, count - 1) == 0)
		{
			value = STMHDMIIO_EDID_STRICT_MODE_HANDLING;
		}		
		else if (strncmp("nonstrict", myString, count - 1) == 0)
		{
			value = STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING;
		}
		else 
		{
			value = STMHDMIIO_EDID_STRICT_MODE_HANDLING;
		}		

		stmhdmiio_set_edid_handling(value);

		
		/* always return count to avoid endless loop */
		ret = count;	
	}
	
out:
	free_page((unsigned long)page);
	kfree(myString);
    	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));
	return ret;
}

int proc_hdmi_edid_handling_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	unsigned int value = 0;


	printk("%s\n", __FUNCTION__);
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	stmhdmiio_get_edid_handling(&value);
	printk("%s - %u\n", __FUNCTION__, value);
	switch (value)
	{
	case STMHDMIIO_EDID_NON_STRICT_MODE_HANDLING:
		len = sprintf(page, "nonstrict\n");
		break;
	case STMHDMIIO_EDID_STRICT_MODE_HANDLING:
	default:
		len = sprintf(page, "strict\n");
		break;
	}

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}
