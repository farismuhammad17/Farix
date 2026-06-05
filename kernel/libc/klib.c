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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "hal.h"

#include "drivers/terminal.h"

#include "klib/stdio.h"
#include "klib/string.h"

// string.h

/* Copy src[0] to src[n-1] into dest[0] to dest[n-1] inclusive */
void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    volatile uint8_t* dst = (volatile uint8_t*) dest;
    const uint8_t* s = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++) dst[i] = s[i];

    cpu_mem_barrier();
    return dest;
}

/* Set value from p[0] to p[n-1] inclusive to c */
void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*) s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t) c;
    return s;
}

/*
It copies bytes by checking for memory overlap: if dest > src,
it copies backwards; otherwise, it performs forward copying.
*/
void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*) dest;
    const uint8_t* s = (const uint8_t*) src;

    // If dest is in the range of source, copy backwards to avoid overwrites
    if (d > s && d < s + n) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }

    return dest;
}

/* Duplicate string into new address */
char* strdup(const char* s) {
    if (s == NULL) return NULL;

    size_t len = strlen(s) + 1;
    char* new_str = (char*) kmalloc(len);

    if (new_str != NULL) {
        strcpy(new_str, s);
    }

    return new_str;
}

/* Compare strings */
int strcmp(const char* s1, const char* s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }

    return (*(unsigned char*) s1 - *(unsigned char*) s2);
}

/* Copy entire string */
char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++) != '\0');
    return original_dest;
}

/* Kernel string n-characters copy */
char* strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

/* String length */
size_t strlen(const char* s) {
    const char* p = s;
    while (*p != '\0') p++;
    return (size_t)(p - s);
}

/* Find first instance of character in string */
char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char) c) {
            return (char*) s;
        }
        s++;
    }

    // Check if we are looking for the null terminator itself
    if ((char) c == '\0') {
        return (char*) s;
    }

    return NULL; // Not found
}

// stdio.h

int vsnprintf(char* str, size_t size, const char* format, va_list args) {
    size_t pos = 0;

    for (const char* p = format; *p != '\0'; p++) {
        if (likely(*p != '%')) {
            if (pos < size - 1) str[pos++] = *p;
            continue;
        }

        p++; // Skip '%'

        int left_align = 0;
        int width = 0;
        int precision = -1; // -1 means no precision specified
        int long_modifier = 0;

        if (*p == '-') {
            left_align = 1;
            p++;
        }

        // Parse Width
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        if (*p == 'l') {
            long_modifier = 1;
            p++;
        }

        if (*p == '*') { width = va_arg(args, int); p++; }

        // Parse Precision (starts with '.')
        if (*p == '.') {
            p++;
            precision = 0; // If you see '.', precision defaults to 0
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                p++;
            }
            if (*p == '*') { precision = va_arg(args, int); p++; }
        }

        switch (*p) {
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";

                int len = 0;
                while (s[len]) len++;
                // If precision is -1, use the full length of the string
                int print_len = (precision >= 0 && precision < len) ? precision : len;

                if (!left_align) while (pos < size - 1 && width-- > print_len) str[pos++] = ' ';
                int i = 0;
                while (pos < size - 1 && i < print_len && s[i]) str[pos++] = s[i++];
                if (left_align) while (pos < size - 1 && width-- > print_len) str[pos++] = ' ';
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                char buf[12]; int i = 0;
                if (val == 0) buf[i++] = '0';
                else while (val > 0) { buf[i++] = (val % 10) + '0'; val /= 10; }
                // Add width padding here if needed
                while (--i >= 0 && pos < size - 1) str[pos++] = buf[i];
                break;
            }
            case 'd': {
                long val;

                if (long_modifier) {
                    val = va_arg(args, long);
                } else {
                    val = va_arg(args, int);
                }

                char buf[12]; int i = 0;
                if (val < 0) { str[pos++] = '-'; val = -val; }
                if (val == 0) buf[i++] = '0';
                else while (val > 0) { buf[i++] = (val % 10) + '0'; val /= 10; }

                // Width padding (spaces)
                int total_len = (val < 0 ? 1 : 0) + i;
                if (!left_align) while (pos < size - 1 && width-- > total_len) str[pos++] = ' ';

                while (--i >= 0 && pos < size - 1) str[pos++] = buf[i];

                if (left_align) while (pos < size - 1 && width-- > total_len) str[pos++] = ' ';
                break;
            }
            case 'p': {
                unsigned long val = va_arg(args, unsigned long);
                char hex[] = "0123456789abcdef";
                char buf[20];
                int i = 0;

                if (val == 0) {
                    buf[i++] = '0';
                } else {
                    while (val > 0) {
                        buf[i++] = hex[val % 16];
                        val /= 16;
                    }
                }

                while (i < 16 && pos < size - 1) str[pos++] = '0';
                while (--i >= 0 && pos < size - 1) str[pos++] = buf[i];
                break;
            }
            case 'x':
            case 'X': {
                // If long_modifier is set, we must pull an unsigned long (8 bytes).
                // Otherwise, we pull an unsigned int (4 bytes).
                unsigned long val = (long_modifier) ? va_arg(args, unsigned long) : va_arg(args, unsigned int);

                char hex[] = "0123456789abcdef";
                char buf[20]; // Increased buffer size for 64-bit longs
                int i = 0;

                if (val == 0) buf[i++] = '0';
                else while (val > 0) {
                    buf[i++] = hex[val % 16];
                    val /= 16;
                }

                // Zero padding for precision
                int pad = (precision > i) ? (precision - i) : 0;
                while (pad-- > 0 && pos < size - 1) str[pos++] = '0';

                while (--i >= 0 && pos < size - 1) str[pos++] = buf[i];
                break;
            }
            case 'c':
                if (pos < size - 1) str[pos++] = (char)va_arg(args, int);
                break;
            case '%':
                if (pos < size - 1) str[pos++] = '%';
                break;
            default:
                if (pos < size - 1) str[pos++] = *p;
                break;
        }
    }

    str[pos] = '\0';
    return (int) pos;
}

/* What? Documentation on printf? No. */
void printf(const char* format, ...) {
    static char buf[1024];
    va_list args;
    va_start(args, format);

    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (likely(len > 0)) {
        echo_raw(buf, (size_t) len);
    }
}
