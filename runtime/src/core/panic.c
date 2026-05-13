#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Minimal string utilities (No <string.h>)
static size_t internal_strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

// Minimal integer to string converter (No <stdio.h> or snprintf)
static void internal_itoa64(int64_t val, char *buf) {
  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  char temp[32];
  int i = 0, j = 0;
  bool is_neg = val < 0;
  uint64_t uval = is_neg ? -val : val;

  while (uval > 0) {
    temp[i++] = (uval % 10) + '0';
    uval /= 10;
  }
  if (is_neg)
    buf[j++] = '-';
  while (i > 0)
    buf[j++] = temp[--i];
  buf[j] = '\0';
}

void moksha_rt_panic(const char *message) {
  const sys_caps_t *caps = sys_get_caps();

  if (caps->has_stdout) {
    size_t written;
    sys_io_write(SYS_IO_FD_STDERR, "PANIC: ", 7, &written);
    sys_io_write(SYS_IO_FD_STDERR, message, internal_strlen(message), &written);
    sys_io_write(SYS_IO_FD_STDERR, "\n", 1, &written);
  }
  __builtin_trap();
}

void moksha_rt_panic_out_of_bounds(int64_t index, int64_t length) {
  char buffer[128];
  char idx_str[32];
  char len_str[32];

  internal_itoa64(index, idx_str);
  internal_itoa64(length, len_str);

  // Manual safe concatenation
  size_t pos = 0;
  const char *p1 = "Index out of bounds. Index is ";
  while (*p1 && pos < 127)
    buffer[pos++] = *p1++;

  char *p2 = idx_str;
  while (*p2 && pos < 127)
    buffer[pos++] = *p2++;

  const char *p3 = " but length is ";
  while (*p3 && pos < 127)
    buffer[pos++] = *p3++;

  char *p4 = len_str;
  while (*p4 && pos < 127)
    buffer[pos++] = *p4++;

  if (pos < 127)
    buffer[pos++] = '.';
  buffer[pos] = '\0';

  moksha_rt_panic(buffer);
}

void moksha_rt_panic_null_deref(void) {
  moksha_rt_panic("Null pointer dereference.");
}

void moksha_rt_panic_key_not_found(void) {
  moksha_rt_panic("Key not found in table.");
}

void moksha_rt_panic_bad_cast(void) {
  moksha_rt_panic("Invalid runtime type cast (downcast failed).");
}
