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
import datetime
import tempfile

from fxtools.core import printer
from fxtools.core import statejson
from fxtools.core.build import proc_run

from fxtools.targets.sign import changes as sign_changes
from fxtools.targets.lint import get_checksum

def get_changelog_changes() -> str:
    # Adding HEAD captures changes whether they are staged (git add) or unstaged
    diff_output = proc_run("git diff HEAD -U0 CHANGELOG.md")

    changes = []
    for line in diff_output.splitlines():
        if line.startswith("+") and not line.startswith("+++"):
            content = line[1:].strip()
            if content:
                changes.append(content)

    return "\n".join(dict.fromkeys(changes))

def run(m: str):
    # Find unstaged files
    status = proc_run("git status --porcelain")

    if status.strip():
        # Iterate through the status lines
        # 'M ' = modified and staged, ' M' = modified and NOT staged, '??' = untracked
        for line in status.splitlines():
            # If the second character is not a space, it's modified and NOT staged
            # '??' means untracked
            if line[1] != ' ' or line.startswith("??"):
                printer.error("Unstaged or untracked files detected")
                print(status)
                return

    msg = m

    msg += f"\n\n{get_checksum()}"

    changes = get_changelog_changes()
    if not changes:
        printer.error("No changes found in CHANGELOG.md")
        return
    msg += f"\n\n{changes}"

    data = statejson.get()
    name = data.get("USER_NAME")
    email = data.get("USER_EMAIL")

    msg += "\n\n---\n\n"
    msg += f"Changes Signature:\n{sign_changes.generate_repository_signature()}\n"

    # Human-readable format
    readable_time = datetime.datetime.now(datetime.timezone.utc).strftime('%B %d, %Y, %I:%M %p UTC')
    # ISO 8601 time format (not recommended for humans)
    unreadable_time = datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')

    msg += f"Timestamp: {readable_time} ({unreadable_time})\n"

    msg += f"\n{name}"
    if email:
        msg += f" <{email}>"
    msg += "\n"

    public_key_file_path = os.path.join(data.get("USER_SIGN_FILES"), ".public_key")
    try:
        with open(public_key_file_path) as f:
            msg += f.read()
    except FileNotFoundError:
        printer.error("Public key not found")
        print("+ If you have no made one, use 'fx sign setup'")
        print("+ Ensure that 'USER_SIGN_FILES' in the state JSON is the correct directory to the files")
        print(printer.F_GREY(f"Note: Checked '{public_key_file_path}'"))
        return

    print()
    print(printer.F_GREEN("------- COMMIT MESSAGE START -------"))
    print(msg)
    print(printer.F_GREEN("------- COMMIT MESSAGE END ---------"))
    print()

    if input("Commit (y) ? ") != "y":
        printer.warn("Cancelled")
        return

    with tempfile.NamedTemporaryFile(mode='w', delete=False) as tmp:
        tmp.write(msg)
        tmp_path = tmp.name

    try:
        proc_run(f'git commit -F "{tmp_path}"', capture=False)
        printer.success("Commit created successfully")
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)

def help():
    return {
        "USAGE": "fx commit <-m>",
        "DESCRIPTION": "Formats the commit message clearly and passes over to git commit",
        "ARGS": {
            "m": "Title message of the commit"
        }
    }
