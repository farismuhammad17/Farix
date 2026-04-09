A kernel is the bridge between hardware and software, but, generally, hardware are never always the exact same. Different CPUs use different "architectures", and they behave fundamentally differently. A portion of the kernel works the same regardless of the actual arch, but certain stuff will need specific definitions.

Such functions and files are defined in the arch folder, and here we only define the headers for it. The code for these architecture specific implementations can be found [here](../../arch).
