#include "abi/sys_io.h"
#include <stdint.h>

// REMOVED the __attribute__((weak)) board_putchar implementation.
// This forces the linker to pull in board.c to find the strong symbol!
extern void board_putchar(char c);

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  (void)fd;
  const char *buf = (const char *)buffer;
  for (size_t i = 0; i < count; i++) {
    board_putchar(buf[i]);
  }
  if (out_bytes_written)
    *out_bytes_written = count;
  return SYS_OK;
}

sys_err_t sys_io_open(const char *path, int mode, int32_t *out_fd) {
  (void)path;
  (void)mode;
  (void)out_fd;
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  (void)fd;
  (void)buffer;
  (void)count;
  (void)out_bytes_read;
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_close(int32_t fd) {
  (void)fd;
  return SYS_ERR_NOTSUP;
}
