############################################################
#
# File    : makefile
# Synopsis: Top level makefile to build the server
#
# Copyright 2001 STMicroelectronics, all right reserved.
#
############################################################

include $(SRC_DIR)/modrules.mak

CDEFS                   = -I$(LINUX_KERNEL_ROOT)/drivers/media/dvb/dvb-core -I$(CONFIG_PLAYER2_LOCATION)/linux/drivers/media/video/stm/ -DDEBUG_DVB
MODULE                  = linuxdvb
EXPORTED_HEADERS        = dvb_stm.h
HEADERS                 = $(EXPORTED_HEADERS) backend.h
OBJECTS                 = dvb_player.o dvb_audio.o dvb_video.o dvb_dvr.o dvb_dvp.o dvb_demux.o backend.o \
DEPENDS                 =
LIBRARIES               = -L$(LIB_DIR)

#

all:    $(LIBRARY_OR_MODULE) headers

include $(SRC_DIR)/make$(LIBRARY_OR_MODULE)

