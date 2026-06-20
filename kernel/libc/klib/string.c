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

#include <stddef.h>
#include <stdint.h>

#include "hal.h"

#include "klib/string.h"

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
    if (unlikely(s == NULL)) return NULL;

    size_t len = strlen(s) + 1;
    char* new_str = (char*) kmalloc(len);

    if (likely(new_str != NULL)) {
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
