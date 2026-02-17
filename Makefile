# Look for cross-compilers
ifeq ($(shell which i686-elf-gcc >/dev/null 2>&1; echo $$?), 0)
    PREFIX = i686-elf-
else ifeq ($(shell which i386-elf-gcc >/dev/null 2>&1; echo $$?), 0)
    PREFIX = i386-elf-
else
    $(error "i686-elf nor i386-elf found")
endif

CC = $(PREFIX)gcc
AS = $(PREFIX)as
LINKER = scripts/linker.ld

CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -Iinclude
LDFLAGS = -T $(LINKER) -ffreestanding -O2 -nostdlib -lgcc

CPP_SOURCES = $(wildcard src/*.cpp)
CPP_OBJECTS = $(patsubst src/%.cpp, build/%.o, $(CPP_SOURCES))

BOOT_OBJECT = build/boot.o

all: farix.bin

farix.bin: $(BOOT_OBJECT) $(CPP_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(BOOT_OBJECT) $(CPP_OBJECTS)

build/boot.o: src/boot.s
	mkdir -p build
	$(AS) $< -o $@

build/%.o: src/%.cpp
	mkdir -p build
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf build farix.bin

run: farix.bin
	qemu-system-i386 -kernel farix.bin

run_fullscreen: farix.bin
	qemu-system-i386 -kernel farix.bin -full-screen
