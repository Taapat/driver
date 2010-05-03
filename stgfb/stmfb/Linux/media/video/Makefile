ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

stm_v4l2-objs := $(SRC_TOPDIR)/Linux/media/video/stm_v4l2.o

stmvout-objs := $(addprefix $(SRC_TOPDIR)/Linux/media/video/,stmvout_buffers.o \
							stmvout_display.o     \
							stmvout_ctrl.o        \
							stmvout_driver.o)
stmvout-objs := $(stmvout-objs)

stmvbi-objs := $(SRC_TOPDIR)/Linux/media/video/stmvbi_driver.o

obj-m += stm_v4l2.o
obj-m += stmvout.o
obj-m += stmvbi.o
