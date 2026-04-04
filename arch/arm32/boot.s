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

/* Reserved areas for stacks */
.section .bss
.align 16

/* Added stack_bottom here so the kernel knows where the memory starts */
stack_bottom:
.global stack_bottom
.skip 16384
stack_svc_top:
.global stack_svc_top

.skip 4096
stack_irq_top:
.global stack_irq_top

.skip 4096
stack_abt_top:
.global stack_abt_top

.section .text
.global _start

_start:
    /* Setup Stacks for different modes */
    /* MSR CPSR_c, #Mode_Bits | I_Bit | F_Bit */

    /* IRQ Mode */
    msr cpsr_c, #0xD2
    ldr sp, =stack_irq_top

    /* Abort Mode */
    msr cpsr_c, #0xD7
    ldr sp, =stack_abt_top

    /* Supervisor Mode (Kernel Default) */
    msr cpsr_c, #0xD3
    ldr sp, =stack_svc_top

    /* Clear BSS */
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0

clear_bss:
    cmp r0, r1
    strlo r2, [r0], #4
    blo clear_bss

    /* Pass Bootloader Arguments to C */
    /* On ARM, bootloaders usually pass:
       r0 = 0
       r1 = Machine Type ID
       r2 = Pointer to Atags or Device Tree (DTB)
    */
    bl arch_kmain

    /* Safety Net */
halt:
    wfi
    b halt
