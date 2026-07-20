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

import sys
import importlib
import inspect
from pathlib import Path

from fxtools.core import printer
from fxtools.core import help_manager

def parse_cli_args(args_list: list[str]) -> dict:
    parsed = {}
    i = 0

    while i < len(args_list):
        arg = args_list[i]

        if arg.startswith("--"):
            key = arg.lstrip("-").replace("-", "_")
            parsed[key] = True
            i += 1

        elif arg.startswith("-"):
            key = arg.lstrip("-").replace("-", "_")

            if i + 1 < len(args_list):
                parsed[key] = args_list[i + 1]
                i += 2
            else:
                printer.error(f"Option '{arg}' requires an associated value.")
                sys.exit(1)

        else:
            # Skip unexpected positional arguments trailing behind flags
            i += 1

    return parsed

def main():
    raw_input = sys.argv[1:]

    if not raw_input:
        import fxtools.targets.make
        fxtools.targets.make.run()
        return

    # Split commands on "+"
    commands = []
    current_command = []

    for arg in raw_input:
        if arg == "+":
            if current_command:
                commands.append(current_command)
                current_command = []
        else:
            current_command.append(arg)
    if current_command:
        commands.append(current_command)

    # targets/ is always relative to fxtools/__main__.py
    targets_base_dir = Path(__file__).parent / "targets"
    if not targets_base_dir.is_dir():
        try:
            targets_mod = importlib.import_module("fxtools.targets")
            mod_file = getattr(targets_mod, "__file__", None)
            targets_base_dir = Path(mod_file).parent if mod_file else targets_base_dir
        except ModuleNotFoundError:
            pass

    for cmd_tokens in commands:
        consumed_tokens = []
        current_disk_path = targets_base_dir
        target_i = 0

        # Resolve tokens to disk layout
        while target_i < len(cmd_tokens):
            token = cmd_tokens[target_i]
            if token.startswith("-"):
                break

            next_dir = current_disk_path / token
            next_file = current_disk_path / f"{token}.py"

            if next_file.is_file():
                consumed_tokens.append(token)
                current_disk_path = next_file
                target_i += 1
                break
            elif next_dir.is_dir():
                consumed_tokens.append(token)
                current_disk_path = next_dir
                target_i += 1
            else:
                break

        if not consumed_tokens:
            printer.error(f"Unknown target route: '{' '.join(cmd_tokens)}'")
            return

        # Handle directory-only target error
        if current_disk_path.is_dir():
            group_name = " ".join(consumed_tokens)
            valid_subs = [
                item.stem if item.is_file() else item.name
                for item in current_disk_path.iterdir()
                if not item.name.startswith("__") and item.name != "init.py"
            ]
            valid_subs.sort()

            printer.error(f"'{group_name}' is a command group, not an executable target.")
            print(f"Available commands under '{group_name}':")
            for sub in valid_subs:
                print(f"  + {printer.F_CYAN(sub)}")
            return

        cli_args = cmd_tokens[target_i:]
        target_name = ".".join(consumed_tokens)
        friendly_name = " ".join(consumed_tokens)

        try:
            target_module = importlib.import_module(f"fxtools.targets.{target_name}")
        except ModuleNotFoundError:
            printer.error(f"Unknown target: '{friendly_name}'")
            return

        if any(h in cli_args for h in ["help", "-help", "--help", "-h"]):
            if hasattr(target_module, "help"):
                help_manager.print_format_help(target_module.help())
                continue
            printer.error(f"Could not find help documentation for target '{friendly_name}'.")
            return

        if not hasattr(target_module, "run"):
            printer.error(f"Target '{friendly_name}' missing run function")
            return

        run_func = target_module.run
        sig = inspect.signature(run_func)
        passed_args = parse_cli_args(cli_args)

        payload = {}
        for param_name, param in sig.parameters.items():
            if param_name in passed_args:
                payload[param_name] = passed_args[param_name]
            elif param.default != inspect.Parameter.empty:
                payload[param_name] = param.default
            else:
                printer.error(f"Missing required parameter '{param_name}' for target '{friendly_name}'.")
                return

        run_func(**payload)

if __name__ == "__main__":
    main()
