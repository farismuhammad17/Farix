/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

/* Multiboot Header constants */

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Declare the Multiboot section */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Reserve a small area for the stack */

.section .bss
.align 16
stack_bottom:
.skip 65536 /* 64 KiB of space */
stack_top:
.global stack_top

/* The actual entry point where the CPU begins execution */

.section .text
.global _start
.type _start, @function

_start:
	/* Setup the Stack */
	mov $stack_top, %esp

	push %ebx    # This becomes 'mbi'
    push %eax    # This becomes 'magic'

	/* Call the C++ kernel_main function */
	call kernel_main

	/* Safety Net */
	/* If kernel_main ever returns, we put the CPU in an infinite loop */
cli
1:	hlt
	jmp 1b

.size _start, . - _start

/* IDT handlers */

.global default_handler_stub
.global keyboard_handler_stub
.global mouse_handler_stub
.global syscall_handler_stub
.extern keyboard_handler
.extern mouse_handler
.extern syscall_handler

default_handler_stub:           # Shouldn't be seen unless it's an error, probably in memory
    movl $0x0F210F21, 0xB8000   # Puts '!' on screen
    cli
    hlt

keyboard_handler_stub:
    pusha                       # Save all general-purpose registers
    call keyboard_handler
    popa                        # Restore all registers
    iret                        # Interrupt Return

mouse_handler_stub:
    cli                         # Disable interrupts to prevent nested IRQs
    pusha
    call mouse_handler
    popa
    sti                         # Re-enable interrupts
    iret

syscall_handler_stub:
    pushl $0                    # Dummy error code to keep stack consistent
    pushl $128                  # Interrupt number (0x80)
    pusha                       # Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    # Switch to Kernel Data Segment
    # User Mode uses 0x23, but the Kernel needs 0x10
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Push the current stack pointer as an argument to syscall_handler(regs*)
    pushl %esp
    call syscall_handler
    addl $4, %esp               # Clean up the pushed ESP

    popa                        # Restore User's registers
    addl $8, %esp               # Clean up the int number and error code
    iret                        # Return to Ring 3
