import os
import sys
import subprocess

OS = None

IGNORES = tuple()

DISK_PATH = None

arch = None

QEMU_BIN = None
QEMU_FLAGS = None

BOCHS_BIN = None
BOCHS_CONFIG = None

PREFIX = None

ASM_ASSEMBLER = None

TARGET_DIR = None

PROJECT_ROOT = None

NEWLIB_SRC = None
NEWLIB_TARGET = None
LIBC_INSTALL_DIR = None
LIBC_DIR = None
LIBC_INC = None
LIBC_LIB = None

ACPICA_ARCH_INDEPENDANT = None
ACPICA_ARCH_DEPENDANT   = None

CC = None
AS = None

CFLAGS = None

BOOT_OBJ = None

CRTBEGIN = None
CRTEND   = None

USER_LIBC_DIR = None
USER_LIBC = None
USER_ASM  = None

USER_BUILD_DIR = None
USER_LIBC_OBJ = None
USER_ASM_OBJ  = None

APPS_ROOT = None
USER_CFLAGS = None

MAKE_CONF_JSON = None

BOOT_USB_PATH   = None
BOCHS_BIOS_PATH = None

GRUB_CFG = None

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

def get_obj_path(src_path):
    ext = src_path.rsplit('.', 1)[-1]
    return src_path.replace(f".{ext}", f".{ext}.o").replace("kernel/", "build/kernel/").replace("arch/", "build/arch/")

def is_ignored(path):
    for ign in IGNORES:
        if path.startswith(ign):
            return True
    return False

def get_sources():
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
