#include "../../abi/sys_io.h"
#include <windows.h>

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  HANDLE h;
  if (fd == SYS_IO_FD_STDOUT)
    h = GetStdHandle(STD_OUTPUT_HANDLE);
  else if (fd == SYS_IO_FD_STDERR)
    h = GetStdHandle(STD_ERROR_HANDLE);
  else
    h = (HANDLE)(intptr_t)fd;

  DWORD written;
  if (WriteFile(h, buffer, (DWORD)count, &written, NULL)) {
    if (out_bytes_written)
      *out_bytes_written = written;
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}

sys_err_t sys_io_close(int32_t fd) {
  if (CloseHandle((HANDLE)(intptr_t)fd))
    return SYS_OK;
  return SYS_ERR_UNKNOWN;
}

sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  HANDLE h;
  if (fd == 0) // Assuming 0 is STDIN for your ABI
    h = GetStdHandle(STD_INPUT_HANDLE);
  else
    h = (HANDLE)(intptr_t)fd;

  DWORD read;
  if (ReadFile(h, buffer, (DWORD)count, &read, NULL)) {
    if (out_bytes_read)
      *out_bytes_read = read;
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}
