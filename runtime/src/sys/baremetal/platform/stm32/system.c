#include <stdint.h>

// Overrides the weak symbol in arch/arm32/io.c
void board_putchar(char c) {
  // Force the exact registers required by the ARM AAPCS Semihosting ABI
  register uint32_t r0 __asm__("r0") = 0x03;         // 0x03 = SYS_WRITEC
  register uint32_t r1 __asm__("r1") = (uint32_t)&c; // Pointer to the character

  __asm__ volatile("bkpt 0xAB\n" : "+r"(r0) : "r"(r1) : "memory");
}
