#include "../../abi/sys_time.h"
#include <time.h>

uint64_t sys_time_now_ms(void) {
  struct timespec ts;

  // CLOCK_MONOTONIC ensures a reliable timer that is unaffected by system time
  // updates
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
  }

  return 0; // Fallback in the highly unlikely event that clock_gettime fails
}

void sys_time_sleep_ms(uint64_t ms) {
  struct timespec req;
  struct timespec rem;

  req.tv_sec = ms / 1000;
  req.tv_nsec = (ms % 1000) * 1000000;

  // nanosleep can be interrupted by signals (like SIGINT).
  // If it is interrupted, it returns -1 and stores the remaining time in 'rem'.
  // We loop to ensure the thread sleeps for the full duration requested.
  while (nanosleep(&req, &rem) == -1) {
    req = rem;
  }
}
