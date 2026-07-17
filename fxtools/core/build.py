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

import sys
import subprocess
from pathlib import Path

from fxtools.core import printer
from fxtools.core import statejson

data = statejson.get()
arch = data["DEFAULT_ARCH"]

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
    f"arch/{arch}/libc/",
    "kernel/libc/user.c",
)

def proc_run(cmd: str, check: bool = True, capture: bool = True) -> str:
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            check=check,
            capture_output=capture,
            text=True,
            encoding="utf-8",
            errors="replace"
        )

        return result.stdout.strip() if capture and result.stdout else ""

    except subprocess.CalledProcessError as e:
        printer.error("BUILD ERROR")
        printer.info("COMMAND", e.cmd)

        # Ensure error output gets decoded cleanly
        err = e.stderr or e.stdout or ""
        if isinstance(err, bytes):
            err = err.decode("utf-8", errors="replace")

        printer.info("DETAILS", err.strip())
        sys.exit(1)

def build_object(src: str, obj: str, cmd_template: str):
    """Performs smart incremental compilation. Only builds if source is newer."""

    src_path, obj_path = Path(src), Path(obj)

    if obj_path.exists():
        # If the compiled object is newer than the source, skip compiling
        if obj_path.stat().st_mtime > src_path.stat().st_mtime:
            return

    # Automatically handle parent directory creation
    obj_path.parent.mkdir(parents=True, exist_ok=True)

    printer.info("COMPILING", f"{src} ({printer.F_GREY(obj)})")
    proc_run(cmd_template.format(src=src, obj=obj))

def is_ignored(path: Path) -> bool:
    return any(str(path).startswith(ign) for ign in IGNORES)

def get_obj_path(src_path: Path) -> Path:
    """Translates 'kernel/fs/ext2.c' to 'build/kernel/fs/ext2.c.o'"""

    parts = list(src_path.parts)
    if parts[0] in ("kernel", "arch"):
        parts.insert(0, "build")

    # Append .o to the existing filename
    obj_path = Path(*parts)
    return obj_path.with_name(obj_path.name + ".o")

def get_sources() -> tuple[list[Path], list[Path]]:
    """Crawls and returns matching (C, ASM) source files for the architecture."""

    c_sources = []
    asm_sources = []

    # Search folders
    search_dirs = [Path("kernel")]
    arch_dir = Path("arch") / arch
    if arch_dir.exists():
        search_dirs.append(arch_dir)

    for directory in search_dirs:
        # Recursive globbing
        for p in directory.rglob("*"):
            if p.is_dir() or is_ignored(p) or p.name == "boot.s":
                continue

            if p.suffix == ".c":
                c_sources.append(p)
            elif p.suffix in (".asm", ".s"):
                asm_sources.append(p)

    return c_sources, asm_sources
