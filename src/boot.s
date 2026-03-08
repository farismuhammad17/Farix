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
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %esp                  # esp_dummy - current kernel stack pointer
    pushl %ebp
    pushl %esi
    pushl %edi

    pushl $128                  # int_no (0x80)
    pushl $0                    # err_code (syscalls don't have an error code from CPU, so 0)

    pushl %ds                   # ds - value of DS when syscall was called (user's DS)

    # Switch to Kernel Data Segment (0x10)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Push the current ESP as the argument to syscall_handler(syscalls_registers_t* regs)
    pushl %esp                  # ESP now points to the 'ds' field of the struct
    call syscall_handler
    addl $4, %esp               # Clean up the pushed argument

    # Restore segment registers for User Mode (0x23) before returning to user space
    movw $0x23, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Pop everything in reverse order of pushing (from stub)
    popl %eax                   # Pop ds
    popl %edi                   # Pop edi
    popl %esi                   # Pop esi
    popl %ebp                   # Pop ebp
    popl %esp                   # Pop esp_dummy
    popl %ebx                   # Pop ebx
    popl %edx                   # Pop edx
    popl %ecx                   # Pop ecx
    popl %eax                   # Pop eax

    # Clean up int_no and err_code
    addl $8, %esp

    iret                        # Return to Ring 3

.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    pushl $0            # Push fake error code
    pushl $\num         # Push interrupt number
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    pushl $\num
    jmp isr_common_stub
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

.extern exception_handler
isr_common_stub:
    pusha

    mov %ds, %ax        # Use % prefix
    pushl %eax          # Push EAX (which contains DS)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp          # Pass the stack pointer to C++
    call exception_handler
    add $4, %esp

    popl %eax           # Get the old DS back
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp
    iret
