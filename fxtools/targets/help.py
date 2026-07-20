"""
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
"""

import os
import textwrap

from fxtools.core import printer

HEADER = f"""
{printer.F_CYAN('Farix Build System')} {printer.F_GREY('(https://github.com/farismuhammad17/Farix)')}
Use {printer.F_MAGENTA('fx [target] help')} for further information.
"""

INFO = {
    "Targets": {
        "make": "Compiles the kernel into bootable ISO and creates disk.img with system files",
        "clean": "Cleans up build artifacts and temporary files",
        "init": "Fetches dependencies and sets up the build requirements",
        "disk": "Create and format the disk.img file",
        "qemu": "Launch the kernel in a QEMU emulator",
        "lint": "Verify code cleanliness and compute a unique hash of the entire project tree",
        "defs": "Launch an interactive program to help find function documentation",
        "commit": "Commit changes to git",
        "sign setup": "Initialise public and private keys",
        "sign file": "Create a signature file",
        "sign changes": "Get signature for generated project checksum",
        "sign verify file": "Check file signature against public key",
        "sign verify commit": "Check commit signature against public key",
        "config list": "Current values in state JSON",
        "config set": "Set value of given field in state JSON",
        "config setup-profile": "Setup user specific fields",
        "config update": "Update current state JSON for new fields in default scheme"
    },

    "Supported Architectures": {
        "x86_64": "64-bit x86 (Default)",
        "x86_32": "32-bit x86",
        "arm32": "32-bit ARM (WIP)"
    },

    "Configuration": {
        "DEFAULT_ARCH": "Architecture to compile to",
        "THREADS": "Default number of threads to use for compiling",
        "BOOT_USB_PATH": "Path to USB to copy kernel to for 'fx usb'",
        "RUNTIME_CORES": "Number of CPU cores during runtime in emulation",
    }
}

def run():
    print(HEADER)

    # Find longest key section for target X
    all_keys = [key for section in INFO.values() for key in section.keys()]
    max_key_len = max(len(key) for key in all_keys) if all_keys else 0

    # Define layout rules
    indent_size = 2                      # Spaces before the key
    spacing = 4                          # Spaces between key and value
    value_x_offset = indent_size + max_key_len + spacing

    terminal_width = os.get_terminal_size().columns
    wrap_width = max(30, terminal_width - value_x_offset)  # Ensure we have at least 30 chars for text

    for section, items in INFO.items():
        print(printer.F_CYAN(section))
        for key, value in items.items():
            # Pad the key so the ANSI escape sequences start at the exact same point
            padded_key = key.ljust(max_key_len)
            key_part = f"  {printer.F_GREEN(padded_key)}" + (" " * spacing)

            # Wrap the value to prevent overflow and align subsequent lines to the value_x_offset
            wrapped_lines = textwrap.wrap(value, width=wrap_width)

            if wrapped_lines:
                # Print the first line inline with the key
                print(f"{key_part}{wrapped_lines[0]}")
                # Print any subsequent wrapped lines indented to the exact same X coordinate
                for line in wrapped_lines[1:]:
                    print(" " * value_x_offset + line)
            else:
                print(key_part)

        print()
