#pragma once
#include <stdint.h>

typedef enum {
  SYS_OK = 0,
  SYS_ERR_INVAL = -1,   // Invalid argument
  SYS_ERR_NOMEM = -2,   // Out of memory
  SYS_ERR_NOENT = -3,   // File/Resource not found
  SYS_ERR_DENIED = -4,  // Permission denied
  SYS_ERR_NOTSUP = -5,  // Not supported on this platform
  SYS_ERR_AGAIN = -6,   // Resource unavailable, try again (EAGAIN)
  SYS_ERR_UNKNOWN = -99 // Catch-all
} sys_err_t;
