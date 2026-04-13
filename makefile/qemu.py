import os

import makefile.globals

def run_qemu(fullscreen=False):
    suffix = "-full-screen" if fullscreen else ""

    cmd = f"{makefile.globals.QEMU_BIN} -kernel farix.bin {makefile.globals.QEMU_FLAGS} {suffix}"
    os.system(cmd)
