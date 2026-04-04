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

.section ".vectors"
.global _vector_table

_vector_table:
    ldr pc, reset_addr       /* 0x00: Reset */
    ldr pc, undef_addr       /* 0x04: Undefined Instruction */
    ldr pc, svc_addr         /* 0x08: Supervisor Call (Syscalls) */
    ldr pc, prefetch_addr    /* 0x0C: Prefetch Abort */
    ldr pc, abort_addr       /* 0x10: Data Abort (Memory/Page Faults) */
    nop                      /* 0x14: Reserved */
    ldr pc, irq_addr         /* 0x18: IRQ (Interrupt Request) */
    ldr pc, fiq_addr         /* 0x1C: FIQ (Fast Interrupt Request) */

/* Address literals */
reset_addr:    .word _start
undef_addr:    .word exception_undef_handler_stub
svc_addr:      .word syscall_handler_stub
prefetch_addr: .word exception_prefetch_handler_stub
abort_addr:    .word exception_data_abort_handler_stub
irq_addr:      .word irq_handler_stub
fiq_addr:      .word default_handler_stub

.section .text

.global exception_undef_handler_stub
.global syscall_handler_stub
.global exception_prefetch_handler_stub
.global exception_data_abort_handler_stub
.global irq_handler_stub
.global default_handler_stub
.extern exception_undef_handler
.extern syscall_handler
.extern exception_prefetch_handler
.extern exception_data_abort_handler
.extern irq_handler

default_handler_stub:
    cpsid if           /* Disable IRQ and FIQ */
1:  wfi                /* Wait for interrupt (low power hang) */
    b 1b               /* Jump back to 1 if woken up */

exception_undef_handler_stub:
    push {r0-r12, lr}
    mov r0, sp              /* Pass stack pointer as registers_t* */
    bl exception_undef_handler
    pop {r0-r12, lr}
    movs pc, lr             /* Return to user code */

syscall_handler_stub:
    push {r0-r12, lr}
    mov r0, sp            /* R0 = pointer to the struct on the stack */
    bl syscall_handler
    pop {r0-r12, lr}
    movs pc, lr

exception_prefetch_handler_stub:
    sub lr, lr, #4          /* Point to the instruction that failed */
    push {r0-r12, lr}
    mov r0, sp
    bl exception_prefetch_handler
    pop {r0-r12, lr}
    movs pc, lr

exception_data_abort_handler_stub:
    /* ARM hardware sets LR to instruction_address + 8 */
    sub lr, lr, #8
    push {r0-r12, lr}

    mov r0, sp              /* Pass pointer to saved registers */
    bl exception_data_abort_handler

    pop {r0-r12, lr}
    movs pc, lr             /* Attempt to re-execute or fail */

irq_handler_stub:
    /* Fix the Return Address */
    sub lr, lr, #4

    /* Save the context */
    push {r0-r12, lr}

    /* Pass the stack pointer to C */
    mov r0, sp
    bl irq_handler

    /* Restore and Return */
    pop {r0-r12, lr}
    movs pc, lr
