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

#include "drivers/storage/ahci.h"

int HBA_PxCMD_ST  = 0x0001; // Start bit
int HBA_PxCMD_FRE = 0x0010; // FIS Receive Enable bit
int HBA_PxCMD_FR  = 0x4000; // FIS Running status
int HBA_PxCMD_CR  = 0x8000; // Command List Running status
