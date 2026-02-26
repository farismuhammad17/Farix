/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad
Based on code from the OSDev Wiki (wiki.osdev.org)

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

ENTRY(_start)

SECTIONS
{
    /* Start at 1 MiB. This is the conventional load address for kernels. */
    /* It leaves the lower memory free for BIOS and VGA hardware.         */
    . = 1M;

    _kernel_start = .;

    /* The Multiboot section comes FIRST.                            */
    /* GRUB needs to find the 'Magic Numbers' at the very beginning. */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)  /* Put the Multiboot header here     */
        *(.text)       /* Then put all the actual code here */
    }

    /* Read-only data (like strings "Hello World") */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Initialized data (variables that have a starting value) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Uninitialized data + the Stack (empty memory to be used) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;

    _end = .;
    end = .;
}
