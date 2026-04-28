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
import sys
import subprocess

OS = None

IGNORES = tuple()

DISK_PATH = None

arch = None

QEMU_BIN = None
QEMU_FLAGS = None

PREFIX = ""

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
USER_LIBC_ARCH = None
USER_ASM  = None

USER_BUILD_DIR = None
USER_LIBC_OBJ = None
USER_LIBC_ARCH_OBJ = None
USER_ASM_OBJ  = None

APPS_ROOT = None
USER_CFLAGS = None

MAKE_CONF_JSON = None

BOOT_USB_PATH   = None
BOCHS_BIOS_PATH = None

GRUB_CFG = None

LOGGING = True
LOG_FILE = open("build.log", 'w', encoding="utf-8")

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

        if LOGGING:
            LOG_FILE.write(f"{cmd}\n\n")
            if result.stdout:
                LOG_FILE.write(result.stdout)
            if result.stderr:
                LOG_FILE.write(result.stderr)

        if not capture_output:
            return ""

        return result.stdout.strip() if result.stdout else ""

    except subprocess.CalledProcessError as e:
        print(f"\n\x1b[1;31m--- BUILD ERROR ---\x1b[0m")
        print(f"Command: {e.cmd}\n")

        err_msg = e.stderr
        if isinstance(err_msg, bytes):
            err_msg = err_msg.decode('utf-8', errors='replace')
        print(f"Error:\n{e.stderr}")

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

def is_in_docker():
    return os.path.exists('/.dockerenv')
