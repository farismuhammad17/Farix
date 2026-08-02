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

#ifndef X86_64_HAL_H
#define X86_64_HAL_H

#include <stdint.h>

#define ARCH_NAME "x86_64"

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} syscalls_registers_x86_64_t;

// VMM
#define PAGE_PRESENT (1ULL << 0) // Bit 0 - If page is in RAM
#define PAGE_RW      (1ULL << 1) // Bit 1 - 0 = Read-only,   1 = Read/Write
#define PAGE_USER    (1ULL << 2) // Bit 2 - 0 = Kernel only, 1 = Everyone
#define PAGE_PWT     (1ULL << 3) // Bit 3 - Writes go to cache and memory immediately
#define PAGE_PCD     (1ULL << 4) // Bit 4 - Completely disables CPU caching for that page
#define PAGE_CACHE   0           // Not present in x86, does nothing, but required stub

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

/* Assembly instruction to get random 64-bit value natively */
static inline uint64_t asm_get_random(uint8_t *success) {
    uint64_t random_val;
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

/* Assembly function to yield current task via 64-bit interrupt gate */
static inline void task_yield() {
    asm volatile("int $0x20");
}

/*
Assembly function to save the current CPU flags state and clear the interrupt
flag to disable hardware interrupts. Upscaled to use 64-bit rflags stack actions.
*/
static inline uint64_t save_disable_interrupts() {
    uint64_t flags;
    asm volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags) :: "memory"
    );
    return flags;
}

/*
Restore the CPU flags state from a previously saved value, resetting the
interrupt status via 64-bit rflags execution stack actions.
*/
static inline void restore_interrupts(uint64_t flags) {
    asm volatile(
        "pushq %0\n\t"
        "popfq"
        : : "r"(flags) : "memory", "cc"
    );
}

/* Assembly read time-stamp counter (TSC) */
static inline uint64_t get_cpu_cycles() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t) hi << 32) | lo;
}

void FREQ_FUNC set_kernel_stack(uint64_t stack);

#endif
