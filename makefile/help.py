HELP = """
\033[1;36mFarix Build System\033[0m
\033[90mhttps://github.com/farismuhammad17/Farix\033[0m

\033[90mTIP: Use 'source make.env' to alias make.py as 'm'\033[0m
Usage: \033[1m m [target] <architecture>\033[0m

\033[1;36mTargets:\033[0m
  \033[1;32mall\033[0m          Build the kernel and all dependencies \033[90m(default)\033[0m
  \033[1;32mlibc\033[0m         Compile the internal C standard library
  \033[1;32mdisk\033[0m         Generate the bootable disk image for emulation
  \033[1;32mget_deps\033[0m     Fetch and verify build-tool dependencies
  \033[1;32mwipe_usb\033[0m     Wipe every single file inside the Boot USB
  \033[1;32musb\033[0m          Copy farix.bin into Boot USB \033[90m(provide in make.conf.json)\033[0m
  \033[1;32mqemu\033[0m         Build and launch in QEMU \033[90m(Fullscreen mode)\033[0m
  \033[1;32mqemu_\033[0m        Build and launch in QEMU \033[90m(Windowed mode)\033[0m
  \033[1;32mclean\033[0m        Remove all build artifacts and object files \033[90m(Optionally provide exclusions after)\033[0m
  \033[1;32mapps\033[0m         Compile and add all apps into disk
  \033[1;32mcompile_apps\033[0m Compile all the files inside apps into ELF
  \033[1;32mdeploy_apps\033[0m  Deploy all ELF files inside apps folder
  \033[1;32mhelp\033[0m         Display this menu

\033[1;36mArchitectures:\033[0m
  \033[1mx86_32\033[0m       32-bit x86 \033[90m(default)\033[0m
  \033[1marm32\033[0m        32-bit ARM \033[90m(Requires arm-none-eabi-gcc)\033[0m

\033[1;36mFlags:\033[0m
  \033[1m-elen\033[0m        Maximum lines for error message
"""
