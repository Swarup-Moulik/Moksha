#include "../../abi/sys_event.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

static int epoll_fd = -1;

typedef struct {
  sys_task_waker_t waker;
  void *ctx;
  int fd; // Keep track of the fd to close it later
} EventCtx;

sys_err_t sys_event_init(void) {
  epoll_fd = epoll_create1(0);
  return epoll_fd >= 0 ? SYS_OK : SYS_ERR_UNKNOWN;
}

int32_t sys_event_poll(int timeout_ms) {
  struct epoll_event event;

  // Wait for events
  int num_events = epoll_wait(epoll_fd, &event, 1, timeout_ms);

  if (num_events > 0) {
    EventCtx *ctx = (EventCtx *)event.data.ptr;

    // If this was a timer, we need to read from it to clear the event
    uint64_t expirations;
    read(ctx->fd, &expirations, sizeof(expirations));

    // Trigger the waker
    ctx->waker(ctx->ctx);

    // Cleanup
    close(ctx->fd);
    free(ctx);

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

  int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (tfd == -1) {
    free(ectx);
    return SYS_ERR_UNKNOWN;
  }
  ectx->fd = tfd;

  struct itimerspec its;
  its.it_value.tv_sec = timeout_ms / 1000;
  its.it_value.tv_nsec = (timeout_ms % 1000) * 1000000;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;
  timerfd_settime(tfd, 0, &its, NULL);

  struct epoll_event ev;
  // ADD EPOLLONESHOT HERE:
  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = ectx;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
    close(tfd);
    free(ectx);
    return SYS_ERR_UNKNOWN;
  }

  return SYS_OK;
}
