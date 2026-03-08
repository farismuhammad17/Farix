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

#include "farix.h"

int _write(int file, char *ptr, int len) {
    return _farix_syscall(1, file, (int) ptr, len);
}

void _exit(int status) {
    _farix_syscall(3, status, 0, 0);
}

int _read(int file, char *ptr, int len) { return -1; }
int _sbrk(int incr) { return -1; }
int _close(int file) { return -1; }
int _fstat(int file, struct stat *st) { return -1; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _kill(int pid, int sig) { return -1; }
int _getpid() { return 1; }

int write(int f, char *p, int l) { return _write(f, p, l); }
void exit(int s) { _exit(s); }
int sbrk(int i) { return _sbrk(i); }
int close(int f) { return _close(f); }
int fstat(int f, struct stat *s) { return _fstat(f, s); }
int isatty(int f) { return _isatty(f); }
int lseek(int f, int p, int d) { return _lseek(f, p, d); }
int read(int f, char *p, int l) { return _read(f, p, l); }
int kill(int p, int s) { return _kill(p, s); }
int getpid() { return _getpid(); }
