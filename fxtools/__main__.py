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

    # Split the raw input list into sub-lists on every "+" token
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

    # Process each command sequentially in the same process context
    for cmd_tokens in commands:
        target_name = cmd_tokens[0]
        cli_args = cmd_tokens[1:]

        try:
            target_module = importlib.import_module(f"fxtools.targets.{target_name}")
        except ModuleNotFoundError:
            printer.error(f"Unknown target: '{target_name}'")
            return

        if any(h in cli_args for h in ["help", "-help", "--help", "-h"]):
            if hasattr(target_module, "help"):
                help_manager.print_format_help(target_module.help())
                continue # Move to next chained command instead of a hard return
            else:
                printer.error(f"Could not find help documentation for target '{target_name}'.")
                return

        if not hasattr(target_module, "run"):
            printer.error(f"Target '{target_name}' missing run function")
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
                printer.error(f"Missing required parameter '{param_name}' for target '{target_name}'.")
                return

        # Execute target block
        run_func(**payload)

if __name__ == "__main__":
    main()
