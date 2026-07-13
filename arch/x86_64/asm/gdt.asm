; -----------------------------------------------------------------------
; Farix Operating System
; Copyright (C) 2026  Faris Muhammad

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU Affero General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Affero General Public License for more details.

; You should have received a copy of the GNU Affero General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[BITS 64]

global gdt_flush
global load_tss

gdt_flush:
    ; RDI holds the pointer to gdt_ptr
    lgdt [rdi]           ; Reload GDT structure descriptor safely

    ; Instead of relying entirely on the existing stack frame layout
    ; which could be clipping limits or unaligned, build an explicit jump sequence:
    mov rax, 0x00
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax           ; Keep data segments zeroed out

    ; Explicitly allocate clean stack alignment properties for the far return frame
    mov rbx, rsp         ; Back up your current stack pointer safely

    push qword 0x08      ; 64-bit Code Segment selector
    lea rax, [rel .flush_cs]
    push rax             ; 64-bit Target Instruction Pointer
    retfq                ; Force absolute far reload jump

.flush_cs:
    ret

load_tss:
    ; GDT layout keeps TSS at entry index 5:
    ; Index 5 * 8 = 40 = 0x28. RPL must be 00 for Ring 0 kernel execution.
    mov ax, 0x28
    ltr ax              ; Load Task Register
    ret
