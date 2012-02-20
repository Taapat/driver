######################################################################
#	Module Name: Makefile
#	Abstract: Makefile for rt73 linux driver on kernel series 2.6 
######################################################################
PLATFORM=PC
#PLATFORM=CMPC

#Enable for wpa_supplicant's spcific driver "driver_ralink.c", default enabled.
HAS_RALINK_WPA_SUPPLICANT_SUPPORT=y

#Enable for wpa_supplicant's generic driver "driver_wext.c", used for NetworkManager
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=n

#Support Block Net-If during TxSw queue full.
HAS_BLOCK_NET_IF=y


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

## Comment/uncomment the following line to enable/disable debugging
#EXTRA_CFLAGS += -DDBG

EXTRA_CFLAGS += $(WFLAGS)

ifeq ($(PLATFORM),PC)
LINUX_SRC =/lib/modules/$(shell uname -r)/build
endif
ifeq ($(PLATFORM),CMPC)
LINUX_SRC = /home/fonchi/vendor/kernel-default-2.6.21/linux-2.6.21
endif

EXTRA_CFLAGS += -I$(LINUX_SRC)/include

obj-m := rt73.o

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

## Configuration files on SuSE
#NDDIR=/etc/sysconfig/network
#MOD_CONF=/etc/modprobe.d/module-renames

## NDDIR/ifcfg-rausb0
ND_CONF=/etc/sysconfig/network-scripts/ifcfg-rausb0

ifdef NDDIR
	NDDIR := $(NDDIR)
else
 	NDDIR := /etc/sysconfig/network-scripts
endif

ifdef MOD_CONF
	MOD_CONF := $(MOD_CONF)	
else
	MOD_CONF := /etc/modprobe.conf 
endif

all: 
	make -C $(LINUX_SRC) SUBDIRS=$(shell pwd) modules

clean:
	rm -rf *.o *~ .*.cmd *.ko *.mod.c .tmp_versions built-in.o
	
install:
	make -C /lib/modules/$(shell uname -r)/build \
	INSTALL_MOD_DIR=extra SUBDIRS=$(shell pwd) \
	modules_install 
	
	@echo "Network device directory $(NDDIR)"; 
	@echo "Module configuration file $(MOD_CONF)";	
	
	@if [ -d "$(NDDIR)" ] ; then \
		if [ ! -f "$(ND_CONF)" ]; then \
			echo "Create 'ifcfg-rausb0' in $(NDDIR)/"; \
			cp ifcfg-rausb0 $(NDDIR)/ ; \
		fi; \
 	fi
	
	@if ! grep -q 'rausb0' $(MOD_CONF) ; then \
		echo "append 'alias rausb0 rt73' to $(MOD_CONF)"; \
		echo "alias rausb0 rt73" >> $(MOD_CONF) ; \
 	fi
	/sbin/depmod -a

#	auto copy *.bin to /etc/Wireless/RT73STA
#	mkdir /etc/Wireless ;
#	mkdir /etc/Wireless/RT73STA ;
#	cp $(shell pwd)/*.bin /etc/Wireless/RT73STA ;
#	cp $(shell pwd)/*.dat /etc/Wireless/RT73STA ;

#	Copy file on Mandriva 2007.1
#	cp $(shell pwd)/*.bin /lib/firmware/ ;

config:
	@touch config.mk
	@./Configure
