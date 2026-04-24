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
import time

import makefile.globals

def farix_bin_x86_32():
    CRTI_SRC = "arch/x86_32/asm/boot/crti.asm"
    CRTN_SRC = "arch/x86_32/asm/boot/crtn.asm"
    BOOT_SRC = "arch/x86_32/boot.s"

    CRTI_OBJ = "build/asm/boot/crti.o"
    CRTN_OBJ = "build/asm/boot/crtn.o"

    c_srcs, asm_srcs = makefile.globals.get_sources()

    c_objs = [makefile.globals.get_obj_path(s) for s in c_srcs]
    other_asm_srcs = [s for s in asm_srcs if "crti.asm" not in s and "crtn.asm" not in s]
    other_asm_objs = [makefile.globals.get_obj_path(s) for s in other_asm_srcs]

    makefile.globals.build_object(BOOT_SRC, makefile.globals.BOOT_OBJ, f"{makefile.globals.AS} {{src}} -o {{obj}}")
    makefile.globals.build_object(CRTI_SRC, CRTI_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")
    makefile.globals.build_object(CRTN_SRC, CRTN_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")

    for s, o in zip(other_asm_srcs, other_asm_objs):
        makefile.globals.build_object(s, o, f"nasm -f elf32 {{src}} -o {{obj}}")
    for s, o in zip(c_srcs, c_objs):
        makefile.globals.build_object(s, o, f"{makefile.globals.CC} -c {{src}} -o {{obj}} {makefile.globals.CFLAGS}")

    ld_flags = "-T arch/x86_32/linker.ld -ffreestanding -O2 -nostdlib"

    all_objs = [makefile.globals.BOOT_OBJ, CRTI_OBJ, makefile.globals.CRTBEGIN] + c_objs + other_asm_objs
    objs = " ".join(all_objs)
    libs = f"-L{makefile.globals.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{makefile.globals.CC} {ld_flags} -o farix.bin {objs} {libs} {makefile.globals.CRTEND} {CRTN_OBJ}"

    print("\x1b[3;33mLinking farix.bin for x86_32...\x1b[0m")
    makefile.globals.run(cmd)
    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin_arm32():
    BOOT_SRC = "arch/arm32/boot.s"

    c_srcs, asm_srcs = makefile.globals.get_sources()

    c_objs   = [makefile.globals.get_obj_path(s) for s in c_srcs]
    asm_objs = [makefile.globals.get_obj_path(s) for s in asm_srcs]

    makefile.globals.build_object(BOOT_SRC, makefile.globals.BOOT_OBJ, f"{makefile.globals.AS} {{src}} -o {{obj}}")

    for s, o in zip(asm_srcs, asm_objs):
        makefile.globals.build_object(s, o, f"{makefile.globals.AS} {{src}} -o {{obj}}")
    for s, o in zip(c_srcs, c_objs):
        cpu_flag = "-mcpu=cortex-a7" if "raspi2b" in makefile.globals.QEMU_FLAGS else "-mcpu=cortex-a53"
        makefile.globals.build_object(s, o, f"{makefile.globals.CC} -c {{src}} -o {{obj}} {makefile.globals.CFLAGS} {cpu_flag} -marm")

    ld_flags = f"-T arch/arm32/linker.ld -ffreestanding -O2 -nostdlib"
    all_objs = [makefile.globals.BOOT_OBJ] + c_objs + asm_objs

    objs = " ".join(all_objs)
    libs = f"-L{makefile.globals.LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{makefile.globals.CC} {ld_flags} -o farix.bin {objs} {libs}"

    print("\x1b[3;33mLinking farix.bin for arm32...\x1b[0m")
    makefile.globals.run(cmd)

    # Create the raw binary for QEMU -kernel or SD card
    makefile.globals.run(f"{makefile.globals.PREFIX}objcopy farix.bin -O binary farix.img")

    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin():
    start_time = time.perf_counter()
    match makefile.globals.arch:
        case "x86_32": farix_bin_x86_32()
        case "arm32": farix_bin_arm32()
    end_time = time.perf_counter()

    print(f"[Compiled in {end_time - start_time:.6f}s]\n")
