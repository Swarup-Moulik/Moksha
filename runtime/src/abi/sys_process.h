#pragma once
#include <stdint.h>

// Clean shutdown for user-level errors (e.g., divide by zero).
// This function should never return.
void sys_process_exit(int32_t exit_code);

// Aggressive crash for internal compiler/runtime bugs.
// Triggers a core dump on OS targets or a hardware trap on bare-metal.
void sys_process_abort(void);
