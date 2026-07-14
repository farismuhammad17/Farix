"""
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
"""

import re
from pathlib import Path

# Regex for function definitions and references
FUNC_DEF_RE = re.compile(r"^[a-zA-Z_][\w\s\*]+\s+([a-zA-Z_]\w*)\s*\([^;]*\)\s*\{", re.MULTILINE)
FUNC_REF_RE = re.compile(r"\b([a-zA-Z_]\w*)\b")

STRUCT_TYPEDEF_RE = re.compile(
    r"typedef\s+struct\s+([a-zA-Z_]\w*)\s*\{(.*?)\}\s*([a-zA-Z_]\w*)\s*;",
    re.DOTALL
)

root = Path.cwd()
extns = ("c", "h", "s", "asm", "ld")

ignores = (
    "newlib-cygwin",
    "libc_build_x86_32",
    "libc_build_x86_64",
    "libc_build_arm32",
    "test",
    "acpi",
    "musl",
)

def check_redundant_struct_tags(content, file_path):
    for match in STRUCT_TYPEDEF_RE.finditer(content):
        top_name = match.group(1)   # The struct tag name
        body = match.group(2)       # Inside the curly braces
        alias = match.group(3)      # The nickname at the bottom

        # Word boundary, so we don't accidentally match substrings.
        self_ref_pattern = rf"\bstruct\s+{re.escape(top_name)}\b"

        if not re.search(self_ref_pattern, body):
            print(f"\x1b[91mSTRUCT\x1b[0m     {file_path}")
            print(f"    \x1b[90mUse 'typedef struct {{ ... }} {alias};'\x1b[0m")

def check_include_order(block, file_path):
    if len(block) > 1:
        sorted_block = sorted(block)
        if block != sorted_block:
            print(f"\x1b[91mINCLUDES\x1b[0m   {file_path}")
            for inc in sorted_block:
                print(f"    \x1b[90m{inc}\x1b[0m")

def lint():
    total_files = 0
    total_lines = 0

    definitions = {}  # {func_name: file_path}
    references = {}   # {func_name: set(file_paths)}

    for file in root.rglob("README-UNFINISHED.md"):
        print(f"\x1b[93mREADME\x1b[0m     {file}")

    if "#define __DEBUG__ 1" in (root / Path("include/kernel.h")).read_text(encoding="utf-8"):
        print(f"\x1b[93mDEBUG MODE\x1b[0m __DEBUG__ enabled")

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

            license_lines = [
                "-----------------------------------------------------------------------",
                "Farix Operating System",
                "Copyright (C) 2026  Faris Muhammad",
                "This program is free software: you can redistribute it and/or modify",
                "it under the terms of the GNU Affero General Public License as published by",
                "the Free Software Foundation, either version 3 of the License, or",
                "(at your option) any later version.",
                "This program is distributed in the hope that it will be useful,",
                "but WITHOUT ANY WARRANTY; without even the implied warranty of",
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the",
                "GNU Affero General Public License for more details.",
                "You should have received a copy of the GNU Affero General Public License",
                "along with this program.  If not, see <https://www.gnu.org/licenses/>.",
                "-----------------------------------------------------------------------",
            ]
            if not all(line in content for line in license_lines):
                print(f"\x1b[91mCOPYRIGHT\x1b[0m  {file}")

            check_redundant_struct_tags(content, file)

            current_block = []

            c_define_count = 0

            for line in lines:
                clean_line = line.strip()

                if "TODO REM" in clean_line:
                    print(f"\x1b[93mTODO REM\x1b[0m   {file}")

                elif "TODO IMP" in clean_line:
                    print(f"\x1b[95mTODO IMP\x1b[0m   {file}")
                    print(f"    \x1b[90m{clean_line}\x1b[0m")

                elif "TODO" in clean_line:
                    print(f"\x1b[92mTODO\x1b[0m       {file}")
                    print(f"    \x1b[90m{clean_line}\x1b[0m")

                elif ("LOG_CALL" in clean_line or "LOG_NUM" in clean_line) and file.parts[-1] != "kernel.h":
                    print(f"\x1b[96mLOG_FUNC\x1b[0m   {file}")

                elif ext == "c":
                    if "RARE_FUNC" in clean_line or "FREQ_FUNC" in clean_line:
                        print(f"\x1b[96mRARE_FREQ\x1b[0m  {file}")

                    elif "#define" in line and line[0] != "#":
                        c_define_count += 1

                    elif "#undef" in line and line[0] != "#":
                        c_define_count -= 1

                if clean_line.startswith("#include"):
                    current_block.append(clean_line)
                else:
                    check_include_order(current_block, file)
                    current_block = []

            if c_define_count != 0:
                print(f"\x1b[96mUNDEF\x1b[0m      {file}")

            check_include_order(current_block, file)

            if file.suffix == ".c":
                for match in FUNC_DEF_RE.finditer(content):
                    full_match = match.group(0)
                    name = match.group(1)

                    # Brace check
                    if not full_match.strip().endswith(") {"):
                        print(f"\x1b[91mBRACE\x1b[0m      {name} \x1b[90m{file}\x1b[0m")

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
