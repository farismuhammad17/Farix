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
import argparse
import time
import re
import json

from makefile        import globals
from makefile.apps   import *
from makefile.bochs  import *
from makefile.bin    import *
from makefile.config import *
from makefile.defs   import *
from makefile.img    import *
from makefile.qemu   import *
from makefile.usb    import *
from makefile.deps   import *
from makefile.iso    import *
from makefile.libc   import *
from makefile.lint   import *
from makefile.utils  import *
from makefile.help   import *

DEFAULT_ARCH = "x86_32"

globals.MAKE_CONF_JSON = "make.conf.json"

if not os.path.exists(globals.MAKE_CONF_JSON):
    with open(globals.MAKE_CONF_JSON, 'w') as mjson:
        json.dump({
            "BOOT_USB_PATH": None,
            "DEFAULT_ARCH": DEFAULT_ARCH,
            "THREADS": 4
        }, mjson, indent=4)

with open(globals.MAKE_CONF_JSON) as mjson:
    conf_data = json.load(mjson)

    globals.BOOT_USB_PATH = conf_data.get("BOOT_USB_PATH", None)

arg_parser = argparse.ArgumentParser(description="Farix Kernel Build System (use help for commands)")

arg_parser.add_argument(
    "-a", "-arch",
    dest="arch",
    choices=["x86_32", "arm32"],
    default=conf_data.get("DEFAULT_ARCH", DEFAULT_ARCH),
    help="Target architecture"
)

arg_parser.add_argument(
    "-t", "-threads",
    dest="threads",
    type=int,
    default=conf_data.get("THREADS", 4),
    help="Number of threads to use for compilation"
)

arg_parser.add_argument(
    "-storage",
    dest="storage_dev",
    choices=["ahci", "ata"],
    default="ahci",
    type=str.lower,
    help="Storage device used by kernel"
)

arg_parser.add_argument(
    "--log", "--l",
    dest="log",
    action="store_true",
    help="Enable build logging"
)

args, _ = arg_parser.parse_known_args()

globals.arch    = args.arch
globals.LOGGING = args.log
globals.THREADS = args.threads

STORAGE_DEV = args.storage_dev

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
    f"arch/{globals.arch}/libc/",
    "kernel/libc/user.c"
)

# --- INITIALIZATION ---

globals.OS = globals.run("uname -s")

globals.DISK_PATH = "build/disk.img"

if globals.arch == "x86_32":
    globals.QEMU_BIN = "qemu-system-i386"

    globals.QEMU_FLAGS = (
        "-machine pc,accel=tcg "
        "-cpu pentium "
        "-m 256 "
        "-boot menu=on,strict=on "
        "-global i8042.extended-state=off "

        "-device virtio-mouse-pci "
        "-vga std "
        "-serial stdio "
    )

    if STORAGE_DEV == "ahci":
        globals.QEMU_FLAGS += (
            f"-drive file={globals.DISK_PATH},if=none,id=disk0,format=raw,media=disk "
            "-device ahci,id=ahci0 "
            "-device ide-hd,drive=disk0,bus=ahci0.0 "
        )

    elif STORAGE_DEV == "ata":
        globals.QEMU_FLAGS += (
            f"-drive file={globals.DISK_PATH},if=none,id=hd0,format=raw,media=disk "
            "-device piix4-ide,id=pci-ide0 "
            "-device ide-hd,bus=pci-ide0.0,drive=hd0 "
        )

    compilers = ("i686-linux-gnu-gcc", "i686-elf-gcc", "i386-elf-gcc")
    for comp in compilers:
        if globals.shell_which(comp):
            globals.PREFIX = comp[:-3]
            break
    else:
        print("Error: x86 cross-compiler not found")
        sys.exit(1)

elif globals.arch == "arm32":
    globals.PREFIX = "arm-none-eabi-"
    globals.QEMU_BIN = "qemu-system-arm"
    globals.QEMU_FLAGS = f"-drive format=raw,file={globals.DISK_PATH},index=0,media=disk -serial stdio -M raspi2b"
    globals.ASM_ASSEMBLER = "arm-none-eabi-as"

else:
    print(f"\x1b[31mInvalid architecture: {globals.arch}\x1b[0m (Fallback: {DEFAULT_ARCH})")
    globals.arch = DEFAULT_ARCH

globals.TARGET_DIR = globals.PREFIX.rstrip('-')

globals.PROJECT_ROOT = os.getcwd()

globals.NEWLIB_SRC = os.path.join(globals.PROJECT_ROOT, "newlib-cygwin")
globals.NEWLIB_TARGET = globals.PREFIX.rstrip('-')
globals.LIBC_INSTALL_DIR = os.path.join(os.getcwd(), f"libc_build_{globals.arch}")
globals.LIBC_DIR = os.path.join(globals.LIBC_INSTALL_DIR, globals.TARGET_DIR)
globals.LIBC_INC = os.path.join(globals.LIBC_DIR, "include")
globals.LIBC_LIB = os.path.join(globals.LIBC_DIR, "lib")

globals.ACPICA_ARCH_INDEPENDANT = os.path.join(globals.PROJECT_ROOT, "include/drivers/acpi")
globals.ACPICA_ARCH_DEPENDANT   = os.path.join(globals.PROJECT_ROOT, f"arch/{globals.arch}/include/acpi")

globals.CC = f"{globals.PREFIX}gcc"
globals.AS = f"{globals.PREFIX}as"

globals.CFLAGS = (
    "-ffreestanding -O2 -Wall -Wextra -fno-exceptions "
    "-fdiagnostics-color=always "
    f"-Iinclude -I{globals.LIBC_INC} "
    f"-I{globals.ACPICA_ARCH_INDEPENDANT} -I{globals.ACPICA_ARCH_DEPENDANT} "
    "-include include/kernel.h "
    f"-Iarch/{globals.arch} "
)

globals.ACPICA_SRC = "kernel/drivers/acpi"
globals.ACPICA_CFLAGS = (
    "-ffreestanding -O2 -Wall -Wextra -fno-exceptions "
    "-fdiagnostics-color=always "
    f"-Iinclude -I{globals.LIBC_INC} "
    f"-I{globals.ACPICA_ARCH_INDEPENDANT} -I{globals.ACPICA_ARCH_DEPENDANT} "
)

globals.BOOT_OBJ = "build/boot.o"

globals.CRTBEGIN = globals.run(f"{globals.CC} {globals.CFLAGS} -print-file-name=crtbegin.o")
globals.CRTEND   = globals.run(f"{globals.CC} {globals.CFLAGS} -print-file-name=crtend.o")

globals.USER_LIBC_DIR = f"arch/{globals.arch}/libc"
globals.USER_LIBC = "kernel/libc/user.c"
globals.USER_LIBC_ARCH = f"{globals.USER_LIBC_DIR}/user.c"
globals.USER_ASM  = f"{globals.USER_LIBC_DIR}/user.asm"

globals.USER_BUILD_DIR = "build/apps"
globals.USER_LIBC_OBJ = "build/kernel/libc/user.o"
globals.USER_LIBC_ARCH_OBJ = f"{globals.USER_BUILD_DIR}/user.o"
globals.USER_ASM_OBJ  = f"{globals.USER_BUILD_DIR}/user_asm.o"

globals.APPS_ROOT = "apps"
globals.USER_CFLAGS = f"-ffreestanding -O2 -Iinclude -I{globals.LIBC_INC} -include include/kernel.h"

# --- MAIN ---

if __name__ == "__main__":
    target = globals.arch
    for arg in sys.argv[1:]:
        if arg[0] != '-' and not arg.isdigit():
            target = arg
            break

    start_time = time.perf_counter()

    if   target == globals.arch: farix_bin()
    elif target in ("all", "*"):
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(globals.DISK_PATH): disk_img()
        compile_apps()
        deploy_apps()
    elif target == "iso":
        create_iso()
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
    elif target == "bochs":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists("farix.iso"): create_iso()
        run_bochs()
    elif target == "apps":
        compile_apps()
        deploy_apps()
    elif target == "compile_apps":
        compile_apps()
    elif target == "deploy_apps":
        deploy_apps()
    elif target == "defs":
        scan_files()
        query_loop()
    elif target == "lint":
        lint()
    elif target == "config":
        config_mjson()
    elif target == "help":
        print(HELP)
    else:
        print("\x1b[31mUnknown command\x1b[0m")

    end_time = time.perf_counter()
    print(f"\x1b[90m[Process completed in {end_time - start_time:.6f}s]\x1b[0m\n")
