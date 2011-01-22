EXTRA_CFLAGS := -I$(STG_TOPDIR) -I$(STG_TOPDIR)/include -I$(STG_TOPDIR)/linux/include
#EXTRA_CFLAGS += -O0

ifeq ($(CONFIG_STG_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_ST_BIOS),y)
EXTRA_CFLAGS += -DHAVE_BIOS
endif

BUILD_SYSTEM_INFO = $(shell /bin/uname -a)
BUILD_USER = $(shell /usr/bin/whoami)
BUILD_DATE = $(shell /bin/date)
STMFB_ORIGINAL_SOURCE_PATH ?= <unknown - are you using a foreign build system?>

# We need the source paths to be relative for the 2.6 build
# system as they get appended to the full directory path of
# this makefile.
SRC_TOPDIR := ../..

include $(STG_TOPDIR)/linux/drivers/stm/coredisplay/Makefile
include $(STG_TOPDIR)/linux/drivers/media/video/Makefile
include $(STG_TOPDIR)/linux/drivers/video/Makefile
