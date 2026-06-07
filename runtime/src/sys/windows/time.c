#include "../../abi/sys_time.h"
#include <windows.h>

uint64_t sys_time_now_ms(void) {
  // Returns the number of milliseconds since the system started.
  // This is a monotonic clock, making it safe from system time changes.
  return GetTickCount64();
}

void sys_time_sleep_ms(uint64_t ms) {
  // Synchronous OS thread sleep
  Sleep((DWORD)ms);
}
