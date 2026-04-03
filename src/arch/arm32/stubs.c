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

#include "arch/stubs.h"

void outb(uint32_t port, uint8_t val) {
    *(volatile uint8_t*) port = val;
}

void outw(uint32_t port, uint16_t val) {
    *(volatile uint16_t*) port = val;
}

uint8_t inb(uint32_t port) {
    return *(volatile uint8_t*) port;
}

uint16_t inw(uint32_t port) {
    return *(volatile uint16_t*) port;
}

void system_halt() {
    asm volatile("wfi");
}

void system_int_on() {
    asm volatile("cpsie i");
}

void system_int_off() {
    asm volatile("cpsid i");
}

void system_pause() {
    asm volatile("yield" ::: "memory");
}

uint32_t asm_get_random(uint8_t *success) {
    uint32_t val = 0;
    uint32_t sec = 0;

    __asm__ volatile (
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

void cpu_mem_barrier() {
    asm volatile("dmb sy" ::: "memory");
}

void task_yield() {
    // 0x80 is a common convention for "yield" or "syscall"
    asm volatile("svc 0x80" ::: "memory");
}

void set_kernel_stack(uint32_t stack) {
    // We need to switch to SVC mode to change its banked SP,
    // then switch back to whatever mode we were in.
    asm volatile (
        "mrs r1, cpsr      \n" // Save current status
        "msr cpsr_c, #0xD3 \n" // Switch to SVC Mode (Interrupts disabled)
        "mov sp, %0        \n" // Update the banked SP
        "msr cpsr_c, r1    \n" // Switch back to original mode
        : : "r" (stack) : "r1", "sp"
    );
}
