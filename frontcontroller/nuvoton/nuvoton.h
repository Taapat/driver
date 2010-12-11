#ifndef _123_nuvoton
#define _123_nuvoton
/*
 */

extern short paramDebug;

#define TAGDEBUG "[nuvoton] "

#ifndef dprintk
#define dprintk(level, x...) do { \
        if ((paramDebug) && (paramDebug >= level)) printk(TAGDEBUG x); \
    } while (0)
#endif

extern int nuvoton_init_func(void);
extern void copyData(unsigned char* data, int len);
extern void getRCData(unsigned char* data, int* len);
void dumpValues(void);

extern int                  errorOccured;

extern struct file_operations vfd_fops;

typedef struct
{
    struct file*      fp;
    int               read;
    struct semaphore  sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC   1
#define LASTMINOR             2

extern tFrontPanelOpen FrontPanelOpen[LASTMINOR];

#define VFD_MAJOR           147

#define SOP              0x02
#define EOP              0x03

/* ioctl numbers ->hacky */
#define VFDBRIGHTNESS         0xc0425a03
#define VFDPWRLED             0xc0425a04 /* added by zeroone, also used in fp_control/global.h ; set PowerLed Brightness on HDBOX*/
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc

#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff

struct set_brightness_s {
    int level;
};

struct set_pwrled_s {
    int level;
};

struct set_icon_s {
    int icon_nr;
    int on;
};

struct set_led_s {
    int led_nr;
    int on;
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s {
    char time[5];
};

struct set_time_s {
    char time[5];
};

/* this setups the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s {
    int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct nuvoton_ioctl_data {
    union
    {
        struct set_icon_s icon;
        struct set_led_s led;
        struct set_brightness_s brightness;
        struct set_pwrled_s pwrled;
        struct set_mode_s mode;
        struct set_standby_s standby;
        struct set_time_s time;
    } u;
};

struct vfd_ioctl_data {
    unsigned char start_address;
    unsigned char data[64];
    unsigned char length;
};

#ifdef OCTAGON1008
struct vfd_buffer {
    u8 buf1;
    u8 buf2;
};
#endif

#endif
