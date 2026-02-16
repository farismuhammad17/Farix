CC = i686-elf-gcc
AS = i686-elf-as
LINKER = scripts/linker.ld

CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
LDFLAGS = -ffreestanding -O2 -nostdlib -lgcc

OBJ = build/boot.o build/kernel.o

all: farix.bin

farix.bin: $(OBJ)
	$(CC) -T $(LINKER) -o $@ $(LDFLAGS) $(OBJ)

build/boot.o: src/boot.s
	mkdir -p build
	$(AS) $< -o $@

build/kernel.o: src/kernel.cpp
	mkdir -p build
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf build farix.bin

run: farix.bin
	qemu-system-i386 -kernel farix.bin -full-screen
