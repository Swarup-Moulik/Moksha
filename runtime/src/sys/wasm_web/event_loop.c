#include "../../abi/sys_event.h"
#include <emscripten.h>
#include <stdlib.h>

typedef struct {
  sys_task_waker_t waker;
  void *ctx;
} WebEventCtx;

// Called by the browser/JS engine when the timeout expires
static void web_timer_callback(void *arg) {
  WebEventCtx *ctx = (WebEventCtx *)arg;
  ctx->waker(ctx->ctx);
  free(ctx);
}

sys_err_t sys_event_init(void) {
  // Browser event loop is inherently initialized
  return SYS_OK;
}

int32_t sys_event_poll(int timeout_ms) {
  // In the browser, the JS environment controls the loop.
  // Emscripten handles yielding back to JS, so we don't block here.
  return 0;
}

sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker,
                                   void *ctx) {
  WebEventCtx *ectx = calloc(1, sizeof(WebEventCtx));
  if (!ectx)
    return SYS_ERR_NOMEM;

  ectx->waker = waker;
  ectx->ctx = ctx;

  // Schedules the callback to run on the JS event loop after 'timeout_ms'
  emscripten_async_call(web_timer_callback, ectx, timeout_ms);

  return SYS_OK;
}
