ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

MODULE_NAME := rt73
IF_NAME :=     wlan*

FIRMWARES :=   rt73.bin
FIRM_DIR :=    /lib/firmware

CONFIG_DIR :=  /etc/Wireless/RT73STA
CONFIG_FILE := rt73sta.dat

EXTRA_CFLAGS += -I$(src)/include 
EXTRA_CFLAGS += -DAGGREGATION_SUPPORT -DWMM_SUPPORT
#EXTRA_CFLAGS += -DRALINK_ATE


EXTRA_CFLAGS += -o $(WFLAGS)

$(MODULE_NAME)-objs := 	rtmp_main.o \
		mlme.o \
		connect.o \
		rtusb_bulk.o \
		rtusb_io.o \
		sync.o \
		assoc.o \
		auth.o \
		auth_rsp.o \
		rtusb_data.o \
		rtmp_init.o  \
		sanity.o \
		rtmp_wep.o \
		rtmp_info.o \
		rtmp_tkip.o \
		wpa.o \
		md5.o \
		rt2x00debug.o

ifdef TOPDIR
obj-m += $(MODULE_NAME).o
endif





