/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018 Microsoft Corporation
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/system.h>
#include <linux/linkage.h>

#ifdef CONFIG_SPL_OPTEE_BOOT

	.globl arch_jump_to_image_optee

/*
 void __noreturn arch_jump_to_image_optee(
	u32 optee_entry_point,
	u32 pageable_part,
	u32 uboot_entry_point
	);
*/
ENTRY(arch_jump_to_image_optee)

	mov r3, r0
	mov r0, r1
	mov lr, r2

	bx r3

ENDPROC(arch_jump_to_image_optee)
#endif
