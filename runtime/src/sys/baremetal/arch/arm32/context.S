.global moksha_coro_swap
.section .text
moksha_coro_swap:
    /* Context switching stub for ARMv7/Cortex-M (R0-R3) */
    bx lr
