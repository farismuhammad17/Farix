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

#include "farix.h"

// For debugging:
// #include "drivers/uart.h"

#define FD_TABLE_MIN 3
#define FD_TABLE_MAX 32

typedef struct {
    File* file;
    uint32_t pos;
} FileOpenHandle;

// A table of strings representing the names of open files
static FileOpenHandle* fd_table[FD_TABLE_MAX] = {NULL};

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
    // TODO: Kill tasks using dedicated task* killer
    // kill_task wastes time finding the task we already have.

    kill_task(current_task->id);
    while(1);
}

int _read(int file, char *ptr, int len) {
    if (file == 0) { // stdin
        int i = 0;
        while (i < len) {
            while (kbd_head == kbd_tail) {
                current_task->state = TASK_SLEEPING;
                task_yield();
            }

            char c = kbd_buffer[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;
            ptr[i++] = c;

            if (c == '\n') break; // Stop reading at newline
        }
        return i;
    }

    else if (file >= 3 && file < 20 && fd_table[file] != NULL) {
        // fs_read returns bool, Newlib wants bytes read
        if (fs_read(fd_table[file]->file->name, ptr, len)) { // TODO: Read directly in the future
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
        if (fs_write(fd_table[file]->file->name, ptr, len)) {
            return len;
        }
    }

    return -1;
}

int _open(const char *name, UNUSED_ARG int flags, UNUSED_ARG int mode) {
    File* f = fs_get(name);

    if (!f) return -1;

    for (int i = FD_TABLE_MIN; i < FD_TABLE_MAX; i++) {
        if (fd_table[i] == NULL) {
            FileOpenHandle* handle = kmalloc(sizeof(FileOpenHandle));
            handle->file = f;
            handle->pos = 0;

            fd_table[i] = handle;
            return i;
        }
    }

    return -1;
}

int _close(int file) {
    if (file < FD_TABLE_MIN || file >= FD_TABLE_MAX || !fd_table[file]) return -1;

    kfree((void*) fd_table[file]);
    fd_table[file] = NULL;
    return 0;
}

int _fstat(int file, struct stat *st) {
    if (!st) return -EFAULT;

    kmemset(st, 0, sizeof(struct stat));

    if (file >= 0 && file < FD_TABLE_MIN) {
        st->st_mode = S_IFCHR;  // Terminal
        st->st_dev = 0;         // TODO: Device ID
        return 0;
    }

    // Handle actual files
    if (file < FD_TABLE_MAX && fd_table[file] != NULL) {
        FileOpenHandle* f = (FileOpenHandle*) fd_table[file];

        st->st_mode    = S_IFREG;
        st->st_size    = f->file->size;
        st->st_blksize = PAGE_SIZE;  // Optimal I/O chunk (one page)

        return 0;
    }

    // If we get here, the ticket (FD) is invalid or empty
    return -EBADF;
}

int _isatty(int file) {
    return file >= 0 && file < FD_TABLE_MIN;
}

int _lseek(int file, int ptr, int dir) {
    if (file >= 0 && file < FD_TABLE_MIN) return 0;

    if (file < FD_TABLE_MAX && fd_table[file]) {
        FileOpenHandle* f = (FileOpenHandle*) fd_table[file];
        size_t f_size = f->file->size;
        int new_pos;

        if (dir == 0)      new_pos = ptr;          // SEEK_SET
        else if (dir == 1) new_pos = f->pos + ptr; // SEEK_CUR
        else if (dir == 2) new_pos = f_size + ptr; // SEEK_END
        else return -EINVAL;

        // Clamp to file boundaries
        if (new_pos < 0) new_pos = 0;
        else if (new_pos > (int) f_size) new_pos = f_size;

        f->pos = new_pos;
        return f->pos;
    }

    return -EBADF;
}

int _getpid() {
    return current_task->id;
}

int _kill(int pid, UNUSED_ARG int sig) {
    kill_task(pid);
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
