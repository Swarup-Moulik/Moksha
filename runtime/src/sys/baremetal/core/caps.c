#include "abi/sys_caps.h"
#include "moksha_rt.h"

// Bare-metal targets typically lack native OS threads, async IO, and
// filesystems. We enable stdout to route via serial (UART) or VGA.
static const sys_caps_t baremetal_caps = {.has_threads = false,
                                          .has_async_io = false,
                                          .has_filesystem = false,
                                          .has_stdout = true};

const sys_caps_t *sys_get_caps(void) { return &baremetal_caps; }
