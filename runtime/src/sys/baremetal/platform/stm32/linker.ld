ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 256K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    .text : {
        KEEP(*(.isr_vector))
        *(.text)
        *(.text.*)      /* Catches libgcc function sections */
        *(.rodata)
        *(.rodata.*)    /* Catches fragmented read-only data */
        *(.glue_7)      /* ARM-to-Thumb glues */
        *(.glue_7t)     /* Thumb-to-ARM glues */
    } > FLASH

    /* ARM Exception unwinding sections (libgcc requires these) */
    .ARM.extab : { 
        *(.ARM.extab* .gnu.linkonce.armextab.*) 
    } > FLASH
    
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
        . = ALIGN(4);
        _sidata = .;    /* End of FLASH, start of DATA payload */
    } > FLASH

    .data : AT(_sidata) {
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss : {
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;

        . = ALIGN(8);
        _heap_start = .;
        . += 32K; /* 32KB Heap for MCU */
        _heap_end = .;
    } > RAM

    _sstack = .;
    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
