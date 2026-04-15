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
#include <sys/types.h>

#include "farix.h"

// Writing inline assembly is tedious, this function just abstract that off.
// Unused arguments are set to 0
static int32_t syscall(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int32_t ret;
    asm volatile (
        "mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "int $0x80\n"
        : "=a"(ret)
        : "g"(num), "g"(arg1), "g"(arg2), "g"(arg3)
        : "ebx", "ecx", "edx"
    );
    return ret;
}

// Since we do not compile this file with the rest of the kernel, defining the
// newlib stubs here does not cause multiple definitions error.

void* _sbrk(int incr) {
    return (void*) syscall(SYS_SBRK, (uint32_t) incr, 0, 0);
}

void _exit(int status) {
    syscall(SYS_EXIT, (uint32_t) status, 0, 0);
    while(1); // Should never reach here
}

int _read(int file, char *ptr, int len) {
    return syscall(SYS_READ, (uint32_t) file, (uint32_t) ptr, (uint32_t) len);
}

int _write(int file, char *ptr, int len) {
    return syscall(SYS_WRITE, (uint32_t) file, (uint32_t) ptr, (uint32_t) len);
}

int _close(UNUSED_ARG int file) { return -1; }
int _fstat(UNUSED_ARG int file, struct stat *st) { return 0; }
int _isatty(UNUSED_ARG int file) { return 1; }
int _lseek(UNUSED_ARG int file, UNUSED_ARG int ptr, UNUSED_ARG int dir) { return 0; }
int _getpid() { return 1; }
int _kill(UNUSED_ARG int pid, UNUSED_ARG int sig) { return -1; }

void* sbrk(int incr) { return _sbrk(incr); }
int read(int file, char *ptr, int len) { return _read(file, ptr, len); }
int write(int file, char *ptr, int len) { return _write(file, ptr, len); }
int close(int file) { return _close(file); }
int lseek(int file, int ptr, int dir) { return _lseek(file, ptr, dir); }
int fstat(int file, struct stat *st) { return _fstat(file, st); }
int isatty(int file) { return _isatty(file); }
int kill(int pid, int sig) { return _kill(pid, sig); }
int getpid() { return _getpid(); }
