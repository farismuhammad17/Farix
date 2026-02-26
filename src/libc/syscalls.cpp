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

#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <errno.h>

#include "memory/heap.h"
#include "drivers/terminal.h"
#include "drivers/keyboard.h"
#include "fs/vfs.h"

extern "C" {

// A table of strings representing the names of open files
static std::string fd_table[20];

int _close(int file) { return -1; }
int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
void _exit(int status) { while(1); }
int _getpid() { return 1; }
int _kill(int pid, int sig) { return -1; }

void* _sbrk(int incr) {
    if (incr == 0) return (void*)0;

    void* res = kmalloc(incr);
    if (res == nullptr) {
        return (void*)-1; // Newlib's standard error signal
    }
    return res;
}

int _read(int file, char *ptr, int len) {
    if (file == 0) { // stdin
        int i = 0;
        while (i < len) {
            while (kbd_head == kbd_tail) {
                __asm__("hlt");
            }

            char c = kbd_buffer[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;
            ptr[i++] = c;

            if (c == '\n') break; // Stop reading at newline
        }
        return i;
    }

    else if (file >= 3 && file < 20 && fd_table[file] != "") {
        // fs_read returns bool, Newlib wants bytes read
        if (fs_read(fd_table[file], ptr, len)) {
            return len;
        }
    }

    return -1;
}

int _write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) { // stdout/stderr
        for (int i = 0; i < len; i++) {
            echo_char(ptr[i]);
        }
        return len;
    }

    else if (file >= 3 && file < 20 && fd_table[file] != "") {
        if (fs_write(fd_table[file], ptr, len)) {
            return len;
        }
    }

    errno = EBADF;
    return -1;
}

int _open(const char *name, int flags, int mode) {
    std::string path = name;
    File* f = fs_get(path);

    if (!f) {
        errno = ENOENT;
        return -1;
    }

    for (int i = 3; i < 20; i++) {
        if (fd_table[i] == "") {
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
        uint32_t random_val;
        unsigned char ok;

        // RDRAND returns 1 on success (entropy available) or 0 on failure
        __asm__ volatile ("rdrand %0; setc %1"
            : "=r" (random_val), "=qm" (ok));

        if (ok) {
            // Copy bytes from the 32-bit result into the buffer
            for (int j = 0; j < 4 && i < len; j++) {
                buf[i++] = (uint8_t)(random_val >> (j * 8));
            }
        } else {
            // If the hardware pool is exhausted, pause briefly
            __asm__ volatile ("pause");
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

void* malloc(size_t size) { return kmalloc(size); }
void free(void* ptr) { kfree(ptr); }

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

}
