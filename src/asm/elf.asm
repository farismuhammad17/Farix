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
    push ebp
    mov ebp, esp

    ; [ebp + 8]  = entry_point
    ; [ebp + 12] = user_stack
    mov eax, [ebp + 8]   ; entry_point
    mov ecx, [ebp + 12]  ; user_stack

    ; Setup Segment Selectors
    mov bx, 0x23        ; User data segment (Index 4, RPL 3)
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; The processor expects: SS, ESP, EFLAGS, CS, EIP
    push 0x23           ; SS (User Data)
    push ecx            ; ESP (User Stack)
    push 0x3202         ; EFLAGS (IF bit set)
    push 0x1B           ; CS (User Code: Index 3, RPL 3)
    push eax            ; EIP (Entry Point)

    ; Perform the jump to Ring 3
    iret
