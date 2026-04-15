#!/usr/bin/env python3

"""
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
"""

import os
import subprocess
import sys
import shutil
import time
import re
import json

from makefile       import globals
from makefile.apps  import *
from makefile.bin   import *
from makefile.img   import *
from makefile.qemu  import *
from makefile.usb   import *
from makefile.deps  import *
from makefile.libc  import *
from makefile.utils import *
from makefile.help  import *

globals.IGNORES = (
    # These ACPI stuff come with ACPICA, having these make it easier to
    # install newer versions of ACPICA without having to manually delete
    # any of these.
    "kernel/drivers/acpi/common/",
    "kernel/drivers/acpi/components/debugger/",
    "kernel/drivers/acpi/components/disassembler/",
    "kernel/drivers/acpi/components/utilities/utclib.c",
    "kernel/drivers/acpi/components/utilities/utprint.c",
    "kernel/drivers/acpi/components/utilities/utcache.c",
    "kernel/drivers/acpi/components/resources/rsdump.c",

    # User system calls would get in the way of the kernel's,
    # so we don't actually compile it with the rest of the
    # kernel, even though it is part of the whole thing.
    "arch/x86_32/libc/",
    "arch/arm32/libc/",
)

# --- INITIALIZATION ---

globals.OS = globals.run("uname -s")

globals.DISK_PATH = "build/disk.img"

globals.arch = "x86_32"
if "arm32" in sys.argv:
    globals.arch = "arm32"

if globals.arch == "x86_32":
    globals.QEMU_BIN = "qemu-system-i386"
    globals.QEMU_FLAGS = (
        "-machine pc,accel=tcg "
        "-cpu pentium "
        "-m 256 "
        "-boot menu=on,strict=on "
        "-global i8042.extended-state=off "
        "-device piix4-ide,id=pci-ide0 "
        "-device ide-hd,bus=pci-ide0.0,drive=hd0 "
        "-device virtio-mouse-pci "
        f"-drive file={globals.DISK_PATH},if=none,id=hd0,format=raw,media=disk "
        "-vga std "
        "-serial stdio "
    )

    if globals.shell_which("i686-elf-gcc"):
        globals.PREFIX = "i686-elf-"
    elif globals.shell_which("i386-elf-gcc"):
        globals.PREFIX = "i386-elf-"
    else:
        print("Error: x86 cross-compiler not found")
        sys.exit(1)
elif arch == "arm32":
    globals.PREFIX = "arm-none-eabi-"
    globals.QEMU_BIN = "qemu-system-arm"
    globals.QEMU_FLAGS = f"-drive format=raw,file={globals.DISK_PATH},index=0,media=disk -serial stdio -M raspi2b"
    globals.ASM_ASSEMBLER = "arm-none-eabi-as"
else:
    print("\x1b[31mInvalid architecture\x1b[0m")
    sys.exit(1)

globals.TARGET_DIR = globals.PREFIX.rstrip('-')

globals.PROJECT_ROOT = os.getcwd()

globals.NEWLIB_SRC = os.path.join(globals.PROJECT_ROOT, "newlib-cygwin")
globals.NEWLIB_TARGET = globals.PREFIX.rstrip('-')
globals.LIBC_INSTALL_DIR = os.path.join(os.getcwd(), f"libc_build_{globals.arch}")
globals.LIBC_DIR = os.path.join(globals.LIBC_INSTALL_DIR, globals.TARGET_DIR)
globals.LIBC_INC = os.path.join(globals.LIBC_DIR, "include")
globals.LIBC_LIB = os.path.join(globals.LIBC_DIR, "lib")

globals.ACPICA_ARCH_INDEPENDANT = os.path.join(globals.PROJECT_ROOT, "include/drivers/acpi")
globals.ACPICA_ARCH_DEPENDANT   = os.path.join(globals.PROJECT_ROOT, f"include/arch/{globals.arch}/acpi")

globals.CC = f"{globals.PREFIX}gcc"
globals.AS = f"{globals.PREFIX}as"

globals.CFLAGS = f"-ffreestanding -O2 -Wall -Wextra -fno-exceptions \
    -fdiagnostics-color=always \
    -Iinclude -I{globals.LIBC_INC} -I{globals.ACPICA_ARCH_INDEPENDANT} -I{globals.ACPICA_ARCH_DEPENDANT}"

globals.BOOT_OBJ = "build/boot.o"

globals.CRTBEGIN = globals.run(f"{globals.CC} {globals.CFLAGS} -print-file-name=crtbegin.o")
globals.CRTEND   = globals.run(f"{globals.CC} {globals.CFLAGS} -print-file-name=crtend.o")

globals.USER_LIBC_DIR = f"arch/{globals.arch}/libc"
globals.USER_LIBC = f"{globals.USER_LIBC_DIR}/user.c"
globals.USER_ASM  = f"{globals.USER_LIBC_DIR}/user.asm"

globals.USER_BUILD_DIR = "build/apps"
globals.USER_LIBC_OBJ = f"{globals.USER_BUILD_DIR}/user.o"
globals.USER_ASM_OBJ  = f"{globals.USER_BUILD_DIR}/user_asm.o"

globals.APPS_ROOT = "apps"
globals.USER_CFLAGS = f"-ffreestanding -O2 -Iinclude -I{globals.LIBC_INC}"

globals.MAKE_CONF_JSON = "make.conf.json"

globals.GRUB_CFG = (
    "set timeout=0\n"
    "set default=0\n\n"
    "menuentry 'Farix OS' {\n"
    "    multiboot /farix.bin\n"
    "    boot\n"
    "}\n"
)

if not os.path.exists(globals.MAKE_CONF_JSON):
    with open(globals.MAKE_CONF_JSON, 'w') as mjson:
        json.dump({
            "BOOT_USB_PATH": None,
            "BOCKS_BIOS_PATH": None
        }, mjson, indent=4)

with open(globals.MAKE_CONF_JSON) as mjson:
    data = json.load(mjson)

    globals.BOOT_USB_PATH   = data.get("BOOT_USB_PATH", None)
    globals.BOCHS_BIOS_PATH = data.get("BOCHS_BIOS_PATH", None)

# --- MAIN ---

if __name__ == "__main__":
    target = globals.arch
    for arg in sys.argv[1:]:
        if arg[0] != '-' and not arg.isdigit():
            target = arg
            break

    if "-elen" in sys.argv: err_len = int(sys.argv[sys.argv.index('-elen') + 1])

    if   target == globals.arch: farix_bin()
    elif target == "all":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(globals.DISK_PATH): disk_img()
        compile_apps()
        deploy_apps()
    elif target == "clean": clean(sys.argv[1:])
    elif target == "libc": libc()
    elif target == "get_deps": get_deps()
    elif target == "disk": disk_img()
    elif target == "iso": farix_iso()
    elif target == "wipe_usb": clean_boot_usb()
    elif target == "usb":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(globals.DISK_PATH): disk_img()
        deploy_usb()
    elif target == "qemu":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(globals.DISK_PATH): disk_img()
        run_qemu(fullscreen=True)
    elif target == "qemu_":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(globals.DISK_PATH): disk_img()
        run_qemu(fullscreen=False)
    elif target == "apps":
        compile_apps()
        deploy_apps()
    elif target == "compile_apps":
        compile_apps()
    elif target == "deploy_apps":
        deploy_apps()
    elif target == "help":
        print(HELP)
    else:
        print("\x1b[31mUnknown command\x1b[0m")
