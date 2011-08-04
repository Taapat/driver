 
#define I2C_DRIVERID_LNB	1
#define LNB_MINOR			0
#define LNB_MAJOR 			149

 
/* IOCTL */
#define LNB_VOLTAGE_OFF   	 	0x2b0010
#define LNB_VOLTAGE_VER   	 	0x2b0011
#define LNB_VOLTAGE_HOR   	 	0x2b0012

extern short debug;

#define dprintk(fmt...) \
	do { \
		if (debug) printk (fmt); \
	} while (0)
