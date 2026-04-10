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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "arch/stubs.h"
#include "cpu/pci.h"
#include "cpu/timer.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "arch/kernel.h"
#include "drivers/acpi/acpi.h"

#define OS_SLEEP_MAX_MICROSECONDS 3000000
#define MAX_DEFERRED_UNMAPS 64

static void*  unmap_queue[MAX_DEFERRED_UNMAPS];
static size_t unmap_count = 0;

static ACPI_PHYSICAL_ADDRESS AcpiRSDP = NULL;

// --- INITIALISATION ---

ACPI_STATUS AcpiOsInitialize() {
    AcpiRSDP = AcpiOsGetRootPointer();

    if (AcpiRSDP == 0) return AE_NOT_FOUND;

    ACPI_PHYSICAL_ADDRESS found_rsdp = 0;
    AcpiFindRootPointer(&found_rsdp);

    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() { return AE_OK; }
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context) { return AE_OK; }
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine) { return AE_OK; }

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, void (*Function)(void *), void *Context) {
    if (Function) {
        (void) Function(Context);
    }
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
    if (AcpiRSDP != NULL) return AcpiRSDP;

    // Scan the BIOS memory for "RSD PTR "
    for (uint32_t addr = 0x000E0000; addr < 0x000FFFFF; addr += 16) {
        char* signature = (char*) PHYSICAL_TO_VIRTUAL(addr);

        if (memcmp(signature, "RSD PTR ", 8) == 0) {
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) sum += signature[i];

            if (sum == 0) return (ACPI_PHYSICAL_ADDRESS) addr;
        }
    }

    uart_printf("ACPI OSL: AcpiOsGetRootPointer: RSDP NOT FOUND\n");
    return 0;
}

// --- MULTITASKING ---

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
    // init->id is 0
    // 0 is reserved for "invalid"
    // ACPICA will think we failed if we return 0
    // doing +1 cheats out of this
    return current_task->id + 1;
}

// --- TIME ---

// ACPICA calls this for waits too short to deserve a task switch
void AcpiOsStall(UINT32 Microseconds) {
    timer_stall((uint32_t) Microseconds);
}

// ACPICA calls this when it wants to take a nap and do nothing,
// I'll make it switch task during this function later, but I'm lazy.
void AcpiOsSleep(UINT64 time) { // Comes in Milliseconds
    time *= 1000; // Convert to microseconds

    // If it's a long sleep, do it in chunks to avoid bit overflow
    while (time > OS_SLEEP_MAX_MICROSECONDS) {
        AcpiOsStall(OS_SLEEP_MAX_MICROSECONDS);
        time -= OS_SLEEP_MAX_MICROSECONDS;
    }

    timer_stall((uint32_t) time);
}

// Contrary to naming conventions, this is NOT for ACPICA to enter sleep,
// but is called when the computer itself is going to shutdown.
ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) {
    return AE_OK;
}

void AcpiOsWaitEventsComplete() {
    return;
}

// Wants per 100-nanosecond as unit
UINT64 AcpiOsGetTimer(void) {
    return get_timer_uptime_microseconds() * 10;
}

// --- MEMORY ---

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    uintptr_t virt_start = (uintptr_t) PHYSICAL_TO_VIRTUAL(PhysicalAddress);

    // Calculate how many pages this request actually spans
    uintptr_t base_page = (uintptr_t) PhysicalAddress & ~0xFFF;
    uintptr_t last_page = ((uintptr_t) PhysicalAddress + Length - 1) & ~0xFFF;

    for (uintptr_t page = base_page; page <= last_page; page += 0x1000) {
        void* virt_page = (void*) PHYSICAL_TO_VIRTUAL(page);

        if (!vmm_is_mapped(kernel_directory, virt_page))
            vmm_map_page(kernel_directory, (void*) page, virt_page, PAGE_PRESENT | PAGE_RW);
    }

    return (void*) virt_start;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
    if (!LogicalAddress || Length == 0) return;

    uintptr_t addr = (uintptr_t) LogicalAddress;
    uintptr_t base_page = addr & ~0xFFF;
    uintptr_t last_page = (addr + Length - 1) & ~0xFFF;

    for (uintptr_t pg = base_page; pg <= last_page; pg += 0x1000) {
        uintptr_t phys = vmm_get_phys(vmm_get_current_directory(), (void*) pg);

        // Never unmap the first 32MB (Initial Kernel/Identity Map)
        if (phys < (VMM_INIT_MAP_SIZE << 20)) continue;

        // Only unmap if it's NOT the page containing the RSDT or DSDT headers
        // ACPICA often keeps these pointers cached internally.
        if (phys == (AcpiRSDP & ~0xFFF)) continue;

        unmap_queue[unmap_count++] = (void*) pg;
        if (unmap_count >= MAX_DEFERRED_UNMAPS) uart_print("ACPI OSL: Unmap queue full");
    }
}

// Defined in arch/kernel.h
void AcpiMappingCleanup() {
    for (size_t i = 0; i < unmap_count; i++)
        pmm_free_page(vmm_unmap_page(unmap_queue[i]));

    unmap_count = 0;
}

void* AcpiOsAllocate(ACPI_SIZE Size) {
    void* ptr = kmalloc(Size);
    if (ptr) kmemset(ptr, 0, Size);
    return ptr;
}

void AcpiOsFree(void *Memory) {
    kfree(Memory);
}

// TODO: Use proper slab allocation
ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize,
                              UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache) {
    *ReturnCache = (ACPI_CACHE_T*)(uintptr_t) ObjectSize;
    return AE_OK;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache) {
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache) {
    return AE_OK;
}

// These override functions would not need any change until I find out some random
// computer doesn't support this kernel, because it doesn't recognize it. If that
// day comes, we have to fix these. Currently, these just mean "I have nothing to
// fix," and that works for me.
ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
    ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal,
                                     ACPI_STRING *NewVal) {
    *NewVal = NULL;
    return AE_OK;
}

void * AcpiOsAcquireObject(ACPI_CACHE_T *Cache) {
    return AcpiOsAllocate((uintptr_t) Cache);
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object) {
    AcpiOsFree(Object);
    return AE_OK;
}

// --- SEMAPHORES ---

// These will crash us and not work if:
// IN the future, if we ever have one thing and another, say keyboard and shell,
// call the ACPI for something, at the same time, then the ACPI will get sad,
// and this is REQUIRED to avoid that.
// Currently, this is not a concern, hence is being skipped.
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    *OutHandle = (ACPI_SEMAPHORE) 1;
    return AE_OK;
}
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    return AE_OK;
}
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    return AE_OK;
}
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    return AE_OK;
}

// --- SPINLOCKS ---

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    *OutHandle = (ACPI_SPINLOCK) 1;
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) { }

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    ACPI_CPU_FLAGS flags;
    asm volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "cli"
        : "=rm"(flags)
        :
        : "memory"
    );
    return flags;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
    // Restore the flags (this will re-enable interrupts IF they were on before)
    asm volatile(
        "push %0\n\t"
        "popf"
        :
        : "rm"(Flags)
        : "memory", "cc"
    );
}

// --- I/O ---

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    switch (Width) {
        case 8:  *Value = inb(Address); break;
        case 16: *Value = inw(Address); break;
        case 32: *Value = inl(Address); break;
        default: return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    switch (Width) {
        case 8:  outb(Address, (uint8_t)  Value); break;
        case 16: outw(Address, (uint16_t) Value); break;
        case 32: outl(Address, (uint32_t) Value); break;
        default: return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    void* virt = AcpiOsMapMemory(Address, (Width + 7) >> 3);
    if (!virt) return AE_NO_MEMORY;

    switch (Width) {
        case 8:  *Value = *(volatile uint8_t*)  virt; break;
        case 16: *Value = *(volatile uint16_t*) virt; break;
        case 32: *Value = *(volatile uint32_t*) virt; break;
        case 64: *Value = *(volatile uint64_t*) virt; break;
        default: return AE_BAD_PARAMETER;
    }

    AcpiOsUnmapMemory(virt, (Width + 7) >> 3);
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    void* virt = AcpiOsMapMemory(Address, (Width + 7) >> 3);
    if (!virt) return AE_NO_MEMORY;

    switch (Width) {
        case 8:  *(volatile uint8_t*)  virt = (uint8_t)  Value; break;
        case 16: *(volatile uint16_t*) virt = (uint16_t) Value; break;
        case 32: *(volatile uint32_t*) virt = (uint32_t) Value; break;
        case 64: *(volatile uint64_t*) virt = (uint64_t) Value; break;
        default: return AE_BAD_PARAMETER;
    }

    AcpiOsUnmapMemory(virt, (Width + 7) >> 3);
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    uint32_t data = pci_read(PciId->Bus, PciId->Device, PciId->Function, Register);

    uint32_t shift = (Register & 0x03) << 3;
    if (Width == 8)       *Value = (data >> shift) & 0xFF;
    else if (Width == 16) *Value = (data >> shift) & 0xFFFF;
    else if (Width == 32) *Value = data;

    return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    if (Width == 32) {
        pci_write(PciId->Bus, PciId->Device, PciId->Function, Register, (uint32_t) Value);
    } else {
        // Read-Modify-Write for 8/16 bit to avoid overwriting neighbors
        uint32_t current = pci_read(PciId->Bus, PciId->Device, PciId->Function, Register);
        uint32_t shift = (Register & 0x03) << 3;
        uint32_t mask = (Width == 8) ? 0xFF : 0xFFFF;

        current &= ~(mask << shift);
        current |= ((uint32_t) Value & mask) << shift;

        pci_write(PciId->Bus, PciId->Device, PciId->Function, Register, current);
    }

    return AE_OK;
}

// --- DEBUGGING ---

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    uart_printf("ACPI Signal: %d\n", Function);
    return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...) {
    va_list Args;
    va_start(Args, Format);
    uart_vprintf(Format, Args);
    va_end(Args);
}

void AcpiOsVprintf(const char *Format, va_list Args) {
    uart_vprintf(Format, Args);
}
