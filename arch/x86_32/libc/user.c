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
#include <sys/stat.h>
#include <sys/types.h>

#include "farix.h"

void* sbrk(int incr) { return _sbrk(incr); }
int read(int file, char *ptr, int len) { return _read(file, ptr, len); }
int write(int file, char *ptr, int len) { return _write(file, ptr, len); }
int open(const char *name, int flags, int mode) { return _open(name, flags, mode); }
int close(int file) { return _close(file); }
int lseek(int file, int ptr, int dir) { return _lseek(file, ptr, dir); }
int fstat(int file, struct stat *st) { return _fstat(file, st); }
int isatty(int file) { return _isatty(file); }
int kill(int pid, int sig) { return _kill(pid, sig); }
int getpid() { return _getpid(); }

// Writing inline assembly is tedious, this function just abstract that off.
// Unused arguments are set to 0
static int32_t farix_syscall(uint32_t sys_id, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int32_t ret;
    asm volatile (
        "mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "int $0x80\n"
        : "=a"(ret)
        : "g"(sys_id), "g"(arg1), "g"(arg2), "g"(arg3)
        : "ebx", "ecx", "edx"
    );
    return ret;
}

// Since we do not compile this file with the rest of the kernel, defining the
// newlib stubs here does not cause multiple definitions error.

void* _sbrk(int incr) {
    return (void*) farix_syscall(SYS_SBRK, (uint32_t) incr, 0, 0);
}

void _exit(int status) {
    farix_syscall(SYS_EXIT, (uint32_t) status, 0, 0);
}

int _read(int file, char *ptr, int len) {
    return farix_syscall(SYS_READ, (uint32_t) file, (uint32_t) ptr, (uint32_t) len);
}

int _write(int file, char *ptr, int len) {
    return farix_syscall(SYS_WRITE, (uint32_t) file, (uint32_t) ptr, (uint32_t) len);
}

int _open(const char *name, int flags, int mode) {
    return farix_syscall(SYS_OPEN, (uint32_t) name, (uint32_t) flags, (uint32_t) mode);
}

int _close(int file) {
    return farix_syscall(SYS_CLOSE, (uint32_t) file, 0, 0);
}

int _fstat(int file, struct stat *st) {
    return farix_syscall(SYS_FSTAT, (uint32_t) file, (uint32_t) st, 0);
}

int _isatty(int file) {
    return farix_syscall(SYS_ISATTY, (uint32_t) file, 0, 0);
}

int _lseek(int file, int ptr, int dir) {
    return farix_syscall(SYS_LSEEK, (uint32_t) file, (uint32_t) ptr, (uint32_t) dir);
}

int _getpid() {
    return farix_syscall(SYS_GETPID, 0, 0, 0);
}

int _kill(int pid, int sig) {
    return farix_syscall(SYS_KILL, (uint32_t) pid, (uint32_t) sig, 0);
}

// Super User functions

int UART_PUTS(const char *data) {
    return farix_syscall(SYS_UART_PUT, (uint32_t) data, 0, 0);
}

int GET_HEAP_DATA(HeapData* buffer, int max_count) {
    return farix_syscall(SYS_GET_HEAP, (uint32_t) buffer, (uint32_t) max_count, 0);
}

int GET_HEAP_SEG_SIZE() {
    return farix_syscall(SYS_GET_HEAP_SEG_SIZE, 0, 0, 0);
}

int GET_HEAP_START() {
    return farix_syscall(SYS_GET_HEAP_START, 0, 0, 0);
}

int GET_HEAP_END() {
    return farix_syscall(SYS_GET_HEAP_END, 0, 0, 0);
}

int HEAP_AUDIT(int *fault_addr) {
    return farix_syscall(SYS_HEAP_AUDIT, (uint32_t) fault_addr, 0, 0);
}

int SYSTEM_INT_EXEC(int int_id) {
    return farix_syscall(SYS_INT_EXEC, int_id, 0, 0);
}

int SYSTEM_INT_ON() {
    return farix_syscall(SYS_INT_ON, 0, 0, 0);
}

int SYSTEM_INT_OFF() {
    return farix_syscall(SYS_INT_OFF, 0, 0, 0);
}

int GET_TASK_DATA(int pid, TaskData* buffer) {
    return farix_syscall(SYS_GET_TASK_INFO, (uint32_t) pid, (uint32_t) buffer, 0);
}

int GET_TASK_LIST(int list_id, TaskListData* buffer) {
    return farix_syscall(SYS_GET_TASK_LIST, (uint32_t) list_id, (uint32_t) buffer, 0);
}

int TASK_KILL(int pid) {
    return farix_syscall(SYS_TASK_KILL, (uint32_t) pid, 0, 0);
}
