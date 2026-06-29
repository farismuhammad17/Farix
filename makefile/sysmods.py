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
import shutil
from concurrent.futures import ThreadPoolExecutor

import makefile.globals as m

def build_sysmods_x86():
    print("\x1b[1;34mFinding System Modules (x86)...\x1b[0m")

    search_dirs = ["sysmods/x86", "sysmods/generic"]
    src_files = []

    for s_dir in search_dirs:
        if not os.path.exists(s_dir):
            continue
        for item in os.listdir(s_dir):
            if item.endswith(".c"):
                src_files.append(os.path.join(s_dir, item))

    tasks = []
    mod_link_data = []

    # Map files and queue object compilation tasks
    for mod_src in src_files:
        filename = os.path.basename(mod_src)
        mod_name = os.path.splitext(filename)[0]

        mod_obj = f"build/sysmods/{mod_name}.o"
        mod_out = f"build/sysmods/{mod_name}.sys"
        os.makedirs(os.path.dirname(mod_obj), exist_ok=True)

        cc_flags = f"{m.CC} -c {mod_src} -o {mod_obj} {m.CFLAGS} -fPIC"
        tasks.append((mod_src, mod_obj, cc_flags))

        mod_link_data.append((mod_name, mod_obj, mod_out))

    with ThreadPoolExecutor(max_workers=m.THREADS) as executor:
        for t in tasks:
            executor.submit(m.build_object, *t)

    # Process linking and target disk deployment
    for mod_name, mod_obj, mod_out in mod_link_data:
        print(f"\n\x1b[1;35mBuilding System Module: {mod_name}\x1b[0m")

        ld_flags = "-T sysmods/linker.ld -ffreestanding -nostdlib -O2 -Wl,--oformat=binary"
        ld_cmd = f"{m.CC} {ld_flags} {mod_obj} -o {mod_out}"

        print(f"\x1b[3;33mLinking {mod_out}...\x1b[0m")
        m.run(ld_cmd)

        print(f"\x1b[33mDeploying {mod_name}.sys to {m.DISK_PATH}::/system/\x1b[0m")
        m.run(f"mcopy -D o -i {m.DISK_PATH} {mod_out} ::/system/{mod_name}.sys")

    print("\x1b[1;32mSystem modules compiled and deployed successfully\x1b[0m")

def build_sysmods_arm():
    raise NotImplementedError("ARM32 is currently unsupported, will add later")

def build_sysmods():
    match m.arch:
        case "x86_32": build_sysmods_x86()
        case "arm32": build_sysmods_arm()
