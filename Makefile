# -----------------------------------------------------------------------
# Copyright (C) 2026 Faris Muhammad

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------

OS := $(shell uname -s)
ifeq ($(shell which i686-elf-gcc >/dev/null 2>&1; echo $$?), 0)
    PREFIX = i686-elf-
else ifeq ($(shell which i386-elf-gcc >/dev/null 2>&1; echo $$?), 0)
    PREFIX = i386-elf-
else
    $(error "i686-elf nor i386-elf found")
endif

TARGET_DIR = $(shell echo $(PREFIX) | sed 's/-$$//')

# --- NEWLIB PATHS ---
PROJECT_ROOT := $(shell pwd)

NEWLIB_SRC := $(PROJECT_ROOT)/newlib-cygwin
NEWLIB_TARGET := $(shell echo $(PREFIX) | sed 's/-$$//')
LIBC_INSTALL_DIR = $(shell pwd)/libc_build
LIBC_DIR = $(LIBC_INSTALL_DIR)/$(TARGET_DIR)
LIBC_INC = $(LIBC_DIR)/include
LIBC_LIB = $(LIBC_DIR)/lib

# --- TOOLS ---
NASM = nasm
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LINKER = scripts/linker.ld

# --- FLAGS ---
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti \
         -Iinclude -I$(LIBC_INC)
LDFLAGS = -T $(LINKER) -ffreestanding -O2 -nostdlib

# --- OBJECTS ---
CPP_SOURCES := $(shell find src -name '*.cpp')
ASM_SOURCES := $(shell find src -name '*.asm')

CRTI_OBJ = build/architecture/boot/crti.o
CRTN_OBJ = build/architecture/boot/crtn.o
BOOT_OBJECT = build/boot.o

CRTBEGIN := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

CPP_OBJECTS := $(patsubst src/%.cpp, build/%.o, $(CPP_SOURCES))
OTHER_ASM_SOURCES := $(filter-out src/architecture/boot/crti.asm src/architecture/boot/crtn.asm, $(ASM_SOURCES))
OTHER_ASM_OBJECTS := $(patsubst src/%.asm, build/%.o, $(OTHER_ASM_SOURCES))

# --- TARGETS ---

all: farix.bin

farix.bin: $(BOOT_OBJECT) $(CRTI_OBJ) $(CRTBEGIN) $(CPP_OBJECTS) $(OTHER_ASM_OBJECTS) $(CRTEND) $(CRTN_OBJ)
	$(CC) $(LDFLAGS) -o $@ \
		$(BOOT_OBJECT) \
		$(CRTI_OBJ) \
		$(CRTBEGIN) \
		$(CPP_OBJECTS) \
		$(OTHER_ASM_OBJECTS) \
		-L$(LIBC_LIB) -lstdc++ -lc -lm -lgcc \
		$(CRTEND) \
		$(CRTN_OBJ)

build/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS)

build/%.o: src/%.asm
	mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@

build/boot.o: src/boot.s
	mkdir -p $(@D)
	$(AS) $< -o $@

get_deps:
	@if [ ! -d "newlib-cygwin" ]; then \
		echo "Installing newlib..."; \
		git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git; \
	fi

	@echo "Installed dependencies."

libc:
	mkdir -p build/newlib-build
	cd build/newlib-build && \
	CC_FOR_TARGET=$(PREFIX)gcc \
	AS_FOR_TARGET=$(PREFIX)as \
	LD_FOR_TARGET=$(PREFIX)ld \
	RANLIB_FOR_TARGET=$(PREFIX)ranlib \
	$(NEWLIB_SRC)/configure \
		--target=$(NEWLIB_TARGET) \
		--prefix=$(LIBC_INSTALL_DIR) \
		--disable-newlib-supplied-syscalls \
		--with-newlib \
		--enable-languages=c,c++
	$(MAKE) -C build/newlib-build
	$(MAKE) -C build/newlib-build install

clean:
	rm -rf build farix.bin disk.img

run: farix.bin disk.img
	qemu-system-i386 -kernel farix.bin -drive format=raw,file=disk.img,index=0,media=disk -full-screen

run_nofs: farix.bin disk.img
	qemu-system-i386 -kernel farix.bin -drive format=raw,file=disk.img,index=0,media=disk

disk.img:
	qemu-img create -f raw disk.img 64M
ifeq ($(OS), Darwin)
	hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE disk.img
	mv disk.img.dmg disk.img
else
	mkfs.fat -F 32 -n FARIX disk.img
endif

-include debug.mk # A seperate makefile for debugging
