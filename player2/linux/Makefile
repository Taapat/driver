##TREE_ROOT?=/home/nick/cvs/blue_ray/player2/
TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CFLAGS+=-I$(CONFIG_KERNEL_BUILD)/include2 \
	-I$(KERNEL_LOCATION)/include \
	-include $(KERNEL_LOCATION)/include/linux/autoconf.h \
	-I$(CONFIG_KERNEL_PATH)/include -D__KERNEL__ \
        -I$(DRIVER_TOPDIR)/include/

CFLAGS+=-I$(TREE_ROOT)/linux/components/inline         \
	-I$(TREE_ROOT)/components/include              \
	-I$(TREE_ROOT)/components/infrastructure       \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor \
	-I$(TREE_ROOT)/linux/drivers/stm/st_relay \
	-I$(TREE_ROOT)/linux/drivers/sound/ksound

CFLAGS+=-I$(TREE_ROOT)/player/class_definitions        \
	-I$(TREE_ROOT)/player/buffer                   \
	-I$(TREE_ROOT)/player/demultiplexor            \
	-I$(TREE_ROOT)/player/collator                 \
	-I$(TREE_ROOT)/player/frame_parser             \
	-I$(TREE_ROOT)/player/codec                    \
	-I$(TREE_ROOT)/player/standards                \
	-I$(TREE_ROOT)/player/manifestor               \
	-I$(TREE_ROOT)/player/player/                  \
	-I$(TREE_ROOT)/player/output_coordinator       \
	-I$(TREE_ROOT)/player/output_timer

CFLAGS+=-I$(CONFIG_STGFB_PATH)/
CFLAGS+=-I$(CONFIG_STGFB_PATH)/Linux/include/

CFLAGS+=-I$(TREE_ROOT)/linux/include/

# promote CONFIG_RELAY to a CFLAGS define so that it can be observed by C++
# code as well and appropriate inlining performed at the earliest possible
# point.
ifdef CONFIG_RELAY
CFLAGS+=-DCONFIG_RELAY=1
endif

# CONFIG_DUAL_DISPLAY is a temporary hack to convert the existing
# picture-in-picture and speaker-in-speaker support into two
# independant displays.
#CFLAGS+=-DCONFIG_DUAL_DISPLAY

CFLAGS+=-I$(KERNEL_LOCATION)/include/linux

# promote CONFIG_MONITOR to a CFLAGS define so that it can be observed by C++
# code as well and appropriate inlining performed at the earliest possible
# point.
#ifdef CONFIG_MONITOR
CFLAGS+=-DCONFIG_MONITOR=1
#endif

# Finally pick up the original system headers, these must be near the '
# back' of the path to ensure the kernel headers are taken in 
# preference to any system supplied ones. Items such as the MME header 
# files (XXXX_DecoderTypes.h) form part of the system headers.
CFLAGS += -I$(DRIVER_TOPDIR)/include/player2

CFLAGS += -D__SH4__       

CFLAGS += -DREPORT

obj-y += drivers/media/dvb/stm/mpeg2_hard_host_transformer/
obj-y += drivers/media/dvb/stm/dvb/
obj-y += drivers/media/dvb/stm/backend/
obj-y += drivers/media/dvb/stm/allocator/
obj-y += drivers/media/dvb/stm/h264_preprocessor/
obj-y += drivers/media/video/stm/
obj-y += drivers/media/sysfs/stm/
obj-y += drivers/sound/ksound/
obj-y += drivers/sound/kreplay/
obj-y += drivers/sound/pseudocard/
obj-y += drivers/sound/silencegen/
obj-y += drivers/stm/

obj-y += dummy.o
