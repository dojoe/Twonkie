/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "config.h"
#include "rwsig.h"

#define STRINGIFY0(name) #name
#define STRINGIFY(name) STRINGIFY0(name)

#define FW_OFF_(section) CONFIG_##section##_MEM_OFF
#define FW_OFF(section) (CONFIG_PROGRAM_MEMORY_BASE + FW_OFF_(section))

#define FW_SIZE_(section) CONFIG_##section##_SIZE
#define FW_SIZE(section) FW_SIZE_(section)


OUTPUT_FORMAT(BFD_FORMAT, BFD_FORMAT, BFD_FORMAT)
OUTPUT_ARCH(BFD_ARCH)
ENTRY(reset)
MEMORY
{
#ifdef CONFIG_SHAREDLIB
    SHARED_LIB (rx) : ORIGIN = FW_OFF(SHAREDLIB), LENGTH = FW_SIZE(SHAREDLIB)
#endif
    FLASH (rx) : ORIGIN = FW_OFF(SECTION), LENGTH = FW_SIZE(SECTION)
    IRAM (rw)  : ORIGIN = CONFIG_RAM_BASE, LENGTH = CONFIG_RAM_SIZE
#ifdef CONFIG_CHIP_MEMORY_REGIONS
#define REGION(name, attr, start, size) \
    name(attr) : ORIGIN = (start), LENGTH = (size)
#include "memory_regions.inc"
#undef REGION
#endif /* CONFIG_MEMORY_REGIONS */
}
SECTIONS
{
#ifdef CONFIG_SHAREDLIB
    .roshared : {
        KEEP(*(.roshared*))
    } > SHARED_LIB
#endif
    .text : {
	*(.text.vecttable)
        . = ALIGN(4);
        __image_data_offset = .;
        KEEP(*(.rodata.ver))

        . = ALIGN(4);
        KEEP(*(.rodata.pstate))

        . = ALIGN(4);
        STRINGIFY(OUTDIR/core/CORE/init.o) (.text)
        *(.text*)
    } > FLASH
    . = ALIGN(4);
    .rodata : {
	/* Symbols defined here are declared in link_defs.h */
        __irqprio = .;
        KEEP(*(.rodata.irqprio))
        __irqprio_end = .;

        . = ALIGN(4);
        __cmds = .;
        KEEP(*(SORT(.rodata.cmds*)))
        __cmds_end = .;

        . = ALIGN(4);
        __extension_cmds = .;
        KEEP(*(.rodata.extensioncmds))
	__extension_cmds_end = .;

        . = ALIGN(4);
        __hcmds = .;
        KEEP(*(SORT(.rodata.hcmds*)))
        __hcmds_end = .;

	. = ALIGN(4);
	__mkbp_evt_srcs = .;
	KEEP(*(.rodata.evtsrcs))
	__mkbp_evt_srcs_end = .;

        . = ALIGN(4);
        __hooks_init = .;
        KEEP(*(.rodata.HOOK_INIT))
        __hooks_init_end = .;

        __hooks_pre_freq_change = .;
        KEEP(*(.rodata.HOOK_PRE_FREQ_CHANGE))
        __hooks_pre_freq_change_end = .;

        __hooks_freq_change = .;
        KEEP(*(.rodata.HOOK_FREQ_CHANGE))
        __hooks_freq_change_end = .;

        __hooks_sysjump = .;
        KEEP(*(.rodata.HOOK_SYSJUMP))
        __hooks_sysjump_end = .;

        __hooks_chipset_pre_init = .;
        KEEP(*(.rodata.HOOK_CHIPSET_PRE_INIT))
        __hooks_chipset_pre_init_end = .;

        __hooks_chipset_startup = .;
        KEEP(*(.rodata.HOOK_CHIPSET_STARTUP))
        __hooks_chipset_startup_end = .;

        __hooks_chipset_resume = .;
        KEEP(*(.rodata.HOOK_CHIPSET_RESUME))
        __hooks_chipset_resume_end = .;

        __hooks_chipset_suspend = .;
        KEEP(*(.rodata.HOOK_CHIPSET_SUSPEND))
        __hooks_chipset_suspend_end = .;

        __hooks_chipset_shutdown = .;
        KEEP(*(.rodata.HOOK_CHIPSET_SHUTDOWN))
        __hooks_chipset_shutdown_end = .;

        __hooks_chipset_reset = .;
        KEEP(*(.rodata.HOOK_CHIPSET_RESET))
        __hooks_chipset_reset_end = .;

        __hooks_ac_change = .;
        KEEP(*(.rodata.HOOK_AC_CHANGE))
        __hooks_ac_change_end = .;

        __hooks_lid_change = .;
        KEEP(*(.rodata.HOOK_LID_CHANGE))
        __hooks_lid_change_end = .;

        __hooks_tablet_mode_change = .;
        KEEP(*(.rodata.HOOK_TABLET_MODE_CHANGE))
        __hooks_tablet_mode_change_end = .;

        __hooks_pwrbtn_change = .;
        KEEP(*(.rodata.HOOK_POWER_BUTTON_CHANGE))
        __hooks_pwrbtn_change_end = .;

        __hooks_battery_soc_change = .;
        KEEP(*(.rodata.HOOK_BATTERY_SOC_CHANGE))
        __hooks_battery_soc_change_end = .;

        __hooks_tick = .;
        KEEP(*(.rodata.HOOK_TICK))
        __hooks_tick_end = .;

        __hooks_second = .;
        KEEP(*(.rodata.HOOK_SECOND))
        __hooks_second_end = .;

        __deferred_funcs = .;
        KEEP(*(.rodata.deferred))
        __deferred_funcs_end = .;

        __usb_desc = .;
        KEEP(*(.rodata.usb_desc_conf))
        KEEP(*(SORT(.rodata.usb_desc*)))
        __usb_desc_end = .;
        . = ALIGN(4);
        KEEP(*(.rodata.usb_ep))
        KEEP(*(.rodata.usb_ep.usb_ep_tx))
        KEEP(*(.rodata.usb_ep.usb_ep_rx))
        KEEP(*(.rodata.usb_ep.usb_ep_event))
        KEEP(*(.rodata.usb_ep.usb_iface_request))

        . = ALIGN(4);
        *(.rodata*)

#if defined(SECTION_IS_RO) && defined(CONFIG_FLASH)
        . = ALIGN(64);
        KEEP(*(.google))
#endif
        . = ALIGN(4);
    } > FLASH
    __ro_end = . ;

    .bss : {
	/*
	 * Vector table must be at the beginning of bss section. The vector
	 * table section contains a RAM copy of the vector table used on
	 * STM chips for relocating the vector table.
	 */
	. = ALIGN(8);
	__bss_start = .;
	*(.bss.vector_table)
	/* Stacks must be 64-bit aligned */
        . = ALIGN(8);
        *(.bss.system_stack)
	/* Rest of .bss takes care of its own alignment */
        *(.bss)
        *(.bss.slow)

	/*
	 * Reserve space for deferred function firing times.  Each time is a
	 * uint64_t, each func is a 32-bit pointer, thus the scaling factor of
	 * two.  The 8 byte alignment of uint64_t is required by the ARM ABI.
	 */
        . = ALIGN(8);
        __deferred_until = .;
	. += (__deferred_funcs_end - __deferred_funcs) * (8 / 4);
        __deferred_until_end = .;

        . = ALIGN(4);
        __bss_end = .;
    } > IRAM
    .data : AT(ADDR(.rodata) + SIZEOF(.rodata)) {
        . = ALIGN(4);
        __data_start = .;
        *(.data.tasks)
        *(.data)
        . = ALIGN(4);
        *(.iram.text)
        . = ALIGN(4);
        __data_end = .;

	/* Shared memory buffer must be at the end of preallocated RAM, so it
	 * can expand to use all the remaining RAM. */
	__shared_mem_buf = .;

	/* NOTHING MAY GO AFTER THIS! */
    } > IRAM

    /* The linker won't notice if the .data section is too big to fit,
     * apparently because we're sending it into IRAM, not FLASH. The following
     * symbol isn't used by the code, but running "objdump -t *.elf | grep hey"
     * will let us check how much flash space we're actually using. The
     * explicit ASSERT afterwards will cause the linker to abort if we use too
     * much. */
    __hey_flash_used = LOADADDR(.data) + SIZEOF(.data) - FW_OFF(SECTION);
    ASSERT((FW_SIZE(SECTION)
#if defined(CONFIG_RWSIG) && defined(SECTION_IS_RO)
        - CONFIG_RO_PUBKEY_SIZE
#endif
#if defined(CONFIG_RWSIG) && defined(SECTION_IS_RW)
        - CONFIG_RW_SIG_SIZE
#endif
        ) >= (LOADADDR(.data) + SIZEOF(.data) - FW_OFF(SECTION)),
                 "No room left in the flash")

    __image_size = __hey_flash_used;

#ifdef CONFIG_CHIP_MEMORY_REGIONS
#define REGION(name, attr, start, size) \
    .name(NOLOAD) : { \
        __##name##_start = .; \
        *(SORT(.name.*)) \
    } > name
#include "memory_regions.inc"
#undef REGION
#endif /* CONFIG_CHIP_MEMORY_REGIONS */

#if !(defined(SECTION_IS_RO) && defined(CONFIG_FLASH))
    /DISCARD/ : {
              *(.google)
              }
#endif

    /DISCARD/ : { *(.ARM.*) }
}
