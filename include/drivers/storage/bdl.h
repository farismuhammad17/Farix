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

#ifndef BDL_H
#define BDL_H

#include <stdint.h>

typedef struct {
    void (*read)(uint64_t lba, uint8_t* buf);
    void (*write)(uint64_t lba, uint8_t* buf);
} BDLDevice;

void RARE_FUNC init_storage();

void RARE_FUNC bdl_mount(BDLDevice* dev);

void bdl_read(uint64_t lba, void* buf);
void bdl_write(uint64_t lba, void* buf);

#endif
