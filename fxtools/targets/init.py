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

from fxtools.core import statejson
from fxtools.core import tools
from fxtools.core.build import proc_run

from fxtools.vars import libc

data = statejson.get()
arch = data["DEFAULT_ARCH"]

TOOLS = tools.get_tools()

def libc_x86_64():
    musl_src = "musl"
    build_dir = "build/musl-x86_64-build"

    os.makedirs(build_dir, exist_ok=True)

    # Configure specifically for x86_64 bare-metal kernel compatibility
    config_cmd = (
        f"cd {build_dir} && "
        f"../../{musl_src}/configure "
        f"--target=x86_64-linux-gnu "
        f"--prefix={libc.LIBC_INSTALL_DIR} "
        f"--disable-shared "
        f"--disable-wrapper "
        f"CC=\"{TOOLS['PREFIX']}gcc\" "
        f"CROSS_COMPILE=\"{TOOLS['PREFIX']}\""
    )

    print("\x1b[1;33mBuilding Musl LibC for x86_64...\x1b[0m")

    proc_run(config_cmd, capture=False)
    proc_run(f"make -C {build_dir} -j$(nproc)", capture=False)
    proc_run(f"make -C {build_dir} install", capture=False)

    print("\x1b[1;32mMusl x86_64 compilation completed!\x1b[0m")

def run():
    if not os.path.exists("musl"):
        print("\x1b[33mInstalling musl...\x1b[0m")
        proc_run("git clone --depth 1 https://github.com/hadean-mirrors/musl.git", capture=False)
        print("\x1b[32mInstalled musl\x1b[0m")

    match arch:
        case "x86_64":
            libc_x86_64()

def help():
    return {
        "USAGE": "fx init",
        "DESCRIPTION": "Sets up the project folder, get dependencies, and compiles the standard C library used."
    }
