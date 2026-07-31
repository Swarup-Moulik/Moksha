#include "abi/sys_io.h"
#include <stddef.h>
#include <stdint.h>

#define EFIAPI __attribute__((ms_abi))

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  char _pad1[8];
  uint64_t(EFIAPI *OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
                                 const uint16_t *String);
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
  char _pad1[64];
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
} EFI_SYSTEM_TABLE;

extern void *g_EfiSystemTable;

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  (void)fd;
  if (!g_EfiSystemTable || !buffer || count == 0) {
    if (out_bytes_written)
      *out_bytes_written = 0;
    return SYS_OK;
  }

  EFI_SYSTEM_TABLE *st = (EFI_SYSTEM_TABLE *)g_EfiSystemTable;
  const char *str = (const char *)buffer;

  uint16_t utf16_buf[128];
  size_t i = 0, buf_idx = 0;

  while (i < count) {
    if (str[i] == '\n') {
      utf16_buf[buf_idx++] = '\r';
      if (buf_idx == 127) {
        utf16_buf[buf_idx] = 0;
        st->ConOut->OutputString(st->ConOut, utf16_buf);
        buf_idx = 0;
      }
    }
    utf16_buf[buf_idx++] = (uint16_t)str[i];

    if (buf_idx == 127) {
      utf16_buf[buf_idx] = 0;
      st->ConOut->OutputString(st->ConOut, utf16_buf);
      buf_idx = 0;
    }
    i++;
  }

  if (buf_idx > 0) {
    utf16_buf[buf_idx] = 0;
    st->ConOut->OutputString(st->ConOut, utf16_buf);
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
