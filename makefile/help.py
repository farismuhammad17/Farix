"""
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
"""

HELP = """
\x1b[1;36mFarix Build System\x1b[0m
\x1b[90mhttps://github.com/farismuhammad17/Farix\x1b[0m

\x1b[90mTIP: Use 'source make.env' to alias make.py as 'm'\x1b[0m
Usage: \x1b[1m m [target] <architecture>\x1b[0m

\x1b[1;36mTargets:\x1b[0m
    \x1b[1;32mall\x1b[0m             Build the kernel and deploy apps
    \x1b[1;32miso\x1b[0m             Create a farix.iso file
    \x1b[1;32mdisk\x1b[0m            Generate the bootable disk image for emulation
    \x1b[1;32mget_deps\x1b[0m        Fetch and verify build-tool dependencies
    \x1b[1;32mlibc\x1b[0m            Compile the internal C standard library
    \x1b[1;32mwipe_usb\x1b[0m        Wipe every single file inside the Boot USB
    \x1b[1;32musb\x1b[0m             Copy farix.bin into Boot USB \x1b[90m(provide in make.conf.json)\x1b[0m
    \x1b[1;32mqemu\x1b[0m            Build and launch in QEMU \x1b[90m(Fullscreen mode)\x1b[0m
    \x1b[1;32mqemu_\x1b[0m           Build and launch in QEMU \x1b[90m(Windowed mode)\x1b[0m
    \x1b[1;32mbochs\x1b[0m           Build and launch in BOCHS
    \x1b[1;32mclean\x1b[0m           Remove all build artifacts and object files \x1b[90m(Optionally provide exclusions after; 'apps' to delete only apps)\x1b[0m
    \x1b[1;32mapps\x1b[0m            Compile and add all apps into disk
    \x1b[1;32mdefs\x1b[0m            Interactive program to return definitions and usage of any function
    \x1b[1;32mlint\x1b[0m            Lint the code \x1b[90m(Useful for developing)\x1b[0m
    \x1b[1;32mconfig\x1b[0m          Change configuration settings
    \x1b[1;32mhelp\x1b[0m            Display this menu

\x1b[1;36mArchitectures:\x1b[0m
    \x1b[1mx86_32\x1b[0m          32-bit x86 \x1b[90m(default)\x1b[0m
    \x1b[1marm32\x1b[0m           32-bit ARM \x1b[90m(Requires arm-none-eabi-gcc)\x1b[0m

\x1b[1;36mFlags:\x1b[0m \x1b[90m(Using first letter of flag also works)\x1b[0m
    \x1b[1m-arch\x1b[0m           Architecture to compile to
    \x1b[1m-threads\x1b[0m        Number of threads to compile with
    \x1b[1m-storage\x1b[0m        Options: AHCI, ATA
    \x1b[1m-cores\x1b[0m          Number of CPU cores to launch from
    \x1b[1m--log\x1b[0m           Write all command output text to \x1b[90mbuild.log\x1b[0m

\x1b[1;36mConfiguration:\x1b[0m \x1b[90m(make.conf.json)\x1b[0m
    \x1b[1mBOOT_USB_PATH\x1b[0m   Path to USB to copy kernel to for 'm usb'
    \x1b[1mDEFAULT_ARCH\x1b[0m    Default architecture to compile and assume
    \x1b[1mTHREADS\x1b[0m         Number of threads to use for compiling
"""

ALL_HELP = """
Set up the entire project dependancies and compile everything. The function would
only do whatever needs to be done, and not compile what is already compiled.

\x1b[90m1.\x1b[0m Fetches all required dependancies, namely:-
    \x1b[1;32mnewlib\x1b[0m          Provides LibC (Standard C library)
\x1b[90m2.\x1b[0m Compile LibC
\x1b[90m4.\x1b[0m Compile the Kernel
\x1b[90m5.\x1b[0m Create disk.img
\x1b[90m6.\x1b[0m Compile the apps
\x1b[90m7.\x1b[0m Deploy apps into disk.img
"""

ISO_HELP = """
Create the farix.iso bootable image. Requires that the farix.bin file be created
beforehand, and uses GRUB as the bootloader. The GRUB bootloader is already given
the following configuration:

\x1b[33mset\x1b[0m timeout=\x1b[32m5\x1b[0m
\x1b[33mset\x1b[0m default=\x1b[32m0\x1b[0m
\x1b[33mmenuentry\x1b[0m 'Farix OS' {
    \x1b[33mmultiboot\x1b[0m /boot/farix.bin
    boot
}
"""

DISK_HELP = """
Uses qemu-img to create a 64M disk.img QEMU uses as for its virtual file system.
Once disk.img is created, it also adds in a "system" folder inside, which the
kernel requires for its system files.
"""

GET_DEPS_HELP = """
Fetches all required dependancies, namely:-
    \x1b[1;32mnewlib\x1b[0m          Provides LibC (Standard C library)

It does not install anything that is already there.
"""

LIBC_HELP = """
Compiles Newlib for the LibC, used as the standard C library for the kernel.
Once it is compiled once, it need not be done again, unless there is a major
change to system calls, or something of sort.
"""

WIPE_USB_HELP = """
Cleans the boot USB, given in make.conf.json. Make note that this will
literally wipe the USB clean, so do not run it on family photos.
"""

USB_HELP = """
Creates the farix.bin file, if not already made, then formats the USB
for partitions, creating the necessary changes to the USB. Note that
function requires a clean USB, and will wipe of EVERYTHING inside it.
If there are contents present inside the USB, the command will ask if
you wish to delete it. Under the licensing, do not sue me for loss of
data (refer LICENSE, subsection titled '16. Limitation of Liability.').

Once the USB is formatted, the farix.bin file is added into the directory,
and GRUB is configured with the following grub.cfg file:

\x1b[33mset\x1b[0m timeout=\x1b[32m0\x1b[0m
\x1b[33mset\x1b[0m default=\x1b[32m0\x1b[0m
\x1b[33mmenuentry\x1b[0m 'Farix OS' {
    \x1b[33mmultiboot\x1b[0m /farix.bin
    boot
}
"""

QEMU_HELP = """
Boots the kernel using QEMU. Flags are as provided in make.py.
    \x1b[1;32mqemu\x1b[0m            Build and launch in QEMU \x1b[90m(Fullscreen mode)\x1b[0m
    \x1b[1;32mqemu_\x1b[0m           Build and launch in QEMU \x1b[90m(Windowed mode)\x1b[0m
"""

BOCHS_HELP = """
Boots the kernel using Bochs.
"""

CLEAN_HELP = """
Cleans out the project folder. It deems the following as stuff that can be
removed: build, farix.bin, and farix.iso. Of course, these files and folders
are remade once the code is compiled, or needs them ever again.

Optionally, if the word 'apps' is mentioned in the command (m clean apps), then
the command would only clean up the compiled apps. With this, cleaning would
not get rid of the compiled kernel as well, in case you are testing.
"""

APPS_HELP = """
Compiles all apps present inside the /apps/ directory. During compilation, the
code will try to check if a linker.ld file is present. If none are present,
the default linker.ld, present in the architecture specific kernel code, will
be used instead. The command compiles the applications into ELF files.

Once compiling is done, the command deploys these files are deployed into the
disk.img, which the kernel can use. These files would be put into the system
folder that disk.img is formatted to have.
"""

DEFS_HELP = """
A helper command used to find a list of every single function, macro, and
variable used in the kernel source code. It scans through the kernel, and
indexes everything in it. It would store the header the thing is declared
and the files they are defined in, as well as wherever they are used in.
Furthermore, if there are comments using /* */, then the program would show
it under "Documentation".
"""

LINT_HELP = """
Lints the code, as per the coding style mentioned inside /README.md:-

* Includes, in any file, must be in alphabetical order.
* Functions must have \x1b[32m{\x1b[0m in the same line as the header:
* Structs must use aliases always, avoid naming structs.
* Do not use \x1b[32mRARE_FUNC\x1b[0m or \x1b[32mFREQ_FUNC\x1b[0m macros in
\x1b[32mC\x1b[0m files, only headers.
* Any macros defined in a C files, inside a function, must be undefined at
the end.
* If a function can be static, it should be static.
* Files named \x1b[32mREADME-UNFINISHED.md\x1b[0m are gitignored, but are
warned by the linter.
* When pushing code, set \x1b[32mkernel.h/__DEBUG__\x1b[0m to \x1b[32m0\x1b[0m.
* Avoid using \x1b[32mLOG_CALL\x1b[0m or \x1b[32mLOG_NUM\x1b[0m, used for
debugging, when pushing code.
* Mention GNU GPL license header on every file.
* Any \x1b[32mTODO\x1b[0m blocks will be listed, \x1b[32mTODO REM\x1b[0m and
\x1b[32mTODO IMP\x1b[0m getting different colours.
"""

CONFIG_HELP = """
Creates an interactive terminal window where you can change out specific values
inside the make.conf.json. Alternatively, you can change it directly inside the
file itself.
"""

HELP_HELP = """
Syntax: m help <command>
If no 'command', then the default help string will be displayed.

* all
* iso
* disk
* get_deps
* libc
* usb
* qemu, qemu_
* bochs
* apps
* clean
* defs
* lint
* config
* help
"""

def parse_help(args: list[str]):
    cmd = " ".join(args)

    if cmd == "":
        print(HELP)
    elif cmd == "all":
        print(ALL_HELP)
    elif cmd == "iso":
        print(ISO_HELP)
    elif cmd == "disk":
        print(DISK_HELP)
    elif cmd == "get_deps":
        print(GET_DEPS_HELP)
    elif cmd == "libc":
        print(LIBC_HELP)
    elif cmd == "usb":
        print(USB_HELP)
    elif cmd in ("qemu", "qemu_"):
        print(QEMU_HELP)
    elif cmd == "bochs":
        print(BOCHS_HELP)
    elif cmd == "apps":
        print(APPS_HELP)
    elif cmd == "clean":
        print(CLEAN_HELP)
    elif cmd == "defs":
        print(DEFS_HELP)
    elif cmd == "lint":
        print(LINT_HELP)
    elif cmd == "config":
        print(CONFIG_HELP)
    elif cmd == "help":
        print(HELP_HELP)
