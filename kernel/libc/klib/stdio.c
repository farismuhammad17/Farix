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
#include <stdbool.h>
#include <stddef.h>

#include "drivers/output.h"
#include "drivers/terminal.h"

#include "klib/stdio.h"

int vsnprintf(char* str, size_t size, const char* format, va_list args) {
    size_t pos = 0;

    for (const char* p = format; *p != '\0'; p++) {
        if (likely(*p != '%')) {
            if (likely(pos < size - 1)) {
                str[pos++] = *p;
            }

            continue;
        }

        p++; // Skip '%'
        if (unlikely(*p == '\0')) break; // Prevent walking past end of string

        bool left_align = false;
        bool zero_pad = false;
        int width = 0;
        int precision = -1;
        int long_modifier = 0;

        if (unlikely(*p == '-')) {
            left_align = true;
            p++;
        }

        if (unlikely(*p == '0')) {
            zero_pad = true;
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        if (*p == '*') {
            width = va_arg(args, int);
            if (width < 0) {
                left_align = true;
                width = -width;
            }
            p++;
        } else {
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }

        if (unlikely(*p == '.')) {
            p++;
            precision = 0;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                p++;
            }
        }

        while (*p == 'l') {
            long_modifier++;
            p++;
        }

        switch (*p) {
            case 's': {
                const char* s = va_arg(args, const char*);

                if (unlikely(!s)) {
                    s = "(null)";
                }

                // Pre-calculate string length up to precision boundary
                int str_len = 0;
                while (s[str_len] != '\0' && (precision < 0 || str_len < precision)) {
                    str_len++;
                }

                // Right-aligned padding (spaces before string)
                if (!left_align && width > str_len) {
                    int pad = width - str_len;
                    while (pad-- > 0) {
                        if (likely(pos < size - 1)) str[pos] = ' ';
                        pos++;
                    }
                }

                // Output the string
                for (int i = 0; i < str_len; i++) {
                    if (likely(pos < size - 1)) {
                        str[pos] = s[i];
                    }
                    pos++;
                }

                // Left-aligned padding (spaces after string)
                if (left_align && width > str_len) {
                    int pad = width - str_len;
                    while (pad-- > 0) {
                        if (likely(pos < size - 1)) str[pos] = ' ';
                        pos++;
                    }
                }

                break;
            }

            case 'u': {
                uint64_t num;

                if (long_modifier == 0) {
                    num = va_arg(args, unsigned int);
                } else if (long_modifier == 1) {
                    num = va_arg(args, unsigned long);
                } else if (long_modifier == 2) {
                    num = va_arg(args, unsigned long long);
                } else {
                    err_print("vsnprintf: Unsupported long modifier for 'u'");
                    return -1;
                }

                uint64_t power = 10000000000000000000ULL;
                bool started = false;

                while (likely(power > 0)) {
                    int digit = num / power;

                    if (digit > 0 || started || power == 1) {
                        started = true;

                        if (likely(pos < size - 1)) {
                            str[pos] = digit + '0';
                        }
                        pos++;
                    }

                    num %= power;
                    power /= 10;
                }

                break;
            }

            case 'i':
            case 'd': {
                int64_t  num;
                uint64_t abs_num;

                if (long_modifier == 0) {
                    num = va_arg(args, int);
                } else if (long_modifier == 1) {
                    num = va_arg(args, long);
                } else if (long_modifier == 2) {
                    num = va_arg(args, long long);
                } else {
                    err_print("vsnprintf: Unsupported long modifier for 'd' or 'i'");
                    return -1;
                }

                if (num < 0) {
                    if (likely(pos < size - 1)) {
                        str[pos++] = '-';
                    }

                    abs_num = (uint64_t) -num;
                } else {
                    abs_num = (uint64_t) num;
                }

                uint64_t power = 10000000000000000000ULL;
                bool started = false;

                while (likely(power > 0)) {
                    int digit = abs_num / power;

                    if (digit > 0 || started || power == 1) {
                        started = true; // We hit a real number or reached the final 0

                        if (likely(pos < size - 1)) {
                            str[pos] = digit + '0';
                        }
                        pos++;
                    }

                    abs_num %= power;
                    power /= 10;
                }

                break;
            }

            case 'p': {
                void* ptr = va_arg(args, void*);
                uint64_t num = (uint64_t)(uintptr_t)ptr;

                // Print the "0x" prefix
                if (likely(pos < size - 1)) {
                    str[pos] = '0';
                }
                pos++;
                if (likely(pos < size - 1)) {
                    str[pos] = 'x';
                }
                pos++;

                uint64_t power = 0x1000000000000000ULL; // 16^15
                bool started = false;

                while (likely(power > 0)) {
                    int digit = num / power;

                    if (digit > 0 || started || power == 1) {
                        started = true;

                        if (likely(pos < size - 1)) {
                            if (digit < 10) {
                                str[pos] = digit + '0';
                            } else {
                                str[pos] = digit - 10 + 'a'; // Pointers traditionally use lowercase 'a'-'f'
                            }
                        }
                        pos++;
                    }

                    num %= power;
                    power /= 16;
                }

                break;
            }

            case 'X':
            case 'x': {
                uint64_t num;

                if (long_modifier == 0) {
                    num = va_arg(args, unsigned int);
                } else if (long_modifier == 1) {
                    num = va_arg(args, unsigned long);
                } else if (long_modifier == 2) {
                    num = va_arg(args, unsigned long long);
                } else {
                    err_print("vsnprintf: Unsupported long modifier for 'x' or 'X'");
                    return -1;
                }

                uint64_t power = 0x1000000000000000ULL; // Maximum base-16 power (16^15)
                bool started = false;

                while (likely(power > 0)) {
                    int digit = num / power;

                    if (digit > 0 || started || power == 1) {
                        started = true;

                        if (likely(pos < size - 1)) {
                            // Map values 0-9 to '0'-'9', and 10-15 to 'a'-'f' or 'A'-'F'
                            if (digit < 10) {
                                str[pos] = digit + '0';
                            } else {
                                str[pos] = digit - 10 + 'A'; // swap out with 'a' for lowercase
                            }
                        }
                        pos++;
                    }

                    num %= power;
                    power /= 16;
                }

                break;
            }

            case 'c': {
                char c = (char) va_arg(args, int);

                if (pos < size - 1) {
                    str[pos++] = c;
                } else {
                    pos++;
                }

                break;
            }
        }
    }

    if (size > 0) {
        str[pos] = '\0';
    }

    return (int) pos;
}

/* What? Documentation on printf? No. */
void printf(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);

    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (likely(len > 0)) {
        echo_raw(buf, (size_t) len);

        output_dev_t* curr = output_dev_head;
        while (curr != NULL) {
            curr->printf(buf);
            curr = curr->next;
        }
    }
}
