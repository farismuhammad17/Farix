[Back to journal.md](../journal.md)

# Shelf

This is what I am going to call my shell, since I am move it into an ELF, so shell.elf is going to be the shelf. It's really fancy, yea, and I can't be bothered about it. I am branding over here.

I also want to note: I started the kernel in VS Code, but it felt like bloatware slowly, so I switched to Sublime Text, and it felt too ugly. I have no clue why, but it felt so lame, and the features felt quite annoying. It was nice, yes, but it wasn't what I liked. So, I switched over to Zed, but it seems that overtime, Zed would get slow, and I figured its most likely memory leaks or something like that, even if it's written in Rust. Vim, NeoVim, etc. all are so annoying by jam-packing movement and everything into keystrokes. I get its for efficiency, but I don't want to learn my editor just to type text, and I am lazy to learn it. Other stuff, like Cursor, seem to shove AI down my throat so much, I started preferring VS Code. So, currently, I am in Pulsar, unless I change out of it.

*15th April, 2026*

I decided that the system calls definitions that are required by newlib were not fully filled out. The majority of it were left to return default values, even if it was wrong. I decided I didn't like that, and filled out every single one, since I would need them in the future, probably. As of now, the kernel ran properly without the rest of them, but, perhaps, newlib uses them in some way that I haven't seen yet. It also makes the kernel programs even more straightforward.
