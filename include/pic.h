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

#ifndef PIC_H
#define PIC_H

#include "io.h"

#define PIC1    		0x20
#define PIC1_COMMAND	PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2	    	0xA0
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void pic_remap();

#endif
