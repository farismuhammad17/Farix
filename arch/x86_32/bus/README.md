# PCI (Peripheral Component Interconnect)

The PCI bus is the standard interface for connecting high-speed hardware, such as network cards, sound cards, and modern storage controllers, to the motherboard. Unlike older "Legacy" hardware (like the Keyboard or PIC) which exists at fixed, hardcoded addresses, PCI devices are discoverable. The kernel must iterate through the PCI bus to identify what hardware is actually plugged into the system and determine its capabilities.

Each PCI device is identified by a **Vendor ID** (who made it) and a **Device ID** (what it is). By reading the **Class Code**, the kernel can identify a device's function (e.g., "Mass Storage Controller" or "Network Controller") even if it doesn't have a specific driver for that exact model.

```c
void init_pci();
```

Performs a "PCI Bus Enumeration." It loops through every possible bus (0–255) and every possible device (0–31) to check for hardware. If it finds a device that does not return `0xFFFF` (a non-existent device), it records the Vendor, Device, and Class information into the `pci_devices` array. This registry acts as the source of truth for the kernel when loading specialized drivers later in the boot process.

```c
static uint32_t read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
```

Accesses the **PCI Configuration Space** using I/O ports `0xCF8` (Address) and `0xCFC` (Data). Since the CPU cannot address PCI configuration memory directly, it must write a specifically formatted 32-bit integer to the Address port—containing the bus, device, and function numbers—and then read the resulting value from the Data port. This function is the fundamental building block for all PCI interaction.

| Field | Bit Range | Description |
| :--- | :--- | :--- |
| **Vendor ID** | 0-15 | Identifies the manufacturer (e.g., `0x8086` for Intel). |
| **Device ID** | 16-31 | Identifies the specific model of the hardware. |
| **Class Code** | 24-31 | Defines the general category (e.g., `0x01` for Storage). |
| **Subclass** | 16-23 | Provides more specific detail (e.g., `0x01` might be IDE). |
