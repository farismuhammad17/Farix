[Back to journal.md](../journal.md)

# Silicon Verification

*12th April, 2026*

I decided to move from having a kernel shell, I wanted the shell to be seperate, like an executable, that we execute in `kmain` instead. This would make it easier to move the shell around, of course, and make the kernel closer to completion.

I made changes to the makefile to accomodate these future changes, so I don't have to worry over it later. I tested it out, and it seems to function perfectly. One problem - I have no idea if the kernel actually works on a real computer or not.

To test this, I made a feature in the makefile to put the compiled BIN file into a given USB. This USB file will be reformatted to be able to run with GRUB, assuming you set it up on the client device properly. Once this is done, I actually tested - it cutely failed. So, I am currently on a treasure hunt to find out where, and it seems it failed on `sti`... somehow, and I have no idea how. It is probably the timer, scheduler, or something assembly, the stack, whatever. It could be anywhere at this point.

*13th April, 2026*

Swapping USB in and out of two computers is starting to get annoying, so I decided I have to up my emulation system. Instead of using QEMU to emulate, the apparent "gold-standard" that is suggested literally anywhere I look, I decided to use bochs. Unfortunately, this required so much new makefile stuff, that I was getting annoyed with it all - it needed me to make a special ISO file and some more extra stuff, and I was not dealing with that.

Instead, I just made QEMU more "realistic", by forcing it to provide everything that a normal computer would also provide. This ensures that the emulation would be a bit more solid. I eventually got it to look like my actual computer; stuck after random points in `kmain`, and slowly solved all these problems.

These "problems" in question are a list of stuff I left in TODO.md and never bothered to fix them because they worked, and I thought the emulation was realistic enough for it. Unfortunately, this is not true, and I had to fix it all today. I can't believe I'm doing all this just for moving the shell into ELF. I think it's working, I'll check tomorrow, it's 11:41 am, and I have school tomorrow.

*14th April, 2026*

On my quest to make QEMU more realistic, I stumbled upon a long dead stick. The ATA driver failed to merit, for it hated what was of wit: old people left "legacy" ATA stuff, and made it more "dynamic", which made it so that I can't just "access" my pretty ATA from anyway - No, that's too simple. Instead, you have to use the PCI to manually scout out your ATA and load it in and then re-adjust the magic numbers.

I also **hated** just how many magic numbers I kept leaving in the code, because I never thought I would have to come back to it. I decided I was stupid, and finally made the ATA code more readable. The whole thing is really simple, but I have no idea why they had to make it like this.

I've also decided to pivot my focus from making my shelf *(shell.elf)*, and instead focus on trying to get the kernel to run on actual computer. Also, since the stuff was failing everywhere, I had to made these `t_print` and `t_printf` just to help me out, and make everything finally work. I still don't think it works on a real computer, but we do feel a step closer.
