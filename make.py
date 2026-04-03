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

HELP = """
\033[1;36mFarix OS Build System\033[0m

\033[90mTIP: Use 'source make.env' to alias build.py as 'm'\033[0m
Usage: \033[1m m [target] [-arch <architecture>]\033[0m

\033[1;36mTargets:\033[0m
  \033[1;32mall\033[0m          Build the kernel and all dependencies \033[90m(default)\033[0m
  \033[1;32mlibc\033[0m         Compile the internal C standard library
  \033[1;32mdisk.img\033[0m     Generate the bootable disk image
  \033[1;32mget_deps\033[0m     Fetch and verify build-tool dependencies
  \033[1;32mrun\033[0m          Build and launch in QEMU \033[90m(Fullscreen mode)\033[0m
  \033[1;32mrun_\033[0m         Build and launch in QEMU \033[90m(Windowed mode)\033[0m
  \033[1;32mclean\033[0m        Remove all build artifacts and object files
  \033[1;32mhelp\033[0m         Display this menu

\033[1;36mArchitectures:\033[0m
  \033[1mx86_32\033[0m       32-bit x86 \033[90m(default)\033[0m
  \033[1marm32\033[0m        32-bit ARM \033[90m(Requires arm-none-eabi-gcc)\033[0m
"""

# --- UTILS ---

def run(cmd, shell=True, check=True):
    try:
        result = subprocess.run(
            cmd,
            shell=shell,
            check=check,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"\n\x1b[1;31m--- BUILD ERROR ---\x1b[0m")
        print(f"Command: {e.cmd}\n")

        err_msg = e.stderr
        if isinstance(err_msg, bytes):
            err_msg = err_msg.decode('utf-8', errors='replace')
        print(f"Error:\n{err_msg}")

        sys.exit(1)

def shell_which(cmd):
    return subprocess.run(f"which {cmd}", shell=True, capture_output=True).returncode == 0

# --- INITIALIZATION ---

OS = run("uname -s")

if shell_which("i686-elf-gcc"):
    PREFIX = "i686-elf-"
elif shell_which("i386-elf-gcc"):
    PREFIX = "i386-elf-"
else:
    print("Error: i686-elf nor i386-elf found")
    sys.exit(1)

TARGET_DIR = PREFIX.rstrip('-')

PROJECT_ROOT = os.getcwd()
NEWLIB_SRC = os.path.join(PROJECT_ROOT, "newlib-cygwin")
NEWLIB_TARGET = PREFIX.rstrip('-')
LIBC_INSTALL_DIR = os.path.join(os.getcwd(), "libc_build")
LIBC_DIR = os.path.join(LIBC_INSTALL_DIR, TARGET_DIR)
LIBC_INC = os.path.join(LIBC_DIR, "include")
LIBC_LIB = os.path.join(LIBC_DIR, "lib")

NASM = "nasm"
CC = f"{PREFIX}gcc"
AS = f"{PREFIX}as"

CFLAGS = f"-ffreestanding -O2 -Wall -Wextra -Werror -fno-exceptions -fdiagnostics-color=always -Iinclude -I{LIBC_INC}"
QEMU_FLAGS = "-drive format=raw,file=disk.img,index=0,media=disk -device virtio-mouse-pci"

BOOT_OBJECT = "build/boot.o"
CRTI_OBJ = "build/asm/boot/crti.o"
CRTN_OBJ = "build/asm/boot/crtn.o"

CRTBEGIN = run(f"{CC} {CFLAGS} -print-file-name=crtbegin.o")
CRTEND   = run(f"{CC} {CFLAGS} -print-file-name=crtend.o")

# --- TARGET FUNCTIONS ---

def build_object(src, obj, cmd_template):
    # Create the directory if it doesn't exist
    os.makedirs(os.path.dirname(obj), exist_ok=True)

    # Check if we actually need to build
    # If the object exists and is newer than the source, skip it
    if os.path.exists(obj):
        src_mtime = os.path.getmtime(src)
        obj_mtime = os.path.getmtime(obj)

        if obj_mtime > src_mtime:
            return

    cmd = cmd_template.format(src=src, obj=obj)
    print(f"\x1b[1;34mCompiling:\x1b[0m {src}")
    run(cmd)

def get_sources(arch):
    c_sources   = []
    asm_sources = []

    for root, dirs, files in os.walk("src"):
        # If we are inside src/arch, remove all dirs except the one we want
        # This is for the HAL so that we can segregate based on the arch
        if root == "src/arch":
            dirs[:] = [d for d in dirs if d == arch]

        for f in files:
            path = os.path.join(root, f)
            if f.endswith(".c"):
                c_sources.append(path)
            elif f.endswith(".asm"):
                asm_sources.append(path)

    return c_sources, asm_sources

def farix_bin(target_arch="x86_32"):
    c_srcs, asm_srcs = get_sources(target_arch)

    # Generate object paths
    c_objs         = [f.replace("src/", "build/").replace(".c", ".c.o") for f in c_srcs]
    other_asm_srcs = [f for f in asm_srcs if "crti.asm" not in f and "crtn.asm" not in f]
    other_asm_objs = [f.replace("src/", "build/").replace(".asm", ".asm.o") for f in other_asm_srcs]

    # Compile assembly boot files
    build_object(f"src/arch/{target_arch}/boot.s", BOOT_OBJECT, f"{AS} {{src}} -o {{obj}}")
    build_object("src/asm/boot/crti.asm", CRTI_OBJ, f"{NASM} -f elf32 {{src}} -o {{obj}}")
    build_object("src/asm/boot/crtn.asm", CRTN_OBJ, f"{NASM} -f elf32 {{src}} -o {{obj}}")

    # Compile C sources
    for s, o in zip(c_srcs, c_objs):
        build_object(s, o, f"{CC} -c {{src}} -o {{obj}} {CFLAGS}")

    # Compile ASM sources (.asm) using nasm
    for s, o in zip(other_asm_srcs, other_asm_objs):
        build_object(s, o, f"{NASM} -f elf32 {{src}} -o {{obj}}")

    ld_flags = f"-T src/arch/{target_arch}/linker.ld -ffreestanding -O2 -nostdlib"
    all_objs = [BOOT_OBJECT, CRTI_OBJ, CRTBEGIN] + c_objs + other_asm_objs

    # Link everything
    objs = " ".join(all_objs)
    libs = f"-L{LIBC_LIB} -lc -lm -lgcc"
    cmd  = f"{CC} {ld_flags} -o farix.bin {objs} {libs} {CRTEND} {CRTN_OBJ}"

    print(f"\x1b[3;33mLinking farix.bin for {target_arch}...\x1b[0m")
    run(cmd)

    print("\x1b[1;32mProcess completed\x1b[0m")

def get_deps():
    if not os.path.exists("newlib-cygwin"):
        print("Installing newlib...")
        run("git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git")
    print("Installed dependencies.")

def libc():
    os.makedirs("build/newlib-build", exist_ok=True)
    config_cmd = (
        f"cd build/newlib-build && "
        f"CC_FOR_TARGET={PREFIX}gcc AS_FOR_TARGET={PREFIX}as "
        f"LD_FOR_TARGET={PREFIX}ld RANLIB_FOR_TARGET={PREFIX}ranlib "
        f"{NEWLIB_SRC}/configure --target={NEWLIB_TARGET} --prefix={LIBC_INSTALL_DIR} "
        f"--disable-newlib-supplied-syscalls --with-newlib --enable-languages=c,c++"
    )
    run(config_cmd)
    run("make -C build/newlib-build")
    run("make -C build/newlib-build install")

def disk_img():
    if OS == "Darwin":
        run("qemu-img create -f raw disk.img 64M")
        run('hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE disk.img')
        os.rename("disk.img.dmg", "disk.img")
    else:
        run("qemu-img create -f raw disk.img 64M")
        run("mkfs.fat -F 32 -n FARIX disk.img")

def clean():
    for path in ["build", "farix.bin", "disk.img"]:
        if os.path.isdir(path): shutil.rmtree(path)
        elif os.path.exists(path): os.remove(path)

def run_qemu(fullscreen=False):
    suffix = "-full-screen" if fullscreen else ""

    try:
        run(f"qemu-system-i386 -kernel farix.bin {QEMU_FLAGS} {suffix}")
    except KeyboardInterrupt:
        print("\nProcess exited (KeyboardInterrupt)\n")

# --- MAIN ---
if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "all"

    arch = "x86_32"
    if "-arch" in sys.argv:
        idx = sys.argv.index("-arch")
        if idx + 1 < len(sys.argv):
            arch = sys.argv[idx + 1]

    if   target == "all": farix_bin(arch)
    elif target == "clean": clean()
    elif target == "libc": libc()
    elif target == "get_deps": get_deps()
    elif target == "disk.img": disk_img()
    elif target == "run":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists("disk.img"): disk_img()
        run_qemu(fullscreen=True)
    elif target == "run_":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists("disk.img"): disk_img()
        run_qemu(fullscreen=False)
    elif target == "help":
        print(HELP)
    else:
        print("make: Unknown command")
