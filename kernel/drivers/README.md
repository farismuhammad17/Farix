# ACPI (Advanced Configuration and Power Interface)

The ACPI implementation provides a hardware abstraction layer for device discovery, power management, and system configuration. By integrating Intel's **ACPICA** (ACPI Component Architecture), the kernel gains a standardized interface to interpret the ACPI Machine Language (AML) provided by the system firmware.

The reason to use ACPICA instead of making it ourself is simple - ACPICA varies slightly from computer to computer. The majority is the same, but if we want a good ACPI implementation, that we can be sure works everywhere, we **need** to use ACPICA to do that for us, or we can manually implement every single nano-difference. The documentation of each of the ACPICA stubs are in the source code itself, inside the OSL file (`acpi_osl.c`).

This subsystem is critical for moving beyond hardcoded hardware addresses. It allows the kernel to dynamically discover processors, interrupt controllers (APIC), and PCI buses, while also enabling soft-off capabilities and sleep states that are otherwise inaccessible through legacy BIOS calls.

Refer [ACPICA](https://acpica.org/downloads) for Intel's website. This kernel's implementation specifically uses the [UNIX* Format Source Code and Build Environment with an Intel License (.tar.gz, 1.94 MB)](https://www.intel.com/content/www/us/en/download/776303/acpi-component-architecture-downloads-unix-format-source-code-and-build-environment-with-an-intel-license.html)

# BDL (Block Device Layer)

The BDL is an abstraction layer to the read and write functions for the drive. Different drives would work differently, and since we wish to accomodate for them all, we need a BDL to abstract off those functions.

```c
void init_storage();
```

The `init_storage` function scans through the PCI found devices. If it finds the AHCI, it uses it, and if AHCI is not found, it resorts to using the legacy ATA driver instead.

```c
void bdl_mount(BDLDevice* dev);
```

The layer requires you to have a `BDLDevice` struct, wherein you pass in the `read` and `write` functions. This struct can be mounted by passing its address to the `bdl_mount` function.

```c
void bdl_read(uint32_t lba, void* buf);
void bdl_write(uint32_t lba, void* buf);
```

The abstracted functions that use the last mounted BDL read/write functions.
