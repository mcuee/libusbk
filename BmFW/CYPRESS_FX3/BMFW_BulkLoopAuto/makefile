## Copyright Cypress Semiconductor Corporation, 2010-2018,
## All Rights Reserved
## UNPUBLISHED, LICENSED SOFTWARE.
##
## CONFIDENTIAL AND PROPRIETARY INFORMATION
## WHICH IS THE PROPERTY OF CYPRESS.
##
## Use of this file is governed
## by the license agreement included in the file
##
##      <install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
##

FX3FWROOT=../../..
MODULE = cyfxbulklpauto

include $(FX3FWROOT)/fw_build/fx3_fw/fx3_build_config.mak

SOURCE = $(MODULE).cpp 		\
	 cyfxbulklpdscr.cpp	\
	 cyfxtx.cpp		\
	 cyfxcppsyscall.cpp

ifeq ($(CYFXBUILD),arm)
SOURCE_ASM=cyfx_startup.S
else
SOURCE_ASM=cyfx_gcc_startup.S
endif

C_OBJECT=$(SOURCE:%.cpp=./%.o)
A_OBJECT=$(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

all:compile

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK)

$(C_OBJECT) : %.o : %.cpp cyfxbulklpauto.h
	$(COMPILE)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

cyfx_startup.S:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfx_startup.S .

cyfx_gcc_startup.S:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfx_gcc_startup.S .

cyfxtx.cpp:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfxtx.cpp .

cyfxcppsyscall.cpp:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfxcppsyscall.cpp .

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o ./*.img
	rm -f cyfxtx.cpp cyfxcppsyscall.cpp cyfx_gcc_startup.S cyfx_startup.S

compile: $(C_OBJECT) $(A_OBJECT) $(EXES)

#[]#
