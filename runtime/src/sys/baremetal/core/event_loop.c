#include "abi/sys_event.h"
#include "moksha_rt.h"

sys_err_t sys_event_init(void) { return SYS_OK; }

int32_t sys_event_poll(int timeout_ms) {
  (void)timeout_ms;
  cpu_relax();
  return 0;
}

sys_err_t sys_event_register_io(int32_t fd, sys_task_waker_t waker, void *ctx) {
  (void)fd;
  (void)waker;
  (void)ctx;
  return SYS_ERR_NOTSUP;
}

sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker,
                                   void *ctx) {
  (void)timeout_ms;
  (void)waker;
  (void)ctx;
  return SYS_ERR_NOTSUP;
}

void sys_process_exit(int code) {
  (void)code;
  while (1) {
    cpu_relax();
  } // Halt indefinitely
}

void sys_process_abort(void) {
  while (1) {
    cpu_relax();
  } // Halt indefinitely
}
