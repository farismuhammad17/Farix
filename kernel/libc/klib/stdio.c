/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stdarg.h>
#include <stddef.h>

#include "drivers/terminal.h"

#include "klib/stdio.h"

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

        if (unlikely(*p == '-')) {
            left_align = 1;
            p++;
        }

        // Parse Width
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        if (unlikely(*p == 'l')) {
            long_modifier = 1;
            p++;
        }

        if (unlikely(*p == '*')) {
            width = va_arg(args, int);
            p++;
        }

        // Parse Precision (starts with '.')
        if (unlikely(*p == '.')) {
            p++;
            precision = 0; // If you see '.', precision defaults to 0
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                p++;
            }
            if (unlikely(*p == '*')) {
                precision = va_arg(args, int);
                p++;
            }
        }

        switch (*p) {
            case 's': {
                const char* s = va_arg(args, const char*);
                if (unlikely(!s)) s = "(null)";

                int len = 0;
                while (s[len]) len++;
                // If precision is -1, use the full length of the string
                int print_len = (precision >= 0 && precision < len) ? precision : len;

                if (likely(!left_align)) {
                    while (pos < size - 1 && width-- > print_len) {
                        str[pos++] = ' ';
                    }
                }

                int i = 0;
                while (pos < size - 1 && i < print_len && s[i]) {
                    str[pos++] = s[i++];
                }

                if (unlikely(left_align)) {
                    while (pos < size - 1 && width-- > print_len) {
                        str[pos++] = ' ';
                    }
                }

                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);

                char buf[12];
                int i = 0;

                if (unlikely(val == 0)) {
                    buf[i++] = '0';
                }

                else {
                    while (val > 0) {
                        buf[i++] = (val % 10) + '0';
                        val /= 10;
                    }
                }

                while (--i >= 0 && pos < size - 1) {
                    str[pos++] = buf[i];
                }

                break;
            }
            case 'd': {
                long val;

                if (unlikely(long_modifier)) {
                    val = va_arg(args, long);
                } else {
                    val = va_arg(args, int);
                }

                char buf[12];
                int i = 0;

                if (val < 0) {
                    str[pos++] = '-';
                    val = -val;
                }

                if (unlikely(val == 0)) {
                    buf[i++] = '0';
                }
                else {
                    while (val > 0) {
                        buf[i++] = (val % 10) + '0';
                        val /= 10;
                    }
                }

                // Width padding (spaces)
                int total_len = (val < 0 ? 1 : 0) + i;
                if (likely(!left_align)) {
                    while (pos < size - 1 && width-- > total_len) {
                        str[pos++] = ' ';
                    }
                }

                while (--i >= 0 && pos < size - 1) {
                    str[pos++] = buf[i];
                }

                if (unlikely(left_align)) {
                    while (pos < size - 1 && width-- > total_len) {
                        str[pos++] = ' ';
                    }
                }

                break;
            }
            case 'p':
            case 'x':
            case 'X': {
                // If long_modifier is set, we must pull an unsigned long (8 bytes).
                // Otherwise, we pull an unsigned int (4 bytes).
                unsigned long val;

                if (unlikely(long_modifier)) {
                    val = va_arg(args, unsigned long);
                } else {
                    val = va_arg(args, unsigned int);
                }

                char hex[] = "0123456789abcdef";
                char buf[20]; // Increased buffer size for 64-bit longs
                int i = 0;

                if (unlikely(val == 0)) {
                    buf[i++] = '0';
                }
                else {
                    while (val > 0) {
                        buf[i++] = hex[val % 16];
                        val /= 16;
                    }
                }

                // Zero padding for precision
                int pad = (precision > i) ? (precision - i) : 0;
                while (pad-- > 0 && pos < size - 1) {
                    str[pos++] = '0';
                }
                while (--i >= 0 && pos < size - 1) {
                    str[pos++] = buf[i];
                }

                break;
            }
            case 'c':
                if (pos < size - 1) {
                    str[pos++] = (char) va_arg(args, int);
                }

                break;
            case '%':
                if (pos < size - 1) {
                    str[pos++] = '%';
                }

                break;
            default:
                if (pos < size - 1) {
                    str[pos++] = *p;
                }

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
