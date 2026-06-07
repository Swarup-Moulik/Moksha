#pragma once
#include <stdbool.h>

// Represents the capabilities of the current target OS / Baremetal platform
typedef struct {
  bool has_threads;
  bool has_async_io;
  bool has_filesystem;
  bool has_stdout;
} sys_caps_t;

// Implemented by sys/*/caps.c
const sys_caps_t *sys_get_caps(void);
