# MinGW64 tdm-gcc (multi-lib) makefile for libusbK example modules.
#
# libusbK, Generic Windows USB Library
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# 
# !! IMPORTANT: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !! Requires multilib GCC
# !! Get it here: http://tdm-gcc.tdragon.net/
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

ARCH=$(arch)

# Compiling and linking applications
CC = gcc
MAKE = make

# File management applications
RM = -rm -fr
RMDIR = -rmdir --ignore-fail-on-non-empty
MKDIR = -mkdir -p

ifeq ("$(ARCH)","amd64")
ARCH=amd64
ARCH_MSVC=x64
MACHINE=-m64
endif

ifneq ("$(ARCH)","amd64")
ARCH=x86
ARCH_MSVC=Win32
MACHINE=-m32
endif

# Output base bin directory name
BIN_DIRNAME=bin

# Application output directory
OUT_DIR=./$(BIN_DIRNAME)/$(ARCH)

# Application object output directory (intermediates)
INT_DIR=./$(BIN_DIRNAME)/$(ARCH)/obj

# Include search paths
INC_SEARCH=-I./ -I../includes -I../src/includes/

# Standard libraries to link with all targets
STDC_LD_LIBS=-lkernel32

# libusbK libraries to link
USBK_LIB_NAME=libusbK

# base Compiler and linker flags used for all targets
CFLAGS=$(MACHINE) $(INC_SEARCH) -mconsole -mwin32
LDFLAGS=-s -Wl,--kill-at,--enable-stdcall-fixup $(STDC_LD_LIBS) -l$(USBK_LIB_NAME)

# Library search paths
l_dirs=
l_base_list=./ $(OUT_DIR)/ ../$(BIN_DIRNAME)/dll/$(ARCH)/ ./$(BIN_DIRNAME)/dll/$(ARCH)/ ../$(BIN_DIRNAME)/Release/libusbK.dll/$(ARCH_MSVC)/ ../$(BIN_DIRNAME)/Debug/libusbK.dll/$(ARCH_MSVC)/
l_find=$(dir $(wildcard $(v_base)$(USBK_LIB_NAME).dll)) $(dir $(wildcard $(v_base)$(USBK_LIB_NAME).a))
l_dirs:=$(foreach v_base,$(l_base_list),$(l_find))
LIB_SEARCH=$(firstword $(l_dirs))

# LIB_SEARCH=$(firstword $(dir $(wildcard ../$(BIN_DIRNAME)/dll/$(ARCH)/*.dll)) $(dir $(wildcard ../../../dll/$(ARCH)/*.dll)) $(dir $(wildcard ../../../fre/dll/$(ARCH)/*.dll)))

.PHONY: all
all: example-device-list

.PHONY: multi-all
multi-all: 
	$(MAKE) -f examples.mak arch=x86 all
	$(MAKE) -f examples.mak arch=amd64 all

.PHONY: example-device-list
example-device-list: compile-and-assemble-examples
example-device-list: example-device-list.exe

example-device-list.exe: $(INT_DIR)/example-device-list.o
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$@ $^ -L$(LIB_SEARCH) $(LDFLAGS)

.PHONY: compile-and-assemble-examples
compile-and-assemble-examples:
	$(MKDIR) $(OUT_DIR)
	$(MKDIR) $(INT_DIR)
$(INT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean: 
	@echo l_dirs=$(l_dirs)
	$(RM) *.err *.o *.ncb *.user *.resharper *.suo
	$(RM) ./Debug ./Release ./output ./chk ./fre ./_ReSharper*
