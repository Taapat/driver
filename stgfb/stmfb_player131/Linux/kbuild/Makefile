ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

EXTRA_CFLAGS := -I$(STG_TOPDIR) -I$(STG_TOPDIR)/include -I$(STG_TOPDIR)/Linux/include
#EXTRA_CFLAGS += -O0

ifeq ($(CONFIG_STG_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

BUILD_SYSTEM_INFO = $(shell /bin/uname -a)
BUILD_USER = $(shell /usr/bin/whoami)
BUILD_DATE = $(shell /bin/date)
STMFB_ORIGINAL_SOURCE_PATH ?= <unknown - are you using a foreign build system?>

EXTRA_CFLAGS += -DKBUILD_SYSTEM_INFO="KBUILD_STR($(BUILD_SYSTEM_INFO))"     \
                -DKBUILD_USER="KBUILD_STR($(BUILD_USER))"                   \
                -DKBUILD_SOURCE="KBUILD_STR($(STMFB_ORIGINAL_SOURCE_PATH))" \
                -DKBUILD_DATE="KBUILD_STR($(BUILD_DATE))"

# We need the source paths to be relative for the 2.6 build
# system as they get appended to the full directory path of
# this makefile.
SRC_TOPDIR := ../..

include $(STG_TOPDIR)/Linux/stm/coredisplay/Makefile
include $(STG_TOPDIR)/Linux/media/video/Makefile
include $(STG_TOPDIR)/Linux/video/Makefile
