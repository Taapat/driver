ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif


HAS_RALINK_WPA_SUPPLICANT_SUPPORT=n
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y
HAS_BLOCK_NET_IF=y

EXTRA_CFLAGS += -I$(src)/include 
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs 


WFLAGS := -Wall -Wstrict-prototypes -Wno-trigraphs

ifeq ($(HAS_RALINK_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DRALINK_WPA_SUPPLICANT_SUPPORT
endif

ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
WFLAGS += -DBLOCK_NET_IF
endif

EXTRA_CFLAGS += -o $(WFLAGS) 


rt73-objs := \
	rtmp_main.o \
	mlme.o \
	connect.o \
	rtusb_bulk.o \
	rtusb_io.o \
	sync.o \
	assoc.o \
	auth.o \
	auth_rsp.o \
	rtusb_data.o \
	rtmp_init.o \
	sanity.o \
	rtmp_wep.o \
	rtmp_info.o \
	rtmp_tkip.o \
	wpa.o md5.o

ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
rt73-objs += rtmp_wext.o
endif

ifeq ($(HAS_BLOCK_NET_IF),y)
rt73-objs += netif_block.o
endif


obj-m := rt73.o
