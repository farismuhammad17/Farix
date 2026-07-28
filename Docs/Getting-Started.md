# Getting Started

Welcome to the Farix development environment! To get started, we must know how to use FXTools. Learn more about FXTools [here](./FXTools).

## Setting up the project

> [!NOTE]
> We assume a Linux, preferably Ubuntu, environment. If you are on any other, it is perfectly fine, and most of the steps should apply, with little difference.

If you are on Ubuntu or any similar Linux distro, you should be good to go. On Windows, WSL generally lets you have near-native Linux support. If either of these are not possible, use the [Dockerfile](../Dockerfile) instead. Use [dock.cmd](../dock.cmd) to automatically set it up. If this is still not possible, try to install everything you require by using the Dockerfile recipe.

Once you have that done, you'd have to run:

```bash
source fx.env
```

Which would assign the shorthand `fx` to the FXTools' main file. This avoids the headache of typing the entire file name everytime. Note that you have to run this everytime you open the project folder, i.e. if you close the terminal and reopen it, you **must** run this again. You would also need Python 3.0+ installed to be able to run it, and FXTools itself does not have any dependencies.

FXTools comes with a pre-built function to setup the entire project repository automatically:

```bash
fx init
```

This fetches all required kernel dependencies and compiles them accordingly.

## Compiling the Kernel

Once you're done, you can compile the kernel into the desired architecture using

```bash
fx make
# Or, simply:-
fx
```

## Emulating

```bash
fx qemu
```

Runs the `farix.iso` file in the QEMU emulator.

## Internal documentation

If you get stuck using FXTools, you can always end a command with `-help` to get a detailed description of the entire command, for example:-

```bash
fx defs -help
```

For a list of functions:

```bash
fx help
```
