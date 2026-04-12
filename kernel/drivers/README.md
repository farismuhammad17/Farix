## ACPI (Advanced Configuration and Power Interface)

The ACPI implementation provides a hardware abstraction layer for device discovery, power management, and system configuration. By integrating Intel's **ACPICA** (ACPI Component Architecture), the kernel gains a standardized interface to interpret the ACPI Machine Language (AML) provided by the system firmware.

The reason to use ACPICA instead of making it ourself is simple - ACPICA varies slightly from computer to computer. The majority is the same, but if we want a good ACPI implementation, that we can be sure works everywhere, we **need** to use ACPICA to do that for us, or we can manually implement every single nano-difference. The documentation of each of the ACPICA stubs are in the source code itself, inside the OSL file (`acpi_osl.c`).

This subsystem is critical for moving beyond hardcoded hardware addresses. It allows the kernel to dynamically discover processors, interrupt controllers (APIC), and PCI buses, while also enabling soft-off capabilities and sleep states that are otherwise inaccessible through legacy BIOS calls.

Refer [ACPICA](https://acpica.org/downloads) for Intel's website. This kernel's implementation specifically uses the [UNIX* Format Source Code and Build Environment with an Intel License (.tar.gz, 1.94 MB)](https://www.intel.com/content/www/us/en/download/776303/acpi-component-architecture-downloads-unix-format-source-code-and-build-environment-with-an-intel-license.html)
