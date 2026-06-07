#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <stddef.h>

static size_t string_len(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

void moksha_debug_log(const char *prefix, const char *message) {
  const sys_caps_t *caps = sys_get_caps();
  if (!caps->has_stdout)
    return; // Silent on embedded without UART

  size_t written;
  sys_io_write(SYS_IO_FD_STDERR, "[", 1, &written);
  sys_io_write(SYS_IO_FD_STDERR, prefix, string_len(prefix), &written);
  sys_io_write(SYS_IO_FD_STDERR, "] ", 2, &written);
  sys_io_write(SYS_IO_FD_STDERR, message, string_len(message), &written);
  sys_io_write(SYS_IO_FD_STDERR, "\n", 1, &written);
}
