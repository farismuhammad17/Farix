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

from fxtools.core import statejson
from fxtools.core.build import proc_run

def run():
    data = statejson.get()

    found_name = proc_run("git config --global user.name", check=False)
    found_email = proc_run("git config --global user.email", check=False)

    if not found_name:
        found_name = input("Name > ")

    if not found_email:
        found_email = input("Mail (Optional) > ")
    if not found_email: # User entered nothing
        found_email = None

    data["USER_NAME"] = found_name
    data["USER_EMAIL"] = found_email

    statejson.save(data)
    statejson.flush()

def help():
    return {
        "USAGE": "fx config setup-profile",
        "DESCRIPTION": "Uses git config to acquire name and email (if found) and sets to state JSON file."
    }
