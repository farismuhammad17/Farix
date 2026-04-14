import os
import shutil
import subprocess

import makefile.globals

def disk_img():
    if makefile.globals.OS == "Darwin":
        makefile.globals.run(f"qemu-img create -f raw {makefile.globals.DISK_PATH} 64M")
        makefile.globals.run(f'hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE {makefile.globals.DISK_PATH}')
        os.rename(f"{makefile.globals.DISK_PATH}.dmg", makefile.globals.DISK_PATH)
    else:
        makefile.globals.run(f"qemu-img create -f raw {makefile.globals.DISK_PATH} 64M")
        makefile.globals.run(f"mkfs.fat -F 32 -n FARIX {makefile.globals.DISK_PATH}")

    print(f"\x1b[32mCreated disk at {makefile.globals.DISK_PATH}\x1b[0m")
