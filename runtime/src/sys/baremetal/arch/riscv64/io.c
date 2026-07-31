#include "abi/sys_io.h"
#include <stdint.h>

// Standard RISC-V QEMU Virt UART address
#define UART0_BASE 0x10000000
#define UART0_TX (*(volatile uint8_t *)(UART0_BASE + 0x00))
#define UART0_LSR (*(volatile uint8_t *)(UART0_BASE + 0x05))

static void write_serial(char c) {
  while ((UART0_LSR & 0x20) == 0)
    ; // Wait for TX buffer to be empty
  UART0_TX = c;
}

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  const char *buf = (const char *)buffer;
  for (size_t i = 0; i < count; i++) {
    write_serial(buf[i]);
  }
  if (out_bytes_written)
    *out_bytes_written = count;
  return SYS_OK;
}

sys_err_t sys_io_open(const char *path, int mode, int32_t *out_fd) {
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_close(int32_t fd) { return SYS_ERR_NOTSUP; }
