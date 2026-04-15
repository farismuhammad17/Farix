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

#include "process/task.h"

#define SYS_DONE                  0
#define SYS_ERROR                -1

#define USER_MIN_SYSCALL          0
#define SUPER_MIN_SYSCALL         1000

// Default user system calls
#define SYS_EXIT                  USER_MIN_SYSCALL + 1 // NOTE: SYS_EXIT is hardcoded in user.asm, changing requires changing there
#define SYS_READ                  USER_MIN_SYSCALL + 2
#define SYS_WRITE                 USER_MIN_SYSCALL + 3
#define SYS_OPEN                  USER_MIN_SYSCALL + 4
#define SYS_CLOSE                 USER_MIN_SYSCALL + 5
#define SYS_LSEEK                 USER_MIN_SYSCALL + 6
#define SYS_ISATTY                USER_MIN_SYSCALL + 7
#define SYS_FSTAT                 USER_MIN_SYSCALL + 8
#define SYS_GETPID                USER_MIN_SYSCALL + 9
#define SYS_KILL                  USER_MIN_SYSCALL + 10
#define SYS_SBRK                  USER_MIN_SYSCALL + 11

// Super user system calls
#define SYS_UART_PUT              SUPER_MIN_SYSCALL + 1
#define SYS_GET_HEAP              SUPER_MIN_SYSCALL + 2
#define SYS_GET_HEAP_SEG_SIZE     SUPER_MIN_SYSCALL + 3
#define SYS_GET_HEAP_START        SUPER_MIN_SYSCALL + 4
#define SYS_GET_HEAP_END          SUPER_MIN_SYSCALL + 5
#define SYS_HEAP_AUDIT            SUPER_MIN_SYSCALL + 6
#define SYS_INT_EXEC              SUPER_MIN_SYSCALL + 7
#define SYS_INT_ON                SUPER_MIN_SYSCALL + 8
#define SYS_INT_OFF               SUPER_MIN_SYSCALL + 9
#define SYS_GET_TASK_INFO         SUPER_MIN_SYSCALL + 10
#define SYS_GET_TASK_LIST         SUPER_MIN_SYSCALL + 11

typedef struct {
    uint32_t address;
    uint32_t size;
    uint32_t caller;
    bool is_free;
} HeapData;

typedef struct {
    uint32_t id;
    uint32_t state;
    uint32_t parent_id;
    uint32_t next_id;
    uint32_t neighbor_id;
    uint32_t stack_ptr;
    uint32_t stack_origin;
    uint32_t page_dir;
    char     name[32];
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, eip;
} TaskData;

typedef struct {
    uint32_t pids[TASKS_LIST_LEN];
    task_list_mask_t mask;
} TaskListData;

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

int UART_PUTS(const char *data);
int GET_HEAP_DATA(HeapData* buffer, int max_count);
int GET_HEAP_SEG_SIZE();
int GET_HEAP_START();
int GET_HEAP_END();
int HEAP_AUDIT(int *fault_addr);
int SYSTEM_INT_EXEC(int int_id);
int SYSTEM_INT_ON();
int SYSTEM_INT_OFF();
int GET_TASK_DATA(int pid, TaskData* buffer);
int GET_TASK_LIST(int list_id, TaskListData* buffer);

#endif
