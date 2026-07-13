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

#include <stdint.h>

#include "klib/string.h"

#include "hal.h"

#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "fs/types/elf.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "farix.h"

/* Handles system calls using EAX, EBX, ECX, and EDX registers */
void syscall_handler(syscalls_registers_x86_32_t* regs) {
    // EAX: Caller function ID, also return value (if any)
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    uint32_t arg5 = regs->edi;

    switch (regs->eax) {
        // TODO: Can be improved with fs_getall(path, count, offset)
        // Getting everything, including what we don't need, then freeing
        // them one-by-one is quite a waste.
        case SYS_DIRSCAN: {
            FileData* buf = (FileData*) arg2;
            size_t count  = (size_t) arg3;

            FileNode* head = fs_getall((const char*) arg1);
            FileNode* temp = NULL;

            if (unlikely(!head)) {
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

                if (unlikely(!head)) break;
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
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            uart_print((const char*) arg1);
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_HEAP: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
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
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = sizeof(HeapSegment);
            break;
        }

        case SYS_GET_HEAP_START: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = (uint32_t) heap_start;
            break;
        }

        case SYS_GET_HEAP_END: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            regs->eax = (uint32_t) heap_end;
            break;
        }

        case SYS_HEAP_AUDIT: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            uint32_t* fault_addr_out = (uint32_t*) arg1;
            HeapSegment* current = first_segment;
            int res_code = 0; // Default: OK

            while (current != NULL) {
                if (unlikely(current->magic != HEAP_MAGIC)) { // Bad Magic
                    res_code = 1;
                    break;
                }

                if (unlikely(((uint32_t) current & 0x3) != 0)) { // Unaligned segment pointer
                    res_code = 2;
                    break;
                }

                if (unlikely(current->next != NULL)) {
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
            if (unlikely(res_code != 0 && fault_addr_out != NULL)) {
                *fault_addr_out = (uint32_t) current;
            }

            regs->eax = res_code;
            break;
        }

        case SYS_INT_EXEC: {
            #define SYS_INT_EXEC_CASE(n) case n: asm volatile("int %0" :: "i"(n)); break;
            #define SYS_INT_EXEC_REP2(n)   SYS_INT_EXEC_CASE(n) SYS_INT_EXEC_CASE(n+1)
            #define SYS_INT_EXEC_REP4(n)   SYS_INT_EXEC_REP2(n) SYS_INT_EXEC_REP2(n+2)
            #define SYS_INT_EXEC_REP8(n)   SYS_INT_EXEC_REP4(n) SYS_INT_EXEC_REP4(n+4)
            #define SYS_INT_EXEC_REP16(n)  SYS_INT_EXEC_REP8(n) SYS_INT_EXEC_REP8(n+8)
            #define SYS_INT_EXEC_REP32(n)  SYS_INT_EXEC_REP16(n) SYS_INT_EXEC_REP16(n+16)
            #define SYS_INT_EXEC_REP64(n)  SYS_INT_EXEC_REP32(n) SYS_INT_EXEC_REP32(n+32)
            #define SYS_INT_EXEC_REP128(n) SYS_INT_EXEC_REP64(n) SYS_INT_EXEC_REP64(n+64)
            #define SYS_INT_EXEC_REP256(n) SYS_INT_EXEC_REP128(n) SYS_INT_EXEC_REP128(n+128)

            if (unlikely(current_task->privilege != PRIV_SUPER)) {
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

            #undef SYS_INT_EXEC_CASE
            #undef SYS_INT_EXEC_REP2
            #undef SYS_INT_EXEC_REP4
            #undef SYS_INT_EXEC_REP8
            #undef SYS_INT_EXEC_REP16
            #undef SYS_INT_EXEC_REP32
            #undef SYS_INT_EXEC_REP64
            #undef SYS_INT_EXEC_REP128
            #undef SYS_INT_EXEC_REP256
        }

        case SYS_INT_ON: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            system_int_on();
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_INT_OFF: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            system_int_off();
            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_TASK_INFO: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            uint32_t  pid = (uint32_t)  arg1;
            TaskData* out = (TaskData*) arg2;

            task* t = get_task(pid);

            if (unlikely(!t)) {
                regs->eax = SYS_ERROR;
                break;
            }

            out->id            = t->id;
            out->state         = t->state;
            out->parent_id     = t->parent   ?   t->parent->id : 0;
            out->next_id       = t->next     ?     t->next->id : 0;
            out->neighbor_id   = t->neighbor ? t->neighbor->id : 0;
            out->stack_ptr     = t->stack_pointer;
            out->stack_origin  = (uint32_t) t->stack_origin;
            out->page_dir      = (uint32_t) t->page_directory;
            strncpy(out->name, t->name, 31);

            regs->eax = SYS_DONE;
            break;
        }

        case SYS_GET_TASK_LIST: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            task_list* tasklist = first_task_list;
            for (size_t i = 0; i < (size_t) arg1; i++) {
                tasklist = tasklist->next;
                if (unlikely(tasklist == NULL)) {
                    regs->eax = SYS_ERROR;
                    break;
                }
            }
            if (unlikely(tasklist == NULL)) break;

            TaskListData* out = (TaskListData*) arg2;

            for (size_t i = 0; i < TASKS_LIST_LEN; i++) {
                if (tasklist->tasks[i] != NULL) {
                    out->pids[i] = tasklist->tasks[i]->id;
                } else {
                    out->pids[i] = 0;
                }
            }
            out->mask = tasklist->mask;

            break;
        }

        case SYS_TASK_KILL: {
            if (unlikely(current_task->privilege != PRIV_SUPER)) {
                regs->eax = SYS_ERROR;
                break;
            }

            task* t = get_task((uint32_t) arg1);
            kill_task(t->id);

            break;
        }

        default: {
            err_printf("Unknown syscall (%s): %d", current_task->name, regs->eax);
            err_printf(" | EBX: %x ECX: %x EDX: %x", arg1, arg2, arg3);
            err_printf(" | ESI: %x EDI %x", arg4, arg5);
            regs->eax = SYS_ERROR;
            break;
        }
    }
}
