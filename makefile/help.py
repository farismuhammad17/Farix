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
  \x1b[1;32mall\x1b[0m          Build the kernel and deploy apps
  \x1b[1;32mlibc\x1b[0m         Compile the internal C standard library
  \x1b[1;32mdisk\x1b[0m         Generate the bootable disk image for emulation
  \x1b[1;32mget_deps\x1b[0m     Fetch and verify build-tool dependencies
  \x1b[1;32mwipe_usb\x1b[0m     Wipe every single file inside the Boot USB
  \x1b[1;32musb\x1b[0m          Copy farix.bin into Boot USB \x1b[90m(provide in make.conf.json)\x1b[0m
  \x1b[1;32mqemu\x1b[0m         Build and launch in QEMU \x1b[90m(Fullscreen mode)\x1b[0m
  \x1b[1;32mqemu_\x1b[0m        Build and launch in QEMU \x1b[90m(Windowed mode)\x1b[0m
  \x1b[1;32mclean\x1b[0m        Remove all build artifacts and object files \x1b[90m(Optionally provide exclusions after; 'apps' to delete only apps)\x1b[0m
  \x1b[1;32mapps\x1b[0m         Compile and add all apps into disk
  \x1b[1;32mcompile_apps\x1b[0m Compile all the files inside apps into ELF
  \x1b[1;32mdeploy_apps\x1b[0m  Deploy all ELF files inside apps folder
  \x1b[1;32mdefs\x1b[0m         Interactive program to return definitions and usage of any function
  \x1b[1;32mlint\x1b[0m         Lint the code \x1b[90m(Useful for developing)\x1b[0m
  \x1b[1;32mconfig\x1b[0m       Change configuration settings
  \x1b[1;32mhelp\x1b[0m         Display this menu

\x1b[1;36mArchitectures:\x1b[0m
  \x1b[1mx86_32\x1b[0m       32-bit x86 \x1b[90m(default)\x1b[0m
  \x1b[1marm32\x1b[0m        32-bit ARM \x1b[90m(Requires arm-none-eabi-gcc)\x1b[0m

\x1b[1;36mFlags:\x1b[0m
  \x1b[1m--log\x1b[0m        Write all command output text to \x1b[90mbuild.log\x1b[0m
"""
