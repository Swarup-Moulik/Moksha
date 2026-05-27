#include "../../abi/sys_caps.h"

static const sys_caps_t linux_caps = {.has_threads = true,
                                      .has_async_io =
                                          true, // Assuming epoll/io_uring
                                      .has_filesystem = true,
                                      .has_stdout = true};

const sys_caps_t *sys_get_caps(void) { return &linux_caps; }
