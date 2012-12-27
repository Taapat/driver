TREE_ROOT?=$(DRIVER_TOPDIR)/
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -DDEBUG_DVB
CCFLAGSY += -I$(TREE_ROOT)/include

ifdef SPARK
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_dvbapi5
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_common
endif

ifdef SPARK7162
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367/stv0367qam
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367/stv0367ter
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/6158
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/tuner

CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_dvbapi5
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_common

CCFLAGSY += -Wall #-Werror
endif

ccflags-y += $(CCFLAGSY)

# >= 2.6.30
ifdef HL101
fe-core-objs +=        hl101/stb0899_algo.o
fe-core-objs +=        hl101/stb0899_drv.o
fe-core-objs +=        hl101/stb6100.o
fe-core-objs +=        hl101/stv090x.o
fe-core-objs +=        hl101/stv6110x.o
fe-core-objs +=        hl101/ix7306.o
fe-core-objs +=        hl101/core.o
obj-m += fe-core.o
else ifdef SPARK
stv090x-objs +=	spark_dvbapi5/stv090x.o
stv090x-objs +=	spark_dvbapi5/stv6110x.o
stv090x-objs +=	spark_dvbapi5/ix7306.o
stv090x-objs +=	spark_dvbapi5/core.o
stv090x-objs += spark_common/vz7903.o
obj-m += stv090x.o
obj-y += lnb/
else ifdef SPARK7162
spark7162-objs +=        spark7162/spark7162.o

spark7162-objs +=        spark_common/ioarch.o

spark7162-objs +=        spark7162/D3501.o

spark7162-objs +=        spark_dvbapi5/ix7306.o
spark7162-objs +=        spark_common/vz7903.o
spark7162-objs +=        base/sharp7803.o
spark7162-objs +=        base/sharp6465.o
spark7162-objs +=        base/sharp5469c.o

spark7162-objs +=        spark7162/stv0367/D0367.o
spark7162-objs +=        spark7162/stv0367/D0367ter.o
spark7162-objs +=        spark7162/stv0367/D0367cab.o

spark7162-objs +=        spark7162/stv0367/stv0367ter/chip_0367ter.o
spark7162-objs +=        spark7162/stv0367/stv0367ter/stv0367ofdm_drv.o
spark7162-objs +=        spark7162/stv0367/stv0367ter/d0367_ter.o

spark7162-objs +=        spark7162/stv0367/stv0367qam/chip_0367qam.o
spark7162-objs +=        spark7162/stv0367/stv0367qam/stv0367qam_drv.o
spark7162-objs +=        spark7162/stv0367/stv0367qam/d0367_qam.o

spark7162-objs +=        spark7162/6158/d6158.o
spark7162-objs +=        spark7162/6158/MN88472_autoctrl.o
spark7162-objs +=        spark7162/6158/MN88472_register.o
spark7162-objs +=        spark7162/6158/MN88472_variable.o

spark7162-objs +=        spark7162/tuner/mxl301rf.o
spark7162-objs +=        spark7162/tuner/mxl_common.o
spark7162-objs +=        spark7162/tuner/mxl_spur_table.o
spark7162-objs +=        spark7162/tuner/mxL_user_define.o

obj-m += spark7162.o
obj-y += lnb/

# other drivers already in kernel

endif
