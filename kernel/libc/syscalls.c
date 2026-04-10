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

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arch/stubs.h"
#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "libc/syscalls.h"

// A table of strings representing the names of open files
static const char* fd_table[20] = {NULL};

int _close(UNUSED_ARG int file) { return -1; }
int _fstat(UNUSED_ARG int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(UNUSED_ARG int file) { return 1; }
int _lseek(UNUSED_ARG int file, UNUSED_ARG int ptr, UNUSED_ARG int dir) { return 0; }
int _getpid() { return 1; }
int _kill(UNUSED_ARG int pid, UNUSED_ARG int sig) { return -1; }

void* _sbrk(int incr) {
    if (current_task->page_directory == NULL) {
        return kmalloc(incr);
    }

    uint32_t old_break = current_task->heap_break;
    uint32_t new_break = old_break + incr;

    if (incr > 0) {
        uint32_t start_page = (old_break + PAGE_SIZE - 1) & ~0xFFF;
        uint32_t end_page   = (new_break + PAGE_SIZE - 1) & ~0xFFF;

        for (uint32_t v = start_page; v < end_page; v += 4096) {
            void* phys = pmm_alloc_page();
            vmm_map_page(vmm_get_current_directory(), phys, (void*) v,
                            PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_CACHE);

            kmemset(PHYSICAL_TO_VIRTUAL(phys), 0, 4096);
        }
    }

    current_task->heap_break = new_break;
    return (void*) old_break;
}

void _exit(UNUSED_ARG int status) {
    current_task->state = TASK_DEAD;
    schedule();

    while(1);
}

int _read(int file, char *ptr, int len) {
    if (file == 0) { // stdin
        int i = 0;
        while (i < len) {
            while (kbd_head == kbd_tail)
                system_halt();

            char c = kbd_buffer[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;
            ptr[i++] = c;

            if (c == '\n') break; // Stop reading at newline
        }
        return i;
    }

    else if (file >= 3 && file < 20 && fd_table[file] != NULL) {
        // fs_read returns bool, Newlib wants bytes read
        if (fs_read(fd_table[file], ptr, len)) {
            return len;
        }
    }

    return -1;
}

int _write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) { // stdout/stderr
        // For debugging:
        // uart_print(ptr);
        echo_raw(ptr, len);
        return len;
    }

    else if (file >= 3 && file < 20 && fd_table[file] != NULL) {
        if (fs_write(fd_table[file], ptr, len)) {
            return len;
        }
    }

    errno = EBADF;
    return -1;
}

int _open(const char *name, UNUSED_ARG int flags, UNUSED_ARG int mode) {
    File* f = fs_get(name);

    if (!f) {
        errno = ENOENT;
        return -1;
    }

    for (int i = 3; i < 20; i++) {
        if (fd_table[i] == NULL) {
            fd_table[i] = name;
            return i;
        }
    }

    errno = EMFILE;
    return -1;
}

int getentropy(void *ptr, size_t len) {
    if (len > 256) {
        errno = EIO;
        return -1;
    }

    uint8_t *buf = (uint8_t *)ptr;
    size_t i = 0;

    while (i < len) {
        unsigned char ok;
        uint32_t random_val = asm_get_random(&ok);

        if (ok) {
            // Copy bytes from the 32-bit result into the buffer
            for (int j = 0; j < 4 && i < len; j++) {
                buf[i++] = (uint8_t)(random_val >> (j << 3));
            }
        } else {
            // If the hardware pool is exhausted, pause briefly
            system_pause();
        }
    }
    return 0;
}

void* sbrk(int incr) { return _sbrk(incr); }
int read(int file, char *ptr, int len) { return _read(file, ptr, len); }
int write(int file, char *ptr, int len) { return _write(file, ptr, len); }
int close(int file) { return _close(file); }
int lseek(int file, int ptr, int dir) { return _lseek(file, ptr, dir); }
int fstat(int file, struct stat *st) { return _fstat(file, st); }
int isatty(int file) { return _isatty(file); }
int kill(int pid, int sig) { return _kill(pid, sig); }
int getpid() { return _getpid(); }

void _free_r(UNUSED_ARG struct _reent *r, void *ptr) {
    kfree(ptr);
}

void* memcpy(void* dest, const void* src, size_t n) {
    kmemcpy(dest, src, n);
    return dest;
}

void* memset(void* s, int c, size_t n) {
    kmemset(s, c, n);
    return s;
}

// Some strings calls memmove
void* memmove(void* dest, const void* src, size_t n) {
    if (dest < src) return memcpy(dest, src, n);
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    return dest;
}
