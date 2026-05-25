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
from concurrent.futures import ThreadPoolExecutor

import makefile.globals as m

def farix_bin_x86_32():
    CRTBEGIN = m.run(f"{m.CC} {m.CFLAGS} -print-file-name=crtbegin.o")
    CRTEND   = m.run(f"{m.CC} {m.CFLAGS} -print-file-name=crtend.o")

    CRTI_SRC = "arch/x86_32/asm/boot/crti.asm"
    CRTN_SRC = "arch/x86_32/asm/boot/crtn.asm"
    BOOT_SRC = "arch/x86_32/boot.s"

    CRTI_OBJ = "build/asm/boot/crti.o"
    CRTN_OBJ = "build/asm/boot/crtn.o"

    c_srcs, asm_srcs = m.get_sources()

    c_objs = [m.get_obj_path(s) for s in c_srcs]
    other_asm_srcs = [s for s in asm_srcs if "crti.asm" not in s and "crtn.asm" not in s]
    other_asm_objs = [m.get_obj_path(s) for s in other_asm_srcs]

    # Use threads to speed up build time
    tasks = []
    tasks.append((BOOT_SRC, m.BOOT_OBJ, f"{m.AS} {{src}} -o {{obj}}"))
    tasks.append((CRTI_SRC, CRTI_OBJ, "nasm -f elf32 {src} -o {obj}"))
    tasks.append((CRTN_SRC, CRTN_OBJ, "nasm -f elf32 {src} -o {obj}"))

    for s, o in zip(other_asm_srcs, other_asm_objs):
        tasks.append((s, o, "nasm -f elf32 {src} -o {obj}"))

    for s, o in zip(c_srcs, c_objs):
        if s.startswith(m.ACPICA_SRC) and not s.endswith("acpi_osl.c"):
            tasks.append((s, o, f"{m.CC} -c {{src}} -o {{obj}} {m.ACPICA_CFLAGS}"))
        else:
            tasks.append((s, o, f"{m.CC} -c {{src}} -o {{obj}} {m.CFLAGS}"))

    # Execute the threads in parallel
    with ThreadPoolExecutor(max_workers=m.THREADS) as executor:
        for t in tasks:
            executor.submit(m.build_object, *t)

    # Link compiled files
    ld_flags = "-T arch/x86_32/linker.ld -ffreestanding -O2 -nostdlib"
    all_objs = [m.BOOT_OBJ, CRTI_OBJ, CRTBEGIN] + c_objs + other_asm_objs
    objs = " ".join(all_objs)
    libs = f"-L{m.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{m.CC} {ld_flags} -o farix.bin {objs} {libs} {CRTEND} {CRTN_OBJ}"

    print("\x1b[3;33mLinking farix.bin for x86_32...\x1b[0m")
    m.run(cmd)
    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin_arm32():
    BOOT_SRC = "arch/arm32/boot.s"

    c_srcs, asm_srcs = m.get_sources()

    c_objs   = [m.get_obj_path(s) for s in c_srcs]
    asm_objs = [m.get_obj_path(s) for s in asm_srcs]

    m.build_object(BOOT_SRC, m.BOOT_OBJ, f"{m.AS} {{src}} -o {{obj}}")

    for s, o in zip(asm_srcs, asm_objs):
        m.build_object(s, o, f"{m.AS} {{src}} -o {{obj}}")
    for s, o in zip(c_srcs, c_objs):
        cpu_flag = "-mcpu=cortex-a7" if "raspi2b" in m.QEMU_FLAGS else "-mcpu=cortex-a53"
        m.build_object(s, o, f"{m.CC} -c {{src}} -o {{obj}} {m.CFLAGS} {cpu_flag} -marm")

    ld_flags = f"-T arch/arm32/linker.ld -ffreestanding -O2 -nostdlib"
    all_objs = [m.BOOT_OBJ] + c_objs + asm_objs

    objs = " ".join(all_objs)
    libs = f"-L{m.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{m.CC} {ld_flags} -o farix.bin {objs} {libs}"

    print("\x1b[3;33mLinking farix.bin for arm32...\x1b[0m")
    m.run(cmd)

    # Create the raw binary for QEMU -kernel or SD card
    m.run(f"{m.PREFIX}objcopy farix.bin -O binary farix.img")

    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin():
    match m.arch:
        case "x86_32": farix_bin_x86_32()
        case "arm32": farix_bin_arm32()
