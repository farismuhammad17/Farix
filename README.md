<p align="center">
    <img src="readme-assets/flowchart.svg">
</p>

> *"I'm doing a (free) operating system, (just a Hobby, won't be big and professional like gnu)"* - Linus Torvalds, 1991

- [Journal](docs/journal.md): Implementation sufferings
- [Changelog](CHANGELOG.md): Brief summary of changes

# Setup

If you're on Linux or Mac, you need to run `source m.env` (and potentially `chmod +x make.py` for Linux), which initialises `m` as a command. If you're on Windows, the [`m.bat`](m.bat) script will automatically provide that `m`. Regardless, to get the list of commands and instructions:

```bash
m help
```

If you have problems with this setup, you can use [`make.py`](make.py) directly, and run it like a normal python file in the terminal itself, and pass the necessary arguments after. The `m` is just for convenience.

---

*This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) for details.*
