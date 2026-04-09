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

ACPI_STATUS AcpiOsInitialize() {
    return AE_OK;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, void (*Function)(void *), void *Context) {
    if (Function) {
        (void) Function(Context);
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context) {
    return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine) {
    return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds) {
    AcpiOsStall(Milliseconds * 1000);
}

void AcpiOsWaitEventsComplete() {
    return;
}

void* AcpiOsAllocate(ACPI_SIZE Size) {
    return malloc(Size);
}

void AcpiOsFree(void *Memory) {
    free(Memory);
}

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    return (void *)(uintptr_t) PHYSICAL_TO_VIRTUAL(PhysicalAddress);
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
    uintptr_t addr = (uintptr_t) LogicalAddress;
    uintptr_t end  = addr + Length;

    for (uintptr_t i = addr & ~0xFFF; i < end; i += 0x1000) {
        vmm_unmap_page((void*) i);
    }
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...) {
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
}

void AcpiOsVprintf(const char *Format, va_list Args) {
    vprintf(Format, Args);
}

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

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    *OutHandle = (ACPI_SPINLOCK) 1;
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) { }

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) { }

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
    ACPI_PHYSICAL_ADDRESS Ret;
    AcpiFindRootPointer(&Ret);
    return Ret;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    return AE_OK;
}

UINT64 AcpiOsGetTimer(void) {
    return 0;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
    return 1;
}

void AcpiOsStall(UINT32 Microseconds) {
    for (volatile uint32_t i = 0; i < Microseconds * 100; i++) {
        __asm__ volatile("nop"); // TODO: Move to arch stubs
    }
}

ACPI_STATUS AcpiOsTerminate() {
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
    ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
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
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    return AE_NOT_IMPLEMENTED;
}

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

void * AcpiOsAcquireObject(ACPI_CACHE_T *Cache) {
    return AcpiOsAllocate((uintptr_t) Cache);
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object) {
    AcpiOsFree(Object);
    return AE_OK;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal,
                                     ACPI_STRING *NewVal) {
    *NewVal = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    AcpiOsPrintf("Farix received fatal ACPI signal: %d\n", Function);
    return AE_OK;
}
