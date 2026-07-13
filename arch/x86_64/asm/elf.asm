; -----------------------------------------------------------------------
; Farix Operating System
; Copyright (C) 2026  Faris Muhammad
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU Affero General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Affero General Public License for more details.
;
; You should have received a copy of the GNU Affero General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[BITS 64]
section .text

global elf_user_trampoline_stub

; void elf_user_trampoline_stub(uint64_t entry, uint64_t stack);
elf_user_trampoline_stub:
    ; System V ABI Register Mapping:
    ; RDI = uint64_t entry  (First Parameter)
    ; RSI = uint64_t stack  (Second Parameter)

    ; Clear out user data segment target tracking registers.
    ; Code Segment Selector = 0x1B (Index 3, plus 3 for RPL Ring 3 privileges)
    ; Data Segment Selector = 0x23 (Index 4, plus 3 for RPL Ring 3 privileges)
    mov bx, 0x23
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; The CPU will pop these explicitly in reverse
    push 0x23           ; SS (User Data Segment Selector)
    push rsi            ; RSP (User Stack Address)
    push 0x202          ; RFLAGS (Interrupts enabled, bit 1 reserved set)
    push 0x1B           ; CS (User Code Segment Selector)
    push rdi            ; RIP (The 64-bit User ELF Entry Point)

    iretq
