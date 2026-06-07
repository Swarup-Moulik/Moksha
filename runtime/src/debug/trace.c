#include <stdbool.h>
#include <stdint.h>

// Set to true via a compiler flag (-DMOKSHA_TRACE_ARC) to enable runtime
// tracking
static bool trace_arc_enabled = false;
static bool trace_coro_enabled = false;

extern void moksha_debug_log(const char *prefix, const char *message);

// Example tracing hook (You can expand this with a simple itoa to print
// addresses/counts)
void moksha_trace_arc_inc(void *ptr, uint32_t new_count) {
  if (!trace_arc_enabled)
    return;
  moksha_debug_log("ARC", "Incremented reference count");
}

void moksha_trace_arc_dec(void *ptr, uint32_t new_count) {
  if (!trace_arc_enabled)
    return;
  if (new_count == 0) {
    moksha_debug_log("ARC", "Object destroyed (ref count reached 0)");
  } else {
    moksha_debug_log("ARC", "Decremented reference count");
  }
}

void moksha_trace_coro_state(void *handle, const char *state) {
  if (!trace_coro_enabled)
    return;
  moksha_debug_log("CORO", state);
}
