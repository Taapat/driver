#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the embx shm transport
#

SHLINUX_KERNEL=1        # build as a kernel module for SHLinux

COMMON_INCLUDES=../include

ifndef EMBXSHM_LIB
EMBXSHM_LIB=embxshm
endif

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY = $(EMBXSHM_LIB)

SRCS  = \
        embxshm_lowlevel.c \
        embxshm_locks.c \
        embxshm_heap.c \
        embxshm_localport.c \
        embxshm_remoteport.c \
	embxshm_connection.c \
        embxshm_transport.c \
	embxshm_mailboxFactory.c \
        embxshm_genericFactory.c \
	embxshm_init.c \
	embxshm_death.c

SRCS_LINUX = embxshm_module.c

COMMON_HDRS   = $(COMMON_INCLUDES)/embx_types.h \
                $(COMMON_INCLUDES)/embx_typesP.h \
                $(COMMON_INCLUDES)/embx.h \
                $(COMMON_INCLUDES)/embxP.h \
                $(COMMON_INCLUDES)/embx_debug.h \
                $(COMMON_INCLUDES)/embx_osheaders.h \
                $(COMMON_INCLUDES)/embx_osinterface.h \

LOCAL_HDRS    = embxshm.h \
                embxshmP.h \
		debug_ctrl.h \
		embxshm_cache.h

HDRS = $(LOCAL_HDRS) $(COMMON_HDRS)

LOCAL_CFLAGS  = -I$(COMMON_INCLUDES) $(EMBXSHM_CFLAGS)
LOCAL_CFLAGS_SPARC_SOLARIS = -D__EXTENSIONS__

#
# derived macros
#

OBJS_ST20_OS20     = $(foreach f, $(SRCS:.c=.$(OBJ_ST20_OS20)), $(OBJDIR_ST20_OS20)/$f)
OBJS_ST40_LINUX_KO = $(foreach f, $(patsubst %.c,%.$(OBJ_ST40_LINUX_KO),$(SRCS) $(SRCS_LINUX)), \
			       $(OBJDIR_ST40_LINUX_KO)/$f)
OBJS_ST40_OS21     = $(foreach f, $(SRCS:.c=.$(OBJ_ST40_OS21)), $(OBJDIR_ST40_OS21)/$f)
OBJS_ST40_WINCE    = $(foreach f, $(SRCS:.c=.$(OBJ_ST40_WINCE)), $(OBJDIR_ST40_WINCE)/$f)
OBJS_ST220_OS21    = $(foreach f, $(SRCS:.c=.$(OBJ_ST220_OS21)), $(OBJDIR_ST220_OS21)/$f)
OBJS_ST231_OS21    = $(foreach f, $(SRCS:.c=.$(OBJ_ST231_OS21)), $(OBJDIR_ST231_OS21)/$f)

#
# No longer build for solaris - there is no OS interface anyway.
#
#OBJS_SPARC_SOLARIS = $(foreach f, $(SRCS:.c=.$(OBJ_SPARC_SOLARIS)), $(OBJDIR_SPARC_SOLARIS)/$f)

$(OBJS_ST20_OS20) $(OBJS_ST40_LINUX_KO) $(OBJS_ST40_OS21) $(OBJS_ST40_WINCE) \
$(OBJS_ST220_OS21) $(OBJS_ST231_OS21) $(OBJS_SPARC_SOLARIS) : $(HDRS) makefile

#
# explicit rules
#

include $(RPC_ROOT)/src/mkfiles/build.mak

all: $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

# The following check prevents the copying of sources into the embxshmc directory
ifndef BUILDING_SHMC
#
# manage distributions
#
DIST_SRC      = $(SRCS) $(SRCS_LINUX) $(LOCAL_HDRS) makefile
DIST_INCLUDE  = embxshm.h
endif

include $(RPC_ROOT)/src/mkfiles/install.mak

