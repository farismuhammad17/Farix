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
