import os
import shutil

JUNK = (
    "build",
    "farix.bin",
    "bochsout.txt",
    "bochsrc.txt",
    "copy.txt",
    "snapshot.txt"
)

def clean(args):
    todelete = list(JUNK)
    for nonjunk in args:
        if nonjunk in todelete:
            todelete.remove(nonjunk)
    for path in todelete:
        if os.path.isdir(path): shutil.rmtree(path)
        elif os.path.exists(path): os.remove(path)
