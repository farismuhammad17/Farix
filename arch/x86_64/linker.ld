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

ENTRY(_start)

PHDRS
{
    boot PT_LOAD FLAGS(7);
    kernel PT_LOAD FLAGS(7);
}

SECTIONS
{
    /* 1. Early 32-bit Bootstrap Section (Physical address 1MB) */
    . = 1M;
    _kernel_start = .; /* Literal physical start marker */

    .boot :
    {
        *(.multiboot)
        /* Force everything from boot.o to live down here in low memory */
        build/boot.o(.text)
        build/boot.o(.rodata)
        build/boot.o(.data)
        build/boot.o(.bss)
        *(.boot_text)
        *(.boot_data)
    } : boot

    /* Align to page boundary before stepping into virtual space */
    . = ALIGN(4K);

    /* 2. Higher-Half Virtual Address Space Initialization */
    . += 0xFFFFFFFF80000000;
    _kernel_virtual_start = .;

    .text : AT(ADDR(.text) - 0xFFFFFFFF80000000)
    {
        *(.text)
    } : kernel

    .rodata : AT(ADDR(.rodata) - 0xFFFFFFFF80000000)
    {
        *(.rodata)
    } : kernel
    . = ALIGN(4K);

    .data : AT(ADDR(.data) - 0xFFFFFFFF80000000)
    {
        *(.data)
    } : kernel
    . = ALIGN(4K);

    .bss : AT(ADDR(.bss) - 0xFFFFFFFF80000000)
    {
        *(COMMON)
        *(.bss)
    } : kernel
    . = ALIGN(4K);

    _kernel_virtual_end = .;
    _kernel_end = . - 0xFFFFFFFF80000000; /* Literal physical end marker */
}
