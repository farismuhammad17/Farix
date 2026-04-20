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

from pathlib import Path

import makefile.globals

root = Path.cwd()
extns = ("c", "h", "s", "asm")

ignores = (
    "newlib-cygwin",
    "libc_build_x86_32",
    "libc_build_arm32",
    "test",
    "acpi",
)

def check_block(block, file_path):
    if len(block) > 1:
        sorted_block = sorted(block)
        if block != sorted_block:
            print(f"\033[91mSORTING\033[0m    {file_path}")
            for inc in sorted_block:
                print(f"    \033[90m{inc}\033[0m")

def lint():
    total_files = 0
    total_lines = 0

    for file in root.rglob("README-UNFINISHED.md"):
        print(f"\033[93mREADME     \033[0m{file}")

    for ext in extns:
        for file in root.rglob(f"*.{ext}"):
            if any(p in ignores for p in file.parts):
                continue

            try:
                with open(file, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
            except (UnicodeDecodeError, PermissionError):
                continue

            total_files += 1
            total_lines += len(lines)

            content = "".join(lines)

            if "Copyright (C)" not in content:
                print(f"\033[93mCOPYRIGHT \033[0m {file}")

            current_block = []

            for line in lines:
                clean_line = line.strip()

                if "TODO REM" in clean_line:
                    print(f"\033[92mTODO REM\033[0m   {file}")

                elif "TODO IMP" in clean_line:
                    print(f"\033[92mTODO IMP\033[0m   {file}")
                    print(f"    \033[90m{clean_line}\033[0m")

                elif "TODO" in clean_line:
                    print(f"\033[92mTODO\033[0m       {file}")
                    print(f"    \033[90m{clean_line}\033[0m")

                elif "LOG_CALL" in clean_line:
                    print(f"\033[92mLOG_CALL\033[0m   {file}")

                if clean_line.startswith("#include"):
                    current_block.append(clean_line)
                else:
                    check_block(current_block, file)
                    current_block = []

            check_block(current_block, file)

    print(f"\nTotal files: {total_files}")
    print(f"      lines: {total_lines}")
