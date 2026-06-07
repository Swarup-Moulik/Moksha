#include "../../abi/sys_caps.h"

static const sys_caps_t wasi_caps = {
    .has_threads = false,
    .has_async_io = false, // Disables OS timers, forces cooperative wait
    .has_filesystem = true,
    .has_stdout = true};

const sys_caps_t *sys_get_caps(void) { return &wasi_caps; }
