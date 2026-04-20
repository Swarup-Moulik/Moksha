// runtime/src/abi/sys_io.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "sys_error.h"

// Standard file descriptors guaranteed across platforms
#define SYS_IO_FD_STDOUT 1
#define SYS_IO_FD_STDERR 2

// Modes for sys_io_open
#define SYS_IO_MODE_READ  0x01
#define SYS_IO_MODE_WRITE 0x02

// Implemented by sys/*/io.c
sys_err_t sys_io_open(const char* path, int mode, int32_t* out_fd);
sys_err_t sys_io_read(int32_t fd, void* buffer, size_t count, size_t* out_bytes_read);
sys_err_t sys_io_write(int32_t fd, const void* buffer, size_t count, size_t* out_bytes_written);
sys_err_t sys_io_close(int32_t fd);
