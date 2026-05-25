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

#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#include "process/task.h"

// ELF Identifiers
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32  1
#define ELFDATA2LSB 1 // Little Endian (Standard for x86)

// Segment Types
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;      // 2 = Executable
    uint16_t e_machine;   // 3 = x86
    uint32_t e_version;
    uint32_t e_entry;     // The virtual address where the program starts
    uint32_t e_phoff;     // Program header table's file offset
    uint32_t e_shoff;     // Section header table's file offset
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;     // Number of segments to load
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset; // Offset in the file
    uint32_t p_vaddr;  // Virtual address in memory
    uint32_t p_paddr;  // Physical address
    uint32_t p_filesz; // Size of the segment in the file
    uint32_t p_memsz;  // Size of the segment in memory
    uint32_t p_flags;  // 1 = Execute, 2 = Write, 4 = Read
    uint32_t p_align;
} __attribute__((packed)) elf_program_header_t;

extern void elf_user_trampoline();

task* exec_elf(const char* path);

#endif
