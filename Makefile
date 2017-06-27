# Makefile for dll

LIBNAME = myipset
TARGET = lib$(LIBNAME).so.1.0.0
LINKTARGET = lib$(LIBNAME).so


# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=ia64-linux-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables

CROSS_COMPILE   ?=
PREFIX          ?=
INC_PREFIX      ?= $(PREFIX)
LIB_PREFIX      ?= $(PREFIX)
BIN_PREFIX		?= $(PREFIX)
ABIFLAGS        ?=

# Make variables (CC, etc...)
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)gcc
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CROSS_COMPILE)g++
CXX             = $(CROSS_COMPILE)g++
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
RANLIB          = $(CROSS_COMPILE)ranlib

CFLAGS =  -I./include -fPIC -g
LDFLAGS = -shared -Wl,-soname,$(LINKTARGET)

CFLAGS += -Wall

SOURCES=$(wildcard *.c *.cpp)
OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))


vpath %.o ./
.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@cp -f $(TARGET) $(LINKTARGET)
.c.o:
	@echo -------------------------------------
	@echo compiling $<
	$(CC) $(OPTIM) $(CFLAGS) -c $< -o $@

.cpp.o:
	@echo -------------------------------------
	@echo compiling $<
	$(CXX) $(OPTIM) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS) $(TARGET) $(LINKTARGET)


PREFIX = 
LIBDIR = /usr/lib/
INCDIR = /usr/include/$(LIBNAME)

install:
	@make uninstall
	@install -m 755 -o 0 -g 0 -d $(PREFIX)$(LIBDIR)
	@install -m 755 -o 0 -g 0 -d $(PREFIX)$(INCDIR)
	
	@install -m 644 -o 0 -g 0 ./include/*.* $(PREFIX)$(INCDIR)/
	@install  --strip-program=$(STRIP) -s -m 755 -o 0 -g 0 ./$(TARGET) $(PREFIX)$(LIBDIR)/
	@ln -sf ./$(TARGET) $(PREFIX)$(LIBDIR)/$(LINKTARGET)
	@echo "$(TARGET) install over!"
	
uninstall:
	@rm -f $(PREFIX)$(LIBDIR)/$(LINKTARGET)  
	@rm -f $(PREFIX)$(LIBDIR)/$(TARGET) 
	@rm -rf $(PREFIX)$(INCDIR)
	
	@echo "$(TARGET) uninstall over!"
	
