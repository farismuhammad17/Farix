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

import re
from pathlib import Path

import makefile.globals

# Regex for function definitions and references
FUNC_DEF_RE = re.compile(r"^[a-zA-Z_][\w\s\*]+\s+([a-zA-Z_]\w*)\s*\([^;]*\)\s*\{", re.MULTILINE)
FUNC_REF_RE = re.compile(r"\b([a-zA-Z_]\w*)\b")

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
            print(f"\x1b[91mSORTING\x1b[0m    {file_path}")
            for inc in sorted_block:
                print(f"    \x1b[90m{inc}\x1b[0m")

def lint():
    total_files = 0
    total_lines = 0

    definitions = {}  # {func_name: file_path}
    references = {}   # {func_name: set(file_paths)}

    for file in root.rglob("README-UNFINISHED.md"):
        print(f"\x1b[93mREADME     \x1b[0m{file}")

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
                print(f"\x1b[93mCOPYRIGHT \x1b[0m {file}")

            current_block = []

            for line in lines:
                clean_line = line.strip()

                if "TODO REM" in clean_line:
                    print(f"\x1b[92mTODO REM\x1b[0m   {file}")

                elif "TODO IMP" in clean_line:
                    print(f"\x1b[92mTODO IMP\x1b[0m   {file}")
                    print(f"    \x1b[90m{clean_line}\x1b[0m")

                elif "TODO" in clean_line:
                    print(f"\x1b[92mTODO\x1b[0m       {file}")
                    print(f"    \x1b[90m{clean_line}\x1b[0m")

                elif "LOG_CALL" in clean_line and file.parts[-1] != "kernel.h":
                    print(f"\x1b[92mLOG_CALL\x1b[0m   {file}")

                if clean_line.startswith("#include"):
                    current_block.append(clean_line)
                else:
                    check_block(current_block, file)
                    current_block = []

            check_block(current_block, file)

            if file.suffix == ".c":
                for match in FUNC_DEF_RE.finditer(content):
                    full_match = match.group(0)
                    name = match.group(1)

                    # Ignore if already static or is a common keyword
                    if not full_match.startswith("static") and name not in ("main", "if", "while", "for", "switch"):
                        definitions[name] = file

            words = FUNC_REF_RE.findall(content)
            for word in words:
                if word not in references:
                    references[word] = set()
                references[word].add(file)

    for func, def_file in definitions.items():
        ref_files = references.get(func, set())

        in_header = any(f.suffix == ".h" for f in ref_files)
        used_elsewhere = any(f != def_file for f in ref_files if f.suffix != ".h")

        if not in_header and not used_elsewhere:
            print(f"\x1b[95mSTATIC\x1b[0m     {def_file} -> {func}")

    print(f"\nTotal files: {total_files}")
    print(f"      lines: {total_lines}")
