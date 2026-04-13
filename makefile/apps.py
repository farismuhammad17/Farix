import os

import makefile.globals

def compile_apps():
    os.makedirs(makefile.globals.USER_BUILD_DIR, exist_ok=True)

    makefile.globals.build_object(makefile.globals.USER_LIBC, makefile.globals.USER_LIBC_OBJ, f"{makefile.globals.CC} -c {{src}} -o {{obj}} {makefile.globals.USER_CFLAGS}")
    makefile.globals.build_object(makefile.globals.USER_ASM, makefile.globals.USER_ASM_OBJ, f"nasm -f elf32 {{src}} -o {{obj}}")

    # Iterate through every folder in /apps
    for app_name in os.listdir(makefile.globals.APPS_ROOT):
        app_dir = os.path.join(makefile.globals.APPS_ROOT, app_name)
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
                    makefile.globals.build_object(f_path, obj_path, f"{makefile.globals.CC} -c {{src}} -o {{obj}} {makefile.globals.USER_CFLAGS}")
                    app_objs.append(obj_path)
                elif f.endswith(".s") or f.endswith(".asm"):
                    makefile.globals.build_object(f_path, obj_path, f"nasm -f elf32 {{src}} -o {{obj}}")
                    app_objs.append(obj_path)
                elif f == "linker.ld":
                    ld_script = f_path

        ld_script = ld_script or os.path.join(makefile.globals.USER_LIBC_DIR, "linker.ld")

        # Link into build/_user/<folder_name>.elf
        out_elf = os.path.join(makefile.globals.USER_BUILD_DIR, f"{app_name}.elf")
        libs = f"-L{makefile.globals.LIBC_LIB} -lc -lm -lgcc"
        objs_str = " ".join(app_objs)

        # Put USER_ASM_OBJ at the front to ensure _start is seen first
        link_cmd = f"{makefile.globals.CC} -T {ld_script} -ffreestanding -nostdlib -o {out_elf} " \
                   f"{makefile.globals.USER_ASM_OBJ} {objs_str} {makefile.globals.USER_LIBC_OBJ} {libs}"

        print(f"\x1b[3;35mLinking {out_elf}...\x1b[0m")
        run(link_cmd)

def deploy_apps():
    if not os.path.exists(makefile.globals.DISK_PATH):
        print("\x1b[31mDisk image not found (run: m disk)\x1b[0m")
        return

    if os.path.exists(makefile.globals.USER_BUILD_DIR):
        for f in os.listdir(makefile.globals.USER_BUILD_DIR):
            if f.endswith(".elf"):
                elf_path = os.path.join(makefile.globals.USER_BUILD_DIR, f)
                makefile.globals.run(f"mcopy -i {makefile.globals.DISK_PATH} -o {elf_path} ::/{f}")
                print(f"\x1b[32mDeployed {f}\x1b[0m")
