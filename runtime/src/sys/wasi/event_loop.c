#include "../../abi/sys_event.h"

sys_err_t sys_event_init(void) { return SYS_OK; }
int32_t sys_event_poll(int timeout_ms) { return 0; }
sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker,
                                   void *ctx) {
  return SYS_ERR_NOTSUP;
}
