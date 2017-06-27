
BINFILE=test

CROSS_COMPILE   ?=
PREFIX          ?=
INC_PREFIX      ?= $(PREFIX)
LIB_PREFIX      ?= $(PREFIX)
BIN_PREFIX	?= $(PREFIX)
ABIFLAGS        ?=

# Make variables (CC, etc...)
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CROSS_COMPILE)gcc
CXX             = $(CROSS_COMPILE)gcc
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
RANLIB          = $(CROSS_COMPILE)ranlib

CFLAGS = -g -I../include -Wall 
LDFLAGS = -lmyipset -lm
SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCES)))

vpath %.o ./


.PHONY:	all clean

all:$(BINFILE)

$(BINFILE):$(OBJS)
	$(CPP) $(LDFLAGS) -o $@ $^
ifeq ($(DEBUG), 0)
	strip $(BINFILE)
endif
	
.c.o:
	@echo -------------------------------------
	@echo compiling $<
	$(CC) $(OPTIM) $(CFLAGS) -c $< -o ./$@

.cpp.o:
	@echo -------------------------------------
	@echo compiling $<
	$(CPP) $(OPTIM) $(CFLAGS) -c $< -o ./$@

clean:
	rm -f $(OBJS) $(BINFILE)
