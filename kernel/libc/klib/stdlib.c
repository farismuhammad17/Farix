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

#include "klib/stdlib.h"

int atoi(const char *str) {
    // Whitespace skipping
    while (*str == ' ' || (*str >= '\t' && *str <= '\r')) {
        str++;
    }

    // Positive/Negative
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Conversion loop
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = (res * 10) + (*str - '0');
        str++;
    }

    return res * sign;
}
