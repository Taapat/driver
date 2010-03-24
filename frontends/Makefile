TREE_ROOT?=$(DRIVER_TOPDIR)/
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CFLAGS += -I$(TREE_ROOT)/linux/include/linux/stm/ 
CFLAGS += -I$(TREE_ROOT)/linux/drivers/media/video/stm/ 
CFLAGS += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core 
CFLAGS += -I$(CONFIG_STGFB_PATH)/include 
CFLAGS += -DDEBUG_DVB
CFLAGS += -I$(TREE_ROOT)/include 

ifdef UFS922
cx21143-objs +=	cx21143/cx21143.o
obj-m += cx21143.o
else ifdef FORTIS_HDBOX
stv090x-objs +=	stv090x/stv090x.o
stv090x-objs +=	stv090x/stv6110x.o
stv090x-objs +=	stv090x/core.o
obj-m += stv090x.o
else ifdef HL101
fe-core-objs +=        stb0899/stb0899_algo.o
fe-core-objs +=        stb0899/stb0899_drv.o
fe-core-objs +=        stb0899/stb6100.o
fe-core-objs +=        stb0899/core.o
obj-m += fe-core.o
else ifdef VIP2
fe-core-objs +=        stv090x_api5/stv090x.o
fe-core-objs +=        stv090x_api5/stv6110x.o
fe-core-objs +=        stv090x_api5/core.o
obj-m += fe-core.o
else
cx24116-objs +=	cx24116/cx24116.o
obj-m += cx24116.o
endif


