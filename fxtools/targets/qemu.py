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
import sys

from fxtools.core import printer
from fxtools.core import env
from fxtools.core import statejson
from fxtools.core.build import proc_run

from fxtools.vars import emulation

data = statejson.get()
cores = data["RUNTIME_CORES"]
fullscreen = data["QEMU_FULLSCREEN"]

def get_qemu_flags_x86_64(storage_dev: str):
    QEMU_BIN = "qemu-system-x86_64"

    QEMU_FLAGS = (
        "-machine q35,accel=tcg "
        "-cpu max "
        "-m 512 "
        "-boot order=d,once=d,menu=on,strict=on "
        "-global i8042.extended-state=off "
        f"-smp {cores} "
        "-device virtio-mouse-pci "
        "-vga std "
        "-serial stdio "
        "-display sdl "
    )

    if storage_dev == "ahci":
        QEMU_FLAGS += (
            f"-drive file={emulation.DISK_PATH},if=none,id=disk0,format=raw,media=disk "
            "-device ahci,id=ahci0 "
            "-device ide-hd,drive=disk0,bus=ahci0.0 "
        )
    elif storage_dev == "ata":
        QEMU_FLAGS += (
            f"-drive file={emulation.DISK_PATH},if=none,id=hd0,format=raw,media=disk "
            "-device piix4-ide,id=pci-ide0 "
            "-device ide-hd,bus=pci-ide0.0,drive=hd0 "
        )
    else:
        printer.error(f"Invalid storage device: {storage_dev}")
        sys.exit(1)

    return QEMU_BIN, QEMU_FLAGS

def run(no_fs: bool = not fullscreen, storage_device: str = "ahci"):
    if env.is_in_docker():
        print("\x1b[31mCannot emulate QEMU in docker. Run on native machine instead.\x1b[0m")
        return

    qemu, flags = get_qemu_flags_x86_64(storage_device)
    suffix = "-full-screen" if not no_fs else ""

    if not os.path.exists(emulation.DISK_PATH):
        printer.error(f"Disk image not found: {emulation.DISK_PATH} (run 'fx disk')")
        return

    proc_run(f"{qemu} -cdrom farix.iso {flags} {suffix}",
        capture=False)

def help():
    return {
        "USAGE": "fx qemu <--no_fs>",
        "DESCRIPTION": "Emulate on QEMU",
        "ARGS": {
            "no_fs": "A boolean flag to disable full-screen mode, since, by default, it is enabled.",
            "storage_device": "The storage device to use: 'ahci' (default) and 'ata'."
        },
        "NOTES": [
            "Currently hard coded to work only on x86_64, although supporting other architectures is trivial."
        ]
    }
