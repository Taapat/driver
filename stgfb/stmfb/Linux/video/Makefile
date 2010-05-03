ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

FBSOURCEFILES := $(addprefix $(SRC_TOPDIR)/Linux/video/, \
					stmfbops.c stmfbioctl.c stmfbsysfs.c \
					stmfbvar.c stmfboutconfig.c stmfb.c)

obj-m += stmfb.o

stmfb-objs := $(patsubst %.c,%.o,$(FBSOURCEFILES))
stmfb-stb7100-objs := $(stmfb-objs)
