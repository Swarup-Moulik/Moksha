#include "../../abi/sys_event.h"
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static int kq_fd = -1;
static uint64_t timer_id_counter = 1;

typedef struct {
  sys_task_waker_t waker;
  void *ctx;
} EventCtx;

sys_err_t sys_event_init(void) {
  kq_fd = kqueue();
  return kq_fd >= 0 ? SYS_OK : SYS_ERR_UNKNOWN;
}

int32_t sys_event_poll(int timeout_ms) {
  struct kevent event;
  struct timespec ts;

  ts.tv_sec = timeout_ms / 1000;
  ts.tv_nsec = (timeout_ms % 1000) * 1000000;

  // Wait for 1 event
  int num_events =
      kevent(kq_fd, NULL, 0, &event, 1, timeout_ms >= 0 ? &ts : NULL);

  if (num_events > 0) {
    EventCtx *ctx = (EventCtx *)event.udata;
    if (ctx) {
      ctx->waker(ctx->ctx);
      free(ctx);
    }
    return 1;
  }
  return 0;
}

sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker,
                                   void *ctx) {
  EventCtx *ectx = calloc(1, sizeof(EventCtx));
  if (!ectx)
    return SYS_ERR_NOMEM;

  ectx->waker = waker;
  ectx->ctx = ctx;

  struct kevent kev;
  uint64_t current_id = timer_id_counter++;

  // Configure a one-shot timer (EV_ONESHOT) using milliseconds (NOTE_MSECONDS)
  EV_SET(&kev, current_id, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT,
         NOTE_MSECONDS, timeout_ms, ectx);

  if (kevent(kq_fd, &kev, 1, NULL, 0, NULL) == -1) {
    free(ectx);
    return SYS_ERR_UNKNOWN;
  }

  return SYS_OK;
}
