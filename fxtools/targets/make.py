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
import struct
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, FIRST_EXCEPTION, wait

from fxtools.core import build
from fxtools.core import tools
from fxtools.core import statejson
from fxtools.core import printer
from fxtools.core.build import proc_run

from fxtools.vars import acpica
from fxtools.vars import c
from fxtools.vars import emulation
from fxtools.vars import libc

from fxtools.targets import disk

data = statejson.get()
arch = data["DEFAULT_ARCH"]
threads = data["THREADS"]

TOOLS = tools.get_tools()

BOOT_OBJ = "build/boot.o"

kernel_asm_path = "build/farix_asm"

def _compile_and_link_kernel(compile_to_asm: bool, threads: int):
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

        done, _ = wait(futures, return_when=FIRST_EXCEPTION)

        for future in done:
            future.result()

    ld_flags = "-T arch/x86_64/linker.ld -ffreestanding -O2 -nostdlib -nodefaultlibs -no-pie"

    all_objs = [BOOT_OBJ, CRTI_OBJ] + c_objs + other_asm_objs
    objs = " ".join(all_objs)
    libs = f"-L{libc.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{TOOLS['CC']} {ld_flags} -o bootloader/x86/boot/farix.bin {objs} {libs} {CRTN_OBJ}"

    printer.wait("Linking farix.bin for x86_64...")
    proc_run(cmd)

    if compile_to_asm:
        os.makedirs(kernel_asm_path, exist_ok=True)
        printer.wait("Dumping full kernel assembly...")
        proc_run(f"{TOOLS['PREFIX']}objdump -d -M intel -S bootloader/x86/boot/farix.bin > build/kernelASM/farix.asm")

    proc_run(f"{TOOLS['PREFIX']}objcopy -I elf64-x86-64 -O elf32-i386 bootloader/x86/boot/farix.bin bootloader/x86/boot/kernel.bin")
    os.remove("bootloader/x86/boot/farix.bin")

    printer.success("Created farix.bin")

def _compile_and_deploy_sysmods(compile_to_asm: bool, threads: int):
    printer.wait("Finding System Modules...")

    search_dirs = ["sysmods/x86", "sysmods/generic"]
    src_files = []

    for s_dir in search_dirs:
        if os.path.exists(s_dir):
            for mod_src in Path(s_dir).rglob("*.c"):
                src_files.append(str(mod_src))

    mod_tasks = []
    mod_link_data = []

    for mod_src in src_files:
        filename = os.path.basename(mod_src)
        mod_name = os.path.splitext(filename)[0]

        mod_obj = f"build/sysmods/{mod_name}.o"
        mod_out = f"build/sysmods/{mod_name}.sys"
        os.makedirs(os.path.dirname(mod_obj), exist_ok=True)

        clean_cflags = c.CFLAGS.replace("-mcmodel=kernel", "").replace("-fno-pic", "")
        cc_flags = f"{TOOLS['CC']} -c {mod_src} -o {mod_obj} {clean_cflags} -mcmodel=large -fno-pie -fno-pic"
        mod_tasks.append((mod_src, mod_obj, cc_flags))
        mod_link_data.append((mod_name, mod_obj, mod_out))

    with ThreadPoolExecutor(max_workers=threads) as executor:
        futures = {executor.submit(build.build_object, *t): t for t in mod_tasks}
        done, _ = wait(futures, return_when=FIRST_EXCEPTION)
        for future in done:
            future.result()

    for mod_name, mod_obj, mod_out in mod_link_data:
        print(f"\n\x1b[1;35mBuilding System Module: {mod_name}\x1b[0m")

        ld_flags = "-T sysmods/linker.ld -ffreestanding -nostdlib -O2 -Wl,--oformat=binary"
        proc_run(f"{TOOLS['CC']} {ld_flags} {mod_obj} -o {mod_out}")

        if compile_to_asm:
            os.makedirs(kernel_asm_path, exist_ok=True)
            print(f"\x1b[36mDumping assembly for system module: {mod_name}\x1b[0m")
            proc_run(f"{TOOLS['PREFIX']}objdump -d -M intel -S {mod_obj} > build/kernelASM/{mod_name}.asm")

        print(f"\x1b[33mDeploying {mod_name}.sys to {emulation.DISK_PATH}/system/\x1b[0m")
        proc_run(f"mcopy -D o -i {emulation.DISK_PATH} {mod_out} ::/system/{mod_name}.sys")

    return mod_link_data

def _create_initboot(mod_link_data: list[tuple[str, str, str]]):
    """
    Creates the initboot.bin blob using a self-describing header format:
    - 4 bytes: Total header size (uint32_t)
    - Metadata entries for each file:
      - 32 bytes: Null-terminated string for the file name/identifier (uppercase)
      - 4 bytes: Absolute byte offset of the binary blob (uint32_t)
      - 4 bytes: Size of the binary blob in bytes (uint32_t)
    - Followed immediately by the raw flat binaries packed back-to-back.
    """

    file_entries = []
    binary_data_chunks = []

    # Extract module names and read their corresponding .sys binary outputs
    initboot_mods = ["ahci", "ata"]

    for mod_name, _, mod_out in mod_link_data:
        if mod_name not in [m for m in initboot_mods]:
            continue

        name_key = mod_name.upper()

        if not os.path.exists(mod_out):
            raise FileNotFoundError(f"System module binary not found: {mod_out}")

        with open(mod_out, "rb") as f:
            content = f.read()

        file_entries.append({
            "name": name_key,
            "content": content
        })

    # Calculate the header size:
    # 4 bytes for the header size field itself + (40 bytes per file entry)
    header_size = 4 + (len(file_entries) * 40)

    current_offset = header_size
    metadata_bytes = bytearray()

    # Pack metadata and compute absolute offsets relative to the start of the blob
    for entry in file_entries:
        name_bytes = entry["name"].encode('utf-8')[:31].ljust(32, b'\x00')
        size = len(entry["content"])

        # Pack metadata: [32-byte name][4-byte offset][4-byte size]
        metadata_bytes.extend(name_bytes)
        metadata_bytes.extend(struct.pack("<I", current_offset))
        metadata_bytes.extend(struct.pack("<I", size))

        binary_data_chunks.append(entry["content"])
        current_offset += size

    output_filename = "bootloader/x86/boot/initboot.bin"

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_filename), exist_ok=True)

    # Construct the final binary blob
    with open(output_filename, "wb") as out:
        out.write(struct.pack("<I", header_size))
        out.write(metadata_bytes)
        for chunk in binary_data_chunks:
            out.write(chunk)

    printer.success(f"Generated '{output_filename}' containing {len(file_entries)} modules. ({current_offset} bytes).")

def _package_bootable_iso():
    printer.wait("Packaging final bootable ISO...")

    proc_run(
        "xorriso -as mkisofs "
        "-R -b boot/grub/stage2_eltorito "
        "-no-emul-boot -boot-load-size 4 -boot-info-table "
        "-untranslated-filenames "
        "-o farix.iso bootloader/x86",
        check=True)

    printer.success("Process completed")

def compile_x86_64(compile_to_asm: bool):
    threads = getattr(build, "threads", os.cpu_count() or 4)

    _compile_and_link_kernel(compile_to_asm, threads)

    printer.wait("Creating disk.img...")
    disk.run()

    mod_link_data = _compile_and_deploy_sysmods(compile_to_asm, threads)
    _create_initboot(mod_link_data)

    _package_bootable_iso()

def run(arch: str = arch, asm: bool = False):
    match arch:
        case "x86_64":
            compile_x86_64(asm)
        case _:
            printer.error(f"Unsupported architecture: {arch}")

def help():
    return {
        "USAGE": "fx make <-arch> <--asm>",
        "ARGS": {
            "arch": "Architecture to build for.",
            "asm": f"When enabled, the entire compiled kernel is placed as assembly files into {kernel_asm_path}."
        },
        "VARIABLES": {
            "DEFAULT_ARCH": "Default architecture to build for, which is taken if no architecture was passed."
        },
        "DESCRIPTION": "Builds the Farix kernel, sets up the system disk, and creates a bootable ISO image."
    }
