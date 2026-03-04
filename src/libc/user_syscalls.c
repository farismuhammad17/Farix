#include "farix.h"

#include <sys/stat.h>
#include <errno.h>

int _write(int file, char *ptr, int len) {
    return _farix_syscall(1, file, (int) ptr, len);
}

void _exit(int status) {
    _farix_syscall(3, status, 0, 0);
    while(1);
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
