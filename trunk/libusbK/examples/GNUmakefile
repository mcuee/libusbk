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

$(info [K] Running examples SOLUTION makefile..)

# make all examples for the specified "arch" (default is x86)
#
.PHONY: all
all: show-device
all: open-device
all: xfer-async-rw
all: xfer-sync-rw
all: xfer-control
all: xfer-stream-r
all: xfer-stream-w
all: xfer-stream-rw
all: load-driver-api
all: config-interface
all: config-pipe
all: config-power
all: hot-plug-monitor
all: xfer-iso
all: xfer-queued-iso

# make the show-device example for the specified "arch" (default is x86)
#
.PHONY: show-device
show-device:
	$(MAKE) --no-print-directory --directory=./show-device $(subst w -- ,,$(MAKEFLAGS)) all
# make the open-device example for the specified "arch" (default is x86)
#
.PHONY: open-device
open-device:
	$(MAKE) --no-print-directory --directory=./open-device $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-async-rw example for the specified "arch" (default is x86)
#
.PHONY: xfer-async-rw
xfer-async-rw:
	$(MAKE) --no-print-directory --directory=./xfer-async-rw $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-sync-rw example for the specified "arch" (default is x86)
#
.PHONY: xfer-sync-rw
xfer-sync-rw:
	$(MAKE) --no-print-directory --directory=./xfer-sync-rw $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-control example for the specified "arch" (default is x86)
#
.PHONY: xfer-control
xfer-control:
	$(MAKE) --no-print-directory --directory=./xfer-control $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-stream-r example for the specified "arch" (default is x86)
#
.PHONY: xfer-stream-r
xfer-stream-r:
	$(MAKE) --no-print-directory --directory=./xfer-stream-r $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-stream-w example for the specified "arch" (default is x86)
#
.PHONY: xfer-stream-w
xfer-stream-w:
	$(MAKE) --no-print-directory --directory=./xfer-stream-w $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-stream-rw example for the specified "arch" (default is x86)
#
.PHONY: xfer-stream-rw
xfer-stream-rw:
	$(MAKE) --no-print-directory --directory=./xfer-stream-rw $(subst w -- ,,$(MAKEFLAGS)) all
# make the load-driver-api example for the specified "arch" (default is x86)
#
.PHONY: load-driver-api
load-driver-api:
	$(MAKE) --no-print-directory --directory=./load-driver-api $(subst w -- ,,$(MAKEFLAGS)) all
# make the config-interface example for the specified "arch" (default is x86)
#
.PHONY: config-interface
config-interface:
	$(MAKE) --no-print-directory --directory=./config-interface $(subst w -- ,,$(MAKEFLAGS)) all
# make the config-pipe example for the specified "arch" (default is x86)
#
.PHONY: config-pipe
config-pipe:
	$(MAKE) --no-print-directory --directory=./config-pipe $(subst w -- ,,$(MAKEFLAGS)) all
# make the config-power example for the specified "arch" (default is x86)
#
.PHONY: config-power
config-power:
	$(MAKE) --no-print-directory --directory=./config-power $(subst w -- ,,$(MAKEFLAGS)) all
# make the hot-plug-monitor example for the specified "arch" (default is x86)
#
.PHONY: hot-plug-monitor
hot-plug-monitor:
	$(MAKE) --no-print-directory --directory=./hot-plug-monitor $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-iso example for the specified "arch" (default is x86)
#
.PHONY: xfer-iso
xfer-iso:
	$(MAKE) --no-print-directory --directory=./xfer-iso $(subst w -- ,,$(MAKEFLAGS)) all
# make the xfer-queued-iso example for the specified "arch" (default is x86)
#
.PHONY: xfer-queued-iso
xfer-queued-iso:
	$(MAKE) --no-print-directory --directory=./xfer-queued-iso $(subst w -- ,,$(MAKEFLAGS)) all

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
	$(MAKE) --directory=./xfer-async-rw $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-sync-rw $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-control $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-stream-r $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-stream-w $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-stream-rw $(MAKEFLAGS) clean
	$(MAKE) --directory=./load-driver-api $(MAKEFLAGS) clean
	$(MAKE) --directory=./config-interface $(MAKEFLAGS) clean
	$(MAKE) --directory=./config-pipe $(MAKEFLAGS) clean
	$(MAKE) --directory=./config-power $(MAKEFLAGS) clean
	$(MAKE) --directory=./hot-plug-monitor $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-iso $(MAKEFLAGS) clean
	$(MAKE) --directory=./xfer-queued-iso $(MAKEFLAGS) clean
	$(RM) ./_ReSharper*
