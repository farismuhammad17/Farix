OS := $(shell uname -s)

# Look for cross-compilers
ifeq ($(shell which i686-elf-gcc >/dev/null 2>&1; echo $$?), 0)
PREFIX = i686-elf-
else ifeq ($(shell which i386-elf-gcc >/dev/null 2>&1; echo $$?), 0)
PREFIX = i386-elf-
else
$(error "i686-elf nor i386-elf found")
endif

NASM = nasm

CC = $(PREFIX)gcc
AS = $(PREFIX)as
LINKER = scripts/linker.ld

# -Iinclude lets us do #include "memory/pmm.h" instead of "../include/memory/pmm.h"
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -Iinclude
LDFLAGS = -T $(LINKER) -ffreestanding -O2 -nostdlib -lgcc

CPP_SOURCES := $(shell find src -name '*.cpp')
ASM_SOURCES := $(shell find src -name '*.asm')

CPP_OBJECTS := $(patsubst src/%.cpp, build/%.o, $(CPP_SOURCES))
ASM_OBJECTS := $(patsubst src/%.asm, build/%.o, $(ASM_SOURCES))

BOOT_OBJECT = build/boot.o

all: farix.bin

farix.bin: $(BOOT_OBJECT) $(CPP_OBJECTS) $(ASM_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(BOOT_OBJECT) $(CPP_OBJECTS) $(ASM_OBJECTS)

build/boot.o: src/boot.s
	mkdir -p $(@D)
	$(AS) $< -o $@

build/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS)

build/%.o: src/%.asm
	mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@

clean:
	rm -rf build farix.bin disk.img

disk.img:
	@echo "Creating disk.img for $(OS)"

	qemu-img create -f raw disk.img 64M

ifeq ($(OS), Darwin)
		hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE disk.img
		mv disk.img.dmg disk.img
		rm -f disk.dmg.sparseimage
else
		mkfs.fat -F 32 -n FARIX disk.img
endif

run: farix.bin disk.img
	qemu-system-i386 -kernel farix.bin -drive format=raw,file=disk.img,index=0,media=disk -full-screen

run_nofs: farix.bin disk.img
	qemu-system-i386 -kernel farix.bin -drive format=raw,file=disk.img,index=0,media=disk

-include debug.mk # A seperate makefile for debugging
