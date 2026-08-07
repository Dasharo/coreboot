# SPDX-License-Identifier: BSD-2-Clause

project_name=STM
project_dir=../../../../3rdparty/stm/
build_dir=$(project_dir)/Stm/build
project_git_branch=$(CONFIG_STM_GIT_BRANCH)

ifeq ($(CONFIG_STM_CONSOLE_DEBUG),y)
STM_BUILD="debug"
endif

ifeq ($(CONFIG_STM_CONSOLE_RELEASE),y)
STM_BUILD="release"
endif

# STM needs string.h, provide it from coreboot includes
STM_COREBOOT_INC=-isystem $(top)/src/include
STM_COREBOOT_INC+= -isystem $(top)/src/commonlib/include
STM_COREBOOT_INC+= -isystem $(top)/src/commonlib/bsd/include
# STM needs elf.h, provide it from cbfstool
STM_COREBOOT_INC+= -isystem $(top)/util/cbfstool

all: build

build:
	echo "STM - Build"
	cd $(project_dir)/Stm; \
	mkdir -p build; \
	cd build; \
	cmake .. -DBIOS=coreboot \
		-DCMAKE_SYSTEM_NAME=Generic \
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
		-DCMAKE_C_COMPILER=$(shell which $(GCC_CC_x86_64)) \
		-DCMAKE_ASM_COMPILER=$(shell which $(GCC_CC_x86_64)) \
		-DCMAKE_OBJCOPY=$(shell which $(OBJCOPY_x86_64)) \
		-DCMAKE_C_FLAGS="$(STM_COREBOOT_INC)" \
		-DUART=$(CONFIG_STM_TTYS0_BASE) \
		-DHEAPSIZE=$(CONFIG_STM_HEAPSIZE) \
		-DCBMEM_ENABLE=$(CONFIG_STM_CBMEM_CONSOLE) \
		-DSTMPE_ENABLED=$(CONFIG_STM_STMPE_ENABLED) \
		-DBUILD=$(STM_BUILD); \
	$(MAKE);


.PHONY: build
