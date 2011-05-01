 # makefile
#
# Copyright (C) STMicroelectronics Ltd. 2004
#
# Makefile for mme common include distribution
#

RPC_ROOT = ../../..

include $(RPC_ROOT)/src/mkfiles/host.mak

all:

clean:

MME_PUBLIC_HDRS = mme.h

DIST_SRC =        $(MME_PUBLIC_HDRS) \
                  mmeP.h \
                  mme_debugP.h \
                  mme_messagesP.h \
                  mme_queueP.h \
                  makefile

DIST_INCLUDE = $(MME_PUBLIC_HDRS)

include $(RPC_ROOT)/src/mkfiles/install.mak
