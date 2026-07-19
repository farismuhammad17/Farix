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

#ifndef INITBOOT_H
#define INITBOOT_H

#include <stdint.h>

#define INITBOOT_TXT_SECTION __attribute__((section(".initboot.text")))
#define INITBOOT_DAT_SECTION __attribute__((section(".initboot.data")))

void initboot();
void kill_bootstrap();

void initboot_timer();

#endif
