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

.section .text
.global elf_user_trampoline_stub

/* void elf_user_trampoline_stub(uint32_t entry, uint32_t stack); */
elf_user_trampoline_stub:
    /* r0 = entry point, r1 = user stack pointer */

    /* Disable interrupts globally before the mode switch */
    cpsid i

    /* Set the SPSR (Saved Program Status Register) */
    /* Bits 0-4: 0x10 is User Mode */
    /* Bit 7: 0 (Enable IRQ) */
    /* Bit 8: 0 (Enable FIQ) */
    ldr r2, =0x10
    msr spsr_cxsf, r2

    /* Set the User Mode Stack Pointer (r13_usr) */
    /* We use the 'cps' hint or move to System mode to set the User SP safely */
    cps #0x1F                /* Switch to System Mode (shares registers with User) */
    mov sp, r1               /* Set the User Stack Pointer */
    mov lr, #0               /* Clear User Link Register for safety */

    cps #0x13                /* Switch back to Supervisor (Kernel) Mode */

    mov lr, r0

    /* 'movs pc, lr' copies SPSR into CPSR and jumps to LR in one atomic step. */
    movs pc, lr
