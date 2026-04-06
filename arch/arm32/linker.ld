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

ENTRY(_start)

SECTIONS
{
    /* Raspberry Pi 2/3 (32-bit) conventional load address is 0x8000 */
    . = 0x8000;

    _kernel_start = .;

    /* The Code Section */
    .text : ALIGN(4K)
    {
        _vector_table_start = .;
        *(.vectors)
        *(.text.boot)
        *(.text)
    }

    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Initialized data */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* Uninitialized data + Stack */
    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    }

    _kernel_end = .;
    _end = .;
    end = .;
}
