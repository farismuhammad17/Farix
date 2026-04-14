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

#include <stdint.h>
#include <stdio.h>

#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "libc/syscalls.h"
#include "memory/vmm.h"
#include "process/task.h"

typedef struct syscalls_registers_x86_32_t {
    uint32_t ds;                                           // Data segment (pushed by us)
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                             // Pushed in stub
    uint32_t eip, cs, eflags, useresp, ss;                 // Pushed by CPU
} syscalls_registers_x86_32_t;

static const char* exception_messages[] = {
    "Division By Zero",             // 0
    "Debug",                        // 1
    "Non Maskable Interrupt",       // 2
    "Breakpoint",                   // 3
    "Into Detected Overflow",       // 4
    "Out of Bounds",                // 5
    "Invalid Opcode",               // 6
    "No Coprocessor",               // 7
    "Double Fault",                 // 8
    "Coprocessor Segment Overrun",  // 9
    "Bad TSS",                      // 10
    "Segment Not Present",          // 11
    "Stack Fault",                  // 12
    "General Protection Fault",     // 13
    "Page Fault",                   // 14
    "Unknown Interrupt",            // 15
    "Floating Point Error",         // 16
    "Alignment Check",              // 17
    "Machine Check",                // 18
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

static uint32_t hex_to_int(char* s) {
    uint32_t res = 0;
    // Skip "0x" if the user typed it
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;

    while (*s) {
        uint8_t byte = *s;
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10; // Handle caps
        else break; // Stop at first non-hex char

        res = (res << 4) | (byte & 0xF);
        s++;
    }
    return res;
}

// Dump general purpose registers for deeper debugging
static void dump_register_info(syscalls_registers_x86_32_t* regs) {
    printf("--- Register values ---\n");
    printf("EAX: %lx EBX: %lx ECX: %lx EDX: %lx\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("EDI: %lx ESI: %lx EBP: %lx ESP: %lx\n", regs->edi, regs->esi, regs->ebp, regs->esp_dummy);
}

// Dumps multitasking information in case of race condition errors
static void dump_multitasking_info() {
    if (current_task == NULL) return;

    printf("--- Multitasking ---\n");
    printf("Name:  %s (ID:%lu)\n", current_task->name, current_task->id);
    printf("Stack: 0x%08lx -> 0x%08lx\n", (uint32_t) current_task->stack_origin, current_task->stack_pointer);
    printf("Page:  %p (PRIV:%lu)\n", (void*) current_task->page_directory, (uint32_t) current_task->privilege);
}

static void panic_cmd_dump(uint32_t addr) {
    uint8_t* ptr = (uint8_t*) addr;
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) printf("\n%08lx: ", (uint32_t)(ptr + i));
        printf("%02x ", ptr[i]);
    }
}

static void execute_panic_cmd(char cmd[]) {
    if (
        cmd[0] == 'd' &&
        cmd[1] == 'u' &&
        cmd[2] == 'm' &&
        cmd[3] == 'p'
    ) {
        panic_cmd_dump(hex_to_int(&cmd[5]));
    } else if (
        cmd[0] == 'r' &&
        cmd[1] == 'e' &&
        cmd[2] == 'b' &&
        cmd[3] == 'o' &&
        cmd[4] == 'o' &&
        cmd[5] == 't'
    ) {
        printf("Rebooting hardware...");
        outb(0x64, 0xFE); // Pulse CPU reset line
    } else if (
        cmd[0] == 'h' &&
        cmd[1] == 'e' &&
        cmd[2] == 'l' &&
        cmd[3] == 'p'
    ) {
        printf("\ndump <hex>\nreboot");
    } else printf("\nUnknown command: %s", cmd);
}

static void panic_shell() {
    // Flush any leftover keys from the crash event
    while (inb(0x64) & 0x01) inb(0x60);

    printf("\n> ");
    char cmd[64];
    int i = 0;

    while (1) {
        char c = keyboard_getc();
        if (c == 0) continue;

        else if (c == '\n') {
            cmd[i] = '\0';
            execute_panic_cmd(cmd);
            i = 0;
            printf("\n> ");
        } else if (c == '\b' && i > 0) {
            i--;
            printf("\b \b");
        } else if (i < 63) {
            cmd[i++] = c;
            printf("%c", c);
        }
    }
}

void syscall_handler(syscalls_registers_x86_32_t* regs) {
    switch (regs->eax) {
        case SYS_WRITE:
            regs->eax = _write(regs->ebx, (char*) regs->ecx, regs->edx);
            break;

        case SYS_READ:
            regs->eax = _read(regs->ebx, (char*) regs->ecx, regs->edx);
            break;

        case SYS_EXIT:
            _exit(regs->ebx);
            break;

        case SYS_SBRK:
            regs->eax = (uint32_t) _sbrk(regs->ebx);
            break;

        default:
            printf("Unknown syscall: %ld\n", regs->eax);
            regs->eax = -1;
            break;
    }
}

// TODO: use t_printf instead, but t_print doesn't support colors yet
void exception_handler(syscalls_registers_x86_32_t* regs) {
    asm volatile("cli");

    uart_printf("Exception: %ld (%s)\n", regs->int_no, exception_messages[regs->int_no]);

    // BSOD
    terminal_change_color(0x1F); // Blue background, White foreground
    terminal_clear();

    if (regs->int_no < 32) {
        printf("Exception: %ld (%s)\n", regs->int_no, exception_messages[regs->int_no]);
    } else {
        printf("Exception: %ld\n", regs->int_no);
    }

    printf("Error Code: %lx\n", regs->err_code);
    printf("EIP: %lx  CS: %lx  EFLAGS: %lx\n", regs->eip, regs->cs, regs->eflags);

    // Specifically for Page Faults
    if (regs->int_no == 14) {
        uint32_t faulting_address;
        asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
        printf("Faulting Address (CR2): %lx\n", faulting_address);

        printf("Reason: %s, %s, %s\n",
            (regs->err_code & PAGE_PRESENT) ? "Page unaccessible" : "Non-present page",
            (regs->err_code & PAGE_RW)      ? "Write fault" : "Read fault",
            (regs->err_code & PAGE_USER)    ? "User fault" : "Kernel fault");
    } else if (regs->int_no == 13) {
        printf("Selector: %lx (%s)\n", regs->err_code & 0xFFF8,
                (regs->err_code & 0x04) ? "LDT" : "GDT");
    }

    dump_register_info(regs);
    dump_multitasking_info();

    panic_shell();
}
