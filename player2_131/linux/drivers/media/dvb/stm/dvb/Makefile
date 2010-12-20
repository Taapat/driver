TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/ 		\
		-I$(TREE_ROOT)/linux/drivers/media/video/stm/		\
		-I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core		\
		-I$(CONFIG_STGFB_PATH)/include
		
CCFLAGSY += -DDEBUG_DVB

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

stmdvb-objs:=   backend.o \
		dvb_audio.o \
		dvb_demux.o \
		dvb_dvr.o \
		dvb_module.o \
		dvb_video.o \
		dvb_ca.o \
		dvb_v4l2.o \
		e2_proc/e2_proc_bus.o \
		e2_proc/e2_proc_misc.o \
		e2_proc/e2_proc_audio.o \
		e2_proc/e2_proc_avs.o \
		e2_proc/e2_proc_denc.o \
		e2_proc/e2_proc_hdmi.o \
		e2_proc/e2_proc_main.o \
		e2_proc/e2_proc_tsmux.o \
		e2_proc/e2_proc_video.o \
		e2_proc/e2_proc_vmpeg.o \
		e2_proc/e2_proc_progress.o \
		e2_proc/e2_proc_stream.o \
		e2_proc/e2_proc_fp.o \
		st-pti.o \
		st-merger.o \
		st-common.o
##		stmdvp.o



obj-m += stmdvb.o

