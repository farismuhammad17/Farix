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

IGNORES = (
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

HELP = """
\033[1;36mFarix Build System\033[0m
\033[90mhttps://github.com/farismuhammad17/Farix\033[0m

\033[90mTIP: Use 'source make.env' to alias make.py as 'm'\033[0m
Usage: \033[1m m [target] <architecture>\033[0m

\033[1;36mTargets:\033[0m
  \033[1;32mall\033[0m          Build the kernel and all dependencies \033[90m(default)\033[0m
  \033[1;32mlibc\033[0m         Compile the internal C standard library
  \033[1;32mdisk\033[0m         Generate the bootable disk image
  \033[1;32mget_deps\033[0m     Fetch and verify build-tool dependencies
  \033[1;32mwipe_usb\033[0m     Wipe every single file inside the Boot USB
  \033[1;32musb\033[0m          Copy farix.bin into Boot USB \033[90m(provide in make.conf.json)\033[0m
  \033[1;32mqemu\033[0m         Build and launch in QEMU \033[90m(Fullscreen mode)\033[0m
  \033[1;32mqemu_\033[0m        Build and launch in QEMU \033[90m(Windowed mode)\033[0m
  \033[1;32mclean\033[0m        Remove all build artifacts and object files
  \033[1;32mapps\033[0m         Compile and add all apps into disk
  \033[1;32mcompile_apps\033[0m Compile all the files inside apps into ELF
  \033[1;32mdeploy_apps\033[0m  Deploy all ELF files inside apps folder
  \033[1;32mhelp\033[0m         Display this menu

\033[1;36mArchitectures:\033[0m
  \033[1mx86_32\033[0m       32-bit x86 \033[90m(default)\033[0m
  \033[1marm32\033[0m        32-bit ARM \033[90m(Requires arm-none-eabi-gcc)\033[0m

\033[1;36mFlags:\033[0m
  \033[1m-elen\033[0m        Maximum lines for error message
"""

# --- UTILS ---

err_len = None
def run(cmd, shell=True, check=True, capture_output=True):
    try:
        result = subprocess.run(
            cmd,
            shell=shell,
            check=check,
            capture_output=capture_output,
            text=True,
            encoding='utf-8',
            errors='replace'
        )

        if not capture_output:
            return ""

        return result.stdout.strip() if result.stdout else ""
    except subprocess.CalledProcessError as e:
        print(f"\n\x1b[1;31m--- BUILD ERROR ---\x1b[0m")
        print(f"Command: {e.cmd}\n")

        err_msg = e.stderr
        if isinstance(err_msg, bytes):
            err_msg = err_msg.decode('utf-8', errors='replace')

        if err_len:
            print(f"Error:\n{'\n'.join(err_msg.splitlines()[:err_len])}")
        else:
            print(f"Error:\n{err_msg}")

        sys.exit(1)

def shell_which(cmd):
    return subprocess.run(f"which {cmd}", shell=True, capture_output=True).returncode == 0

def get_obj_path(src_path):
    ext = src_path.rsplit('.', 1)[-1]
    return src_path.replace(f".{ext}", f".{ext}.o").replace("kernel/", "build/kernel/").replace("arch/", "build/arch/")

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
    print(f"\x1b[1;34mCompiling:\x1b[0m {src} \x1b[90m({obj})\x1b[0m")
    run(cmd)

def is_ignored(path):
    for ign in IGNORES:
        if path.startswith(ign):
            return True
    return False

def get_sources(arch):
    c_sources   = []
    asm_sources = []

    for root, _, files in os.walk("kernel"):
        for f in files:
            path = os.path.join(root, f)
            if is_ignored(path): continue

            elif f.endswith(".c"):
                c_sources.append(path)
            # ASM in kernel is rare, but we'll check just in case
            elif (f.endswith(".asm") or f.endswith(".s")) and f != "boot.s":
                asm_sources.append(path)

    arch_path = os.path.join("arch", arch)
    if os.path.exists(arch_path):
        for root, _, files in os.walk(arch_path):
            for f in files:
                path = os.path.join(root, f)
                if is_ignored(path): continue

                elif f.endswith(".c"):
                    c_sources.append(path)
                elif (f.endswith(".asm") or f.endswith(".s")) and f != "boot.s":
                    asm_sources.append(path)

    return c_sources, asm_sources

# --- INITIALIZATION ---

OS = run("uname -s")

DISK_PATH = "build/disk.img"

arch = "x86_32"
if "arm32" in sys.argv:
    arch = "arm32"

if arch == "x86_32":
    QEMU_BIN = "qemu-system-i386"
    QEMU_FLAGS = f"-drive format=raw,file={DISK_PATH},index=0,media=disk -device virtio-mouse-pci -serial stdio"
    if shell_which("i686-elf-gcc"):
        PREFIX = "i686-elf-"
    elif shell_which("i386-elf-gcc"):
        PREFIX = "i386-elf-"
    else:
        print("Error: x86 cross-compiler not found")
        sys.exit(1)
elif arch == "arm32":
    PREFIX = "arm-none-eabi-"
    QEMU_BIN = "qemu-system-arm"
    QEMU_FLAGS = f"-drive format=raw,file={DISK_PATH},index=0,media=disk -serial stdio -M raspi2b"
    ASM_ASSEMBLER = "arm-none-eabi-as"
else:
    print("\x1b[31mInvalid architecture\x1b[0m")
    sys.exit(1)

TARGET_DIR = PREFIX.rstrip('-')

PROJECT_ROOT = os.getcwd()

NEWLIB_SRC = os.path.join(PROJECT_ROOT, "newlib-cygwin")
NEWLIB_TARGET = PREFIX.rstrip('-')
LIBC_INSTALL_DIR = os.path.join(os.getcwd(), f"libc_build_{arch}")
LIBC_DIR = os.path.join(LIBC_INSTALL_DIR, TARGET_DIR)
LIBC_INC = os.path.join(LIBC_DIR, "include")
LIBC_LIB = os.path.join(LIBC_DIR, "lib")

ACPICA_ARCH_INDEPENDANT = os.path.join(PROJECT_ROOT, "include/drivers/acpi")
ACPICA_ARCH_DEPENDANT   = os.path.join(PROJECT_ROOT, f"include/arch/{arch}/acpi")

CC = f"{PREFIX}gcc"
AS = f"{PREFIX}as"

CFLAGS = f"-ffreestanding -O2 -Wall -Wextra -fno-exceptions \
    -fdiagnostics-color=always \
    -Iinclude -I{LIBC_INC} -I{ACPICA_ARCH_INDEPENDANT} -I{ACPICA_ARCH_DEPENDANT}"

BOOT_OBJ = "build/boot.o"

CRTBEGIN = run(f"{CC} {CFLAGS} -print-file-name=crtbegin.o")
CRTEND   = run(f"{CC} {CFLAGS} -print-file-name=crtend.o")

USER_LIBC_DIR = f"arch/{arch}/libc"
USER_LIBC = f"{USER_LIBC_DIR}/user.c"
USER_ASM  = f"{USER_LIBC_DIR}/user.asm"

USER_BUILD_DIR = "build/apps"
USER_LIBC_OBJ = f"{USER_BUILD_DIR}/user.o"
USER_ASM_OBJ  = f"{USER_BUILD_DIR}/user_asm.o"

APPS_ROOT = "apps"
USER_CFLAGS = f"-ffreestanding -O2 -Iinclude -I{LIBC_INC}"

MAKE_CONF_JSON = "make.conf.json"

if not os.path.exists(MAKE_CONF_JSON):
    with open(MAKE_CONF_JSON, 'w') as mjson:
        json.dump({
            "BOOT_USB_PATH": "path/to/usb"
        }, mjson, indent=4)

with open(MAKE_CONF_JSON) as mjson:
    data = json.load(mjson)
    BOOT_USB_PATH = data["BOOT_USB_PATH"]

# --- TARGET FUNCTIONS ---

def farix_bin_x86_32():
    CRTI_SRC = "arch/x86_32/asm/boot/crti.asm"
    CRTN_SRC = "arch/x86_32/asm/boot/crtn.asm"
    BOOT_SRC = "arch/x86_32/boot.s"

    CRTI_OBJ = "build/asm/boot/crti.o"
    CRTN_OBJ = "build/asm/boot/crtn.o"

    c_srcs, asm_srcs = get_sources("x86_32")

    c_objs = [get_obj_path(s) for s in c_srcs]
    other_asm_srcs = [s for s in asm_srcs if "crti.asm" not in s and "crtn.asm" not in s]
    other_asm_objs = [get_obj_path(s) for s in other_asm_srcs]

    build_object(BOOT_SRC, BOOT_OBJ, f"{AS} {{src}} -o {{obj}}")
    build_object(CRTI_SRC, CRTI_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")
    build_object(CRTN_SRC, CRTN_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")

    for s, o in zip(other_asm_srcs, other_asm_objs):
        build_object(s, o, f"nasm -f elf32 {{src}} -o {{obj}}")
    for s, o in zip(c_srcs, c_objs):
        build_object(s, o, f"{CC} -c {{src}} -o {{obj}} {CFLAGS}")

    ld_flags = "-T arch/x86_32/linker.ld -ffreestanding -O2 -nostdlib"

    all_objs = [BOOT_OBJ, CRTI_OBJ, CRTBEGIN] + c_objs + other_asm_objs
    objs = " ".join(all_objs)
    libs = f"-L{LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{CC} {ld_flags} -o farix.bin {objs} {libs} {CRTEND} {CRTN_OBJ}"

    print("\x1b[3;33mLinking farix.bin for x86_32...\x1b[0m")
    run(cmd)
    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin_arm32():
    BOOT_SRC = "arch/arm32/boot.s"

    c_srcs, asm_srcs = get_sources("arm32")

    c_objs   = [get_obj_path(s) for s in c_srcs]
    asm_objs = [get_obj_path(s) for s in asm_srcs]

    build_object(BOOT_SRC, BOOT_OBJ, f"{AS} {{src}} -o {{obj}}")

    for s, o in zip(asm_srcs, asm_objs):
        build_object(s, o, f"{AS} {{src}} -o {{obj}}")
    for s, o in zip(c_srcs, c_objs):
        cpu_flag = "-mcpu=cortex-a7" if "raspi2b" in QEMU_FLAGS else "-mcpu=cortex-a53"
        build_object(s, o, f"{CC} -c {{src}} -o {{obj}} {CFLAGS} {cpu_flag} -marm")

    ld_flags = f"-T arch/arm32/linker.ld -ffreestanding -O2 -nostdlib"
    all_objs = [BOOT_OBJ] + c_objs + asm_objs

    objs = " ".join(all_objs)
    libs = f"-L{LIBC_LIB} -lc -lm -lgcc"

    cmd = f"{CC} {ld_flags} -o farix.bin {objs} {libs}"

    print("\x1b[3;33mLinking farix.bin for arm32...\x1b[0m")
    run(cmd)

    # 7. Create the raw binary for QEMU -kernel or SD card
    run(f"{PREFIX}objcopy farix.bin -O binary farix.img")

    print("\x1b[1;32mProcess completed\x1b[0m")

def farix_bin():
    match arch:
        case "x86_32": farix_bin_x86_32()
        case "arm32": farix_bin_arm32()

def get_deps():
    if not os.path.exists("newlib-cygwin"):
        print("Installing newlib...")
        run("git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git")
    print("Installed dependencies.")

def libc_x86_32():
    # Ensure we use the i686-elf prefix and the arch-specific build folder
    build_dir = "build/newlib-x86_32-build"
    os.makedirs(build_dir, exist_ok=True)

    config_cmd = (
        f"cd {build_dir} && "
        f"CC_FOR_TARGET={PREFIX}gcc AS_FOR_TARGET={PREFIX}as "
        f"LD_FOR_TARGET={PREFIX}ld RANLIB_FOR_TARGET={PREFIX}ranlib "
        f"{NEWLIB_SRC}/configure --target={NEWLIB_TARGET} --prefix={LIBC_INSTALL_DIR} "
        f"--disable-newlib-supplied-syscalls --with-newlib --enable-languages=c,c++"
    )
    print(f"\x1b[1;33mBuilding LibC for x86_32... (This may take a while)\x1b[0m")
    run(config_cmd, capture_output=False)
    run(f"make -C {build_dir} -j$(nproc)", capture_output=False)
    run(f"make -C {build_dir} install", capture_output=False)

def libc_arm32():
    build_dir = "build/newlib-arm32-build"
    os.makedirs(build_dir, exist_ok=True)

    config_cmd = (
        f"cd {build_dir} && "
        f"CC_FOR_TARGET={CC} AS_FOR_TARGET={AS} "
        f"LD_FOR_TARGET={PREFIX}ld RANLIB_FOR_TARGET={PREFIX}ranlib "
        f"{NEWLIB_SRC}/configure --target={NEWLIB_TARGET} --prefix={LIBC_INSTALL_DIR} "
        f"--disable-newlib-supplied-syscalls --with-newlib --enable-languages=c"
    )

    print(f"\x1b[1;33mBuilding LibC for arm32... (This may take a while)\x1b[0m")
    run(config_cmd, capture_output=False)
    run(f"make -C {build_dir} -j$(nproc)", capture_output=False)
    run(f"make -C {build_dir} install", capture_output=False)

def libc():
    match arch:
        case "x86_32": libc_x86_32()
        case "arm32": libc_arm32()

def disk_img():
    if os.path.exists(DISK_PATH):
        os.remove(DISK_PATH)

    if OS == "Darwin": # MacOS
        run(f'hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -layout NONE {DISK_PATH}.dmg > /dev/null')
        os.rename(f"{DISK_PATH}.dmg", DISK_PATH)
    else: # Linux
        run(f"qemu-img create -f raw {DISK_PATH} 64M > /dev/null")
        run(f"mkfs.fat -F 32 -n FARIX {DISK_PATH}")

    print(f"\x1b[32mCreated disk at {DISK_PATH}\x1b[0m")

def compile_apps():
    os.makedirs(USER_BUILD_DIR, exist_ok=True)

    build_object(USER_LIBC, USER_LIBC_OBJ, f"{CC} -c {{src}} -o {{obj}} {USER_CFLAGS}")
    build_object(USER_ASM, USER_ASM_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")

    # Iterate through every folder in /apps
    for app_name in os.listdir(APPS_ROOT):
        app_dir = os.path.join(APPS_ROOT, app_name)
        if not os.path.isdir(app_dir):
            continue

        print(f"\n\x1b[1;35mBuilding App: {app_name}\x1b[0m")

        app_objs = []
        ld_script = None

        for root, _, files in os.walk(app_dir):
            for f in files:
                f_path = os.path.join(root, f)
                obj_path = os.path.join("build", f_path + ".o")

                if f.endswith(".c"):
                    build_object(f_path, obj_path, f"{CC} -c {{src}} -o {{obj}} {USER_CFLAGS}")
                    app_objs.append(obj_path)
                elif f.endswith(".s") or f.endswith(".asm"):
                    build_object(f_path, obj_path, f"nasm -f elf32 {{src}} -o {{obj}}")
                    app_objs.append(obj_path)
                elif f == "linker.ld":
                    ld_script = f_path

        ld_script = ld_script or os.path.join(USER_LIBC_DIR, "linker.ld")

        # Link into build/_user/<folder_name>.elf
        out_elf = os.path.join(USER_BUILD_DIR, f"{app_name}.elf")
        libs = f"-L{LIBC_LIB} -lc -lm -lgcc"
        objs_str = " ".join(app_objs)

        # Put USER_ASM_OBJ at the front to ensure _start is seen first
        link_cmd = f"{CC} -T {ld_script} -ffreestanding -nostdlib -o {out_elf} " \
                   f"{USER_ASM_OBJ} {objs_str} {USER_LIBC_OBJ} {libs}"

        print(f"\x1b[3;35mLinking {out_elf}...\x1b[0m")
        run(link_cmd)

def deploy_apps():
    if not os.path.exists(DISK_PATH):
        disk_img()

    if os.path.exists(USER_BUILD_DIR):
        for f in os.listdir(USER_BUILD_DIR):
            if f.endswith(".elf"):
                elf_path = os.path.join(USER_BUILD_DIR, f)
                run(f"mcopy -i {DISK_PATH} -o {elf_path} ::/{f}")
                print(f"\x1b[32mDeployed {f}\x1b[0m")

def clean():
    for path in ["build", "farix.bin", DISK_PATH]:
        if os.path.isdir(path): shutil.rmtree(path)
        elif os.path.exists(path): os.remove(path)

def run_qemu(fullscreen=False):
    suffix = "-full-screen" if fullscreen else ""

    cmd = f"{QEMU_BIN} -kernel farix.bin {QEMU_FLAGS} {suffix}"
    os.system(cmd)

def deploy_usb():
    if not os.path.exists(BOOT_USB_PATH):
        print(f"\x1b[31m(make.conf.json) Boot USB path '{BOOT_USB_PATH}' not found.\x1b[0m")
        return

    usb_files = os.listdir(BOOT_USB_PATH)
    visible_files = [f for f in usb_files if not f.startswith('.') and f != "System Volume Information"]

    if visible_files:
        print("\x1b[33mWARNING: There are files inside the Boot USB\x1b[0m")
        if input("Wipe everything? (y) > ") == 'y':
            clean_boot_usb()
        else:
            return

    # Ensure Binary Exists
    if not os.path.exists("farix.bin"): farix_bin()

    dest_path = os.path.join(BOOT_USB_PATH, "farix.bin")

    if OS == "Darwin":  # macOS
        print("Formatting USB for MacOS")

        # We need the device identifier (e.g., /dev/disk4) not the mount point
        # This command finds the device associated with your mount path
        df_out = subprocess.check_output(['df', BOOT_USB_PATH]).decode()
        device = df_out.splitlines()[1].split()[0] # e.g., /dev/disk4s1
        raw_device = re.sub(r's[0-9]+$', '', device)

        # Format: Name=FARIX, Table=MBR, Format=FAT32
        subprocess.run(['diskutil', 'eraseDisk', 'FAT32', 'FARIX', 'MBR', raw_device], check=True)

        print("Waiting for USB to remount...", end="", flush=True)
        timeout = 20  # Max seconds to wait
        start_time = time.time()

        while not os.path.exists(BOOT_USB_PATH):
            if time.time() - start_time > timeout:
                print(f"\n\x1b[31mError: Timeout ({timeout}s) waiting for USB to remount.\x1b[0m")
                return
            print(".", end="", flush=True)
            time.sleep(0.1)
        print(" Done.")

        eject_cmd = ['diskutil', 'eject', raw_device]

    elif OS == "Windows":
        print("Formatting USB for Window")

        # Windows requires a diskpart script or a specific format call
        # Note: This usually requires the script to be run as Administrator
        drive_letter = BOOT_USB_PATH.split(':')[0]
        subprocess.run(['format', f'{drive_letter}:', '/FS:FAT32', '/V:FARIX', '/Q', '/Y'], check=True)

        eject_cmd = ['powershell', '-Command', f"(New-Object -ComObject Shell.Application).Namespace(17).ParseName('{drive_letter}:').InvokeVerb('Eject')"]

    elif OS == "Linux":
        print("Formatting USB for Linux")

        # Find device from mount point
        findmnt = subprocess.check_output(['findmnt', '-n', '-o', 'SOURCE', BOOT_USB_PATH]).decode().strip()

        # Unmount first
        subprocess.run(['umount', findmnt], check=True)

        # Format
        subprocess.run(['mkfs.vfat', '-F', '32', '-n', 'FARIX', findmnt], check=True)

        eject_cmd = ['eject', findmnt]

    else:
        print(f"\x1b[31mUnable to format: unknown OS: {OS}\x1b[0m")
        return

    try:
        shutil.copy("farix.bin", dest_path)
        if hasattr(os, 'sync'):
            os.sync()

    except PermissionError:
        print("\x1b[31mError: Permission denied. Is the USB open in another app?\x1b[0m")
        if os.path.exists(dest_path):
            os.remove(dest_path)

    except Exception as e:
        print(f"\x1b[31mFailed to copy: {e}\x1b[0m")
        if os.path.exists(dest_path):
            os.remove(dest_path)

    grub_dir = os.path.join(BOOT_USB_PATH, "boot", "grub")
    os.makedirs(grub_dir, exist_ok=True)

    grub_cfg = (
        "set timeout=0\n"
        "set default=0\n\n"
        "menuentry 'Farix OS' {\n"
        "    multiboot /farix.bin\n"
        "    boot\n"
        "}\n"
    )

    with open(os.path.join(grub_dir, "grub.cfg"), "w") as f:
        f.write(grub_cfg)

    subprocess.run(eject_cmd, check=True)

    print(f"\x1b[32mFarix deployed to {dest_path}\x1b[0m")

def clean_boot_usb():
    shutil.rmtree(BOOT_USB_PATH, ignore_errors=True)

# --- MAIN ---

if __name__ == "__main__":
    target = arch
    for arg in sys.argv[1:]:
        if arg[0] != '-' and not arg.isdigit():
            target = arg
            break

    if "-elen" in sys.argv: err_len = int(sys.argv[sys.argv.index('-elen') + 1])

    if target == arch: farix_bin()
    elif target == "clean": clean()
    elif target == "libc": libc()
    elif target == "get_deps": get_deps()
    elif target == "disk": disk_img()
    elif target == "wipe_usb": clean_boot_usb()
    elif target == "usb":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(DISK_PATH): disk_img()
        deploy_usb()
    elif target == "qemu":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(DISK_PATH): disk_img()
        run_qemu(fullscreen=True)
    elif target == "qemu_":
        if not os.path.exists("farix.bin"): farix_bin()
        if not os.path.exists(DISK_PATH): disk_img()
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
