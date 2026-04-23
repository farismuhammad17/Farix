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

#include "hal.h"

#include "cpu/ints.h"
#include "cpu/pci.h"
#include "cpu/timer.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "memory/pmm.h"
#include "memory/slab.h"
#include "process/task.h"

#include "drivers/acpi/acpi.h"

#define OS_SLEEP_MAX_MICROSECONDS 3000000
#define MAX_DEFERRED_UNMAPS 64

typedef struct {
    uint32_t units;
    uint32_t max_units;
    uint32_t magic;
} __attribute__((aligned(8))) acpi_semaphore_t;

static void*  unmap_queue[MAX_DEFERRED_UNMAPS];
static size_t unmap_count = 0;

static ACPI_PHYSICAL_ADDRESS AcpiRSDP = NULL;

static Slab64* acpi_head64 = NULL;
static Slab32* acpi_head32 = NULL;
static Slab16* acpi_head16 = NULL;
static Slab8*  acpi_head8  = NULL;

// --- INITIALISATION ---

// This is the first init_acpi function
ACPI_STATUS AcpiOsInitialize() {
    acpi_head64 = create_slab64(64);  // 64-byte objects
    acpi_head32 = create_slab32(128); // 128-byte objects
    acpi_head16 = create_slab16(256); // 256-byte objects
    acpi_head8  = create_slab8(512);  // 512-byte objects

    if (unlikely(!acpi_head64 || !acpi_head32 || !acpi_head16 || !acpi_head8)) {
        uart_printf("ACPI OSL: Failed to initialize slab pools\n");
        return AE_NO_MEMORY;
    }

    AcpiRSDP = AcpiOsGetRootPointer();

    if (AcpiRSDP == 0) return AE_NOT_FOUND;

    ACPI_PHYSICAL_ADDRESS found_rsdp = 0;
    AcpiFindRootPointer(&found_rsdp);

    return AE_OK;
}

// This is a cleanup function ACPICA uses once it is done, and is part
// of the shutdown sequence.
ACPI_STATUS AcpiOsTerminate() {
    AcpiMappingCleanup();
    return AE_OK;
}

// This function registers an OS-level interrupt handler for a specific
// ACPI event. It maps the hardware InterruptNumber to the ServiceRoutine
// function. When the interrupt fires, the kernel executes that routine
// with the provided Context. It returns AE_OK if the mapping succeeds
// or AE_ALREADY_EXISTS if the slot is taken.
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context) {
    // ACPICA gives us an IRQ (0-15). We map it to the IDT range (32-47).
    uint8_t vector = (uint8_t)(32 + InterruptNumber);
    set_interrupt_kernel(vector, (void*) ServiceRoutine);
    return AE_OK;
}

// This function uninstalls the interrupt handler previously registered for
// InterruptNumber. It ensures the ServiceRoutine is no longer executed when
// the hardware interrupt triggers. You must call this during driver unloading
// or system shutdown to prevent the kernel from jumping to invalid memory
// addresses when an ACPI interrupt occurs.
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine) {
    uint8_t vector = (uint8_t)(32 + InterruptNumber);
    clear_interrupt(vector);
    return AE_OK;
}

// AcpiOsExecute() schedules a function for deferred execution, typically by
// placing it in a kernel work queue or a dedicated thread. It prevents long-running
// ACPI tasks from blocking the main execution flow. Use it to run background service
// routines or asynchronous callbacks without stalling the core interpreter.
ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, void (*Function)(void *), void *Context) {
    if (Function) {
        (void) Function(Context);
    }
    return AE_OK;
}

// This function locates and returns the physical address of the Root System Description
// Pointer (RSDP). The RSDP is the anchor for all ACPI tables. On EFI systems, the OSL
// usually finds this via the EFI configuration table; on legacy BIOS systems, it scans
// specific physical memory ranges (like the EBDA) for the "RSD PTR " signature.
// In this implementation, we search for "RSD PTR ", and once found, store it at the
// AcpiOsInitialize function, because the location of it is specific, and its not
// going to move away from where it is.
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

    t_print("AcpiGetRootPointer (ACPI): AcpiOsGetRootPointer: RSDP NOT FOUND");
    uart_printf("AcpiGetRootPointer (ACPI): AcpiOsGetRootPointer: RSDP NOT FOUND\n");
    return 0;
}

// --- MULTITASKING ---

// AcpiOsGetThreadId() returns a unique identifier for the currently executing thread.
// ACPICA uses this to track resource ownership, manage mutexes, and prevent deadlocks
// within the interpreter. In a kernel, you typically return the address of the
// current thread's control block or a unique ID from your task scheduler.
ACPI_THREAD_ID AcpiOsGetThreadId() {
    return current_task->id;
}

// --- TIME ---

// ACPICA calls this for waits too short to deserve a task switch
void AcpiOsStall(UINT32 Microseconds) {
    timer_stall((uint32_t) Microseconds);
}

// ACPICA calls this when it wants to take a nap and do nothing,
// TODO: I'll make it switch task during this function later, but I'm lazy.
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

// This function blocks the execution of the calling thread until all asynchronous
// events currently queued via AcpiOsExecute() have finished processing. It acts
// as a synchronization barrier to ensure that no background ACPI tasks are pending
// before the system proceeds with state changes or resource deallocation.
void AcpiOsWaitEventsComplete() {
    return;
}

// Wants per 100-nanosecond as unit
UINT64 AcpiOsGetTimer() {
    return get_timer_uptime_microseconds() * 10;
}

// --- MEMORY ---

// This function maps a physical memory range containing ACPI tables into the kernel's
// virtual address space. It accepts a physical start address and the region length,
// returning a virtual pointer to the mapped memory. This allows the ACPICA interpreter
// to read hardware-defined tables within a virtual memory environment.
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

// This function invalidates a previously established virtual mapping. It accepts the virtual
// (logical) address and the length of the memory region to be released. It returns nothing
// (void), but ensures the virtual address space is freed and any associated page table entries
// are cleared by the kernel.
void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
    // idk why, but leaving this crashes us, even though this won't actually
    // unmap any memory. To avoid a page fault on literal boot, I am going to
    // just comment it out, since we don't unmap any of the ACPICA pages anyway.

    // if (!LogicalAddress || Length == 0) return;

    // uintptr_t addr = (uintptr_t) LogicalAddress;
    // uintptr_t base_page = addr & ~0xFFF;
    // uintptr_t last_page = (addr + Length - 1) & ~0xFFF;

    // for (uintptr_t pg = base_page; pg <= last_page; pg += 0x1000) {
    //     uintptr_t phys = vmm_get_phys(vmm_get_current_directory(), (void*) pg);

    //     // Never unmap the first 32MB (Initial Kernel/Identity Map)
    //     if (phys < (VMM_INIT_MAP_SIZE << 20)) continue;

    //     // Only unmap if it's NOT the page containing the RSDT or DSDT headers
    //     // ACPICA often keeps these pointers cached internally.
    //     if (phys == (AcpiRSDP & ~0xFFF)) continue;

    //     unmap_queue[unmap_count++] = (void*) pg;
    //     if (unmap_count >= MAX_DEFERRED_UNMAPS) uart_print("ACPI OSL: Unmap queue full");
    // }
}

// Defined in arch/kernel.h
void AcpiMappingCleanup() {
    for (size_t i = 0; i < unmap_count; i++)
        pmm_free_page(vmm_unmap_page(unmap_queue[i]));

    unmap_count = 0;
}

// This function performs dynamic memory allocation for the ACPICA subsystem. It requires the
// Size of the requested block in bytes and returns a virtual pointer to the allocated memory.
// If the allocation fails due to insufficient heap space, it returns NULL. It must provide
// 8-byte aligned memory.
void* AcpiOsAllocate(ACPI_SIZE Size) {
    void* ptr = NULL;

    // We check from smallest capacity (large objects) to largest capacity (small objects)

    if (Size <= 64) {
        ptr = slab_alloc64(acpi_head64);
    } else if (Size <= 128) {
        ptr = slab_alloc32(acpi_head32);
    } else if (Size <= 256) {
        ptr = slab_alloc16(acpi_head16);
    } else if (Size <= 512) {
        ptr = slab_alloc8(acpi_head8);
    } else {
        // Fallback for stuff we can't slab
        ptr = kmalloc(Size);
    }

    if (ptr) kmemset(ptr, 0, Size);
    return ptr;
}

// This function releases a block of memory previously allocated via AcpiOsAllocate. It takes a
// virtual pointer to the memory buffer as its argument and returns nothing (void). Its primary
// purpose is to return the specified memory to the kernel's heap, preventing leaks within the
// ACPI subsystem.
void AcpiOsFree(void *Memory) {
    if (!Memory) return;

    // This works because every Slab header is at the very beginning of a PMM page.
    uintptr_t page_base = (uintptr_t) Memory & 0xFFFFF000;

    // Cast it to a generic Slab pointer to check the magic
    // We can use Slab8 as a template since slab_magic is at the same offset in all of them
    Slab8* slab = (Slab8*) page_base;

    if (likely(slab->slab_magic == SLAB_MAGIC)) {
        // We look at the capacity (free_slots at init) or obj_shift.
        // Based on our initialization: 6=64, 7=128, 8=256, 9=512
        switch (slab->obj_shift) {
            case 6:  slab_free64(Memory); break;
            case 7:  slab_free32(Memory); break;
            case 8:  slab_free16(Memory); break;
            case 9:  slab_free8(Memory);  break;
            default:
                uart_printf("AcpiOsFree: Slab has magic but invalid shift: %d\n", slab->obj_shift);
                break;
        }
    }

    // Not a slab
    else kfree(Memory);
}

// This function initializes a local memory cache for frequently allocated fixed-size objects.
// It requires a name string, object size, and maximum depth. It returns an ACPI_STATUS code and
// provides a handle to the new cache via a pointer. This optimization reduces fragmentation and
// overhead during frequent interpreter allocations.
ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize, UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache) {
    if (!ReturnCache || ObjectSize == 0) return AE_BAD_PARAMETER;

    // We use the Size as the handle.
    *ReturnCache = (ACPI_CACHE_T*)(uintptr_t) ObjectSize;

    return AE_OK;
}

// This function removes all entries currently residing in the specified memory cache. It takes
// a pointer to the cache handle as input and returns an ACPI_STATUS code. This operation frees
// the memory associated with unused objects in the cache without destroying the cache object itself.
ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache) {
    return AE_OK;
}

// This function destroys a previously created memory cache and releases all associated resources.
// It takes a pointer to the cache handle as its argument and returns an ACPI_STATUS code. Before
// deletion, the function ensures all cached objects are freed to the system heap, invalidating
// the cache handle.
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

// This function allows the host operating system to override a specific ACPI hardware
// table with a different version. It takes a pointer to the existing table header,
// returning a new physical address and table length if an override is provided.
// It returns AE_OK, regardless of whether an override occurs.
ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
    return AE_SUPPORT;
}

// This function allows the operating system to override the default value of a predefined
// ACPI object. It takes a pointer to the initial predefined name structure and returns a
// new string value if an override is available. It returns AE_OK to indicate the check was
// performed, facilitating platform-specific adjustments to standard ACPI names.
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal) {
    *NewVal = NULL;
    return AE_OK;
}

// This function retrieves a pre-allocated memory object from a specific cache. It takes a
// pointer to the cache handle and returns a virtual pointer to an available object. If the
// cache is empty, the function typically allocates a new object from the system heap to
// fulfill the request.
void * AcpiOsAcquireObject(ACPI_CACHE_T *Cache) {
    return AcpiOsAllocate((uintptr_t) Cache);
}

// This function returns a previously acquired memory object to the specified cache for future
// reuse. It requires a pointer to the cache handle and the virtual pointer to the object being
// released. It returns an ACPI_STATUS code to indicate whether the object was successfully
// reintegrated into the cache pool.
ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object) {
    AcpiOsFree(Object);
    return AE_OK;
}

// --- SEMAPHORES ---

// This function initializes a new counting semaphore for synchronization. It requires the
// maximum unit count, the initial unit count, and a pointer to an output handle. It returns
// an ACPI_STATUS code and populates the handle if successful. This is used by the interpreter
// to manage shared resource access.

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    acpi_semaphore_t* sem = kmalloc(sizeof(acpi_semaphore_t));
    if (!sem) return AE_NO_MEMORY;

    sem->units = InitialUnits;
    sem->max_units = MaxUnits;

    *OutHandle = (ACPI_SEMAPHORE) sem;
    return AE_OK;
}

// This function blocks the calling thread until the requested number of units is available in
// the semaphore. It takes the semaphore handle, the number of units to acquire, and a timeout value
// in milliseconds. It returns AE_OK on acquisition or AE_TIME if the timeout period expires before
// units become available.
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    if (!Handle) return AE_BAD_PARAMETER;
    acpi_semaphore_t* sem = (acpi_semaphore_t*) Handle;

    // TODO: check if (sem->units >= Units).
    // If not, you'd yield the thread.
    // But we aren't letting ACPICA anywhere yet, so it doesn't matter
    if (sem->units >= Units) {
        sem->units -= Units;
        return AE_OK;
    }

    sem->units -= Units;
    return AE_OK;
}

// This function increments the unit count of a semaphore, potentially unblocking waiting threads.
// It requires the semaphore handle and the number of units to release. It returns an ACPI_STATUS code.
// This is the primary mechanism for signaling that a shared resource or event is now available.
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    if (!Handle) return AE_BAD_PARAMETER;
    acpi_semaphore_t* sem = (acpi_semaphore_t*) Handle;
    sem->units += Units;
    return AE_OK;
}

// This function destroys an existing semaphore and releases all kernel resources associated with it.
// It takes the semaphore handle as input and returns an ACPI_STATUS code. This should only be called
// when the semaphore is no longer needed and no threads are currently blocked waiting on it.
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    if (Handle) kfree(Handle);

    return AE_OK;
}

// --- SPINLOCKS ---

// This function initializes a new spinlock for low-level synchronization in multi-core environments.
// It requires a pointer to an output handle and returns an ACPI_STATUS code. This primitive is used
// to protect small, critical sections of code where blocking is not permitted, ensuring mutual exclusion
// at the hardware level.
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    void* lock = kmalloc(sizeof(uint32_t));
    if (!lock) return AE_NO_MEMORY;

    *(uint32_t*) lock = 0;
    *OutHandle = (ACPI_SPINLOCK) lock;

    return AE_OK;
}

// This function destroys a previously created spinlock and releases any associated kernel resources.
// It takes the spinlock handle as its argument and returns nothing (void). This must only be called
// when the lock is no longer required and is not currently held by any processor.
void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) { }

// This function disables interrupts on the local processor and acquires the specified spinlock. It
// takes the spinlock handle and returns the current CPU flags (interrupt state). This prevents the
// current execution flow from being interrupted or preempted while holding a resource that other
// cores may also attempt to access.
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

// This function releases a previously acquired spinlock and restores the processor to its previous
// interrupt state. It requires the spinlock handle and the CPU flags returned by the initial acquisition.
// This ensures that the local interrupt state is correctly balanced and the resource is made available
// to other processors.
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

// This function reads a value from a hardware I/O port. It accepts the port address, a pointer to
// store the result, and the bit width (8, 16, or 32). It returns an ACPI_STATUS code. This is essential
// for the interpreter to interact with legacy hardware mapped to the I/O space.
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    switch (Width) {
        case 8:  *Value = inb(Address); break;
        case 16: *Value = inw(Address); break;
        case 32: *Value = inl(Address); break;
        default: return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

// This function writes a specific value to a hardware I/O port. It requires the port address, the value to
// write, and the bit width. It returns an ACPI_STATUS code. It allows the ACPI subsystem to send commands
// or data to hardware devices through the processor's I/O instructions.
ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    switch (Width) {
        case 8:  outb(Address, (uint8_t)  Value); break;
        case 16: outw(Address, (uint16_t) Value); break;
        case 32: outl(Address, (uint32_t) Value); break;
        default: return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

// This function reads data from a specified physical memory location. It takes the physical address, a pointer
// for the value, and the bit width (up to 64). It returns an ACPI_STATUS code. This is typically used to read
// memory-mapped I/O (MMIO) registers defined in the ACPI namespace.
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

// This function writes a value to a physical memory address. It requires the physical address, the value to be
// written, and the bit width. It returns an ACPI_STATUS code. This enables the subsystem to configure hardware
// components that utilize memory-mapped registers for control and status reporting.
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

// This function reads from the configuration space of a specific PCI device. It accepts a PCI ID structure (bus,
// device, function), the register offset, a pointer for the data, and the width. It returns an ACPI_STATUS code,
// facilitating hardware discovery and resource management within the PCI hierarchy.
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    uint32_t data = pci_read(PciId->Bus, PciId->Device, PciId->Function, Register);

    uint32_t shift = (Register & 0x03) << 3;
    if (Width == 8)       *Value = (data >> shift) & 0xFF;
    else if (Width == 16) *Value = (data >> shift) & 0xFFFF;
    else if (Width == 32) *Value = data;

    return AE_OK;
}

// This function writes a value to the configuration space of a designated PCI device. It takes the PCI ID, the
// register offset, the value, and the bit width. It returns an ACPI_STATUS code. This allows the ACPI interpreter
// to modify PCI device states, such as power management registers or interrupt configurations.
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

// This function allows the ACPICA interpreter to signal the host operating system regarding specific events
// or fatal errors. It accepts a function code representing the signal type and a pointer to context-specific
// information. It returns an ACPI_STATUS code. This is primarily used for debugger breakpoints or reporting
// fatal ACPI errors to the kernel.
ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    t_printf("ACPI: %d", Function);
    uart_printf("ACPI: %d\n", Function);
    return AE_OK;
}

// This function provides formatted string output for ACPICA, similar to the standard C printf. It accepts a
// format string and a variable number of arguments. It returns nothing (void). The OSL must direct this output
// to the appropriate kernel console, serial port, or log buffer for debugging and status reporting.
void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...) {
    va_list Args;
    va_start(Args, Format);
    uart_vprintf(Format, Args);
    va_end(Args);
}

// This function serves as the underlying implementation for formatted output, handling variable argument lists.
// It takes a format string and a va_list of arguments. It returns nothing (void). This allows the interpreter
// to pass pre-processed argument lists from various internal diagnostic routines to the host's output hardware.
void AcpiOsVprintf(const char *Format, va_list Args) {
    uart_vprintf(Format, Args);
}
