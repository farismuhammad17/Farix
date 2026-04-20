<p align="center">
    <img src="readme-assets/flowchart.svg">
</p>

> *"I'm doing a (free) operating system, (just a Hobby, won't be big and professional like gnu)"* - Linus Torvalds, 1991

- [Journal](docs/journal.md): Implementation sufferings
- [Changelog](CHANGELOG.md): Brief summary of changes

# Setup

To set up the development environment without manual configuration, use Docker. You can run `dock.cmd` to setup the container in one command.

> [!NOTE]
> The container provides a consistent Linux environment for compilation only. Due to display and performance limitations in containerised environments, emulation (QEMU) should be run natively on your host machine. If you choose not to use Docker, refer to the Dockerfile for the list of dependencies required for your local system.

## m-Functions

- **Inside Docker / Windows:** The `m` command is pre-configured.
- **Linux / MacOS:** Run `source m.env` (and potentially `chmod +x make.py` for Linux) to initialise the alias.

Regardless, to get the list of commands and instructions:

```bash
m help
```

*If you have problems with this setup, you can use [`make.py`](make.py) directly, and run it like a normal python file in the terminal itself, and pass the necessary arguments after. The `m` is just for convenience.*

---

*This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) for details.*
