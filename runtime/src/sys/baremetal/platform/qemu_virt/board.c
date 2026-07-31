#include <stdint.h>

// PL011 UART mapped at 0x09000000 on QEMU ARM Virt
#define UART0_BASE 0x09000000
#define UART0_DR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR (*(volatile uint32_t *)(UART0_BASE + 0x18))

// Overrides the weak symbol in arch/aarch64/io.c
void board_putchar(char c) {
  // Wait until TX FIFO is not full
  while (UART0_FR & (1 << 5))
    ;
  UART0_DR = c;
}
