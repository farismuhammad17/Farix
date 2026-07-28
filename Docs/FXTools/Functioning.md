# Functioning

FXTools is a successor to a bloated Python tool that was terrible to add anything to, which itself was a successor to the GNU Makefile, which wasn't build to do anything FXTools was for. For convenience, FXTools has been designed to let the programmer focus on the actual project and not the tooling. For this, it is separated into multiple categories of files.

## Core

The "core" of FXTools is simply a set of files with functions and toolings that act as an internal library for FXTools. It is desired that FXTools not use any external libraries. Most files within it are straight-forward, and require no explanation, but here is a breif summary:

* `editor`: A suite of files to help manage the built-in TUI editor.
* `build.py`: Functions to help compile the project right.
* `env.py`: Related to the environment that FXTools is running in.
* `help_manager.py`: Creates a standard formatting for the internal documentation outputs.
* `printer.py`: Standardises the `print` function into multiple functions for specific purposes.
* `statejson.py`: Manages the configurations inside `fx.state.json`.
* `tools.py`: Checks available tools to see if the required ones are present.

## Targets

When executing anything followed by `fx`, the `fxtools/__main__.py` file is executed, which gets all the arguments passed, and route the arguments to an appropriate target. The name of a target is decided upon the name of the Python file inside the "targets" folder. If its a folder, the function keeps going through the system arguments to find the target Python file. Once found, the rest of the arguments are passed through to the target into a function named `run`. If the function isn't found, an error is thrown.
