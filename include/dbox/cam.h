#ifndef __dbox2_cam_h__
#define __dbox2_cam_h__

#ifdef __KERNEL__
extern unsigned int cam_poll(struct file *file, poll_table *wait);
extern int cam_read_message(char *buf, size_t count);
extern int cam_reset(void);
extern int cam_write_message(char *buf, size_t count);
#endif

#endif
