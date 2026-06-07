#include "../../abi/sys_caps.h"

static const sys_caps_t web_caps = {.has_threads = false,
                                    .has_async_io =
                                        true, // Hooked into JS Event Loop
                                    .has_filesystem = false,
                                    .has_stdout = true};

const sys_caps_t *sys_get_caps(void) { return &web_caps; }
