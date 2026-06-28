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
#include <sys/stat.h>
#include <sys/types.h>

#include "farix.h"

// Custom user functions

int fx_dirscan(char* path, FileData* buffer, size_t count) {
    return farix_syscall(SYS_DIRSCAN, (uint32_t) path, (uint32_t) buffer, (uint32_t) count, 0, 0);
}

// Super User functions

int UART_PUTS(const char *data) {
    return farix_syscall(SYS_UART_PUT, (uint32_t) data, 0, 0, 0, 0);
}

int GET_HEAP_DATA(HeapData* buffer, int max_count) {
    return farix_syscall(SYS_GET_HEAP, (uint32_t) buffer, (uint32_t) max_count, 0, 0, 0);
}

int GET_HEAP_SEG_SIZE() {
    return farix_syscall(SYS_GET_HEAP_SEG_SIZE, 0, 0, 0, 0, 0);
}

int GET_HEAP_START() {
    return farix_syscall(SYS_GET_HEAP_START, 0, 0, 0, 0, 0);
}

int GET_HEAP_END() {
    return farix_syscall(SYS_GET_HEAP_END, 0, 0, 0, 0, 0);
}

int HEAP_AUDIT(int *fault_addr) {
    return farix_syscall(SYS_HEAP_AUDIT, (uint32_t) fault_addr, 0, 0, 0, 0);
}

int SYSTEM_INT_EXEC(int int_id) {
    return farix_syscall(SYS_INT_EXEC, int_id, 0, 0, 0, 0);
}

int SYSTEM_INT_ON() {
    return farix_syscall(SYS_INT_ON, 0, 0, 0, 0, 0);
}

int SYSTEM_INT_OFF() {
    return farix_syscall(SYS_INT_OFF, 0, 0, 0, 0, 0);
}

int GET_TASK_DATA(int pid, TaskData* buffer) {
    return farix_syscall(SYS_GET_TASK_INFO, (uint32_t) pid, (uint32_t) buffer, 0, 0, 0);
}

int GET_TASK_LIST(int list_id, TaskListData* buffer) {
    return farix_syscall(SYS_GET_TASK_LIST, (uint32_t) list_id, (uint32_t) buffer, 0, 0, 0);
}

int TASK_KILL(int pid) {
    return farix_syscall(SYS_TASK_KILL, (uint32_t) pid, 0, 0, 0, 0);
}
