.global moksha_coro_swap

.section .text
moksha_coro_swap:
    /* Context switching stub for future bare-metal async support */
    /* RAX/RDI/RSI setup will go here depending on the calling convention */
    ret
