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

import os
import shutil
import subprocess
import time
import re

import makefile.globals

def clean_boot_usb():
    shutil.rmtree(makefile.globals.BOOT_USB_PATH, ignore_errors=True)

def deploy_usb():
    if not os.path.exists(makefile.globals.BOOT_USB_PATH):
        print(f"\x1b[31m(make.conf.json) Boot USB path '{makefile.globals.BOOT_USB_PATH}' not found.\x1b[0m")
        return

    usb_files = os.listdir(makefile.globals.BOOT_USB_PATH)
    visible_files = [f for f in usb_files if not f.startswith('.') and f != "System Volume Information"]

    if visible_files:
        print("\x1b[33mWARNING: There are files inside the Boot USB\x1b[0m")
        if input("Wipe everything? (y) > ") == 'y':
            clean_boot_usb()
        else:
            return

    # Ensure Binary Exists
    if not os.path.exists("farix.bin"): farix_bin()

    dest_path = os.path.join(makefile.globals.BOOT_USB_PATH, "farix.bin")

    if makefile.globals.OS == "Darwin":  # macOS
        print("Formatting USB for MacOS")

        # We need the device identifier (e.g., /dev/disk4) not the mount point
        # This command finds the device associated with your mount path
        df_out = subprocess.check_output(['df', makefile.globals.BOOT_USB_PATH]).decode()
        device = df_out.splitlines()[1].split()[0] # e.g., /dev/disk4s1
        raw_device = re.sub(r's[0-9]+$', '', device)

        # Format: Name=FARIX, Table=MBR, Format=FAT32
        subprocess.run(['diskutil', 'eraseDisk', 'FAT32', 'FARIX', 'MBR', raw_device], check=True)

        print("Waiting for USB to remount...", end="", flush=True)
        timeout = 20  # Max seconds to wait
        start_time = time.time()

        while not os.path.exists(makefile.globals.BOOT_USB_PATH):
            if time.time() - start_time > timeout:
                print(f"\n\x1b[31mError: Timeout ({timeout}s) waiting for USB to remount.\x1b[0m")
                return
            print(".", end="", flush=True)
            time.sleep(0.1)
        print(" Done.")

        eject_cmd = ['diskutil', 'eject', raw_device]

    elif makefile.globals.OS == "Windows":
        print("Formatting USB for Window")

        # Windows requires a diskpart script or a specific format call
        # Note: This usually requires the script to be run as Administrator
        drive_letter = makefile.globals.BOOT_USB_PATH.split(':')[0]
        subprocess.run(['format', f'{drive_letter}:', '/FS:FAT32', '/V:FARIX', '/Q', '/Y'], check=True)

        eject_cmd = ['powershell', '-Command', f"(New-Object -ComObject Shell.Application).Namespace(17).ParseName('{drive_letter}:').InvokeVerb('Eject')"]

    elif makefile.globals.OS == "Linux":
        print("Formatting USB for Linux")

        # Find device from mount point
        findmnt = subprocess.check_output(['findmnt', '-n', '-o', 'SOURCE', makefile.globals.BOOT_USB_PATH]).decode().strip()

        # Unmount first
        subprocess.run(['umount', findmnt], check=True)

        # Format
        subprocess.run(['mkfs.vfat', '-F', '32', '-n', 'FARIX', findmnt], check=True)

        eject_cmd = ['eject', findmnt]

    else:
        print(f"\x1b[31mUnable to format: unknown OS: {makefile.globals.OS}\x1b[0m")
        return

    try:
        shutil.copy("farix.bin", dest_path)
        if hasattr(os, 'sync'):
            os.sync()

    except PermissionError:
        print("\x1b[31mError: Permission denied. Is the USB open in another app?\x1b[0m")
        if os.path.exists(dest_path):
            os.remove(dest_path)

    except Exception as e:
        print(f"\x1b[31mFailed to copy: {e}\x1b[0m")
        if os.path.exists(dest_path):
            os.remove(dest_path)

    grub_dir = os.path.join(makefile.globals.BOOT_USB_PATH, "boot", "grub")
    os.makedirs(grub_dir, exist_ok=True)

    with open(os.path.join(grub_dir, "grub.cfg"), "w") as f:
        f.write(makefile.globals.GRUB_CFG)

    subprocess.run(eject_cmd, check=True)

    print(f"\x1b[32mFarix deployed to {dest_path}\x1b[0m")
