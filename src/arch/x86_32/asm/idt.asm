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

section .text

global default_handler_stub
global keyboard_handler_stub
global mouse_handler_stub
global syscall_handler_stub
extern keyboard_handler
extern mouse_handler
extern syscall_handler
extern exception_handler

default_handler_stub:
    mov dword [0xB8000], 0x0F210F21
    cli
    hlt

keyboard_handler_stub:
    pushad
    call keyboard_handler
    popad
    iretd

mouse_handler_stub:
    cli
    pushad
    call mouse_handler
    popad
    sti
    iretd

syscall_handler_stub:
    push 0            ; err_code
    push 128          ; int_no
    pushad            ; Pushes 8 registers at once

    push ds           ; Push current data segment

    mov ax, 0x10      ; Load kernel data segment
    mov ds, ax
    mov es, ax

    push esp          ; Pointer to the stack for the C function
    call syscall_handler
    add esp, 4

    pop ds            ; Restore user data segment
    popad             ; Pop 8 registers at once
    add esp, 8        ; Clean up int_no and err_code

    iretd             ; Return to Ring 3

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0             ; Push fake error code
    push %1            ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1
    jmp isr_common_stub
%endmacro

; Generate the ISRs
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

isr_common_stub:
    pushad              ; Save all general-purpose registers

    mov ax, ds          ; Use AX to grab the current Data Segment
    push eax            ; Push EAX (which contains DS)

    mov ax, 0x10        ; Load the Kernel Data Segment descriptor (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pass the stack pointer (as a struct pointer) to C++
    call exception_handler
    add esp, 4          ; Clean up the stack after the call

    pop eax             ; Get the old DS back into EAX
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popad               ; Restore all general-purpose registers
    add esp, 8          ; Clean up the interrupt number and error code
    iretd               ; Final return from interrupt
