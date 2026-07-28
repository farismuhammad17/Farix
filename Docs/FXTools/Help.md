# Internal Documentation

FXTools comes with internal documentation, so that a manual is not required to be able to use it. To get a list of commands, simply run:

```bash
fx help
```

This outputs a clean list of all the functions, as well as important information.

If, at any point, the user needs documentation on a specific function, simply run the command without arguments, and suffix it with `help`, though `-help`, `--help`, `-h` also trigger the same function. For example:

```bash
fx commit help
```

## Help Formatter

The help function in each target has a standardised system of actually functioning. Each target with a `help` function can return a dictionary of the following format:

```py
{
    "USAGE": str,
    "DESCRIPTION": str,
    "ARGS": dict[str, str],
    "VARIABLES": dict[str, str],
    "NOTES": list[str]
}
```

* `USAGE`: A single-line on the exact syntax of the command.
* `DESCRIPTION`: A multi-line explanation regarding the command.
* `ARGS`: A dictionary that maps the name of each argument that the command uses to a breif desciption of the argument.
* `VARIABLES`: Same as the argument, it is used for any variables the function uses from the `fx.state.json` configurations.
* `NOTES`: A list of additional notes regarding the command.
