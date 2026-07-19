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

import os
import subprocess
import hashlib
import re
from pathlib import Path

from fxtools.core import printer

# Constants and Configuration
FUNC_DEF_RE = re.compile(r"^[a-zA-Z_][\w\s\*]+\s+([a-zA-Z_]\w*)\s*\([^;]*\)\s*\{", re.MULTILINE)
FUNC_REF_RE = re.compile(r"\b([a-zA-Z_]\w*)\b")
STRUCT_TYPEDEF_RE = re.compile(r"typedef\s+struct\s+([a-zA-Z_]\w*)\s*\{(.*?)\}\s*([a-zA-Z_]\w*)\s*;", re.DOTALL)
HEADER_GUARD_RE = re.compile(r"^#(?:ifndef|define)\s+([A-Za-z0-9_]+)")

LICENSE_LINES = (
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
    "-----------------------------------------------------------------------"
)

root = Path.cwd()
extns = ("c", "h", "s", "asm", "ld")
ignores = (
    "libc_build_x86_32",
    "libc_build_x86_64",
    "libc_build_arm32",
    "test",
    "acpi",
    "musl"
)

# --- Individual Checker Modules ---

def check_license(content: str, file_issues: list):
    if not all(line in content for line in LICENSE_LINES):
        file_issues.append({
            "type": "COPYRIGHT",
            "message": "Missing or mismatched license block header."
        })

def check_redundant_struct_tags(content: str, file_issues: list):
    for match in STRUCT_TYPEDEF_RE.finditer(content):
        top_name, body, alias = match.group(1), match.group(2), match.group(3)
        self_ref_pattern = rf"\bstruct\s+{re.escape(top_name)}\b"
        if not re.search(self_ref_pattern, body):
            file_issues.append({
                "type": "STRUCT",
                "message": f"Use 'typedef struct {{ ... }} {alias};'"
            })

def check_include_order(block: list, file_issues: list):
    if len(block) > 1 and block != sorted(block):
        file_issues.append({
            "type": "INCLUDES",
            "message": sorted(block)
        })

def check_line_tokens(lines: list, file: Path, ext: str, file_issues: list):
    current_block = []
    c_define_count = 0

    for line in lines:
        clean_line = line.strip()

        # Match TODO signatures
        if "TODO REM" in clean_line:
            file_issues.append({"type": "TODO_REM", "message": clean_line})
        elif "TODO IMP" in clean_line:
            file_issues.append({"type": "TODO_IMP", "message": clean_line})
        elif "TODO" in clean_line:
            file_issues.append({"type": "TODO", "message": clean_line})

        # Match logging hooks
        if ("LOG_CALL" in clean_line or "LOG_NUM" in clean_line) and file.name != "kernel.h":
            file_issues.append({"type": "LOG_FUNC", "message": clean_line})

        # C-specific preprocessor macro tracking
        if ext == "c":
            if "RARE_FUNC" in clean_line or "FREQ_FUNC" in clean_line:
                file_issues.append({"type": "RARE_FREQ", "message": clean_line})
            elif "#define" in line and line[0] != "#":
                c_define_count += 1
            elif "#undef" in line and line[0] != "#":
                c_define_count -= 1

        # Track #include blocks
        if clean_line.startswith("#include"):
            current_block.append(clean_line)
        else:
            check_include_order(current_block, file_issues)
            current_block = []

    check_include_order(current_block, file_issues)

    if c_define_count != 0:
        file_issues.append({
            "type": "UNDEF",
            "message": f"Indentation error or mismatch matching #define/#undef blocks ({c_define_count})"
        })

def check_functions(content: str, file: Path, definitions: dict, file_issues: list):
    if file.suffix != ".c":
        return

    for match in FUNC_DEF_RE.finditer(content):
        full_match, name = match.group(0), match.group(1)

        if not full_match.strip().endswith(") {"):
            file_issues.append({
                "type": "BRACE",
                "message": f"Brace layout style mismatch on definition of '{name}'"
            })

        if not full_match.startswith("static") and name not in ("main", "if", "while", "for", "switch"):
            definitions[name] = file

def track_references(content: str, file: Path, references: dict):
    for word in FUNC_REF_RE.findall(content):
        if word not in references:
            references[word] = set()
        references[word].add(file)

def extract_header_exports(content: str) -> set:
    # Extracts identifiers (structs, macros, functions) exported by a header.
    exports = set()
    for match in HEADER_EXPORT_RE.finditer(content):
        for group in match.groups():
            if group and not group.startswith(("_", "if", "while", "for", "switch")):
                exports.add(group)
    return exports

# --- Post-Processing Global Passes ---

def process_static_visibility(definitions: dict, references: dict, files_report: dict):
    for func, def_file in definitions.items():
        ref_files = references.get(func, set())
        in_header = any(f.suffix == ".h" for f in ref_files)
        used_elsewhere = any(f != def_file for f in ref_files if f.suffix != ".h")

        if not in_header and not used_elsewhere:
            def_file_str = str(def_file)
            if def_file_str not in files_report:
                files_report[def_file_str] = []

            files_report[def_file_str].append({
                "type": "STATIC",
                "message": f"Function '{func}' is only used locally; change declaration to static."
            })

def collect_global_warnings(results: dict):
    for file in root.rglob("README-UNFINISHED.md"):
        results["global_warnings"].append({"type": "README", "path": str(file)})

    debug_header = root / "include/kernel.h"
    if debug_header.exists() and "#define __DEBUG__ 1" in debug_header.read_text(encoding="utf-8"):
        results["global_warnings"].append({
            "type": "DEBUG_MODE",
            "message": "__DEBUG__ enabled"
        })

# --- Main Entry Runner ---

def lint() -> dict:
    results = {
        # Dynamically tracks file extensions in a nested dict
        "stats": {"total_files": 0, "total_lines": 0, "extensions": {}},
        "global_warnings": [],
        "files": {}
    }

    definitions = {}
    references = {}
    guard_tracker = {}  # Tracks macro_name -> original_file_path

    collect_global_warnings(results)

    for ext in extns:
        for file in sorted(root.rglob(f"*.{ext}")):
            if any(p in ignores for p in file.parts):
                continue

            try:
                with open(file, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
            except (UnicodeDecodeError, PermissionError):
                continue

            results["stats"]["total_files"] += 1
            results["stats"]["total_lines"] += len(lines)

            # Increment extension breakdown dictionary
            results["stats"]["extensions"][ext] = results["stats"]["extensions"].get(ext, 0) + 1

            file_issues = []
            content = "".join(lines)

            # --- Duplicate Header Guard Check ---
            if ext == "h":
                last_ifndef = None
                for line in lines:
                    clean_line = line.strip()
                    if not clean_line:
                        continue  # skip empty lines between the pair

                    # Look for the opening #ifndef
                    if clean_line.startswith("#ifndef"):
                        parts = clean_line.split()
                        if len(parts) > 1:
                            last_ifndef = parts[1]
                        continue

                    # Check if the next active line is the matching #define
                    if clean_line.startswith("#define") and last_ifndef:
                        parts = clean_line.split()
                        if len(parts) > 1 and parts[1] == last_ifndef:
                            macro = last_ifndef
                            file_str = str(file)

                            if macro in guard_tracker and guard_tracker[macro] != file_str:
                                file_issues.append({
                                    "type": "GUARD_COLLISION",
                                    "message": f"Duplicate header guard prologue '{macro}' collision with {guard_tracker[macro]}"
                                })
                            else:
                                guard_tracker[macro] = file_str

                        # Reset after inspecting the line following the #ifndef
                        last_ifndef = None
                    else:
                        # If any other statement follows the #ifndef, reset the window
                        last_ifndef = None

            # Execution pipeline for individual file evaluation
            check_license(content, file_issues)
            check_redundant_struct_tags(content, file_issues)
            check_line_tokens(lines, file, ext, file_issues)
            check_functions(content, file, definitions, file_issues)
            track_references(content, file, references)

            if file_issues:
                results["files"][str(file)] = file_issues

    process_static_visibility(definitions, references, results["files"])

    return results

def print_checksum():
    try:
        # Query git using -z to get raw, unquoted paths separated by null bytes (\0)
        result = subprocess.run(
            ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'],
            capture_output=True,
            check=True
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        printer.error("Error: git failed")
        return

    raw_paths = result.stdout.split(b'\x00') if result.stdout else []

    extensions = (
        '.c',
        '.h',
        '.asm',
        '.s',
        '.ld',
        '.py',
        '.cmd',
        '.bat',
        '.env',
        'Dockerfile',
        'CONTRIBUTING.md',
        'DOCUMENTATION.md',
        'LICENSE',
        'legal/ATTRIBUTIONS.txt',
        'legal/CLA.md',
        'legal/LICENSE_EXCEPTIONS.md'
    )

    file_paths = []
    for path_bytes in raw_paths:
        if not path_bytes:
            continue
        try:
            path_str = path_bytes.decode('utf-8').strip()
            if path_str.endswith(extensions):
                file_paths.append(path_str)
        except UnicodeDecodeError:
            continue

    # Sort ascending to guarantee cross-platform deterministic order
    file_paths.sort()

    hasher = hashlib.sha256()

    for path in file_paths:
        # Check physical existence first to handle deleted unstaged files consistently
        if not os.path.exists(path):
            continue

        # Normalize the structural path representation
        normalized_path = path.replace(os.sep, '/')
        hasher.update(normalized_path.encode('utf-8'))

        # Process the contents in binary mode to neutralize CRLF/LF variations
        with open(path, 'rb') as f:
            while chunk := f.read(8192):
                normalized_chunk = chunk.replace(b'\r\n', b'\n')
                hasher.update(normalized_chunk)

    print(hasher.hexdigest())

def run():
    print()

    report = lint()

    # Handle Global Top-Level Warnings First
    for warn_item in report["global_warnings"]:
        w_type = warn_item["type"]
        if w_type == "README":
            print(f"{printer.F_YELLOW('README')}      {warn_item['path']}")
        elif w_type == "DEBUG_MODE":
            print(f"{printer.F_YELLOW('DEBUG MODE')} {warn_item['message']}")

    # Iterate File Issues
    for file_path, issues in report["files"].items():
        for issue in issues:
            itype = issue["type"]
            msg = issue["message"]

            if itype == "GUARD_COLLISION":
                print(f"{printer.F_RED('GUARD_DUP')}   {file_path}")
                print(f"    {printer.F_GREY(msg)}")

            elif itype == "STRUCT":
                print(f"{printer.F_RED('STRUCT')}      {file_path}")
                print(f"    {printer.F_GREY(msg)}")

            elif itype == "INCLUDES":
                print(f"{printer.F_RED('INCLUDES')}    {file_path}")
                for inc_line in msg:
                    print(f"    {printer.F_GREY(inc_line)}")

            elif itype == "COPYRIGHT":
                print(f"{printer.F_RED('COPYRIGHT')}   {file_path}")

            elif itype == "BRACE":
                print(f"{printer.F_RED('BRACE')}       {msg} {printer.F_GREY(file_path)}")

            elif itype == "TODO_REM":
                print(f"{printer.F_YELLOW('TODO REM')}   {file_path}")

            elif itype == "TODO_IMP":
                print(f"{printer.F_MAGENTA('TODO IMP')}   {file_path}")
                print(f"    {printer.F_GREY(msg)}")

            elif itype == "TODO":
                print(f"{printer.F_GREEN('TODO')}       {file_path}")
                print(f"    {printer.F_GREY(msg)}")

            elif itype == "LOG_FUNC":
                print(f"{printer.F_CYAN('LOG_FUNC')}   {file_path}")

            elif itype == "RARE_FREQ":
                print(f"{printer.F_CYAN('RARE_FREQ')}  {file_path}")

            elif itype == "UNDEF":
                print(f"{printer.F_CYAN('UNDEF')}      {file_path}")
                print(f"    {printer.F_GREY(msg)}")

            elif itype == "STATIC":
                print(f"{printer.F_MAGENTA('STATIC')}     {file_path} -> {msg}")

    # Print Final Performance Summary Counters
    stats = report["stats"]
    print(f"\nTotal files: {stats['total_files']}")
    for k, v in sorted(stats["extensions"].items()):
        print(f"      {printer.F_GREY(f'+ {k:<4}')} {v}")
    print(f"Total lines: {stats['total_lines']}")
    print()

    print_checksum()
    print()

def help():
    return {
        "USAGE": "fx lint",
        "DESCRIPTION":
            "Enforces strict architectural code compliance, structural validation constraints,\n"
            "and code styling rules against your source tree prior to milestone deployments.\n"
            "Furthermore, it generates a deterministic \x1b[1;32mSHA-256 cryptographic fingerprint\x1b[0m of the\n"
            "Farix source tree. This unique ID maps directly to the current state of the code\n"
            "and legal boundaries, ensuring absolute tamper-verification."
    }
