#
# Makefile for module providing procfs functionality to enigma2
#

ifeq ($(CONFIG_PLAYER_131),y)
CCFLAGSY+= -DPLAYER_131
else ifeq ($(CONFIG_PLAYER_179),y)
CCFLAGSY+= -DPLAYER_179
else ifeq ($(CONFIG_PLAYER_191),y)
CCFLAGSY+= -DPLAYER_191
endif

ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m 		+= e2_proc.o
e2_proc-objs	+= e2_proc_main.o
