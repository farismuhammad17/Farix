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
import json

from fxtools.core import printer

STATE_JSON_FILE = "fx.state.json"

# Default configuration template
DEFAULT_SCHEME = {
    "DEFAULT_ARCH": "x86_64",
    "THREADS": 4,
    "BOOT_USB_PATH": None,
    "RUNTIME_CORES": 4,
    "QEMU_FULLSCREEN": True
}

# Use a cache to make the state loading efficient
_state_cache = None

def get() -> dict:
    """Returns the current state, loading it from disk if necessary."""

    global _state_cache

    if _state_cache is not None:
        return _state_cache
    else:
        return load()

def load() -> dict:
    """Force disk IO operations to load the state from disk, and also cache it."""

    global _state_cache

    # If the file doesn't exist, create it with defaults
    if not os.path.exists(STATE_JSON_FILE):
        _state_cache = DEFAULT_SCHEME.copy()
        flush()
        return _state_cache

    try:
        with open(STATE_JSON_FILE, "r") as f:
            _state_cache = json.load(f)
    except (json.JSONDecodeError, IOError):
        printer.error(f"State file '{STATE_JSON_FILE}' is corrupted. Falling back to defaults.")
        _state_cache = DEFAULT_SCHEME.copy()

    return _state_cache

def save(data: dict):
    """Sets the cache data to the provided dictionary."""

    global _state_cache
    _state_cache = data

def flush():
    """Flushes the current in-memory state back down to the disk."""

    global _state_cache
    if _state_cache is None:
        return

    try:
        with open(STATE_JSON_FILE, "w") as f:
            json.dump(_state_cache, f, indent=4)
    except IOError:
        printer.error(f"Failed to write state out to '{STATE_JSON_FILE}'.")
