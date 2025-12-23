ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot))
        *(.text .text.*)    /* Added .* to catch sub-sections */
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata .rodata.*)
        *(.rdata .rdata.*)  /* Map Windows .rdata into .rodata */
    }

    .data ALIGN(4K) :
    {
        *(.data .data.*)
    }

    .bss ALIGN(4K) :
    {
        *(.COMMON)
        *(.bss .bss.*)
        stack_bottom = .;
        . += 0x4000;
        stack_top = .;
    }

    /* CRITICAL: Discard sections that cause "below image base" errors */
    /DISCARD/ :
    {
        *(.reloc)
        *(.pdata)
        *(.xdata)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}