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

import makefile.globals as m

def libc_x86_32():
    musl_src = "musl"
    build_dir = "build/musl-x86_32-build"

    os.makedirs(build_dir, exist_ok=True)

    config_cmd = (
        f"cd {build_dir} && "
        f"../../{musl_src}/configure "
        f"--target=i686-linux-gnu "
        f"--prefix={m.LIBC_INSTALL_DIR} "
        f"--disable-shared "
        f"--disable-wrapper "
        f"CC=\"{m.PREFIX}gcc\" "
        f"CROSS_COMPILE=\"{m.PREFIX}\""
    )

    print(f"\x1b[1;33mBuilding Musl LibC for x86_32...\x1b[0m")
    m.run(config_cmd, capture_output=False)
    m.run(f"make -C {build_dir} -j$(nproc)", capture_output=False)
    m.run(f"make -C {build_dir} install", capture_output=False)

def libc_arm32():
    raise NotImplementedError("ARM32 is currently unsupported, will add later")

def libc():
    match m.arch:
        case "x86_32": libc_x86_32()
        case "arm32": libc_arm32()
