import os

import makefile.globals

def libc_x86_32():
    # Ensure we use the i686-elf prefix and the arch-specific build folder
    build_dir = "build/newlib-x86_32-build"
    os.makedirs(build_dir, exist_ok=True)

    config_cmd = (
        f"cd {build_dir} && "
        f"CC_FOR_TARGET={makefile.globals.PREFIX}gcc AS_FOR_TARGET={makefile.globals.PREFIX}as "
        f"LD_FOR_TARGET={makefile.globals.PREFIX}ld RANLIB_FOR_TARGET={makefile.globals.PREFIX}ranlib "
        f"{makefile.globals.NEWLIB_SRC}/configure --target={makefile.globals.NEWLIB_TARGET} --prefix={makefile.globals.LIBC_INSTALL_DIR} "
        f"--disable-newlib-supplied-syscalls --with-newlib --enable-languages=c,c++"
    )

    print(f"\x1b[1;33mBuilding LibC for x86_32... (This may take a while)\x1b[0m")

    makefile.globals.run(config_cmd, capture_output=False)
    makefile.globals.run(f"make -C {build_dir} -j$(nproc)", capture_output=False)
    makefile.globals.run(f"make -C {build_dir} install", capture_output=False)

def libc_arm32():
    build_dir = "build/newlib-arm32-build"
    os.makedirs(build_dir, exist_ok=True)

    config_cmd = (
        f"cd {build_dir} && "
        f"CC_FOR_TARGET={makefile.globals.CC} AS_FOR_TARGET={makefile.globals.AS} "
        f"LD_FOR_TARGET={makefile.globals.PREFIX}ld RANLIB_FOR_TARGET={makefile.globals.PREFIX}ranlib "
        f"{makefile.globals.NEWLIB_SRC}/configure --target={makefile.globals.NEWLIB_TARGET} --prefix={makefile.globals.LIBC_INSTALL_DIR} "
        f"--disable-newlib-supplied-syscalls --with-newlib --enable-languages=c"
    )

    print(f"\x1b[1;33mBuilding LibC for arm32... (This may take a while)\x1b[0m")

    makefile.globals.run(config_cmd, capture_output=False)
    makefile.globals.run(f"make -C {build_dir} -j$(nproc)", capture_output=False)
    makefile.globals.run(f"make -C {build_dir} install", capture_output=False)

def libc():
    match makefile.globals.arch:
        case "x86_32": libc_x86_32()
        case "arm32": libc_arm32()
