[Back to journal.md](../journal.md)

# ELF Shell

*12th April, 2026*

I decided to move from having a kernel shell, I wanted the shell to be seperate, like an executable, that we execute in `kmain` instead. This would make it easier to move the shell around, of course, and make the kernel closer to completion.

I made changes to the makefile to accomodate these future changes, so I don't have to worry over it later. I tested it out, and it seems to function perfectly. One problem - I have no idea if the kernel actually works on a real computer or not.

To test this, I made a feature in the makefile to put the compiled BIN file into a given USB. This USB file will be reformatted to be able to run with GRUB, assuming you set it up on the client device properly. Once this is done, I actually tested - it cutely failed. So, I am currently on a treasure hunt to find out where, and it seems it failed on `sti`... somehow, and I have no idea how. It is probably the timer, scheduler, or something assembly, the stack, whatever. It could be anywhere at this point.
