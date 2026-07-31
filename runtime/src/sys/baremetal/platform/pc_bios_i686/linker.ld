ENTRY(_start)

SECTIONS
{
    /* Load kernel at 1MB standard physical address */
    . = 1M;

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

    /* Stack boundaries required by arc.c's is_stack_ptr() heuristic */
    . = ALIGN(16);
    _sstack = .;
    . += 16K; /* 16KB initial stack */
    _estack = .;
}
