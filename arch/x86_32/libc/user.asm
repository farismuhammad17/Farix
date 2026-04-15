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
global _start
extern main

_start:
    mov ebp, 0
    push ebp
    mov ebp, esp

    call main

    mov ebx, eax ; status code
    mov eax, 1   ; SYS_EXIT
    int 0x80     ; yield

    jmp $        ; Not good to get here, kernel should've killed us by now
.halt:
    jmp .halt
