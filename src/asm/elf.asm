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

[BITS 32]
section .text
global elf_user_trampoline_stub

; void elf_user_trampoline_stub(uint32_t entry, uint32_t stack);
elf_user_trampoline_stub:
    mov eax, [esp + 4]   ; eax = entry_point
    mov ecx, [esp + 8]   ; ecx = user_stack

    ; We do this before messing with the stack to be safe
    mov bx, 0x23
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; The CPU will pop these in order: EIP, CS, EFLAGS, ESP, SS
    push 0x23           ; SS (User Data)
    push ecx            ; ESP (User Stack)
    push 0x202          ; EFLAGS (Interrupts enabled, bit 1 reserved set)
    push 0x1B           ; CS (User Code)
    push eax            ; EIP (ELF Entry Point)

    iret
