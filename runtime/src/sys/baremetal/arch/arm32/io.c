#include "abi/sys_io.h"
#include <stddef.h>
#include <stdint.h>

// Force the linker to extract the platform-specific driver (system.c) from the
// archive
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
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_close(int32_t fd) { return SYS_ERR_NOTSUP; }

void abort(void) {
  extern void moksha_rt_panic(const char *message);
  moksha_rt_panic("Fatal error: abort() called by libgcc unwinder.");
  while (1) {
    // Halt execution permanently
  }
}

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

// memset is often requested alongside memcpy by GCC, so we include it to be
// safe
void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}
