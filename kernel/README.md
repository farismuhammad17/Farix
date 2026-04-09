The source files are divided one architecture independant (in this folder) and dependant code (in arch folder).

# Default Kernel boot

After the bootloader is done, it passes it to the architecture specific boot, which passes us to the `kmain` function, which is a general set of init functions that are the same regardless of the architecture. It also comes with an `early_kmain` function that every architecture's init calls before it loads its stuff, so we can have important debug stuff there.

Once the kernel is done making its stuff, we enter a halt function. This stops the CPU from moving, and the CPU stays there, doing nothing. This is exactly what we need, so when something happens, like a keyboard press, the interrupt handler will move us from the halt instruction to where we need to be to deal with that, and then come back to the while loop when its done, only to halt again, and do nothing. This is much more energy efficient, and doesn't make your CPU a frying pan.
