All files related to the Memory Management Unit will be here.

# Physical Memory Management (PMM)

The implementation details may vary through architectures.

The Physical Memory Manager is the most fundamental layer of the system's memory stack. It acts as the "bookkeeper" for the actual silicon RAM chips installed in the computer. Its primary responsibility is to track every individual block of physical memory, a **Page**, and determine whether it is currently free or in use by the kernel or a process.

* **Resource Tracking:** Maintaining a map (often a bitmap or a stack) of all available physical RAM.
* **Page Allocation:** Providing a unique physical address when a system component needs a new 4KB block of memory.
* **Hardware Protection:** Ensuring that critical regions, such as the space where the kernel code itself resides, are never handed out to other processes.

# Virtual Memory Management (VMM)

The implementation details may vary through architectures.

The Virtual Memory Manager provides an abstraction layer that sits between the software and the physical RAM. It creates a "Virtual Address Space" for every process, making it appear as though each program has access to the entire range of memory addresses, completely isolated from other programs.

* **Address Translation:** Mapping "Virtual" addresses used by code to "Physical" addresses tracked by the PMM.
* **Memory Isolation:** Preventing a process from accessing or modifying the memory of another process or the kernel.
* **Permission Control:** Assigning specific attributes to memory regions, such as marking code as "Read-Only" or data as "No-Execute" to prevent security exploits.
* **On-Demand Mapping:** Linking physical pages to virtual addresses only when they are actually needed.

# The Heap

While the PMM and VMM deal with large, fixed-size blocks (pages), the Heap provides a way to allocate memory of **arbitrary sizes**. It is a higher-level allocator that manages a large pool of memory, allowing software to request exactly the number of bytes it needs for objects, strings, or data structures.

* **Granular Allocation:** Managing small requests by carving them out of the larger pages provided by the VMM/PMM.
* **Fragmentation Management:** Efficiently organizing used and free "chunks" of memory to ensure that space isn't wasted.
* **Object Lifecycle:** Tracking the start and end of allocated blocks so they can be safely freed and reused by other parts of the system later.
