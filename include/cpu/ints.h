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

#ifndef INTS_H
#define INTS_H

void RARE_FUNC init_interrupts();

// Use void* for the handler to remain flexible for different function types
void set_interrupt_kernel (uint8_t vector, void* handler);
void set_interrupt_user   (uint8_t vector, void* handler);
void clear_interrupt      (uint8_t vector);

#endif
