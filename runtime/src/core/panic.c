// runtime/src/core/panic.c
#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <stddef.h>

// Minimal strlen (no <string.h> needed)
static size_t internal_strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

void moksha_rt_panic(const char *message) {
  const sys_caps_t *caps = sys_get_caps();

  // If we have a console/terminal, print the panic message
  if (caps->has_stdout) {
    size_t written;
    sys_io_write(SYS_IO_FD_STDERR, "PANIC: ", 7, &written);
    sys_io_write(SYS_IO_FD_STDERR, message, internal_strlen(message), &written);
    sys_io_write(SYS_IO_FD_STDERR, "\n", 1, &written);
  }

  // __builtin_trap() emits the target-specific hardware trap instruction
  // (e.g., `ud2` on x86, `brk` on AArch64). This safely halts the CPU on
  // bare-metal and causes an immediate abort/core-dump on OS targets.
  __builtin_trap();
}
