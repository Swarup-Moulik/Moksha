#include "../../abi/sys_io.h"
#include <unistd.h>

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  // Emscripten binds fd 1 and 2 to console.log and console.error
  int real_fd = fd;
  if (fd == SYS_IO_FD_STDOUT)
    real_fd = STDOUT_FILENO;
  else if (fd == SYS_IO_FD_STDERR)
    real_fd = STDERR_FILENO;

  ssize_t written = write(real_fd, buffer, count);
  if (written >= 0) {
    if (out_bytes_written)
      *out_bytes_written = (size_t)written;
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}

sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  // Blocking read is rarely supported in pure web wasm without
  // SharedArrayBuffers
  return SYS_ERR_NOTSUP;
}

sys_err_t sys_io_close(int32_t fd) {
  return SYS_OK; // No real files to close
}
