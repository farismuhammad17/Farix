import os

import makefile.globals

def get_deps():
    if not os.path.exists("newlib-cygwin"):
        print("Installing newlib...")
        makefile.globals.run("git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git")

    print("\x1b[33mInstalled dependencies.\x1b[0m")
