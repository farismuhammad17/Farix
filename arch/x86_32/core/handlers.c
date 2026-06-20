/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stdarg.h>
#include <stdint.h>

#include "klib/string.h"

#include "hal.h"

#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/vmm.h"
#include "process/task.h"

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

/* Panic shell helper to convert hexadecimal to integer */
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

/* Panic shell printf function */
static void panic_err_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    if (len > 0) {
        echo_raw(buffer, len);
        uart_print(buffer);
    }

    va_end(args);
}

/* Dump general purpose registers for deeper debugging upon crash */
static inline void dump_register_info(syscalls_registers_x86_32_t* regs) {
    panic_err_printf("--- Register values ---\n");
    panic_err_printf("EAX: %x EBX: %x ECX: %x EDX: %x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    panic_err_printf("EDI: %x ESI: %x EBP: %x ESP: %x\n", regs->edi, regs->esi, regs->ebp, regs->esp_dummy);
}

/* Dumps multitasking information in case of race condition errors upon crash */
static inline void dump_multitasking_info() {
    if (unlikely(current_task == NULL)) return;

    panic_err_printf("--- Multitasking ---\n");
    panic_err_printf("Name:  %s (ID:%u)\n", current_task->name, current_task->id);
    panic_err_printf("Stack: 0x%08x -> 0x%08x\n", (uint32_t) current_task->stack_origin, current_task->stack_pointer);
    panic_err_printf("Page:  %p (PRIVILEGE:%u)\n", (void*) current_task->page_directory, (uint32_t) current_task->privilege);
}

/* Dumps call log upon crash */
static inline void dump_call_log(int funcs_per_line) {
    if (likely(!__DEBUG__)) return;

    panic_err_printf("--- Call log ---\n");
    for (size_t i = 0; i < MAX_LOG_LEN; i++) {
        panic_err_printf("%d:%s ", i, call_log[i]);
        if ((i + 1) % funcs_per_line == 0) {
            panic_err_printf("\n");
        }
    }
    panic_err_printf("(%s at %d)\n", last_call_finished ? "Finished" : "Unfinished", log_index - 1);
}

/* Panic shell dump command */
static void panic_cmd_dump(uint32_t addr) {
    uint8_t* ptr = (uint8_t*) addr;
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) panic_err_printf("\n%08lx: ", (uint32_t)(ptr + i));
        panic_err_printf("%02x ", ptr[i]);
    }
    panic_err_printf("\r\n");
}

/* Panic shell peek command */
static void panic_cmd_peek(uint32_t addr) {
    uint32_t value = *(volatile uint32_t*) addr;
    panic_err_printf("\n0x%08lx = 0x%08lx", addr, value);
}

/* Panic shell parser function */
static void execute_panic_cmd(char cmd[]) {
    if (
        cmd[0] == 'd' &&
        cmd[1] == 'u' &&
        cmd[2] == 'm' &&
        cmd[3] == 'p'
    ) {
        panic_cmd_dump(hex_to_int(&cmd[5]));
    } else if (
        cmd[0] == 'p' &&
        cmd[1] == 'e' &&
        cmd[2] == 'e' &&
        cmd[3] == 'k'
    ) {
        panic_cmd_peek(hex_to_int(&cmd[5]));
    } else if (
        cmd[0] == 'r' &&
        cmd[1] == 'e' &&
        cmd[2] == 'b' &&
        cmd[3] == 'o' &&
        cmd[4] == 'o' &&
        cmd[5] == 't'
    ) {
        panic_err_printf("Rebooting...\r\n");
        outb(0x64, 0xFE); // Pulse CPU reset line
    } else if (
        cmd[0] == 'h' &&
        cmd[1] == 'e' &&
        cmd[2] == 'l' &&
        cmd[3] == 'p'
    ) {
        panic_err_printf("\ndump <hex>\npeek <addr>\nreboot");
    } else panic_err_printf("\nUnknown command: %s (use help)", cmd);
}

// TODO: Move panic shell out into dedicated file

/* Panic shell main function */
static void panic_shell() {
    // Flush any leftover keys from the crash event
    while (inb(0x64) & 0x01) inb(0x60);

    echo_raw("\n> ", 3);
    char cmd[64];
    int i = 0;

    while (1) {
        char c = keyboard_getc();
        if (c != 0) {
            if (c == '\n') {
                cmd[i] = '\0';
                execute_panic_cmd(cmd);
                i = 0;
                echo_raw("\n> ", 3);
            } else if (c == '\b' && i > 0) {
                i--;
                echo_raw("\b \b", 3);
            } else if (i < 63) {
                cmd[i++] = c;
                echo_raw((const char*) &c, 1);
            }
        }
    }
}

// TODO: use t_printf instead, but t_print doesn't support colors yet

/* Called upon exception caught by IDT */
void exception_handler(syscalls_registers_x86_32_t* regs) {
    asm volatile("cli");

    // BSOD
    terminal_change_color(0x1F); // Blue background, White foreground
    terminal_clear();

    if (regs->int_no < 32) {
        panic_err_printf("Exception: %d (%s)\n", regs->int_no, exception_messages[regs->int_no]);
    } else {
        panic_err_printf("Exception: %d\n", regs->int_no);
    }

    if (unlikely(__DEBUG__)) {
        panic_err_printf("Logged number: %d | ", logged_num);
    }

    panic_err_printf("Error Code: %x\n", regs->err_code);
    panic_err_printf("EIP: %x  CS: %x  EFLAGS: %x\n", regs->eip, regs->cs, regs->eflags);

    switch (regs->int_no) {
        case 13: { // GPF
            panic_err_printf("Selector: %x (%s)\n", regs->err_code & 0xFFF8,
                    (regs->err_code & 0x04) ? "LDT" : "GDT");
            break;
        }

        case 14: { // Page fault
            uint32_t faulting_address;
            asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

            panic_err_printf("Faulting Address (CR2): %x\n", faulting_address);
            panic_err_printf("Reason: %s, %s, %s\n",
                (regs->err_code & PAGE_PRESENT) ? "Page unaccessible" : "Non-present page",
                (regs->err_code & PAGE_RW)      ? "Write fault" : "Read fault",
                (regs->err_code & PAGE_USER)    ? "User-mode" : "Kernel-mode");

            break;
        }
    }

    dump_call_log(3);
    dump_register_info(regs);
    dump_multitasking_info();

    panic_shell();
}
