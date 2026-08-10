ENTRY(_start)

SECTIONS
{
    /* The __BASE_ADDR symbol is injected dynamically by the compiler */
    . = __BASE_ADDR;

    .text : ALIGN(4K) {
        *(.multiboot)
        *(.text.boot)
        *(.text)
        *(.text.*)      /* Catches libgcc/compiler-rt function sections */
    }

    .rodata : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)    /* Catches fragmented read-only data */
    }

    .data : ALIGN(4K) {
        *(.data)
        *(.data.*)      /* Catches fragmented data */
    }

    .bss : ALIGN(4K) {
        _sbss = .;
        *(.bss)
        *(.bss.*)       /* Catches fragmented bss */
        *(COMMON)
        _ebss = .;

        . = ALIGN(8);
        _heap_start = .;
        . += 4M;
        _heap_end = .;
    }

    . = ALIGN(16);
    _sstack = .;
    . += 16K;
    _estack = .;
}
