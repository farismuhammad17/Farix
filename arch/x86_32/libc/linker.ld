ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text : {
        *(.text.prologue)  /* Ensure _start is first */
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
