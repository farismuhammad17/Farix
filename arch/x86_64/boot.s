/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

/* Multiboot Header constants */

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot header must be at the top, within the first 8KB */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096

/* Reserve an area for the stack */

.align 16
stack_bottom:
.global stack_bottom
.skip 65536 /* 64 KiB stack */
stack_top:
.global stack_top

/* GDT */

.section .data
.align 8
gdt64:
    .quad 0                                     /* Null descriptor */
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53) /* Code descriptor (64-bit kernel) */
gdt64_pointer:
    .word . - gdt64 - 1
    .long gdt64

/* Early 32-bit */

.section .boot_text
.global _start
.type _start, @function

.code32
_start:
    /* Disable interrupts immediately */
    cli

    /* Setup a temporary 32-bit stack while we bootstrap */
    mov $stack_top, %esp

    /* Save Multiboot registers before we overwrite them */
    mov %eax, %edi /* Store magic into EDI  (becomes 1st parameter) */
    mov %ebx, %esi /* Store mbi into ESI    (becomes 2nd parameter) */

    /* Set up the 4-level page structures */

    /* Link P4 entries to the P3 table address */
    mov $p3_table, %eax
    or $0x3, %eax                 /* Present flag + Writable flag (0x1 | 0x2) */
    mov %eax, (p4_table)          /* Entry 0: Identity map lower memory */
    mov %eax, p4_table + 511 * 8  /* Entry 511: Higher-half kernel space virtual mapping */

    /* Link P3 entries to the P2 table address */
    mov $p2_table, %eax
    or $0x3, %eax                 /* Present flag + Writable flag */
    mov %eax, (p3_table)          /* Entry 0: For lower identity map */
    mov %eax, p3_table + 510 * 8  /* Entry 510: For higher-half 0xFFFFFFFF80000000 mapping */

    /* Identity map the first 1 GiB of physical memory using large 2 MiB pages */
    mov $0, %ecx /* Loop counter */

map_p2_table:
    mov $0x200000, %eax /* 2 MiB in hex */
    mul %ecx            /* EAX = ECX * 2 MiB */
    or $0x83, %eax      /* Present + Writable + Huge Page Flag (0x80) */
    mov %eax, p2_table(, %ecx, 8)

    inc %ecx
    cmp $512, %ecx      /* Map all 512 entries of the P2 table */
    jne map_p2_table

    /* Load the P4 base address into the CR3 control register */
    mov $p4_table, %eax
    mov %eax, %cr3

    /* Enable PAE (Physical Address Extension) in CR4 */
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    /* Switch the EFER MSR to activate Long Mode */
    mov $0xC0000080, %ecx /* EFER MSR target */
    rdmsr
    or $(1 << 8), %eax    /* Set the Long Mode Enable (LME) bit */
    wrmsr

    /* Enable Paging in CR0 to fully engage Long Mode */
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    /* Load our 64-bit Global Descriptor Table */
    lgdt (gdt64_pointer)

    /* Far jump into 64-bit target segment space to flush execution queues */
    ljmp $0x08, $_start64

/* Mapped via *(.text) into the 0xFFFFFFFF80000000 virtual layout region */

.section .text
.code64

_start64:
    /* Clear and update all internal CPU segment registers */
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Setup the final 64-bit execution stack pointer */
    mov $stack_top, %rsp

    mov %edi, %edi
    mov %esi, %esi

    call arch_kmain
