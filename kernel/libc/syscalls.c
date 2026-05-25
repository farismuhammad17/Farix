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
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "hal.h"

#include "cpu/multicore.h"
#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "farix.h"

#define FD_TABLE_MIN 3
#define FD_TABLE_MAX 32

typedef struct {
    File* file;
    uint32_t pos;
} FileOpenHandle;

// A table of strings representing the names of open files
static FileOpenHandle* fd_table[FD_TABLE_MAX] = {NULL};

static spinlock heap_lock = 0;
static spinlock fd_lock   = 0;
static spinlock kbd_lock  = 0;
static spinlock log_lock  = 0;

/*
System break, called when more memory is required. Calls `kmalloc` if it is for
the kernel, and carves out memory for anything else.
*/
void* _sbrk(int incr) {
    spin_lock(&heap_lock);

    if (current_task->page_directory == NULL) {
        void* res = kmalloc(incr);
        spin_unlock(&heap_lock);
        return res;
    }

    uint32_t old_break = current_task->heap_break;
    uint32_t new_break = old_break + incr;

    if (incr > 0) {
        uint32_t start_page = (old_break + PAGE_SIZE - 1) & ~0xFFF;
        uint32_t end_page   = (new_break + PAGE_SIZE - 1) & ~0xFFF;

        for (uint32_t v = start_page; v < end_page; v += PAGE_SIZE) {
            void* phys = pmm_alloc_page();
            vmm_map_page(vmm_get_current_directory(), phys, (void*) v,
                            PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_CACHE);

            kmemset(PHYSICAL_TO_VIRTUAL(phys), 0, PAGE_SIZE);
        }
    }

    current_task->heap_break = new_break;

    spin_unlock(&heap_lock);
    return (void*) old_break;
}

/* Kill the current executing task */
void _exit(int status) {
    // TODO: Kill tasks using dedicated task* killer
    // kill_task wastes time finding the task we already have.

    kill_task(current_task->id);
    while(1) system_pause();
}

/* Read a file or take user input */
int _read(int file, char *ptr, int len) {
    if (file == 0) { // stdin
        int i = 0;
        while (i < len - 1) {
            spin_lock(&kbd_lock);

            while (kbd_head == kbd_tail) {
                spin_unlock(&kbd_lock);

                current_task->state = TASK_SLEEPING;
                task_yield();

                spin_lock(&kbd_lock);
            }

            char c = kbd_buffer[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;

            spin_unlock(&kbd_lock);

            if (c == '\b') {
                if (i > 0) {
                    i--;
                    echo_char('\b');
                }
                continue;
            }

            // Store and Echo
            ptr[i++] = c;
            echo_char(c);

            if (c == '\n' || c == '\r') break;
        }
        return i;
    }

    else if (file >= 3 && file < 20) {
        spin_lock(&fd_lock);

        if (fd_table[file] != NULL) {
            FileOpenHandle* h = (FileOpenHandle*) fd_table[file];

            const char* filename = h->file->name;
            uint32_t current_pos = h->pos;

            spin_unlock(&fd_lock);

            if (fs_read(filename, ptr, len, current_pos)) {
                return len;
            }
            return -1;
        }

        spin_unlock(&fd_lock);
    }

    return -1;
}

/* Write to file or print data */
int _write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) { // stdout/stderr
        spin_lock(&log_lock);
        echo_raw(ptr, len);
        spin_unlock(&log_lock);

        return len;
    }

    else if (file >= 3 && file < 20) {
        spin_lock(&fd_lock);

        if (fd_table[file] != NULL) {
            FileOpenHandle* h = (FileOpenHandle*) fd_table[file];

            const char* filename = h->file->name;
            uint32_t current_pos = h->pos;

            spin_unlock(&fd_lock);

            if (fs_write(filename, ptr, len, current_pos)) {
                return len;
            }
            return -1;
        }

        spin_unlock(&fd_lock);
    }

    return -1;
}

/* Open file */
int _open(const char *name, int flags, int mode) {
    File* f = fs_get(name);

    if (unlikely(!f)) {
        if ((flags & O_CREAT) && fs_create(name)) {
            // TODO: Make fs_create write into a File* to use,
            // instead of looking up the same file that was just made
            f = fs_get(name);
        } else {
            return -1; // No O_CREAT, file wasn't found, or disk full
        }
    }

    spin_lock(&fd_lock);

    for (int i = FD_TABLE_MIN; i < FD_TABLE_MAX; i++) {
        if (fd_table[i] == NULL) {
            FileOpenHandle* handle = kmalloc(sizeof(FileOpenHandle));
            handle->file = f;
            handle->pos = 0;

            fd_table[i] = handle;

            spin_unlock(&fd_lock);
            return i;
        }
    }

    spin_unlock(&fd_lock);

    return -1;
}

/* Close file */
int _close(int file) {
    if (unlikely(file < FD_TABLE_MIN || file >= FD_TABLE_MAX)) return -1;

    spin_lock(&fd_lock);

    if (!fd_table[file]) {
        spin_unlock(&fd_lock);
        return -1;
    }

    void* handle = (void*) fd_table[file];
    fd_table[file] = NULL;

    spin_unlock(&fd_lock);

    kfree(handle);

    return 0;
}

/* Create directory */
int _mkdir(const char *path, mode_t mode) {
    return fs_mkdir(path) ? SYS_DONE : SYS_ERROR;
}

/* Return file status */
int _fstat(int file, struct stat *st) {
    if (unlikely(!st)) return -EFAULT;

    kmemset(st, 0, sizeof(struct stat));

    if (file >= 0 && file < FD_TABLE_MIN) {
        st->st_mode = S_IFCHR;  // Terminal
        st->st_dev = 0;         // TODO: Device ID
        return 0;
    }

    spin_lock(&fd_lock);

    if (file < FD_TABLE_MAX && fd_table[file] != NULL) {
        FileOpenHandle* f = (FileOpenHandle*) fd_table[file];

        st->st_mode    = S_IFREG;
        st->st_size    = f->file->size;
        st->st_blksize = PAGE_SIZE;  // Optimal I/O chunk (one page)

        spin_unlock(&fd_lock);
        return 0;
    }

    spin_unlock(&fd_lock);

    // If we get here, the ticket (FD) is invalid or empty
    return -EBADF;
}

/* Check if the given file is in a valid index */
int _isatty(int file) {
    return file >= 0 && file < FD_TABLE_MIN;
}

/* Change file offset from reading and writing */
int _lseek(int file, int ptr, int dir) {
    if (unlikely(file >= 0 && file < FD_TABLE_MIN)) return 0;

    spin_lock(&fd_lock);

    if (likely(file < FD_TABLE_MAX && fd_table[file])) {
        FileOpenHandle* f = (FileOpenHandle*) fd_table[file];
        size_t f_size = f->file->size;
        int new_pos;

        if (dir == 0)      new_pos = ptr;          // SEEK_SET
        else if (dir == 1) new_pos = f->pos + ptr; // SEEK_CUR
        else if (dir == 2) new_pos = f_size + ptr; // SEEK_END
        else {
            spin_unlock(&fd_lock);
            return -EINVAL;
        }

        // Clamp to file boundaries
        if (new_pos < 0) new_pos = 0;
        else if (new_pos > (int) f_size) new_pos = f_size;

        f->pos = new_pos;
        int final_pos = f->pos;

        spin_unlock(&fd_lock);
        return final_pos;
    }

    spin_unlock(&fd_lock);

    return -EBADF;
}

/* Get current task ID */
int _getpid() {
    return current_task->id;
}

/* Kill given task */
int _kill(int pid, int sig) {
    kill_task(pid);
}

/* Get a random number */
int getentropy(void *ptr, size_t len) {
    if (len > 256) {
        errno = EIO;
        return SYS_ERROR;
    }

    uint8_t *buf = (uint8_t*) ptr;
    size_t i = 0;

    while (i < len) {
        unsigned char ok;
        uint32_t random_val = asm_get_random(&ok);

        if (likely(ok)) {
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
int   read(int file, char *ptr, int len) { return _read(file, ptr, len); }
int   write(int file, char *ptr, int len) { return _write(file, ptr, len); }
int   close(int file) { return _close(file); }
int   lseek(int file, int ptr, int dir) { return _lseek(file, ptr, dir); }
int   fstat(int file, struct stat *st) { return _fstat(file, st); }
int   isatty(int file) { return _isatty(file); }
int   kill(int pid, int sig) { return _kill(pid, sig); }
int   getpid() { return _getpid(); }

/* Free the given pointer from memory */
void _free_r(struct _reent *r, void *ptr) {
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

void* memmove(void* dest, const void* src, size_t n) {
    if (dest < src) return memcpy(dest, src, n);
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    return dest;
}
