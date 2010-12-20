INC		= -I $(STG_TOPDIR) -I /opt/STM/STLinux-2.3/devkit/sh4/target/usr/include/directfb
CFLAGS		= -Wall $(INC)
LDFLAGS		= -ldirectfb
CROSS_COMPILE	:= sh4-linux-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)gcc
TARGET		= v4l2lut8
TARGET2		= v4l2lut8_2
SRCS		= $(addprefix ./,$(TARGET).c)
SRCS2		= $(addprefix ./,$(TARGET2).c)
OBJS		= $(patsubst %.c,%.o,$(SRCS))
OBJS2		= $(patsubst %.c,%.o,$(SRCS2))

INCLUDES = $(STG_TOPDIR)/Linux/video/stmfb.h \
           $(STG_TOPDIR)/Linux/media/video/stmvout.h \
           $(STG_TOPDIR)/Linux/tests/v4l2_helper.h

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJS)
$(TARGET2): $(OBJS2)

dep:

clean :
	find . -name "*.o" -exec rm \{\} \;
	find . -name $(TARGET) -exec rm \{\} \;

$(OBJS): $(INCLUDES)
$(OBJS2): $(INCLUDES)

