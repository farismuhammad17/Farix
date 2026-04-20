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
\033[1;36mFarix Build System\033[0m
\033[90mhttps://github.com/farismuhammad17/Farix\033[0m

\033[90mTIP: Use 'source make.env' to alias make.py as 'm'\033[0m
Usage: \033[1m m [target] <architecture>\033[0m

\033[1;36mTargets:\033[0m
  \033[1;32mall\033[0m          Build the kernel and deploy apps
  \033[1;32mlibc\033[0m         Compile the internal C standard library
  \033[1;32mdisk\033[0m         Generate the bootable disk image for emulation
  \033[1;32mget_deps\033[0m     Fetch and verify build-tool dependencies
  \033[1;32mwipe_usb\033[0m     Wipe every single file inside the Boot USB
  \033[1;32musb\033[0m          Copy farix.bin into Boot USB \033[90m(provide in make.conf.json)\033[0m
  \033[1;32mqemu\033[0m         Build and launch in QEMU \033[90m(Fullscreen mode)\033[0m
  \033[1;32mqemu_\033[0m        Build and launch in QEMU \033[90m(Windowed mode)\033[0m
  \033[1;32mclean\033[0m        Remove all build artifacts and object files \033[90m(Optionally provide exclusions after; 'apps' to delete only apps)\033[0m
  \033[1;32mapps\033[0m         Compile and add all apps into disk
  \033[1;32mcompile_apps\033[0m Compile all the files inside apps into ELF
  \033[1;32mdeploy_apps\033[0m  Deploy all ELF files inside apps folder
  \033[1;32mdefs\033[0m         Interactive program to return definitions and usage of any function
  \033[1;32mlint\033[0m         Lint the code \033[90m(Useful for developing)\033[0m
  \033[1;32mhelp\033[0m         Display this menu

\033[1;36mArchitectures:\033[0m
  \033[1mx86_32\033[0m       32-bit x86 \033[90m(default)\033[0m
  \033[1marm32\033[0m        32-bit ARM \033[90m(Requires arm-none-eabi-gcc)\033[0m
"""
