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

#ifndef X86_32_HAL_H
#define X86_32_HAL_H

#include <stdint.h>

typedef struct {
    uint32_t ds;                                           // push ds
    uint32_t edi, esi, ebp, esp_dummy, ebx, ecx, edx, eax; // pushad
    uint32_t int_no, err_code;                             // push 128; push 0
    uint32_t eip, cs, eflags, useresp, ss;                 // Pushed by CPU
} syscalls_registers_x86_32_t;

/* Assembly outb */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Assembly outw */
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Assembly outl */
static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* Assembly inb */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Assembly inw */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Assembly inl */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Assembly halt instruction */
static inline void system_halt() {
    asm volatile("hlt");
}

/* Assembly pause instruction */
static inline void system_pause() {
    asm volatile("pause" ::: "memory");
}

/* Assembly sti to turn on system interrupts */
static inline void system_int_on() {
    asm volatile("sti");
}

/* Assembly cli to turn off system interrupts */
static inline void system_int_off() {
    asm volatile("cli");
}

/* Assembly instruction to get random value */
static inline uint32_t asm_get_random(uint8_t *success) {
    uint32_t random_val;

    asm volatile(
        "rdrand %0; "
        "setc %1"
        : "=r" (random_val), "=m" (*success) : : "cc"
    );

    return random_val;
}

/* Assembly CPU memory barrier instruction */
static inline void cpu_mem_barrier() {
    asm volatile("" : : : "memory");
}

/* Assembly function to yield current task */
static inline void task_yield() {
    asm volatile("int $0x20");
}

/*
Assembly function to save the current CPU flags state and clear the interrupt
flag to disable hardware interrupts.
*/
static inline uint32_t save_disable_interrupts() {
    uint32_t flags;
    asm volatile(
        "refe: \n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "cli"
        : "=r"(flags) :: "memory"
    );
    return flags;
}

/*
Restore the CPU flags state from a previously saved value, resetting the
interrupt status.
*/
static inline void restore_interrupts(uint32_t flags) {
    asm volatile(
        "pushl %0\n\t"
        "popfl"
        : : "r"(flags) : "memory", "cc"
    );
}

void FREQ_FUNC set_kernel_stack(uint32_t stack);

#endif
