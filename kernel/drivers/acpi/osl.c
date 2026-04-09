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

#include "drivers/acpi/acpi.h"
#include "memory/heap.h"
#include "memory/vmm.h"

#include "drivers/uart.h" // TODO REM

ACPI_STATUS AcpiOsInitialize() {
    uart_print("Initilize\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, void (*Function)(void *), void *Context) {
    uart_print("EXec\n");
    if (Function) {
        (void) Function(Context);
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context) {
    uart_print("inst int\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine) {
    uart_print("rem int\n");
    return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds) {
    uart_print("slep\n");
    AcpiOsStall(Milliseconds * 1000);
}

void AcpiOsWaitEventsComplete() {
    uart_print("wait\n");
    return;
}

void* AcpiOsAllocate(ACPI_SIZE Size) {
    uart_print("aloc\n");
    void* ptr = kmalloc(Size);
    if (!ptr) {
        uart_printf("ACPI: osl.AcpiOsAllocate: Heap Exhaustion (Required %d)\n", Size);
    } else {
        kmemset(ptr, 0, Size);
    }
    return ptr;
}

void AcpiOsFree(void *Memory) {
    uart_print("fre\n");
    kfree(Memory);
}

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    uart_print("map\n");
    uintptr_t start_addr = (uintptr_t) PhysicalAddress;
    uintptr_t end_addr   = start_addr + Length;

    uintptr_t base_page  = start_addr & ~0xFFF;
    uintptr_t last_page  = (end_addr - 1) & ~0xFFF;

    for (uintptr_t page = base_page; page <= last_page; page += 0x1000) {
        vmm_map_page(kernel_directory, (void*)page, (void*) PHYSICAL_TO_VIRTUAL(page), PAGE_PRESENT | PAGE_RW);
    }

    return (void*)(uintptr_t) PHYSICAL_TO_VIRTUAL(start_addr);
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
    uart_print("unmap\n");
    uintptr_t addr = (uintptr_t) LogicalAddress;
    uintptr_t end  = addr + Length;

    for (uintptr_t i = addr & ~0xFFF; i < end; i += 0x1000) {
        vmm_unmap_page((void*) i);
    }
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...) {
    uart_print("prnt\n");
    va_list Args;
    va_start(Args, Format);
    uart_printf(Format, Args);
    va_end(Args);
}

void AcpiOsVprintf(const char *Format, va_list Args) {
    uart_print("vprnt\n");
    uart_printf(Format, Args);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    uart_print("create sm\n");
    *OutHandle = (ACPI_SEMAPHORE) 1;
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    uart_print("wait sm\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    uart_print("sig sm\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    uart_print("del sm\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    uart_print("create lk\n");
    *OutHandle = (ACPI_SPINLOCK) 1;
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) { uart_print("del lk\n"); }

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    uart_print("aq lk\n");
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) { uart_print("rel lk\n");}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
    uart_printf("ACPI: Entering AcpiOsGetRootPointer\n");
    ACPI_PHYSICAL_ADDRESS Ret = 0;

    // If it page faults here, your 0x0E0000 - 0x0FFFFF range is unmapped.
    ACPI_STATUS status = AcpiFindRootPointer(&Ret);

    if (ACPI_SUCCESS(status)) {
        uart_printf("ACPI: Found RSDP at 0x%p\n", (void*)(uintptr_t)Ret);
    } else {
        uart_printf("ACPI: Failed to find RSDP\n");
    }
    return Ret;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    uart_print("read prt\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    uart_print("write port\n");
    return AE_OK;
}

UINT64 AcpiOsGetTimer(void) {
    uart_print("get timer\n");
    return 0;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
    uart_print("get thread\n");
    return 1;
}

void AcpiOsStall(UINT32 Microseconds) {
    uart_print("stall\n");
    for (volatile uint32_t i = 0; i < Microseconds * 100; i++) {
        __asm__ volatile("nop"); // TODO: Move to arch stubs
    }
}

ACPI_STATUS AcpiOsTerminate() {
    uart_print("term\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    uart_print("tbl ov\n");
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
    ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
        uart_print("phy tbl ov\n");
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    uart_print("rd mmem\n");
    void* virt = AcpiOsMapMemory(Address, Width >> 3);
    if (!virt) return AE_NO_MEMORY;

    switch (Width) {
        case 8:  *Value = *(volatile uint8_t*)  virt; break;
        case 16: *Value = *(volatile uint16_t*) virt; break;
        case 32: *Value = *(volatile uint32_t*) virt; break;
        case 64: *Value = *(volatile uint64_t*) virt; break;
        default: return AE_BAD_PARAMETER;
    }

    AcpiOsUnmapMemory(virt, Width >> 3);
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    uart_print("write mem\n");
    void* virt = AcpiOsMapMemory(Address, Width >> 3);
    if (!virt) return AE_NO_MEMORY;

    switch (Width) {
        case 8:  *(volatile uint8_t*)  virt = (uint8_t)  Value; break;
        case 16: *(volatile uint16_t*) virt = (uint16_t) Value; break;
        case 32: *(volatile uint32_t*) virt = (uint32_t) Value; break;
        case 64: *(volatile uint64_t*) virt = (uint64_t) Value; break;
        default: return AE_BAD_PARAMETER;
    }

    AcpiOsUnmapMemory(virt, Width >> 3);
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) {
    uart_print("slep 2\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    uart_print("read pci\n");
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    uart_print("write pci\n");
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize,
                              UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache) {
                                uart_print("cr chce\n");
    *ReturnCache = (ACPI_CACHE_T*)(uintptr_t) ObjectSize;
    return AE_OK;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache) {
    uart_print("prge cache\n");
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache) {
    uart_print("del cache\n");
    return AE_OK;
}

void * AcpiOsAcquireObject(ACPI_CACHE_T *Cache) {
    uart_print("aq obj\n");
    return AcpiOsAllocate((uintptr_t) Cache);
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object) {
    uart_print("rel obj\n");
    AcpiOsFree(Object);
    return AE_OK;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal,
                                     ACPI_STRING *NewVal) {
                                         uart_print("pre def ov\n");
    *NewVal = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    uart_print("sig\n");
    AcpiOsPrintf("Farix received fatal ACPI signal: %d\n", Function);
    return AE_OK;
}
