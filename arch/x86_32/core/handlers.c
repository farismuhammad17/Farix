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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "hal.h"

#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "farix.h"

// For syscall_handler > SYS_INT_EXEC
#define SYS_INT_EXEC_CASE(n) case n: asm volatile("int %0" :: "i"(n)); break;
#define SYS_INT_EXEC_REP2(n)   SYS_INT_EXEC_CASE(n) SYS_INT_EXEC_CASE(n+1)
#define SYS_INT_EXEC_REP4(n)   SYS_INT_EXEC_REP2(n) SYS_INT_EXEC_REP2(n+2)
#define SYS_INT_EXEC_REP8(n)   SYS_INT_EXEC_REP4(n) SYS_INT_EXEC_REP4(n+4)
#define SYS_INT_EXEC_REP16(n)  SYS_INT_EXEC_REP8(n) SYS_INT_EXEC_REP8(n+8)
#define SYS_INT_EXEC_REP32(n)  SYS_INT_EXEC_REP16(n) SYS_INT_EXEC_REP16(n+16)
#define SYS_INT_EXEC_REP64(n)  SYS_INT_EXEC_REP32(n) SYS_INT_EXEC_REP32(n+32)
#define SYS_INT_EXEC_REP128(n) SYS_INT_EXEC_REP64(n) SYS_INT_EXEC_REP64(n+64)
#define SYS_INT_EXEC_REP256(n) SYS_INT_EXEC_REP128(n) SYS_INT_EXEC_REP128(n+128)

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

static void err_printf(const char* format, ...) {
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

// Dump general purpose registers for deeper debugging
static inline void dump_register_info(syscalls_registers_x86_32_t* regs) {
    err_printf("--- Register values ---\n");
    err_printf("EAX: %lx EBX: %lx ECX: %lx EDX: %lx\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    err_printf("EDI: %lx ESI: %lx EBP: %lx ESP: %lx\n", regs->edi, regs->esi, regs->ebp, regs->esp_dummy);
}

// Dumps multitasking information in case of race condition errors
static inline void dump_multitasking_info() {
    if (current_task == NULL) return;

    err_printf("--- Multitasking ---\n");
    err_printf("Name:  %s (ID:%lu)\n", current_task->name, current_task->id);
    err_printf("Stack: 0x%08lx -> 0x%08lx\n", (uint32_t) current_task->stack_origin, current_task->stack_pointer);
    err_printf("Page:  %p (PRIV:%lu)\n", (void*) current_task->page_directory, (uint32_t) current_task->privilege);
}

static void panic_cmd_dump(uint32_t addr) {
    uint8_t* ptr = (uint8_t*) addr;
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) err_printf("\n%08lx: ", (uint32_t)(ptr + i));
        err_printf("%02x ", ptr[i]);
    }
    err_printf("\r\n");
}

static void panic_cmd_peek(uint32_t addr) {
    uint32_t value = *(volatile uint32_t*)addr;
    err_printf("\n0x%08lx = 0x%08lx", addr, value);
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
        err_printf("Rebooting...\r\n");
        outb(0x64, 0xFE); // Pulse CPU reset line
    } else if (
        cmd[0] == 'h' &&
        cmd[1] == 'e' &&
        cmd[2] == 'l' &&
        cmd[3] == 'p'
    ) {
        err_printf("\ndump <hex>\npeek <addr>\nreboot");
    } else err_printf("\nUnknown command: %s (use help)", cmd);
}

// TODO IMP: Move panic shell out into dedicated file
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
void exception_handler(syscalls_registers_x86_32_t* regs) {
    asm volatile("cli");

    // BSOD
    terminal_change_color(0x1F); // Blue background, White foreground
    terminal_clear();

    if (regs->int_no < 32) {
        err_printf("Exception: %ld (%s)\n", regs->int_no, exception_messages[regs->int_no]);
    } else {
        err_printf("Exception: %ld\n", regs->int_no);
    }

    err_printf("Error Code: %lx\n", regs->err_code);
    err_printf("EIP: %lx  CS: %lx  EFLAGS: %lx\n", regs->eip, regs->cs, regs->eflags);

    switch (regs->int_no) {
        case 13: { // GPF
            err_printf("Selector: %lx (%s)\n", regs->err_code & 0xFFF8,
                    (regs->err_code & 0x04) ? "LDT" : "GDT");
            break;
        }

        case 14: { // Page fault
            uint32_t faulting_address;
            asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

            err_printf("Faulting Address (CR2): %lx\n", faulting_address);
            err_printf("Reason: %s, %s, %s\n",
                (regs->err_code & PAGE_PRESENT) ? "Page unaccessible" : "Non-present page",
                (regs->err_code & PAGE_RW)      ? "Write fault" : "Read fault",
                (regs->err_code & PAGE_USER)    ? "User-mode" : "Kernel-mode");

            break;
        }
    }

    err_printf("Last init: %s\n", last_init);
    err_printf("Last call: %s\n", last_call);

    dump_register_info(regs);
    dump_multitasking_info();

    panic_shell();
}

void syscall_handler(syscalls_registers_x86_32_t* regs) {
    // EAX: Caller function ID, also return value (if any)
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;

    switch (regs->eax) {
        case SYS_EXIT: {
            _exit(arg1);
            break;
        }

        case SYS_READ: {
            regs->eax = (uint32_t) _read((int) arg1, (char*) arg2, (int) arg3);
            break;
        }

        case SYS_WRITE: {
            regs->eax = (uint32_t) _write((int) arg1, (char*) arg2, (int) arg3);
            break;
        }

        case SYS_OPEN: {
            regs->eax = (uint32_t) _open((const char*) arg1, (int) arg2, (int) arg3);
            break;
        }

        case SYS_CLOSE: {
            regs->eax = (uint32_t) _close((int) arg1);
            break;
        }

        case SYS_EXEC: {
            regs->eax = (uint32_t) exec_elf((const char*) arg1) ? SYS_DONE : SYS_ERROR;
            break;
        }

        case SYS_MKDIR: {
            regs->eax = (uint32_t) _mkdir((const char*) arg1, (mode_t) arg2);
            break;
        }

        case SYS_REMOVE: {
            regs->eax = (uint32_t) (fs_remove((const char*) arg1) ? SYS_DONE : SYS_ERROR);
            break;
        }

        case SYS_LSEEK: {
            regs->eax = (uint32_t) _lseek((int) arg1, (int) arg2, (int) arg3);
            break;
        }

        case SYS_GETPID: {
            regs->eax = (uint32_t) _getpid();
            break;
        }

        case SYS_KILL: {
            regs->eax = (uint32_t) _kill((int) arg1, (int) arg2);
            break;
        }

        case SYS_SBRK: {
            regs->eax = (uint32_t) _sbrk((int) arg1);
            break;
        }

        case SYS_ISATTY: {
            regs->eax = (uint32_t) _isatty((int) arg1);
            break;
        }

        case SYS_FSTAT: {
            regs->eax = (uint32_t) _fstat((int) arg1, (struct stat*) arg2);
            break;
        }

        case SYS_DIRSCAN: {
            FileData* buf = (FileData*) arg2;
            size_t count  = (size_t) arg3;

            FileNode* head = fs_getall((const char*) arg1);
            FileNode* temp = NULL;

            if (!head) {
                regs->eax = 0;
                break;
            }

            size_t total_count = 0;

            for (size_t i = 0; i < count; i++) {
                buf[i].isdir = head->file.is_directory;
                strncpy(buf[i].name, head->file.name, SYSCALL_FILENAME_LEN - 1);

                temp = head->next;
                kfree(head);
                head = temp;

                total_count++;

                if (!head) break;
            }

            // Cleanup unused nodes
            while (head) {
                temp = head->next;
                kfree(head);
                head = temp;
            }

            regs->eax = total_count;
            break;
        }

        case SYS_UART_PUT: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            uart_print((const char*) arg1);
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_HEAP: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            HeapData* buf = (HeapData*) arg1;
            int max_entries = (int) arg2;
            int count = 0;

            HeapSegment* current = first_segment;

            while (current != NULL && count < max_entries) {
                buf[count].address = (uint32_t) current;
                buf[count].size    = current->size;
                buf[count].is_free = current->is_free;
                buf[count].caller  = current->caller;

                current = current->next;
                count++;
            }

            regs->eax = count;
            break;
        }

        case SYS_GET_HEAP_SEG_SIZE: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = sizeof(HeapSegment);
            break;
        }

        case SYS_GET_HEAP_START: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = (uint32_t) heap_start;
            break;
        }

        case SYS_GET_HEAP_END: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = (uint32_t) heap_end;
            break;
        }

        case SYS_HEAP_AUDIT: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            uint32_t* fault_addr_out = (uint32_t*) arg1;
            HeapSegment* current = first_segment;
            int res_code = 0; // Default: OK

            while (current != NULL) {
                if (current->magic != HEAP_MAGIC) { // Bad Magic
                    res_code = 1; break;
                }

                if (((uint32_t) current & 0x3) != 0) { // Unaligned segment pointer
                    res_code = 2; break;
                }

                if (current->next != NULL) {
                    if (current->next <= current) { // Circular or backwards link
                        res_code = 3; break;
                    }
                    if (current->next->prev != current) { // Broken backlink
                        res_code = 4; break;
                    }
                }

                current = current->next;
            }

            // If error, write fault address
            if (res_code != 0 && fault_addr_out != NULL) {
                *fault_addr_out = (uint32_t) current;
            }

            regs->eax = res_code;
            break;
        }

        case SYS_INT_EXEC: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            switch((int) arg1) {
                SYS_INT_EXEC_REP256(0)
                default:
                    regs->eax = SYS_ERROR;
                    break;
            }
            break;
        }

        case SYS_INT_ON: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            system_int_on();
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_INT_OFF: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            system_int_off();
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_TASK_INFO: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            uint32_t  pid = (uint32_t)  arg1;
            TaskData* out = (TaskData*) arg2;

            task* t = get_task(pid);

            if (!t) { regs->eax = SYS_ERROR; break; }

            out->id            = t->id;
            out->state         = t->state;
            out->parent_id     = t->parent   ?   t->parent->id : 0;
            out->next_id       = t->next     ?     t->next->id : 0;
            out->neighbor_id   = t->neighbor ? t->neighbor->id : 0;
            out->stack_ptr     = t->stack_pointer;
            out->stack_origin  = (uint32_t) t->stack_origin;
            out->page_dir      = (uint32_t) t->page_directory;
            strncpy(out->name, t->name, 31);

            // Snapshot registers from the saved stack
            task_registers_t* kregs = (task_registers_t*) t->stack_pointer;
            out->eax = kregs->eax;
            out->ebx = kregs->ebx;
            out->ecx = kregs->ecx;
            out->edx = kregs->edx;
            out->esi = kregs->esi;
            out->edi = kregs->edi;
            out->eip = kregs->eip;
            out->ebp = kregs->ebp;

            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_TASK_LIST: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            task_list* tasklist = first_task_list;
            for (size_t i = 0; i < (size_t) arg1; i++) {
                tasklist = tasklist->next;
                if (tasklist == NULL) {
                    regs->eax = SYS_ERROR;
                    break;
                }
            }
            if (tasklist == NULL) break;

            TaskListData* out = (TaskListData*) arg2;

            for (size_t i = 0; i < TASKS_LIST_LEN; i++) {
                if (tasklist->tasks[i] != NULL) {
                    out->pids[i] = tasklist->tasks[i]->id;
                } else {
                    out->pids[i] = NULL;
                }
            }
            out->mask = tasklist->mask;

            break;
        }

        case SYS_TASK_KILL: {
            if (current_task->privilege != PRIV_SUPER) {
                regs->eax = SYS_ERROR;
                break;
            }

            task* t = get_task(arg1);
            kill_task(t->id);

            break;
        }

        default: {
            printf("Unknown syscall (%s): %ld\n", current_task->name, regs->eax);
            regs->eax = SYS_ERROR;
            break;
        }
    }
}
