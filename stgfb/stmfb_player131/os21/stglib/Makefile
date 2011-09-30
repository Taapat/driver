STG_TOPDIR = ../..


# We must make sure that devfb.cpp is the first file on the link path and
# devfbfini.cpp is the last, or automatic constructors will not work correctly
DEVFBSOURCEFILES := stgos21.c application_helpers.cpp

SRC_TOPDIR := $(STG_TOPDIR)

GENINITSRCS := $(addprefix $(SRC_TOPDIR)/Generic/,DisplayDevice.cpp            \
				DisplayPlane.cpp Output.cpp MetaDataQueue.cpp)

#Generic Gamma
STGGAMMASRC := $(addprefix $(SRC_TOPDIR)/Gamma/, GenericGammaDevice.cpp             \
				GenericGammaOutput.cpp GammaMixer.cpp               \
				GammaCompositorPlane.cpp GammaCompositorCursor.cpp  \
				GammaCompositorGDP.cpp                              \
				GammaCompositorVideoPlane.cpp GammaVideoPlug.cpp)


STMCOMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmawg.cpp stmdenc.cpp      \
			stmfsynth.cpp stmvtg.cpp stmblitter.cpp                \
			stmbdisp.cpp stmbdispaq.cpp stmbdispoutput.cpp         \
			stmvirtplane.cpp stmteletext.cpp)

HDTVOUTCOMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmsdvtg.cpp            \
			stmhdfhdmi.cpp stmhdfawg.cpp stmfdvo.cpp               \
			stmtvoutteletext.cpp)

HDMI_COMMON := $(addprefix $(SRC_TOPDIR)/STMCommon/,                       \
			stmhdmi.cpp stmiframemanager.cpp)

HDMI_FDMA_IFRAME := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmdmaiframes.cpp)

HDMI_V2_9 := $(addprefix $(SRC_TOPDIR)/STMCommon/, stmv29iframes.cpp)

ALLFBSRCS := $(DEVFBSOURCEFILES) $(GENINITSRCS) $(STMCOMMON) $(STGGAMMASRC) $(HDMI_COMMON)

ifeq ($(CONFIG_STB7100),y)

#STb710x
ALLFBSRCS += $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, GammaDisplayFilter.cpp VDPFilter.cpp \
				GammaCompositorDISP.cpp GammaBlitter.cpp GDPBDispOutput.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100device.cpp stb7100analogueouts.cpp \
				stb7100dvo.cpp stb7100hdmi.cpp stb7100denc.cpp                   \
				stb7100vtg.cpp stb7100mixer.cpp stb710xgdp.cpp                   \
				stb710xcursor.cpp stb7109dei.cpp stb7100AWGAnalog.cpp)

else
ifeq ($(CONFIG_STI7200C1),y)
ALLFBSRCS += $(HDTVOUTCOMMON) $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

# We reuse some bits from 710x for 7200 as well
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200device.cpp               \
				sti7200cut1devicefactory.cpp                           \
				sti7200cut1remotedevice.cpp sti7200cut1localdevice.cpp \
				sti7200cut1bdisp.cpp                                   \
				sti7200remoteoutput.cpp sti7200hdmi.cpp                \
				sti7200cut1localauxoutput.cpp                          \
				sti7200localmainoutput.cpp                             \
				sti7200cut1mainvideopipe.cpp                           \
				sti7200denc.cpp sti7200gdp.cpp sti7200mixer.cpp        \
				sti7200cursor.cpp sti7200hdfdvo.cpp                    \
				sti7200flexvp.cpp sti7200xvp.cpp                       \
				sti7200iqi.cpp)

else
ifeq ($(CONFIG_STI7200C2),y)
ALLFBSRCS += $(HDTVOUTCOMMON) $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111hdfoutput.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200device.cpp               \
				sti7200cut2devicefactory.cpp                           \
				sti7200cut2remotedevice.cpp sti7200cut2localdevice.cpp \
				sti7200cut2bdisp.cpp                                   \
				sti7200remoteoutput.cpp sti7200hdmi.cpp                \
				sti7200cut2localauxoutput.cpp                          \
				sti7200cut2localmainoutput.cpp                         \
				sti7200cut2mainvideopipe.cpp                           \
				sti7200denc.cpp sti7200gdp.cpp sti7200mixer.cpp        \
				sti7200cursor.cpp sti7200hdfdvo.cpp                    \
				sti7200flexvp.cpp sti7200xvp.cpp                       \
				sti7200iqi.cpp)

else
ifeq ($(CONFIG_STI7111)$(CONFIG_STI7141),y)
ALLFBSRCS += $(HDTVOUTCOMMON) $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

# We reuse some bits from 710x and 7200 as well
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200cursor.cpp \
			sti7200cut2mainvideopipe.cpp sti7200iqi.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111gdp.cpp          \
			sti7111bdisp.cpp sti7111mixer.cpp sti7111dvo.cpp       \
			sti7111hdfoutput.cpp sti7111mainoutput.cpp             \
			sti7111auxoutput.cpp sti7111hdmi.cpp                   \
			sti7111device.cpp sti7111devicecreate.cpp)

else
ifeq ($(CONFIG_STI7105),y)
ALLFBSRCS += $(HDTVOUTCOMMON) $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200cursor.cpp \
			sti7200cut2mainvideopipe.cpp sti7200iqi.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111gdp.cpp          \
			sti7111bdisp.cpp sti7111mixer.cpp sti7111dvo.cpp       \
			sti7111hdfoutput.cpp sti7111mainoutput.cpp             \
			sti7111auxoutput.cpp sti7111hdmi.cpp sti7111device.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7105/, sti7105device.cpp       \
			sti7105devicecreate.cpp)

else
ifeq ($(CONFIG_STI7106),y)
ALLFBSRCS += $(HDTVOUTCOMMON) $(HDMI_V2_9)
# Because we subclass the 7111 hdmi implementation we have to include
# the DMA IFrame manager even though we are not going to use it.
ALLFBSRCS += $(HDMI_FDMA_IFRAME)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/, VDPFilter.cpp GDPBDispOutput.cpp MainVideoPipe.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/stb7100/, stb7100denc.cpp stb7109dei.cpp)
ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7200/, sti7200cursor.cpp \
			sti7200cut2mainvideopipe.cpp sti7200iqi.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7111/, sti7111gdp.cpp          \
			sti7111bdisp.cpp sti7111mixer.cpp sti7111dvo.cpp       \
			sti7111hdfoutput.cpp sti7111mainoutput.cpp             \
			sti7111auxoutput.cpp sti7111hdmi.cpp sti7111device.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7105/, sti7105device.cpp)

ALLFBSRCS += $(addprefix $(SRC_TOPDIR)/Gamma/sti7106/, sti7106hdmi.cpp sti7106device.cpp)
else
$(error Unknown chip configuration)
endif
endif
endif
endif
endif
endif

ALLFBOBJS := $(patsubst %.cpp,%.o,$(ALLFBSRCS))
ALLFBOBJS := $(patsubst %.c,%.o,$(ALLFBOBJS))

all: libstgos21.a

libstgos21.a: $(ALLFBOBJS)
	$(AR) $(ARFLAGS) $@ $(ALLFBOBJS)

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.o:%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

