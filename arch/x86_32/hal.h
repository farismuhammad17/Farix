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

#ifndef X86_32_HAL_H
#define X86_32_HAL_H

#include <stdint.h>

#include "memory/vmm.h"

static inline void outb(uint32_t port, uint8_t  val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outw(uint32_t port, uint16_t val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint32_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline uint16_t inw(uint32_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void system_halt() {
    asm volatile("hlt");
}

static inline void system_int_on() {
    asm volatile("sti");
}

static inline void system_int_off() {
    asm volatile("cli");
}

static inline void system_pause() {
    asm volatile("pause" ::: "memory");
}

static inline uint32_t asm_get_random(uint8_t *success) {
    uint32_t random_val;

    asm volatile(
        "rdrand %0; "
        "setc %1"
        : "=r" (random_val), "=m" (*success)
        :
        : "cc"
    );

    return random_val;
}

static inline void cpu_mem_barrier() {
    asm volatile("" : : : "memory");
}

static inline void task_yield() {
    asm volatile("int $0x20");
}

static inline uint32_t save_disable_interrupts() {
    uint32_t flags;
    asm volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "cli"
        : "=rm"(flags) :: "memory"
    );
    return flags;
}

static inline void restore_interrupts(uint32_t flags) {
    asm volatile(
        "push %0\n\t"
        "popf"
        : : "rm"(flags) : "memory", "cc"
    );
}

#endif
