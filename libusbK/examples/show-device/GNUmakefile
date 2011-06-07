# MinGW64 tdm-gcc (multi-lib) PROJECT makefile for examples.
#
# !! IMPORTANT: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !! Requires multilib GCC
# !! Get it here: http://tdm-gcc.tdragon.net/
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
# 	  
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON 
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
# THE POSSIBILITY OF SUCH DAMAGE. 
#

# The examples-project.mkinc is included by all of the example
# application. When a file is "included", it is the same as if it were
# copy and pasted in.  This include file configures variable that are
# shared be each of the examples applications.
#
include ../examples-project.mkinc

$(info [K] Running show-device PROJECT makefile..)

# all -----------------------------------------------------------------
# Build all targets.
# NOTE: Only one target exists in this makefile.
#
.PHONY: all
all: show-device
# ---------------------------------------------------------------------

# multi-all -----------------------------------------------------------
# Build all targets for both x86 and amd64.
#
.PHONY: multi-all
multi-all: 
	$(MAKE) $(MAKEFLAGS) arch=x86 all
	$(MAKE) $(MAKEFLAGS) arch=amd64 all
# ---------------------------------------------------------------------

# ---------------------------------------------------------------------
# show-device
# Compile, assemble and link show-device.
#
.PHONY: show-device
show-device: compile-and-assemble-examples
show-device: show-device.exe
show-device.exe: $(INT_DIR)/show-device_rc.o $(INT_DIR)/show-device.o $(INT_DIR)/examples.o
	$(CC) $(CFLAGS) -o $(OUT_DIR)/$@ $^ $(LIB_SEARCH) $(LDFLAGS)

.PHONY: compile-and-assemble-examples
compile-and-assemble-examples:
	$(MKDIR) $(OUT_DIR)
	$(MKDIR) $(INT_DIR)
$(INT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
$(INT_DIR)/examples.o: ../examples.c
	$(CC) $(CFLAGS) -c $< -o $@
$(INT_DIR)/show-device_rc.o: ./show-device_rc.rc
	$(RC) $(RCFLAGS) $< -o $@
# ---------------------------------------------------------------------

# clean ---------------------------------------------------------------
# reamove all temporary/output files and directories
#
.PHONY: clean
clean: 
	$(RM) *.err *.o *.ncb *.user *.resharper *.suo
	$(RM) $(OUT_BASE_DIR) ./_ReSharper*
# ---------------------------------------------------------------------
