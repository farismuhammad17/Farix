# User LibC

The user applications compile separately from the kernel. The files here are excluded for the total compilation of the kernel. These two classes are compiles alongside every user application defined in the apps folder. They also provide the framework for the `main` function.

The `linker.ld` is the default linker for all these programs. Since it is always the same, there is no reason to rewrite it over and over, so we just reuse a common template defined here.

The `farix_syscall` is the function required to push the required registers and call the proper system call to let the application communicate with the kernel.
