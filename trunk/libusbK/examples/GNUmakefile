# MinGW64 tdm-gcc (multi-lib) SOLUTION makefile for examples.
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

# Commands uses in this makefile:
# (- prefix means ignore errors)
#
MAKE	= make
RM		= -rm -fr

SLN_OUTPUT_DIR=$(CURDIR)/bin

$(info [K] Running examples SOLUTION makefile..)

# make all examples for the specified "arch" (default is x86)
#
.PHONY: all
all: show-device
all: open-device

# make the show-device example for the specified "arch" (default is x86)
#
.PHONY: show-device
show-device:
	$(MAKE) --directory=./show-device $(subst w -- ,,$(MAKEFLAGS)) SLN_OUTPUT_DIR=$(SLN_OUTPUT_DIR) all
# make the open-device example for the specified "arch" (default is x86)
#
.PHONY: open-device
open-device:
	$(MAKE) --directory=./open-device $(subst w -- ,,$(MAKEFLAGS)) SLN_OUTPUT_DIR=$(SLN_OUTPUT_DIR) all

# make all examples for x86 and amd64 archs. ("arch" argument not used)
#
.PHONY: multi-all
multi-all: 
	$(MAKE) $(MAKEFLAGS) arch=x86 all
	$(MAKE) $(MAKEFLAGS) arch=amd64 all

# reamove all temporary/output files and directories
#
.PHONY: clean
clean:
	$(MAKE) --directory=./show-device $(MAKEFLAGS) clean
	$(MAKE) --directory=./open-device $(MAKEFLAGS) clean
	$(RM) $(SLN_OUTPUT_DIR) ./_ReSharper*
