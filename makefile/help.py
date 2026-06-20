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

HELP = """
\x1b[1;36mFarix Build System\x1b[0m
\x1b[90mhttps://github.com/farismuhammad17/Farix\x1b[0m

\x1b[90mTIP: Use 'source make.env' to alias make.py as 'm'\x1b[0m
Further information: \x1b[1mm help [target]\x1b[0m

\x1b[1;36mTargets:\x1b[0m
    \x1b[1;32mall\x1b[0m             Build the kernel and deploy apps
    \x1b[1;32miso\x1b[0m             Create a farix.iso file
    \x1b[1;32mdisk\x1b[0m            Generate the bootable disk image for emulation
    \x1b[1;32mget_deps\x1b[0m        Fetch and verify build-tool dependencies
    \x1b[1;32mlibc\x1b[0m            Compile the internal C standard library
    \x1b[1;32mwipe_usb\x1b[0m        Wipe every single file inside the Boot USB
    \x1b[1;32musb\x1b[0m             Copy farix.bin into Boot USB \x1b[90m(provide in make.conf.json)\x1b[0m
    \x1b[1;32mqemu\x1b[0m            Build and launch in QEMU \x1b[90m(Fullscreen mode)\x1b[0m
    \x1b[1;32mqemu_\x1b[0m           Build and launch in QEMU \x1b[90m(Windowed mode)\x1b[0m
    \x1b[1;32mbochs\x1b[0m           Build and launch in BOCHS
    \x1b[1;32mclean\x1b[0m           Remove all build artifacts and object files \x1b[90m(Optionally provide exclusions after; 'apps' to delete only apps)\x1b[0m
    \x1b[1;32mapps\x1b[0m            Compile and add all apps into disk
    \x1b[1;32mdefs\x1b[0m            Interactive program to return definitions and usage of any function
    \x1b[1;32mlint\x1b[0m            Lint the code \x1b[90m(Useful for developing)\x1b[0m
    \x1b[1;32mchecksum\x1b[0m        Finds the hash of the source code \x1b[90m(Ensures source integrity)\x1b[0m
    \x1b[1;32mconfig\x1b[0m          Change configuration settings
    \x1b[1;32mhelp\x1b[0m            Display this menu

\x1b[1;36mArchitectures:\x1b[0m
    \x1b[1mx86_32\x1b[0m          32-bit x86 \x1b[90m(default)\x1b[0m
    \x1b[1marm32\x1b[0m           32-bit ARM \x1b[90m(Requires arm-none-eabi-gcc)\x1b[0m

\x1b[1;36mFlags:\x1b[0m \x1b[90m(Using first letter of flag also works)\x1b[0m
    \x1b[1m-arch\x1b[0m           Architecture to compile to
    \x1b[1m-threads\x1b[0m        Number of threads to compile with
    \x1b[1m-storage\x1b[0m        Options: AHCI, ATA
    \x1b[1m-cores\x1b[0m          Number of CPU cores to launch from
    \x1b[1m--log\x1b[0m           Write all command output text to \x1b[90mbuild.log\x1b[0m

\x1b[1;36mConfiguration:\x1b[0m \x1b[90m(make.conf.json)\x1b[0m
    \x1b[1mBOOT_USB_PATH\x1b[0m   Path to USB to copy kernel to for 'm usb'
    \x1b[1mDEFAULT_ARCH\x1b[0m    Default architecture to compile and assume
    \x1b[1mTHREADS\x1b[0m         Number of threads to use for compiling
"""

ALL_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Sets up the entire toolchain environment, fetches dependencies, and compiles
the full operating system stack into a deployable state.

\x1b[1mMECHANICS:\x1b[0m
Executes an incremental build pipeline, skipping files that have not changed:
  \x1b[90m1.\x1b[0m Fetches core dependencies \x1b[90m(namely \x1b[1;32mmusl\x1b[0m for LibC)\x1b[90m\x1b[0m
  \x1b[90m2.\x1b[0m Compiles the internal C standard libraries \x1b[90m(\x1b[1;32mklib\x1b[0m / LibC)\x1b[90m\x1b[0m
  \x1b[90m3.\x1b[0m Compiles the core Farix kernel subsystems
  \x1b[90m4.\x1b[0m Generates the virtual filesystem master storage block \x1b[90m(\x1b[1;32mdisk.img\x1b[0m)\x1b[90m\x1b[0m
  \x1b[90m5.\x1b[0m Compiles all user-space applications into native ELF format
  \x1b[90m6.\x1b[0m Mounts and deploys the compiled ELF applications into the disk image

\x1b[1mBEHAVIOR:\x1b[0m
* Safe and incremental; checks timestamps to avoid redundant compilation.
* Prepares the complete system payload required for emulator boot targets.
"""

ISO_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Packages the compiled kernel into a bootable El Torito ISO image format
(\x1b[1;32mfarix.iso\x1b[0m) suitable for optical disc emulation or burning to live media.

\x1b[1mMECHANICS:\x1b[0m
  \x1b[90m1.\x1b[0m Destroys any pre-existing \x1b[1;31mfarix.iso\x1b[0m file to prevent state pollution.
  \x1b[90m2.\x1b[0m Asserts the presence of \x1b[1;32mfarix.bin\x1b[0m; aborts if missing.
  \x1b[90m3.\x1b[0m Spawns a staging environment directory tree at \x1b[1;34mbuild/iso_root/boot/grub/\x1b[0m.
  \x1b[90m4.\x1b[0m Stages the kernel binary and dynamically writes an inline \x1b[1;32mgrub.cfg\x1b[0m.
  \x1b[90m5.\x1b[0m Invokes \x1b[1;33mgrub-mkrescue\x1b[0m to master the final bootable ISO container.

\x1b[1mBEHAVIOR:\x1b[0m
Generates a 5-second countdown multiboot GRUB screen using the following profile:

  \x1b[33mset\x1b[0m timeout=\x1b[32m5\x1b[0m
  \x1b[33mset\x1b[0m default=\x1b[32m0\x1b[0m
  \x1b[33mmenuentry\x1b[0m 'Farix OS' {
      \x1b[33mmultiboot\x1b[0m /boot/farix.bin
      boot
  }
"""

DISK_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Initializes an empty, formatted 64-megabyte raw storage container image
(\x1b[1;32mdisk.img\x1b[0m) configured with a FAT32 filesystem layout for kernel emulation.

\x1b[1mMECHANICS:\x1b[0m
Allocates the storage envelope blocks based on the host operating system platform:
  \x1b[1;34mOn macOS (Darwin):\x1b[0m
    \x1b[90m1.\x1b[0m Spawns a raw 64M backing container block via \x1b[1;33mqemu-img\x1b[0m.
    \x1b[90m2.\x1b[0m Formats a temporary DMG layout with \x1b[1;33mhdiutil\x1b[0m using standard FAT32.
    \x1b[90m3.\x1b[0m Flattens and renames the resulting system image bundle back to raw.
  \x1b[1;34mOn Linux/Other platforms:\x1b[0m
    \x1b[90m1.\x1b[0m Spawns a raw 64M backing container block via \x1b[1;33mqemu-img\x1b[0m.
    \x1b[90m2.\x1b[0m Formats the raw target sector grid directly via native \x1b[1;33mmkfs.fat\x1b[0m.

\x1b[1mBEHAVIOR:\x1b[0m
* Labels the filesystem cluster namespace partition volume as \x1b[1;32mFARIX\x1b[0m.
* Uses Mtools (\x1b[1;33mmmd\x1b[0m) to safely inject the vital internal \x1b[1;34m::/system\x1b[0m root tree
  directory directly into the raw loop image layout without requiring host sudo privileges.
"""

GET_DEPS_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Queries the local host development layout and securely pulls necessary upstream
subsystem dependencies required to build the Farix software environment stack.

\x1b[1mMECHANICS:\x1b[0m
  \x1b[90m1.\x1b[0m Scans for an active initialization path block matching \x1b[1;34mmusl/\x1b[0m.
  \x1b[90m2.\x1b[0m If missing, executes a high-speed \x1b[1;33mgit clone --depth 1\x1b[0m payload stream
     targeting the reference mirror repository to fetch the C standard library core.
  \x1b[90m3.\x1b[0m Aborts network calls gracefully if the repository framework already exists.

\x1b[1mBEHAVIOR:\x1b[0m
* Safe and non-destructive; it will never overwrite or re-download an existing
  \x1b[1;32mmusl\x1b[0m directory tree profile, keeping local workspace shifts intact.
"""

LIBC_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Configures and cross-compiles the \x1b[1;32mMusl LibC\x1b[0m source tree into a static
freestanding C standard library tailored specifically for the Farix kernel architecture.

\x1b[1mMECHANICS:\x1b[0m
  \x1b[90m1.\x1b[0m Allocates an isolated out-of-tree workspace block at \x1b[1;34mbuild/musl-x86_32-build/\x1b[0m.
  \x1b[90m2.\x1b[0m Invokes the Musl downstream configure script with targeted constraints:
     * \x1b[1m--target=i686-linux-gnu\x1b[0m : Generates 32-bit x86 execution binaries.
     * \x1b[1m--disable-shared\x1b[0m       : Forces pure static linking boundaries.
     * Passes your custom cross-toolchain components (\x1b[1;33mCC\x1b[0m and \x1b[1;33mCROSS_COMPILE\x1b[0m definitions).
  \x1b[90m3.\x1b[0m Fires parallel build loops scaled to matching CPU execution nodes via \x1b[1;33m-j$(nproc)\x1b[0m.
  \x1b[90m4.\x1b[0m Stages and installs headers and library objects into your localized installation directory.

\x1b[1mBEHAVIOR:\x1b[0m
* Highly persistent; once compiled, this target does not need to be rebuilt
  unless structural modifications are introduced to kernel system calls.
"""

WIPE_USB_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Forces a recursive filesystem deletion sweep across the targeted physical boot
media mount point currently defined in your configuration.

\x1b[1mMECHANICS:\x1b[0m
* Reads the custom path variable explicitly mapping to \x1b[1;34mBOOT_USB_PATH\x1b[0m.
* Invokes a high-level recursive deletion tool tree sweep (\x1b[1;33mshutil.rmtree\x1b[0m).
* Completely bypasses safely hidden system volume tree index clusters.

\x1b[1mBEHAVIOR:\x1b[0m
* \x1b[1;31mCRITICAL WARNING:\x1b[0m Destroys all visible data payloads inside the target path.
* Ensure your configuration properties point directly to a dedicated testing
  USB stick before running this utility to prevent catastrophic data loss.
"""

USB_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Executes hardware block erasure, re-initializes a low-level FAT32 system track,
deploys the kernel payload (\x1b[1;32mfarix.bin\x1b[0m), and configures direct live boot hooks.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mDocker Containment Check:\x1b[0m Evaluates workspace boundaries; explicitly aborts
  inside isolated container trees due to physical hardware access limitations.
* \x1b[1mPre-flight Inspection:\x1b[0m Scans for un-indexed file objects; forces a manual interactive
  confirmation prompt loop if active data blocks are discovered on the drive.
* \x1b[1mCross-Platform Low-Level Formatting Engine:\x1b[0m
  * \x1b[1;34mOn macOS (Darwin):\x1b[0m Parses \x1b[1;33mdf\x1b[0m logs to locate raw block mount devices, issues an
    destructive MBR \x1b[1;33mdiskutil eraseDisk\x1b[0m routine, and spins until re-mounted.
  * \x1b[1;34mOn Windows:\x1b[0m Targets active drive volume lettering and forces a quick flag raw format
    (\x1b[1;33mformat /FS:FAT32 /V:FARIX /Q /Y\x1b[0m). Requires Administrator terminal contexts.
  * \x1b[1;34mOn Linux:\x1b[0m Locates device paths via \x1b[1;33mfindmnt\x1b[0m, unmounts the volume node grid safely,
    and directly updates sector headers via a native \x1b[1;33mmkfs.vfat\x1b[0m call.

\x1b[1mBEHAVIOR:\x1b[0m
* Deploys \x1b[1;32mfarix.bin\x1b[0m directly into the drive root directory.
* Syncs filesystem caches and injects an instant-boot \x1b[1;32mgrub.cfg\x1b[0m layout script:
    \x1b[33mset\x1b[0m timeout=\x1b[32m0\x1b[0m
    \x1b[33mset\x1b[0m default=\x1b[32m0\x1b[0m
    \x1b[33mmenuentry\x1b[0m 'Farix OS' {
        \x1b[33mmultiboot\x1b[0m /farix.bin
        boot
    }
* Automatically triggers a safe storage media eject execution signal via host sub-calls.
"""

QEMU_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Launches the compiled Farix kernel image directly inside the high-performance
Quick Emulator (QEMU) virtual machine environment.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mNested Emulation Check:\x1b[0m Queries active container namespaces; immediately aborts
  execution inside Docker due to missing physical hardware pass-through contexts.
* \x1b[1mExecution Routing:\x1b[0m Spawns the system engine process via your configured binaries:
  * \x1b[1;32mm qemu\x1b[0m  : Appends the \x1b[1m-full-screen\x1b[0m flag to secure direct graphical focus.
  * \x1b[1;32mm qemu_\x1b[0m : Runs a standard decoupled desktop display wrapper environment window.

\x1b[1mBEHAVIOR:\x1b[0m
* Feeds your built \x1b[1;32mfarix.bin\x1b[0m directly into QEMU via the standalone \x1b[1m-kernel\x1b[0m parameter.
* Merges and resolves all lower-level core layout properties, machine emulation matrix
  definitions, and configuration parameters passed from your central build parameters.
"""

BOCHS_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Initializes and spins up the deterministic, cycle-accurate Bochs x86 PC emulation
environment to debug low-level hardware interactions and kernel state changes.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mBios Path Discovery:\x1b[0m Automatically tests system layout trees across prominent
  host environments (Homebrew, MacPorts, native Linux paths) to find official core payloads.
* \x1b[1mConfiguration Generation:\x1b[0m Dynamically drops an optimized \x1b[1;34mbochsrc.txt\x1b[0m containing:
  * 128MB RAM configuration baseline boundaries (\x1b[1mmegs: 128\x1b[0m).
  * Platform-adjusted display rendering drivers (\x1b[1;32mx11\x1b[0m on Linux, \x1b[1;32msdl2\x1b[0m on macOS systems).
  * Pinpointed Prescott Celeron hardware structures (\x1b[1mcpu: model=p4_prescott_celeron_336\x1b[0m).
* \x1b[1mMedia Insertion:\x1b[0m Attaches your generated \x1b[1;32mfarix.iso\x1b[0m directly onto the virtual master
  bus interface (\x1b[1mata0-master\x1b[0m) and overrides execution priorities to boot from CD-ROM.

\x1b[1mBEHAVIOR:\x1b[0m
* Validates configuration presence before initializing the emulator engine loop.
* Drops clear system logs and intercepts low-level hardware panics or exceptions directly.
"""

CLEAN_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Sanitizes the active workspace directory by systematically purging compiled binaries,
intermediate object structures, and generated storage block images.

\x1b[1mMECHANICS:\x1b[0m
Evaluates runtime trailing arguments to isolate or scale the deletion footprint:
* \x1b[1mStandard Mode (No targeted arguments):\x1b[0m
  Populates a target list mapping to global scratch patterns:
  - \x1b[1;34mbuild/\x1b[0m    : Deletes the intermediate compiler artifact directory tree.
  - \x1b[1;32mfarix.bin\x1b[0m : Deletes the compiled kernel distribution payload block.
  - \x1b[1;32mfarix.iso\x1b[0m : Deletes the mastered CD-ROM boot image container.
* \x1b[1mSelective App Exclusion Mode (\x1b[1;32mm clean apps\x1b[0m):\x1b[0m
  Overrides default sweep scopes to explicitly isolate and flush ONLY the compiled
  user-space applications directory tree (\x1b[1;34mbuild/user-build/\x1b[0m).
* \x1b[1mDynamic Exclusion Filtering:\x1b[0m
  Allows passing explicit junk targets as arguments to dynamically drop them from
  the deletion array (preserving them locally).

\x1b[1mBEHAVIOR:\x1b[0m
* Safely inspects path metadata types to dynamically deploy directory trees purges
  (\x1b[1;33mshutil.rmtree\x1b[0m) or simple flat file unlinking (\x1b[1;33mos.remove\x1b[0m).
* Ideal for cross-testing application adjustments without dropping your pre-compiled
  core kernel modules or C standard library foundations.
"""

APPS_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Discovers, multi-threads, and cross-compiles all user-space program trees located
in the root applications folder into statically linked executable ELF binaries.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mDependency Assembly:\x1b[0m Automatically queues structural runtime libraries
  (\x1b[1;32mklib\x1b[0m, architecture stubs, assembly entry shims).
* \x1b[1mParallel Source Parsing:\x1b[0m Traverses individual subdirectory trees inside \x1b[1;34mapps/\x1b[0m:
  * Targets \x1b[1;32m*.c\x1b[0m files for freestanding compilation via cross-GCC.
  * Targets \x1b[1;32m*.s\x1b[0m / \x1b[1;32m*.asm\x1b[0m files for evaluation via NASM (\x1b[1;33melf32\x1b[0m target layout).
  * Extracts dedicated local linker templates (\x1b[1;34mlinker.ld\x1b[0m) if packaged alongside source.
* \x1b[1mThreaded Building Execution:\x1b[0m Spawns a parallel optimization batch processing cluster
  throttled precisely to your configured machine thread parameters (\x1b[1;33mThreadPoolExecutor\x1b[0m).
* \x1b[1mDeterministic Binary Linking:\x1b[0m Generates the final \x1b[1;32m*.elf\x1b[0m binary files. Forces your
  compiled assembly entry object (\x1b[1;32m_start\x1b[0m) right to the front block to ensure strict layout order.

\x1b[1mBEHAVIOR:\x1b[0m
* Falls back to your central library linker script if an application lacks a custom blueprint.
* Uses Mtools (\x1b[1;33mmcopy\x1b[0m) to safely inject the finished ELF executables directly into the active
  \x1b[1;34m::/system/\x1b[0m folder layout inside \x1b[1;32mdisk.img\x1b[0m, overwriting stale code fragments automatically.
"""

DEFS_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Launches an interactive kernel workspace indexer and lookup terminal interface
to map out declarations, source definitions, usages, and internal documentation blocks.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mTwo-Pass Deep Codebase Scan:\x1b[0m
  * \x1b[1mPass 0 (Discovery):\x1b[0m Traverses the root directory tree for \x1b[1;32m*.c\x1b[0m, \x1b[1;32m*.h\x1b[0m, \x1b[1;32m*.s\x1b[0m, and \x1b[1;32m*.asm\x1b[0m
    files, matching signatures via regex to map functions, definitions, and variables.
  * \x1b[1mPass 1 (Cross-Reference):\x1b[0m Evaluates word boundary patterns against your compiled dictionary
    to trace global usage vectors across independent translation units.
* \x1b[1mRetroactive Docstring Extraction:\x1b[0m Walks backward from a symbol's declaration line, stripping
  formatting tokens to reconstruct matching multi-line C-style \x1b[1;32m/* ... */\x1b[0m documentation blocks.
* \x1b[1mInteractive Shell Loop Engine:\x1b[0m Drops users into a persistent query terminal (\x1b[1;34m>>>\x1b[0m):
  * Supports wildcard expansions via asterisks (\x1b[1m*\x1b[0m) to group related layout modules.
  * Deploys string similarity checks (\x1b[1;33mdifflib.get_close_matches\x1b[0m) to yield type-safe syntax suggestions.

\x1b[1mBEHAVIOR:\x1b[0m
* Prints clean header/source paths with dedicated trace identifiers (\x1b[1;33m[Header]\x1b[0m / \x1b[1;36m[Body]\x1b[0m).
* Flags internal usages within the defining source file versus active external references.
"""

LINT_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Enforces strict architectural code compliance, structural validation constraints,
and code styling rules against your source tree prior to milestone deployments.

\x1b[1mMECHANICS:\x1b[0m
Performs targeted validation sweeps, ignoring external dependency blocks (\x1b[1;34mmusl/\x1b[0m):
* \x1b[1mInclude Block Sorting:\x1b[0m Asserts that grouped preprocessor include lines follow alphabetical order.
* \x1b[1mRedundant Struct Tag Checks:\x1b[0m Detects legacy struct naming conventions. Warns if a typedef tag
  lacks an inner self-reference, forcing clean anonymous struct definitions.
* \x1b[1mScope and Linkage Verification:\x1b[0m Cross-references non-static functions against global usage layouts.
  Flags loose declarations that can be converted down to private translation units via the \x1b[1mstatic\x1b[0m keyword.
* \x1b[1mBrace Layout Analysis:\x1b[0m Enforces a strict Egyptian brace layout, verifying that functions place
  their opening token (\x1b[1;32m{\x1b[0m) directly on the definition header line.

\x1b[1mBEHAVIOR:\x1b[0m
Displays color-coded alert flags directly in the terminal interface layout:
* \x1b[91mCOPYRIGHT\x1b[0m  : Triggered if standard GNU GPL identification frames are omitted from file headers.
* \x1b[93mDEBUG MODE\x1b[0m : Warns if production guards are disabled (\x1b[1m__DEBUG__\x1b[0m is set to 1 inside \x1b[1;34mkernel.h\x1b[0m).
* \x1b[96mRARE_FREQ\x1b[0m  : Catches optimization attributes like \x1b[1mRARE_FUNC\x1b[0m leaked inside raw source files rather than headers.
* \x1b[96mUNDEF\x1b[0m      : Verifies function-scoped macros are properly neutralized using matching trailing \x1b[1m#undef\x1b[0m statements.
* \x1b[96mLOG_FUNC\x1b[0m   : Prevents shipping diagnostic hooks (\x1b[1mLOG_CALL\x1b[0m / \x1b[1mLOG_NUM\x1b[0m) within live source commits.
* \x1b[93mREADME\x1b[0m     : Scans git-ignored paths to print visibility notifications for uncompleted draft logs.
* \x1b[92mTODO Framework\x1b[0m : Maps standard items (\x1b[1;32mTODO\x1b[0m), removal targets (\x1b[1;33mTODO REM\x1b[0m), and core priorities (\x1b[1;35mTODO IMP\x1b[0m).
"""

CONFIG_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Launches a real-time, interactive configuration portal to modify local workspace
parameters and compiler target properties dynamically.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[1mTerminal Window Looping:\x1b[0m Spawns an interactive shell interface that actively monitors
  and parses mutations applied to your local configuration backend (\x1b[1;34mmake.conf.json\x1b[0m).
* \x1b[1mDisplay Redrawing:\x1b[0m Clears terminal histories dynamically (\x1b[1;33m\\033[H\\033[J\x1b[0m) on each step
  to present clean, enumerated parameter index maps.
* \x1b[1mFlexible Selection Matching:\x1b[0m Accepts explicit property string tokens or zero-indexed numeric
  choices interchangeably to pinpoint individual parameters.

\x1b[1mBEHAVIOR:\x1b[0m
* Safely flushes non-empty parameter changes instantly back to the raw JSON file layout.
* Provides an alternative to manual text-editor modifications, preventing raw format breakage.
"""

CHECKSUM_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Generates a deterministic \x1b[1;32mSHA-256 cryptographic fingerprint\x1b[0m of the Farix source tree.
This unique ID maps directly to the current state of the code and legal boundaries,
ensuring absolute tamper-verification.

\x1b[1mMECHANICS:\x1b[0m
* \x1b[90m1.\x1b[0m Gathers all tracked core source files (\x1b[1;32m*.c\x1b[0m, \x1b[1;32m*.h\x1b[0m, \x1b[1;32m*.asm\x1b[0m, \x1b[1;32m*.s\x1b[0m, \x1b[1;32m*.ld\x1b[0m, \x1b[1;32m*.py\x1b[0m, \x1b[1;32m*.cmd\x1b[0m, \x1b[1;32m*.bat\x1b[0m, \x1b[1;32m*.env\x1b[0m).
* \x1b[90m2.\x1b[0m Includes foundational repository frames (\x1b[1;34mLICENSE\x1b[0m, \x1b[1;34mDOCUMENTATION.md\x1b[0m, \x1b[1;34mCONTRIBUTING.md\x1b[0m, \x1b[1;34mDockerfile\x1b[0m).
* \x1b[90m3.\x1b[0m Automatically parses and respects your local \x1b[1;34m.gitignore\x1b[0m exclusion trees.
* \x1b[90m4.\x1b[0m Sorts paths alphabetically and streams contents sequentially into the \x1b[1;33mSHA-256\x1b[0m pipeline.

\x1b[1mBEHAVIOR:\x1b[0m
* Implements the \x1b[1;33mAvalanche Effect\x1b[0m: even a single character or whitespace mutation anywhere
  in the tracked source grid will completely rewrite the resulting signature block output.
* Explicitly disregards \x1b[1;34mCHANGELOG.md\x1b[0m file modifications to completely eliminate nested
  infinite validation loops.

\x1b[1mREQUIREMENT:\x1b[0m
* When finalizing a development milestone or submitting a pull request, you \x1b[1;31mMUST\x1b[0m execute this
  utility and append the resulting hash to the bottom of your \x1b[1;34mCHANGELOG.md\x1b[0m entry to maintain
  the official historical ledger integrity.
"""

HELP_HELP = """
\x1b[1mPURPOSE:\x1b[0m
Provides an interactive help guide and structural documentation breakdowns for all
available targets and sub-modules inside the Farix build automation engine.

\x1b[1mMECHANICS:\x1b[0m
* Parses trailing execution string tokens to isolate specific operational targets.
* Evaluates matching help profile mappings to dump detailed module mechanics.
* Automatically falls back to printing the generalized, top-level manual framework
  if no target argument is actively passed to the utility parameter loop.

\x1b[1mBEHAVIOR:\x1b[0m
* \x1b[1mSyntax:\x1b[0m \x1b[1;32mm help <command>\x1b[0m
* Maps explicit contextual metadata outlines for the following target indices:
  * \x1b[1;33mall\x1b[0m
  * \x1b[1;33miso\x1b[0m
  * \x1b[1;33mdisk\x1b[0m
  * \x1b[1;33mget_deps\x1b[0m
  * \x1b[1;33mlibc\x1b[0m
  * \x1b[1;33musb\x1b[0m
  * \x1b[1;33mqemu / qemu_\x1b[0m
  * \x1b[1;33mbochs\x1b[0m
  * \x1b[1;33mapps\x1b[0m
  * \x1b[1;33mclean\x1b[0m
  * \x1b[1;33mdefs\x1b[0m
  * \x1b[1;33mlint\x1b[0m
  * \x1b[1;33mconfig\x1b[0m
  * \x1b[1;33mhelp\x1b[0m
"""

def parse_help(args: list[str]):
    cmd = " ".join(args)

    if cmd == "":
        print(HELP)
    elif cmd == "all":
        print(ALL_HELP)
    elif cmd == "iso":
        print(ISO_HELP)
    elif cmd == "disk":
        print(DISK_HELP)
    elif cmd == "get_deps":
        print(GET_DEPS_HELP)
    elif cmd == "libc":
        print(LIBC_HELP)
    elif cmd == "wipe_usb":
        print(WIPE_USB_HELP)
    elif cmd == "usb":
        print(USB_HELP)
    elif cmd in ("qemu", "qemu_"):
        print(QEMU_HELP)
    elif cmd == "bochs":
        print(BOCHS_HELP)
    elif cmd == "apps":
        print(APPS_HELP)
    elif cmd == "clean":
        print(CLEAN_HELP)
    elif cmd == "defs":
        print(DEFS_HELP)
    elif cmd == "lint":
        print(LINT_HELP)
    elif cmd == "config":
        print(CONFIG_HELP)
    elif cmd == "checksum":
        print(CHECKSUM_HELP)
    elif cmd == "help":
        print(HELP_HELP)
