# Why I Started Farix

Mainly, it is fun, but part of me is making this out of the want of an OS that solves all my annoyances with modern operating systems. Consider any other major OS currently:

1. **Windows:** Uses up an insane amount of RAM simply to exist, and feels like it treats the user like a child by taking away control behind unreasonably fancy UI abstractions.
2. **MacOS:** Locked behind Apple hardware. Furthermore, it frustratingly drops support for perfectly capable older hardware, turning machines into paperweights (personal experience).
3. **Linux:** Admittedly, we got the name from here, and it's fantastic. However, standard distributions often carry massive overhead, and configuring a minimal footprint requires stripping down a monolithic tree designed to support every piece of hardware ever made. Also, it's just too big, I have no clue how to add anything I would want into it without just writing a standard app.

*Simply put:* I want an OS that doesn't eat half the computer RAM to exist, treats the user as the absolute owner of the machine, doesn't drop support because hardware is old, and remains fast.

# The IDLE RAM Problem 

To solve bloated idle RAM usage, we want to load only what is necessary at any given moment. 

* **Monolithic Kernel:** Everything compiled directly into the kernel image. It is fast, but bloated.
* **Microkernel:** Drivers run as isolated user-space processes. It is memory efficient, but traditionally slow due to heavy IPC context-switching overhead due to the fact it is in user-space.

This is where I got the idea for **Kernel System Modules**. Instead of running modular drivers in slow user-space, just dynamically loads system modules directly into **kernel space** on demand. They sit on the disk as files until needed, saving RAM and storage space, but execute natively without the performance penalty of traditional microkernel IPC.

The idea is to let the bootloader hand off control to the kernel, which then loads itself completely, then kills all the stuff that it had to initialise itself from RAM (i.e. Initboot) by simply erasing them. After this point, the kernel is but a manager, loading and unloading system modules based on requirements.

This also solves backwards compatibility: old hardware drivers can live quietly in the system folder, loaded only if that specific hardware is physically present.

# The User is The Owner

The user has full access to their computer, hardware, and data. The only hard boundary Farix enforces is protecting the kernel from permanent physical bricking. Everything else is permitted. It's your machine; Farix just runs it.

# Transparency & Predictability

A modern computer should not feel like a black box. You should know what code is running, why it's running, and where every byte of your RAM is going. Farix aims to be completely transparent: no hidden telemetry, no forced background updates, and no mysterious daemons acting without your explicit consent. If it's on your machine, you should be able to inspect it, modify it, or delete it.

*Note: This philosophy applies strictly to the core operating system and built-in tooling. Farix provides an unopinionated, clean foundation; what third-party software users choose to run on top of it is entirely up to them.*
