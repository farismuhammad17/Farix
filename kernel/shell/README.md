This section documents the **Kernel Shell**, the interactive command-line interface (CLI) for the system. It handles user input via the keyboard, parses commands and arguments, and supports basic piping between commands.

# Kernel Shell

The shell operates as a persistent loop that polls the keyboard buffer, manages a line buffer for the current command, and dispatches tasks to a command table once a newline is detected.

## Input Management

```c
void shell_update();
```

This is the main state-machine update for the CLI. It continuously polls `kbd_buffer` to process characters:
* **Echoing:** Every character typed is immediately sent back to the terminal for visual feedback.
* **Line Editing:** Handles backspaces (`\b`) by decrementing the buffer size and updating the display.
* **Execution:** Once `\n` is hit, it null-terminates the buffer, sets `shell_buffer_ready`, and triggers the parser.

## Parsing and Dispatch

```c
void shell_parse(const char* input);
```

The parser breaks the raw input string into manageable segments:
1.  **Pipe Detection:** It looks for the `|` character. If found, it splits the input into two segments to be executed sequentially.
2.  **Trimming:** Removes leading and trailing whitespace to ensure clean command lookups.
3.  **Command Extraction:** Splits the segment into the `cmd_name` (first word) and `args` (everything else).
4.  **Lookup:** Iterates through a global `command_table`. If a name matches, it executes the associated function pointer, passing in the arguments.

## Output Redirection and Piping

The shell implements a specialized output buffering system to facilitate piping between internal commands.

* **Output Buffer:** Instead of printing directly to the screen, shell commands use `sh_print()`. This writes into `shell_output_buffer`.
* **Piping Logic:** If `is_piping` is true, the output of the first command is captured into `last_cmd_output`. The subsequent command can then use this data as its input source.
* **Flushing:** Once the command chain is complete, `shell_flush()` is called to dump any remaining buffered text to the physical terminal via `printf`.

Buffering also makes the terminal print everything faster, since `printf` is slow, it is much more efficient to just buffer the text and just print it. We store the buffer in an array, and if the array is full, we flush, and we just keep adding to it.
