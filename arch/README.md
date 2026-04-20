Each architecture implementation has a set of requirements, such as a boot.s, linker.ld, and a stubs.c. These files are responsible for managing the boot sequence as well as establish certain required assembly functions as C wrappers that the rest of the kernel can use.

Different architectures also have their own way of "initialising" themselves, so we let the `kmain` function be global, so that these architecture files can have some init.c file to load the entire arch specific stuff in. These init files MUST call `early_kmain` at the start, and `kmain` at the end, and may do whatever in the middle.

Due to the structure of the make file, we only compile by the specific architecture we're compiling the kernel for. You may thus design this as however you want, so long as you implement all the requirements.
