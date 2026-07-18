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

from concurrent.futures import ThreadPoolExecutor, FIRST_EXCEPTION, wait

from fxtools.core import build
from fxtools.core import tools
from fxtools.core import statejson
from fxtools.core import printer
from fxtools.core.build import proc_run

from fxtools.vars import c
from fxtools.vars import acpica
from fxtools.vars import libc
from fxtools.vars import emulation

from fxtools.targets import disk

data = statejson.get()
arch = data["DEFAULT_ARCH"]
threads = data["THREADS"]

TOOLS = tools.get_tools()

BOOT_OBJ = "build/boot.o"

def compile_x86_64():
    CRTI_SRC = "arch/x86_64/asm/boot/crti.asm"
    CRTN_SRC = "arch/x86_64/asm/boot/crtn.asm"
    BOOT_SRC = "arch/x86_64/boot.s"

    CRTI_OBJ = "build/asm/boot/crti.o"
    CRTN_OBJ = "build/asm/boot/crtn.o"

    c_srcs, asm_srcs = build.get_sources()

    c_objs = [str(build.get_obj_path(s)) for s in c_srcs]

    other_asm_srcs = [s for s in asm_srcs if "crti.asm" not in s.name and "crtn.asm" not in s.name]
    other_asm_objs = [str(build.get_obj_path(s)) for s in other_asm_srcs]

    tasks = []
    tasks.append((BOOT_SRC, BOOT_OBJ, f"{TOOLS['AS']} {{src}} -o {{obj}}"))
    tasks.append((CRTI_SRC, CRTI_OBJ, "nasm -f elf64 {src} -o {obj}"))
    tasks.append((CRTN_SRC, CRTN_OBJ, "nasm -f elf64 {src} -o {obj}"))

    for s, o in zip(other_asm_srcs, other_asm_objs):
        tasks.append((s, o, "nasm -f elf64 {src} -o {obj}"))

    for s, o in zip(c_srcs, c_objs):
        if s.match(f"{acpica.ACPICA_SRC}/**/*") and s.name != "acpi_osl.c":
            tasks.append((s, o, f"{TOOLS['CC']} -c {{src}} -o {{obj}} {acpica.ACPICA_CFLAGS}"))
        else:
            tasks.append((s, o, f"{TOOLS['CC']} -c {{src}} -o {{obj}} {c.CFLAGS}"))

    with ThreadPoolExecutor(max_workers=threads) as executor:
        futures = {executor.submit(build.build_object, *t): t for t in tasks}

        # Wait for all to finish OR any one to fail
        done, not_done = wait(futures, return_when=FIRST_EXCEPTION)

        # Check for any exceptions that occurred
        for future in done:
            future.result() # Re-raises exception if the task failed

    ld_flags = "-T arch/x86_64/linker.ld -ffreestanding -O2 -nostdlib -nodefaultlibs -no-pie"

    all_objs = [BOOT_OBJ, CRTI_OBJ] + c_objs + other_asm_objs
    objs = " ".join(all_objs)
    libs = f"-L{libc.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{TOOLS['CC']} {ld_flags} -o bootloader/x86/boot/farix.bin {objs} {libs} {CRTN_OBJ}"

    printer.wait("Linking farix.bin for x86_64...")
    proc_run(cmd)

    proc_run(f"{TOOLS['PREFIX']}objcopy -I elf64-x86-64 -O elf32-i386 bootloader/x86/boot/farix.bin bootloader/x86/boot/farix_elf32.bin")

    printer.success("Created farix.bin")

    # Create disk.img
    printer.wait("Creating disk.img...")
    disk.run()

    # Compile all Kernel System Modules
    printer.wait("Finding System Modules...")

    search_dirs = ["sysmods/x86", "sysmods/generic"]
    src_files = [os.path.join(s_dir, item) for s_dir in search_dirs if os.path.exists(s_dir)
                 for item in os.listdir(s_dir) if item.endswith(".c")]

    mod_tasks = []
    mod_link_data = []

    # Map files and queue object compilation tasks
    for mod_src in src_files:
        filename = os.path.basename(mod_src)
        mod_name = os.path.splitext(filename)[0]

        mod_obj = f"build/sysmods/{mod_name}.o"
        mod_out = f"build/sysmods/{mod_name}.sys"
        os.makedirs(os.path.dirname(mod_obj), exist_ok=True)

        # Remove kernel-space models and ensure -fno-pic is set
        clean_cflags = c.CFLAGS.replace("-mcmodel=kernel", "").replace("-fno-pic", "")
        cc_flags = f"{TOOLS['CC']} -c {mod_src} -o {mod_obj} {clean_cflags} -mcmodel=large -fno-pie -fno-pic"
        mod_tasks.append((mod_src, mod_obj, cc_flags))
        mod_link_data.append((mod_name, mod_obj, mod_out))

    with ThreadPoolExecutor(max_workers=threads) as executor:
        futures = {executor.submit(build.build_object, *t): t for t in mod_tasks}
        done, _ = wait(futures, return_when=FIRST_EXCEPTION)
        for future in done:
            future.result()

    # Process linking and target disk deployment
    for mod_name, mod_obj, mod_out in mod_link_data:
        print(f"\n\x1b[1;35mBuilding System Module: {mod_name}\x1b[0m")

        ld_flags = "-T sysmods/linker.ld -ffreestanding -nostdlib -O2 -Wl,--oformat=binary"
        proc_run(f"{TOOLS['CC']} {ld_flags} {mod_obj} -o {mod_out}")
        print(f"\x1b[33mDeploying {mod_name}.sys to {emulation.DISK_PATH}/system/\x1b[0m")
        proc_run(f"mcopy -D o -i {emulation.DISK_PATH} {mod_out} ::/system/{mod_name}.sys")

    # Create farix.iso
    printer.wait("Packaging final bootable ISO...")

    proc_run(
        "xorriso -as mkisofs "
        "-R -b boot/grub/stage2_eltorito " # Relative to bootloader/x86
        "-no-emul-boot -boot-load-size 4 -boot-info-table "
        "-untranslated-filenames "
        "-o farix.iso bootloader/x86",
        check=True)

    printer.success("Process completed")

def run(arch: str = arch):
    match arch:
        case "x86_64":
            compile_x86_64()
        case _:
            printer.error(f"Unsupported architecture: {arch}")

def help():
    return {
        "USAGE": "fx make [arch]",
        "ARGS": {
            "arch": "Architecture to build for."
        },
        "VARIABLES": {
            "DEFAULT_ARCH": "Default architecture to build for, which is taken if no architecture was passed."
        },
        "DESCRIPTION": "Builds the Farix kernel, sets up the system disk, and creates a bootable ISO image."
    }
