/*
 * optee_jump - Code for jumping to OPTEE
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/system.h>
#include <linux/linkage.h>

	.globl psci_resume
	.globl psci_cpu_on_handler
	.arch_extension sec

// void print_hex(uint32_t val)
ENTRY(print_hex)

        movw  r3, #0x0000             // load UART1 PA
        movt  r3, #0x3086

        mov     r1, #48                 // print '0'
        str     r1, [r3, #0x40]
        mov     r1, #120                // print 'x'
        str     r1, [r3, #0x40]

        rev     r0, r0                  // byte-reverse the word
print_byte:
        lsr     r1, r0, #4              // Get 2nd nibble (bits 4-7)
        and     r1, r1, #0xf

        cmp     r1, #10                 // Less than 10?
        addlt   r1, r1, #48             // Add '0'
        addge   r1, r1, #55             // Add 'A' - 10
        str     r1, [r3, #0x40]         // Write to UART TXd

        and     r1, r0, #0xf            // Get first nibble (bits 0 - 3)
        cmp     r1, #10                 // Less than 10?
        addlt   r1, r1, #48             // Add '0'
        addge   r1, r1, #55             // Add 'A' - 10
        str     r1, [r3, #0x40]         // Write to UART TXd

        lsrs    r0, r0, #8              // right shift by 8
        bne     print_byte              // until 0

        mov     r1, #13                 // print CR
        str     r1, [r3, #0x40]
        mov     r1, #10                 // print LF
        str     r1, [r3, #0x40]

wait_fifo_empty:
        ldr     r1, [r3, #0xB4]         // Load UART Test register
        tst     r1, #(1 << 6)
        beq     wait_fifo_empty

        bx lr
ENDPROC(print_hex)


/*
 void psci_resume(u32 context_id);
*/
ENTRY(psci_resume)
	blx print_hex
	b .

ENDPROC(psci_resume)

/*
 void psci_cpu_on_handler(u32 context_id);

 Immediately call psci_cpu_suspend again
*/
ENTRY(psci_cpu_on_handler)
	bl	print_hex

	ldr	r0, =0x84000001		// FN_CPU_SUSPEND
	ldr	r1, =0x40000005		// Core power down
	ldr	r2, =psci_cpu_on_handler	// we don't intend to resume
	ldr	r3, =0xCAFEBABE		// context_id
	smc 	0
	
	ldr	r0, =0xBADC0DE
	bl	print_hex
	b	.

ENDPROC(psci_cpu_on_handler)

