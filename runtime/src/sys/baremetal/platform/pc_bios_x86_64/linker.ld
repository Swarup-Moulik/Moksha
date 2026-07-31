ENTRY(_start)

SECTIONS
{
    /* Load kernel at 2MB standard boundary for x86_64 */
    . = 2M;

    .text : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)

        /* 4MB Bump Allocator Heap Boundary */
        . = ALIGN(8);
        _heap_start = .;
        . = . + 4M;
        _heap_end = .;
    }

    . = ALIGN(16);
    _sstack = .;
    . += 16K;
    _estack = .;
}
