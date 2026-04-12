Anything that interacts and changes how the CPU works goes here.

# Interrupts

Making a kernel, a piece of software, manually manage everything by constantly checking stuff like keyboard inputs, mouse inputs, page faults, etc. is really slow, and throttles our performance. Often times, when such situations show up, the hardware comes to the rescue.

Interrupts are architecture specified addresses that the CPU jumps to, whenever a specific 'event' is triggered. If a key on the keyboard is pressed, it jumps to the keyboard interrupt, and if we enounter a page fault, it jumps to its address. This keeps everything lean, and throws all the overhead of these checks to the CPU.

This is actually architecture dependant, so you will find its respective implementation in each of the arch files under the unique name of that architecture (eg: IDT for x86, EVT for ARM).

```c
void init_interrupts();
```

This function sets all the addresses up, and informs whatever architecture the kernel is running on of all the things to watch out for. Since the CPU only jumps to these addresses, we actually have to tell it what to exactly do once we reach these addresses. These are defined in assembly functions that call functions defined in C.

# System Timer

Without a hardware timer, the kernel has no concept of time passing. It wouldn't be able to multitask, and a single process could hold the CPU forever. To solve this, we use a hardware timer chip that acts like the "heartbeat" of the system.

The timer is a hardware device that sends a periodic interrupt signal to the CPU at a fixed frequency. Every time this "tick" occurs, the CPU pauses current execution and jumps to the timer interrupt handler. This allows the kernel to regain control, update system uptime, and decide if it's time to switch to a different task (preemptive multitasking).

This is architecture specfic, and you will find the specific implementations in the arch folder.

```c
void init_timer(uint32_t frequency);
```

This function calculates the necessary divisors for the hardware clock to fire at the requested frequency (in Hz). It then registers the timer interrupt handler so the kernel can begin tracking time and performing context switches.

```c
uint64_t get_timer_uptime_microseconds();
```

This gets the number of microseconds that has passed since `init_timer`

```c
void timer_stall(uint32_t microseconds);
```

This stalls the entire computer by the given microseconds.

# Peripheral Component Interconnect (PCI)

The PCI bus is the primary way the kernel discovers and communicates with hardware peripherals like network cards, storage controllers, and graphics adapters. Without a PCI driver, the kernel cannot "see" the hardware attached to the motherboard.

The system treats the PCI bus as a tree-like structure composed of **Buses**, **Devices**, and **Functions**. The kernel probes this structure to identify what is plugged in by reading unique Vendor and Device IDs from each device's configuration space. This process allows the kernel to map hardware to the correct software drivers.

```c
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint8_t  class_code;
    uint8_t  subclass;
} pci_device_t;
```

This structure stores essential identification data for a discovered device. The `class_code` and `subclass` are used to determine the device type (e.g., Mass Storage or Network Controller).

```c
void init_pci();
```

This function scans the PCI bus for connected hardware. It iterates through possible bus/device/function combinations, populates the `pci_devices` array with found hardware, and updates `pci_device_count`.

```c
uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
```

Reads a 32-bit value from a specific register in a device's configuration space. This is used to gather detailed hardware information or status.

```c
void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
```

Writes a 32-bit value to a device register. This is necessary for configuring hardware, such as enabling Bus Mastering or setting up Memory Mapped I/O (MMIO).
