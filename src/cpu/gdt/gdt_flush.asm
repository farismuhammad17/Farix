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

global load_tss
load_tss:
    mov ax, 0x2B      ; Index 5 in GDT (5 * 8 = 40, plus 3 for RPL)
    ltr ax            ; Load Task Register
    ret

global jump_to_user_mode
jump_to_user_mode:
    ; Argument 1: [esp + 4] -> entry_point
    ; Argument 2: [esp + 8] -> user_stack

    mov ebx, [esp + 4] ; Save entry point
    mov ecx, [esp + 8] ; Save stack pointer

    ; Set up segment registers for User Data (Selector 0x20 | 3 = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; We create a fake interrupt frame for IRET
    ; IRET expects: SS, ESP, EFLAGS, CS, EIP (in that order)
    push 0x23           ; SS (User Data)
    push ecx            ; ESP (User Stack)
    push 0x202          ; EFLAGS (Standard value, Interrupts enabled)
    push 0x1B           ; CS (User Code: 0x18 | 3)
    push ebx            ; EIP (Entry Point)

    iret                ; Jump to Ring 3
