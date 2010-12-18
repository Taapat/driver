##TREE_ROOT?=/home/nick/cvs/blue_ray/player2/

CFLAGS+=-DPLAYER2_PRIMARY_VIDEO_BUFFER_MEMORY=$(PLAYER2_PRIMARY_VIDEO_BUFFER_MEMORY) \
	-DPLAYER2_SECONDARY_VIDEO_BUFFER_MEMORY=$(PLAYER2_SECONDARY_VIDEO_BUFFER_MEMORY) \
	-DPLAYER2_AVR_VIDEO_BUFFER_MEMORY=$(PLAYER2_AVR_VIDEO_BUFFER_MEMORY) \
	-DPLAYER2_H264_MACROBLOCK_STRUCTURE_MEMORY=$(PLAYER2_H264_MACROBLOCK_STRUCTURE_MEMORY) \
	-DPLAYER2_H264_FRAME_MEMORY_SIZE=$(PLAYER2_H264_FRAME_MEMORY_SIZE) \
	-DPLAYER2_H264_STREAM_PARAMETERS_COUNT=$(PLAYER2_H264_STREAM_PARAMETERS_COUNT) \
	-DPLAYER2_H264_FRAME_PARAMETERS_COUNT=$(PLAYER2_H264_FRAME_PARAMETERS_COUNT) \
	-DPLAYER2_AUDIO_DEFAULT_CODED_MEMORY_SIZE=$(PLAYER2_AUDIO_DEFAULT_CODED_MEMORY_SIZE) \
	-DPLAYER2_VIDEO_DEFAULT_CODED_MEMORY_SIZE=$(PLAYER2_VIDEO_DEFAULT_CODED_MEMORY_SIZE)

CFLAGS+=-I$(CONFIG_KERNEL_BUILD)/include2 \
	-I$(CONFIG_KERNEL_BUILD)/include \
	-include $(CONFIG_KERNEL_BUILD)/include/linux/autoconf.h \
	-I$(CONFIG_KERNEL_PATH)/include -D__KERNEL__

CFLAGS+=-I$(TREE_ROOT)/linux/components/inline         \
	-I$(TREE_ROOT)/components/include              \
	-I$(TREE_ROOT)/components/infrastructure       \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor \
	-I$(TREE_ROOT)/linux/drivers/stm/st_relay \
	-I$(TREE_ROOT)/linux/drivers/sound/ksound \
	-I$(TREE_ROOT)/linux/drivers/sound/pcm_transcoder

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
CFLAGS+=-I$(CONFIG_STGFB_PATH)/linux/include/

CFLAGS+=-I$(TREE_ROOT)/linux/include/

# promote CONFIG_RELAY to a CFLAGS define so that it can be observed by C++
# code as well and appropriate inlining performed at the earliest possible
# point.
ifdef CONFIG_RELAY
CFLAGS+=-DCONFIG_RELAY=1
endif

# CONFIG_DUAL_DISPLAY is a temporary hack to convert the existing
# picture-in-picture and speaker-in-speaker support into two
# independant displays also used for the compositor capture now.
ifdef CONFIG_DUAL_DISPLAY
CFLAGS+=-DCONFIG_DUAL_DISPLAY
endif

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
CFLAGS += -I$(CONFIG_MODULES_PATH)/usr/include

CFLAGS += -D__SH4__       

CFLAGS += -DREPORT

obj-y += drivers/media/dvb/stm/mpeg2_hard_host_transformer/
obj-y += drivers/media/dvb/stm/dvb/
obj-y += drivers/media/dvb/stm/backend/
obj-y += drivers/media/dvb/stm/allocator/
obj-y += drivers/media/dvb/stm/h264_preprocessor/
obj-y += drivers/media/sysfs/stm/
obj-y += drivers/sound/ksound/
obj-y += drivers/sound/kreplay/
obj-y += drivers/sound/pseudocard/
obj-y += drivers/sound/silencegen/
obj-y += drivers/sound/pcm_transcoder/
obj-y += drivers/stm/

obj-y += dummy.o
