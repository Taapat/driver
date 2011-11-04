TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb

ifdef UFS922
 PART1=1
endif
ifdef TF7700
 PART1=1
endif
ifdef FORTIS_HDBOX
 PART1=1
endif
ifdef HL101
 PART1=1
endif
ifdef VIP1_V2
 PART1=1
endif
ifdef VIP2_V1
 PART1=1
endif
ifdef OCTAGON1008
 PART1=1
endif
ifdef CUBEREVO
 PART1=1
endif
ifdef CUBEREVO_MINI2
 PART1=1
endif
ifdef CUBEREVO_MINI
 PART1=1
endif
ifdef CUBEREVO_250HD
 PART1=1
endif
ifdef CUBEREVO_2000HD
 PART1=1
endif
ifdef CUBEREVO_9500HD
 PART1=1
endif
ifdef CUBEREVO_MINI_FTA
 PART1=1
endif
ifdef IPBOX9900
 PART1=1
endif
ifdef IPBOX99
 PART1=1
endif
ifdef IPBOX55
 PART1=1
endif

ifdef UFS912
 PART2=1
endif
ifdef SPARK
 PART2=1
endif
ifdef SPARK7162
 PART2=1
endif
ifdef ATEVIO7500
 PART2=1
endif
ifdef HS7810A
 PART2=1
endif
ifdef HS7110
 PART2=1
endif

ifndef PART1
ifndef PART2
 PART3=1
endif
endif

ifdef PART1
CCFLAGSY+=-DPLAYER2_PRIMARY_VIDEO_BUFFER_MEMORY=0x01B00000 \
	-DPLAYER2_SECONDARY_VIDEO_BUFFER_MEMORY=0x00300000 \
	-DPLAYER2_AVR_VIDEO_BUFFER_MEMORY=0x00000000 \
	-DPLAYER2_H264_MACROBLOCK_STRUCTURE_MEMORY=0x580000 \
	-DPLAYER2_H264_FRAME_MEMORY_SIZE=0x800000 \
	-DPLAYER2_H264_STREAM_PARAMETERS_COUNT=64 \
	-DPLAYER2_H264_FRAME_PARAMETERS_COUNT=64 \
	-DPLAYER2_AUDIO_DEFAULT_CODED_MEMORY_SIZE=0x400000 \
	-DPLAYER2_VIDEO_DEFAULT_CODED_MEMORY_SIZE=0x800000
endif

ifdef PART2
#UFS912, SPARK,SPARK7162, atevio7500, hs7810a, hs7110
CCFLAGSY+=-DPLAYER2_PRIMARY_VIDEO_BUFFER_MEMORY=0x02400000 \
	-DPLAYER2_SECONDARY_VIDEO_BUFFER_MEMORY=0x00900000 \
	-DPLAYER2_AVR_VIDEO_BUFFER_MEMORY=0x00000000 \
	-DPLAYER2_H264_MACROBLOCK_STRUCTURE_MEMORY=0xa00000 \
	-DPLAYER2_H264_FRAME_MEMORY_SIZE=0x1000000 \
	-DPLAYER2_H264_STREAM_PARAMETERS_COUNT=128 \
	-DPLAYER2_H264_FRAME_PARAMETERS_COUNT=128 \
	-DPLAYER2_AUDIO_DEFAULT_CODED_MEMORY_SIZE=0x400000 \
	-DPLAYER2_VIDEO_DEFAULT_CODED_MEMORY_SIZE=0x1000000
endif

ifdef PART3
#UFS910 and others
CCFLAGSY+=-DPLAYER2_PRIMARY_VIDEO_BUFFER_MEMORY=0x01B00000 \
        -DPLAYER2_SECONDARY_VIDEO_BUFFER_MEMORY=0x00000000 \
        -DPLAYER2_AVR_VIDEO_BUFFER_MEMORY=0x00000000 \
        -DPLAYER2_AVR_VIDEO_BUFFER_MEMORY=0x00000000 \
        -DPLAYER2_H264_MACROBLOCK_STRUCTURE_MEMORY=0x580000 \
        -DPLAYER2_H264_FRAME_MEMORY_SIZE=0x800000 \
        -DPLAYER2_H264_STREAM_PARAMETERS_COUNT=64 \
        -DPLAYER2_H264_FRAME_PARAMETERS_COUNT=64 \
        -DPLAYER2_AUDIO_DEFAULT_CODED_MEMORY_SIZE=0x400000 \
        -DPLAYER2_VIDEO_DEFAULT_CODED_MEMORY_SIZE=0x800000
endif

# workaround for stm24
ifdef CONFIG_CPU_SUBTYPE_STX7100
CCFLAGSY+=-DCONFIG_CPU_SUBTYPE_STB7100
endif

CCFLAGSY+=-D__TDT__

CCFLAGSY+=-I$(CONFIG_KERNEL_BUILD)/include2 \
	-I$(CONFIG_KERNEL_BUILD)/include \
	-include $(KERNEL_LOCATION)/include/linux/autoconf.h \
	-I$(CONFIG_KERNEL_PATH)/include -D__KERNEL__ \
	-I$(DRIVER_TOPDIR)/include/player2

CCFLAGSY+=-I$(TREE_ROOT)/linux/components/inline         \
	-I$(TREE_ROOT)/components/include              \
	-I$(TREE_ROOT)/components/infrastructure       \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/allocator \
	-I$(TREE_ROOT)/linux/drivers/media/dvb/stm/h264_preprocessor \
	-I$(TREE_ROOT)/linux/drivers/stm/st_relay \
	-I$(TREE_ROOT)/linux/drivers/sound/ksound \
	-I$(TREE_ROOT)/linux/drivers/sound/pcm_transcoder

CCFLAGSY+=-I$(TREE_ROOT)/player/class_definitions        \
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

CCFLAGSY+=-I$(CONFIG_STGFB_PATH)/
CCFLAGSY+=-I$(CONFIG_STGFB_PATH)/linux/include/

CCFLAGSY+=-I$(TREE_ROOT)/linux/include/

# promote CONFIG_RELAY to a CCFLAGSY define so that it can be observed by C++
# code as well and appropriate inlining performed at the earliest possible
# point.
ifdef CONFIG_RELAY
CCFLAGSY+=-DCONFIG_RELAY=1
endif

# CONFIG_DUAL_DISPLAY is a temporary hack to convert the existing
# picture-in-picture and speaker-in-speaker support into two
# independant displays also used for the compositor capture now.
ifdef CONFIG_DUAL_DISPLAY
CCFLAGSY+=-DCONFIG_DUAL_DISPLAY
endif

# promote CONFIG_MONITOR to a CCFLAGSY define so that it can be observed by C++
# code as well and appropriate inlining performed at the earliest possible
# point.
#ifdef CONFIG_MONITOR
CCFLAGSY+=-DCONFIG_MONITOR=1
#endif

# Finally pick up the original system headers, these must be near the '
# back' of the path to ensure the kernel headers are taken in
# preference to any system supplied ones. Items such as the MME header
# files (XXXX_DecoderTypes.h) form part of the system headers.
CCFLAGSY += -I$(CONFIG_MODULES_PATH)/usr/include

CCFLAGSY += -D__SH4__

CCFLAGSY += -DREPORT

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
export CCFLAGSY

obj-y += drivers/media/dvb/stm/mpeg2_hard_host_transformer/
obj-y += drivers/media/dvb/stm/dvb/
obj-y += drivers/media/dvb/stm/backend/
obj-y += drivers/media/dvb/stm/allocator/
obj-y += drivers/media/dvb/stm/h264_preprocessor/
obj-y += drivers/media/sysfs/stm/
obj-y += drivers/sound/ksound/
obj-y += drivers/sound/pseudocard/
obj-y += drivers/sound/silencegen/
obj-y += drivers/sound/pcm_transcoder/
obj-y += drivers/stm/

obj-y += dummy.o
