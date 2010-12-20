manifestor-objs:=       manifestor_base.o \
			manifestor_video.o \
			manifestor_video_stmfb.o \
			manifestor_audio.o \
			manifestor_audio_ksound.o \
			mixer_mme.o \
			pcmplayer_ksound.o

CCFLAGSY+=-I$(TREE_ROOT)/linux/drivers/sound/pseudocard/
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-y += $(manifestor-objs)
