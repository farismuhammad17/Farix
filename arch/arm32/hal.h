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

#ifndef ARM32_HAL_H
#define ARM32_HAL_H

#include <stdint.h>

static inline void outb(uint32_t port, uint8_t val) {
    *(volatile uint8_t*) port = val;
}

static inline void outw(uint32_t port, uint16_t val) {
    *(volatile uint16_t*) port = val;
}

static inline void outl(uint32_t port, uint32_t val) {
    *(volatile uint32_t*) port = val;
}

static inline uint8_t inb(uint32_t port) {
    return *(volatile uint8_t*) port;
}

static inline uint16_t inw(uint32_t port) {
    return *(volatile uint16_t*) port;
}

static inline uint32_t inl(uint32_t port) {
    return *(volatile uint32_t*) port;
}

static inline void system_halt() {
    asm volatile("wfi");
}

static inline void system_int_on() {
    asm volatile("cpsie i");
}

static inline void system_int_off() {
    asm volatile("cpsid i");
}

static inline void system_pause() {
    asm volatile("yield" ::: "memory");
}

static inline uint32_t asm_get_random(uint8_t *success) {
    uint32_t val = 0;
    uint32_t sec = 0;

    asm volatile (
        "ldr r1, =0x3F104004 \n\t"
        "ldr r0, [r1]        \n\t"
        "lsr r0, r0, #24     \n\t"
        "cmp r0, #0          \n\t"
        "moveq %1, #0        \n\t"
        "beq 2f              \n\t"

        "ldr r1, =0x3F104008 \n\t"
        "ldr %0, [r1]        \n\t"
        "mov %1, #1          \n\t"

        "2:                  "
        : "=r" (val), "=r" (sec)
        :
        : "r0", "r1", "cc", "memory"
    );

    if (success) *success = (uint8_t) sec;
    return val;
}

static inline void cpu_mem_barrier() {
    asm volatile("dmb sy" ::: "memory");
}

static inline void task_yield() {
    // 0x80 is a common convention for "yield" or "syscall"
    asm volatile("svc 0x80" ::: "memory");
}

static inline uint32_t save_disable_interrupts() {
    uint32_t flags;
    asm volatile(
        "mrs %0, cpsr      \n\t" // Move CPSR to register
        "cpsid i           \n\t" // Change Processor State: Disable Interrupts
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline void restore_interrupts(uint32_t flags) {
    asm volatile(
        "msr cpsr_c, %0    \n\t" // Move register to CPSR (control field)
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

// TODO REM
static inline void set_kernel_stack(uint32_t stack) {
    // We need to switch to SVC mode to change its banked SP,
    // then switch back to whatever mode we were in.
    asm volatile (
        "mrs r1, cpsr      \n" // Save current status
        "msr cpsr_c, #0xD3 \n" // Switch to SVC Mode (Interrupts disabled)
        "mov sp, %0        \n" // Update the banked SP
        "msr cpsr_c, r1    \n" // Switch back to original mode
        : : "r" (stack) : "r1", "cc", "memory"
    );
}

#endif
