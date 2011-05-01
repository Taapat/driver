#
# makefile
#
# Copyright (C) STMicroelectronics Ltd. 2002
#
# Makefile for embx common include distribution
#

RPC_ROOT = ../../..

include $(RPC_ROOT)/src/mkfiles/host.mak
include $(RPC_ROOT)/src/mkfiles/platform.mak

all:

clean:

EMBX_PUBLIC_HDRS = embx.h embx_types.h

DIST_SRC = $(EMBX_PUBLIC_HDRS) \
	   embxP.h \
	   embx_debug.h \
	   embx_osheaders.h \
	   embx_osinterface.h \
	   embx_typesP.h \
	   makefile

DIST_INCLUDE = $(EMBX_PUBLIC_HDRS)

include $(RPC_ROOT)/src/mkfiles/install.mak
