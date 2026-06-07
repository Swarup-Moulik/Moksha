#pragma once
#include <stdint.h>

// Returns a monotonically increasing timestamp in milliseconds
uint64_t sys_time_now_ms(void);

// Blocks the current OS thread for 'ms' milliseconds
void sys_time_sleep_ms(uint64_t ms);
