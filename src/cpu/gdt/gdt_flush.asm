; -----------------------------------------------------------------------
; Copyright (C) 2026 Faris Muhammad

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[bits 32]               ; We are in 32-bit protected mode

[global gdt_flush]      ; Allows C++ to call this function

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]          ; Inform the CPU about the new GDT

    ; 0x10 is the offset to our Data Segment (Entry 2: 2 * 8 = 16 = 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump
    ; 0x08 is the offset to our Code Segment (Entry 1: 1 * 8 = 8 = 0x08)
    jmp 0x08:.flush_cs

.flush_cs:
    ret                 ; Return to gdt_init() in C++
