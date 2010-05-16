ifeq ($(KERNELRELEASE),)
DRIVER_TOPDIR:=$(shell pwd)/../..
include $(DRIVER_TOPDIR)/kernel.make
else
obj-m           += lfs.o
lfs-objs	:= util.o segment.o inode.o dir.o file.o super.o \
    		   tree.o symlink.o ifile.o seguse.o segsum.o ioctl.o

endif