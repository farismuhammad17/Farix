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

#ifndef FARIX_H
#define FARIX_H

#include <stdbool.h>
#include <sys/stat.h>

#define SYS_DONE     0
#define SYS_ERROR   -1

#define SYS_EXIT     1 // NOTE: SYS_EXIT is hardcoded in user.asm, changing requires changing there
#define SYS_READ     2
#define SYS_WRITE    3
#define SYS_OPEN     4
#define SYS_CLOSE    5
#define SYS_LSEEK    6
#define SYS_ISATTY   7
#define SYS_FSTAT    8
#define SYS_GETPID   9
#define SYS_KILL     10
#define SYS_SBRK     11

// Super user
#define SYS_UART_PUT          1000
#define SYS_GET_HEAP          1001
#define SYS_GET_HEAP_SEG_SIZE 1002
#define SYS_GET_HEAP_START    1003
#define SYS_GET_HEAP_END      1004
#define SYS_INT_ON            1005
#define SYS_INT_OFF           1006

void  _exit(int status);
int   _read(int file, char *ptr, int len);
int   _write(int file, char *ptr, int len);
int   _open(const char *name, int flags, int mode);
int   _close(int file);
int   _lseek(int file, int ptr, int dir);
int   _getpid();
int   _kill(int pid, int sig);
void* _sbrk(int incr);
int   _isatty(int file);
int   _fstat(int file, struct stat *st);

// Super user

typedef struct {
    uint32_t address;
    uint32_t size;
    uint32_t caller;
    bool is_free;
} HeapData;

int UART_PUTS(const char *data);
int GET_HEAP_DATA(HeapData* buffer, int max_count);
int GET_HEAP_SEG_SIZE();
int GET_HEAP_START();
int GET_HEAP_END();
int SYSTEM_INT_ON();
int SYSTEM_INT_OFF();

#endif
