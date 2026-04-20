[Back to journal.md](../journal.md)

# Advanced Host Controller Interface

*20th April, 2026*

My kernel was not testable on my real computer, because it apparently runs on whatever AHCI is (sounds like someone sneezed). ATA worked perfectly, on any emulation, so I needed AHCI to work. Unfortunately, I found out we needed an abstraction layer, so I decided to call this layer - Block Device Layer. Why? I have no clue, Chat GPT suggested it when I asked it what to name it.

It's literally the same as the VFS, but for the hardware. It feels so stupid: literally all the abstraction layers in my kernel (as of now) are in the file management stuff, unless I'm forgetting someone. File systems are dumb, why couldn't it all just be easier? To be honest: I can't get it working, I don't know why, and I am 100% sure its QEMU's fault.

While I was bored, I decided to make these macros:

```c
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define FREQ_FUNC   __attribute__((hot))
#define RARE_FUNC   __attribute__((cold))
```

This was purely for the fun of spamming these everywhere. It felt nice, even though it didn't matter, because the machine code executed that fast already, it felt good to have. I will definitely be spamming these more. In fact, I make the compiler `-include include/kernel.h` just to make this more automated. The compile time will be slightly slower, but since the `kernel.h` file is small, it will mostly pass by quickly. Runtime will not at all be affected, except the code is slightly faster, and even if it's by literal microseconds, I'm happy.

While moving through my own code, I realised there were plenty of... flaws, I suppose, just problems. I left `TODO` markings wherever I felt like it could have improvements, and I also removed a couple duplications.

I'll think of how to fix AHCI later, I'm just going to push this WIP into a branch so that I can keep track of everything.
