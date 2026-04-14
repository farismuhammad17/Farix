# Setup

Assuming you have cloned the GitHub repository, the program requries some external modules. You can download these easily with the [make.py](../make.py) (assuming you have python on your machine). You may run it directly with python, but for making it nicer to use, you can use an alias:

```bash
source make.env
```

Keep in mind that the make file, by default, assumes x86_32 architecture everytime, and you must specify the architecture somewhere after the whole command to run the command specifically for that architecture.

> [!NOTE]  
> These commands may take some time to finish executing, so be sure to have coffee ready.

```bash
m get_deps
m libc
```

## Running the OS

To run it, use either of the following, and the [make.py](../make.py) handles the rest:

```bash
m qemu        # Full screen
m qemu_       # Runs in a window
```

Alternatively, if you want to compile the OS:

```bash
m [ARCHITECTURE NAME]
```

Create the disk image:

```bash
m disk
```

For more functions:

```bash
m help
```
