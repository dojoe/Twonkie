# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Embedded Controller firmware build system
#

ifndef BOARD
$(warning Please specify a board to build for via the BOARD=<board> variable.)
$(warning Options:)
$(warning .   BOARD=twinkie   - OG Twinkie)
$(warning .   BOARD=twonkie   - Twonkie v1)
$(warning .   BOARD=twonkiev2 - Twonkie v2)
$(error Missing BOARD)
endif

# Directory where the board is configured (includes /$(BOARD) at the end)
BDIR:=$(wildcard board/$(BOARD) private-*/board/$(BOARD))
# There can be only one <insert exploding windows here>
ifeq (,$(BDIR))
$(error unable to locate BOARD $(BOARD))
endif
ifneq (1,$(words $(BDIR)))
$(error multiple definitions for BOARD $(BOARD): $(BDIR))
endif
ifneq ($(filter private-%,$(BDIR)),)
PDIR=$(subst /board/$(BOARD),,$(BDIR))
endif

PROJECT?=ec

# Output directory for build objects
ifdef CTS_MODULE
# CTS builds need different directories per board per suite.
out?=build/$(BOARD)/cts_$(CTS_MODULE)
else
out?=build/$(BOARD)
endif

# File containing configuration information
config=$(out)/.config

# If no key file is provided, use the default dev key
PEM ?= $(BDIR)/dev_key.pem

include Makefile.toolchain

# Define the traditional first target. The dependencies of this are near the
# bottom as they can be altered by chip and board files.
.PHONY: all
all:

# The board makefile sets $CHIP and the chip makefile sets $CORE.
# Include those now, since they must be defined for _flag_cfg below.
include $(BDIR)/build.mk
include chip/$(CHIP)/build.mk

# Create uppercase config variants, to avoid mixed case constants.
# Also translate '-' to '_', so 'cortex-m' turns into 'CORTEX_M'.  This must
# be done before evaluating config.h.
uppercase = $(shell echo $(1) | tr '[:lower:]-' '[:upper:]_')
UC_BOARD:=$(call uppercase,$(BOARD))
UC_CHIP:=$(call uppercase,$(CHIP))
UC_CHIP_FAMILY:=$(call uppercase,$(CHIP_FAMILY))
UC_CHIP_VARIANT:=$(call uppercase,$(CHIP_VARIANT))
UC_CORE:=$(call uppercase,$(CORE))
UC_PROJECT:=$(call uppercase,$(PROJECT))

# Transform the configuration into make variables.  This must be done after
# the board/project/chip/core variables are defined, since some of the configs
# are dependent on particular configurations.
includes=include core/$(CORE)/include $(dirs) $(out) test
ifdef CTS_MODULE
includes+=cts/$(CTS_MODULE) cts
endif
ifeq "$(TEST_BUILD)" "y"
	_tsk_lst_file:=ec.tasklist
	_tsk_lst_flags:=-Itest -DTEST_BUILD -imacros $(PROJECT).tasklist
else ifdef CTS_MODULE
	_tsk_lst_file:=ec.tasklist
	_tsk_lst_flags:=-I cts/$(CTS_MODULE) -Icts -DCTS_MODULE=$(CTS_MODULE) \
			-imacros cts.tasklist
else
	_tsk_lst_file:=$(PROJECT).tasklist
	_tsk_lst_flags:=
endif

_tsk_lst_flags+=-I$(BDIR) -DBOARD_$(UC_BOARD) -D_MAKEFILE \
		-imacros $(_tsk_lst_file)

_tsk_lst_ro:=$(shell $(CPP) -P -DSECTION_IS_RO \
	$(_tsk_lst_flags) include/task_filter.h)
_tsk_lst_rw:=$(shell $(CPP) -P -DSECTION_IS_RW \
	$(_tsk_lst_flags) include/task_filter.h)

_tsk_cfg_ro:=$(foreach t,$(_tsk_lst_ro) ,HAS_TASK_$(t))
_tsk_cfg_rw:=$(foreach t,$(_tsk_lst_rw) ,HAS_TASK_$(t))

_tsk_cfg:= $(filter $(_tsk_cfg_ro), $(_tsk_cfg_rw))
_tsk_cfg_ro:= $(filter-out $(_tsk_cfg), $(_tsk_cfg_ro))
_tsk_cfg_rw:= $(filter-out $(_tsk_cfg), $(_tsk_cfg_rw))

CPPFLAGS_RO+=$(foreach t,$(_tsk_cfg_ro),-D$(t))
CPPFLAGS_RW+=$(foreach t,$(_tsk_cfg_rw),-D$(t))
CPPFLAGS+=$(foreach t,$(_tsk_cfg),-D$(t))

_flag_cfg_ro:=$(shell $(CPP) $(CPPFLAGS) -P -dM -Ichip/$(CHIP) \
	-I$(BDIR) -DSECTION_IS_RO include/config.h | \
	grep -o "\#define CONFIG_[A-Z0-9_]*" | cut -c9- | sort)
_flag_cfg_rw:=$(_tsk_cfg_rw) $(shell $(CPP) $(CPPFLAGS) -P -dM -Ichip/$(CHIP) \
	-I$(BDIR) -DSECTION_IS_RW include/config.h | \
	grep -o "\#define CONFIG_[A-Z0-9_]*" | cut -c9- | sort)

_flag_cfg:= $(filter $(_flag_cfg_ro), $(_flag_cfg_rw))
_flag_cfg_ro:= $(filter-out $(_flag_cfg), $(_flag_cfg_ro))
_flag_cfg_rw:= $(filter-out $(_flag_cfg), $(_flag_cfg_rw))

$(foreach c,$(_tsk_cfg_rw) $(_flag_cfg_rw),$(eval $(c)=rw))
$(foreach c,$(_tsk_cfg_ro) $(_flag_cfg_ro),$(eval $(c)=ro))
$(foreach c,$(_tsk_cfg) $(_flag_cfg),$(eval $(c)=y))

ifneq "$(CONFIG_COMMON_RUNTIME)" "y"
	_irq_list:=$(shell $(CPP) $(CPPFLAGS) -P -Ichip/$(CHIP) -I$(BDIR) \
		-D"ENABLE_IRQ(x)=EN_IRQ x" -imacros chip/$(CHIP)/registers.h \
		$(BDIR)/ec.irqlist | grep "EN_IRQ .*" | cut -c8-)
	CPPFLAGS+=$(foreach irq,$(_irq_list),\
		    -D"irq_$(irq)_handler_optional=irq_$(irq)_handler")
endif

# Compute RW firmware size and offset
_rw_off_str:=$(shell echo "CONFIG_RW_MEM_OFF" | $(CPP) $(CPPFLAGS) -P \
		-Ichip/$(CHIP) -I$(BDIR) -imacros include/config.h)
_rw_off:=$(shell echo "$$(($(_rw_off_str)))")
_rw_size_str:=$(shell echo "CONFIG_RW_SIZE" | $(CPP) $(CPPFLAGS) -P \
		-Ichip/$(CHIP) -I$(BDIR) -imacros include/config.h)
_rw_size:=$(shell echo "$$(($(_rw_size_str)))")
_program_memory_base_str:=$(shell echo "CONFIG_PROGRAM_MEMORY_BASE" | \
		$(CPP) $(CPPFLAGS) -P \
		-Ichip/$(CHIP) -I$(BDIR) -imacros include/config.h)
_program_memory_base=$(shell echo "$$(($(_program_memory_base_str)))")

$(eval BOARD_$(UC_BOARD)=y)
$(eval CHIP_$(UC_CHIP)=y)
$(eval CHIP_VARIANT_$(UC_CHIP_VARIANT)=y)
$(eval CHIP_FAMILY_$(UC_CHIP_FAMILY)=y)

# Private subdirectories may call this from their build.mk
# First arg is the path to be prepended to configured *.o files.
# Second arg is the config variable (ie, "FOO" to select with $(FOO-$3)).
# Third arg is the config variable value ("y" for configuration options
#   that are set for both RO and RW, "rw" for RW-only configuration options)
objs_from_dir_p=$(foreach obj, $($(2)-$(3)), $(1)/$(obj))
objs_from_dir=$(call objs_from_dir_p,$(1),$(2),y)

# Get build configuration from sub-directories
# Note that this re-includes the board and chip makefiles

ifdef CTS_MODULE
include cts/build.mk
endif
include $(BDIR)/build.mk
include chip/$(CHIP)/build.mk
include core/$(CORE)/build.mk
include common/build.mk
include driver/build.mk
-include private/build.mk
ifneq ($(PDIR),)
include $(PDIR)/build.mk
endif

includes+=$(includes-y)

# Wrapper for fetching all the sources relevant to this build
# target.
# First arg is "y" indicating sources for all segments,
#   or "rw" indicating sources for rw segment.
define get_sources =
# Get sources to build for this target
all-obj-$(1)+=$(call objs_from_dir_p,core/$(CORE),core,$(1))
all-obj-$(1)+=$(call objs_from_dir_p,chip/$(CHIP),chip,$(1))
all-obj-$(1)+=$(call objs_from_dir_p,$(BDIR),board,$(1))
all-obj-$(1)+=$(call objs_from_dir_p,private,private,$(1))
ifneq ($(PDIR),)
all-obj-$(1)+=$(call objs_from_dir_p,$(PDIR),$(PDIR),$(1))
endif
all-obj-$(1)+=$(call objs_from_dir_p,common,common,$(1))
all-obj-$(1)+=$(call objs_from_dir_p,driver,driver,$(1))
all-obj-$(1)+=$(call objs_from_dir_p,power,power,$(1))
ifdef CTS_MODULE
all-obj-$(1)+=$(call objs_from_dir_p,cts,cts,$(1))
endif
all-obj-$(1)+=$(call objs_from_dir_p,test,$(PROJECT),$(1))
endef

# Get all sources to build
$(eval $(call get_sources,y))
$(eval $(call get_sources,ro))

dirs=core/$(CORE) chip/$(CHIP) $(BDIR) common power test cts/common cts/$(CTS_MODULE)
dirs+= private $(PDIR)
dirs+=$(shell find driver -type d)
common_dirs=util

ifeq ($(custom-ro_objs-y),)
ro-common-objs := $(sort $(foreach obj, $(all-obj-y), $(out)/RO/$(obj)))
ro-only-objs := $(sort $(foreach obj, $(all-obj-ro), $(out)/RO/$(obj)))
ro-objs := $(sort $(ro-common-objs) $(ro-only-objs))
else
ro-objs := $(sort $(foreach obj, $(custom-ro_objs-y), $(out)/RO/$(obj)))
endif

# Add RW-only sources to build
$(eval $(call get_sources,rw))

rw-common-objs := $(sort $(foreach obj, $(all-obj-y), $(out)/RW/$(obj)))
rw-only-objs := $(sort $(foreach obj, $(all-obj-rw), $(out)/RW/$(obj)))
rw-objs := $(sort $(rw-common-objs) $(rw-only-objs))

# Don't include the shared objects in the RO/RW image if we're enabling
# the shared objects library.
ifeq ($(CONFIG_SHAREDLIB),y)
ro-objs := $(filter-out %_sharedlib.o, $(ro-objs))
endif
ro-deps := $(ro-objs:%.o=%.o.d)
rw-deps := $(rw-objs:%.o=%.o.d)
deps := $(ro-deps) $(rw-deps)

.PHONY: ro rw
$(config): $(out)/$(PROJECT).bin
	@printf '%s=y\n' $(_tsk_cfg) $(_flag_cfg) > $@

def_all_deps:=utils ro rw notice $(config) $(PROJECT_EXTRA)
all_deps?=$(def_all_deps)
all: $(all_deps)

ro: override BLD:=RO
ro: $(libsharedobjs_elf-y) $(out)/RO/$(PROJECT).RO.flat

rw: override BLD:=RW
rw: $(libsharedobjs_elf-y) $(out)/RW/$(PROJECT).RW.flat

# Shared objects library
SHOBJLIB := libsharedobjs
sharedlib-objs := $(filter %_sharedlib.o, $(all-obj-y))
sharedlib-objs := $(foreach obj, $(sharedlib-objs), $(out)/$(SHOBJLIB)/$(obj))
sharedlib-deps := $(sharedlib-objs:%.o=%.o.d)
deps += $(sharedlib-deps)
def_libsharedobjs_deps := $(sharedlib-objs)
libsharedobjs_deps ?= $(def_libsharedobjs_deps)

libsharedobjs-$(CONFIG_SHAREDLIB) := $(out)/$(SHOBJLIB)/$(SHOBJLIB).flat
libsharedobjs_elf-$(CONFIG_SHAREDLIB) := \
	$(libsharedobjs-$(CONFIG_SHAREDLIB):%.flat=%.elf)
libsharedobjs: $(libsharedobjs-y)

include Makefile.rules
export CROSS_COMPILE CFLAGS CC CPP LD NM AR OBJCOPY OBJDUMP
