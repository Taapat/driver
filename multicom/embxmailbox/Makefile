#
# makefile
#
# Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
#
# Makefile for the embx shm transport
#

COMMON_INCLUDES=../include

RPC_ROOT = ../../..
include $(RPC_ROOT)/src/mkfiles/host.mak

#
# configurable macros
#

LIBRARY       = embxmailbox

SRCS          = embx_mailbox.c
DIST_HDRS     = embxmailbox.h
LOCAL_HDRS    = embxmailboxP.h $(DIST_HDRS)

COMMON_HDRS   = $(COMMON_INCLUDES)/embx_types.h \
                $(COMMON_INCLUDES)/embx_typesP.h \
                $(COMMON_INCLUDES)/embx.h \
                $(COMMON_INCLUDES)/embxP.h \
                $(COMMON_INCLUDES)/embx_debug.h \
                $(COMMON_INCLUDES)/embx_osheaders.h \
                $(COMMON_INCLUDES)/embx_osinterface.h

HDRS = $(LOCAL_HDRS) $(COMMON_HDRS)

LOCAL_CFLAGS  = -I$(COMMON_INCLUDES)

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

$(OBJS_ST20_OS20) $(OBJS_ST40_LINUX_KO) \
$(OBJS_ST40_OS21) $(OBJS_ST40_WINCE) $(OBJS_ST220_OS21) $(OBJS_ST231_OS21) : $(HDRS)

#
# explicit rules
#

include $(RPC_ROOT)/src/mkfiles/build.mak

all: $(MAKE_DIRS) $(MAKE_TARGETS)

clean:
	-$(RM) $(call DOSCMD,$(CLEAN_DIRS))

#
# manage distributions
#

DIST_SRC      = $(SRCS) $(LOCAL_HDRS) makefile token_protocol.txt
DIST_INCLUDE  = $(DIST_HDRS)

include $(RPC_ROOT)/src/mkfiles/install.mak

