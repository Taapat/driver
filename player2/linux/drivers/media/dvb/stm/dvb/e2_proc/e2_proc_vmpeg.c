/* 
 * e2_proc_vmpeg.c
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

extern struct stmfb_info* stmfb_get_fbinfo_ptr(void);

extern struct DeviceContext_s* ProcDeviceContext;

int proc_vmpeg_0_dst_left_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	int 		value, err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;
	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %d - ", __FUNCTION__, (unsigned int)count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

        fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		sscanf(myString, "%x", &value);

		if (ProcDeviceContext != NULL)
		{
			int l,t,w,h;
			err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
			else	
				printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);

			l = x*value/720;

		    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream, l, t, w, h);
				
			if (err != 0)
				printk("failed to set output window %d\n", err);
			else	
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
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


int proc_vmpeg_0_dst_left_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int    len = 0;
	int    l,t,w,h;
	int    err, x, y;
        void   *fb;
        struct fb_info *info;
	struct fb_var_screeninfo screen_info;
	
	printk("%s\n", __FUNCTION__);
	
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	if (ProcDeviceContext != NULL)
	{
		err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
		if (err != 0)
			printk("failed to get output window %d\n", err);
		else	
			printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);
	}
	len = sprintf(page, "%x\n", 720*l/x);

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}

int proc_vmpeg_0_dst_top_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	int 		value, err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;
	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %d - ", __FUNCTION__, (unsigned int)count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		sscanf(myString, "%x", &value);

		if (ProcDeviceContext != NULL)
		{
			int l,t,w,h;
			err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
			else	
				printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);

			t = y*value/576;

		    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream, l, t, w, h);
				
			if (err != 0)
				printk("failed to set output window %d\n", err);
			else	
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
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


int proc_vmpeg_0_dst_top_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	int l,t,w,h;
	int err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;

	printk("%s\n", __FUNCTION__);

	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	if (ProcDeviceContext != NULL)
	{
		err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
		if (err != 0)
			printk("failed to get output window %d\n", err);
		else	
			printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);
	}
	len = sprintf(page, "%x\n", 576*t/y);

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}


int proc_vmpeg_0_dst_width_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	int 		value, err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %d - ", __FUNCTION__, (unsigned int)count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		sscanf(myString, "%x", &value);

		if (ProcDeviceContext != NULL)
		{
			int l,t,w,h;
			err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
			else	
				printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);

			w = x*value/720;

		    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream, l, t, w, h);
				
			if (err != 0)
				printk("failed to set output window %d\n", err);
			else	
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
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


int proc_vmpeg_0_dst_width_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	int l,t,w,h;
	int err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;

	printk("%s\n", __FUNCTION__);
	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	if (ProcDeviceContext != NULL)
	{
		err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
		if (err != 0)
			printk("failed to get output window %d\n", err);
		else	
			printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);
	}
	len = sprintf(page, "%x\n", 720*w/x);

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}

int proc_vmpeg_0_dst_height_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	int 		value, err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;

	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %d - ", __FUNCTION__, (unsigned int)count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		sscanf(myString, "%x", &value);

		if (ProcDeviceContext != NULL)
		{
			int l,t,w,h;
			err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
			if (err != 0)
				printk("failed to get output window %d\n", err);
			else	
				printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);

			h = y*value/576;

		    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream, l, t, w, h);
				
			if (err != 0)
				printk("failed to set output window %d\n", err);
			else	
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
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


int proc_vmpeg_0_dst_height_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	int l,t,w,h;
	int err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;

	printk("%s\n", __FUNCTION__);

	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	if (ProcDeviceContext != NULL)
	{
		err = StreamGetOutputWindow(ProcDeviceContext->VideoStream, &l,&t,&w,&h);
		if (err != 0)
			printk("failed to get output window %d\n", err);
		else	
			printk("get output window to %d %d %d, %d ok\n",  l,  t, w, h);
	}
	len = sprintf(page, "%x\n", 576*h/y);

	mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

        return len;
}

int proc_vmpeg_0_yres_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
    int len = 0;
    
    printk("%s\n", __FUNCTION__);

    mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

    len = sprintf(page, "%x\n", ProcDeviceContext->VideoSize.h);

    mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

    return len;
}

int proc_vmpeg_0_xres_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
    int len = 0;
    
    printk("%s\n", __FUNCTION__);

    mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

    len = sprintf(page, "%x\n", ProcDeviceContext->VideoSize.w);

    mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

    return len;
}

int proc_vmpeg_0_framerate_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
    int len = 0;
    
    printk("%s\n", __FUNCTION__);

    mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

    len = sprintf(page, "%x\n", ProcDeviceContext->FrameRate);

    mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

    return len;
}

int proc_vmpeg_0_aspect_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
    int len = 0;
    
    printk("%s\n", __FUNCTION__);

    mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

    len = sprintf(page, "%x\n", ProcDeviceContext->VideoSize.aspect_ratio);

    mutex_unlock (&(ProcDeviceContext->DvbContext->Lock));

    return len;
}

int proc_vmpeg_0_dst_all_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	ssize_t 	ret = -ENOMEM;
	int 		err, x, y;
        void*           fb;
        struct fb_info  *info;
	struct fb_var_screeninfo screen_info;
	char* myString = kmalloc(count + 1, GFP_KERNEL);

	printk("%s %d - ", __FUNCTION__, (unsigned int)count);

    	mutex_lock (&(ProcDeviceContext->DvbContext->Lock));

	fb =  stmfb_get_fbinfo_ptr();
			
	info = (struct fb_info*) fb;

	memcpy(&screen_info, &info->var, sizeof(struct fb_var_screeninfo));
		
	if (fb != NULL)
	{
		y=screen_info.yres;
		x=screen_info.xres;
	} else
	{
		y=576;
		x=720;
        }

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		int l,t,w,h;
		
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		sscanf(myString, "%x %x %x %x", &l ,&t, &w, &h);

                printk("%x, %x, %x, %x\n", l, t, w, h);

		if (ProcDeviceContext != NULL)
		{
			h = y*h/576;
			w = x*w/720;
			l = x*l/720;
			t = y*t/576;

                        printk("%x, %x, %x, %x\n", l, t, w, h);

		    	err = StreamSetOutputWindow(ProcDeviceContext->VideoStream, l, t, w, h);
				
			if (err != 0)
				printk("failed to set output window %d\n", err);
			else	
				printk("set output window ok %d %d %d %d\n", l, t, w, h);
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

