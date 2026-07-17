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
from concurrent.futures import ThreadPoolExecutor

import makefile.globals as m

APPS_ROOT = "apps"

def compile_apps():
    os.makedirs(m.USER_BUILD_DIR, exist_ok=True)

    # Use threads to speed up build time
    tasks = []
    tasks.append((m.USER_LIBC_ARCH, m.USER_LIBC_ARCH_OBJ, f"{m.CC} -c {{src}} -o {{obj}} {m.USER_CFLAGS}"))
    tasks.append((m.USER_LIBC, m.USER_LIBC_OBJ, f"{m.CC} -c {{src}} -o {{obj}} {m.USER_CFLAGS}"))
    tasks.append((m.USER_ASM, m.USER_ASM_OBJ, "nasm -f elf64 {src} -o {obj}"))

    # Iterate through every folder in /apps
    app_link_data = []
    for app_name in os.listdir(APPS_ROOT):
        app_dir = os.path.join(APPS_ROOT, app_name)
        if not os.path.isdir(app_dir):
            continue

        app_objs = []
        ld_script = None

        for root, _, files in os.walk(app_dir):
            for f in files:
                f_path = os.path.join(root, f)
                obj_path = os.path.join("build", f_path + ".o")

                if f.endswith(".c"):
                    tasks.append((f_path, obj_path, f"{m.CC} -c {{src}} -o {{obj}} {m.USER_CFLAGS}"))
                    app_objs.append(obj_path)
                elif f.endswith(".s") or f.endswith(".asm"):
                    tasks.append((f_path, obj_path, "nasm -f elf64 {src} -o {obj}"))
                    app_objs.append(obj_path)
                elif f == "linker.ld":
                    ld_script = f_path

        app_link_data.append((app_name, app_objs, ld_script))

    # Ensure parent directories for all target objects exist
    for _, obj_path, _ in tasks:
        os.makedirs(os.path.dirname(obj_path), exist_ok=True)

    # Execute the threads in parallel
    with ThreadPoolExecutor(max_workers=m.THREADS) as executor:
        for t in tasks:
            executor.submit(m.build_object, *t)

    for app_name, app_objs, ld_script in app_link_data:
        print(f"\n\x1b[1;35mBuilding App: {app_name}\x1b[0m")

        ld_script = ld_script or os.path.join(m.USER_LIBC_DIR, "linker.ld")

        out_elf = os.path.join(m.USER_BUILD_DIR, f"{app_name}.elf")
        libs = f"-nodefaultlibs -Wl,-Bstatic -L{m.LIBC_LIB} -Wl,--whole-archive -lc -Wl,--no-whole-archive -lm -lgcc"
        objs_str = " ".join(app_objs)

        link_cmd = f"{m.CC} -T {ld_script} -ffreestanding -nostdlib -o {out_elf} " \
                   f"{m.USER_ASM_OBJ} {objs_str} {m.USER_LIBC_ARCH_OBJ} {m.USER_LIBC_OBJ} {libs}"

        print(f"\x1b[3;33mLinking {out_elf}...\x1b[0m")
        m.run(link_cmd)

def deploy_apps():
    if not os.path.exists(m.DISK_PATH):
        print("\x1b[31mDisk image not found (run: m disk)\x1b[0m")
        return

    if os.path.exists(m.USER_BUILD_DIR):
        for f in os.listdir(m.USER_BUILD_DIR):
            if f.endswith(".elf"):
                elf_path = os.path.join(m.USER_BUILD_DIR, f)
                m.run(f"mcopy -i {m.DISK_PATH} -o {elf_path} ::/system/{f}")
                print(f"\x1b[32mDeployed {f}\x1b[0m")
