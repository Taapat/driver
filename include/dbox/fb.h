#ifndef __DBOX_FB_H__
#define __DBOX_FB_H__

#include <asm/types.h>

//FIXME
/* deleted due to definition of fb.h in cdkroot/usr/include/linux (from package kernelheaders)
typedef struct {
	__u32 sx;*/	/* screen-relative */ /*
	__u32 sy;
	__u32 width;
	__u32 height;
	__u32 dx;
	__u32 dy;
} fb_copyarea;
*/

#define AVIA_GT_GV_SET_BLEV	0	/* blend level */
#define AVIA_GT_GV_SET_POS	1	/* position of graphics frame */
#define AVIA_GT_GV_HIDE		2	/* hide framebuffer */
#define AVIA_GT_GV_SHOW		3	/* show framebuffer */
#define AVIA_GT_GV_COPYAREA	4	/* copy area */
#define AVIA_GT_GV_GET_BLEV	5	/* blend level */

#endif /* __DBOX_FB_H__ */
