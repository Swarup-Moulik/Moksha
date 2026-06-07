#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include "../abi/sys_event.h"
#include "../abi/sys_time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  void *coro_handle;
  bool is_completed;
  void *result_data;
  void *waiting_coro;
  bool is_rejected;
  bool was_awaited;
} InternalPromise;

extern void *moksha_rt_make_resolved_promise(void *result_data);
extern void *moksha_rt_make_unresolved_promise(void);
extern void moksha_rt_resolve_promise(void *promise_handle, void *result_data);
extern void moksha_scheduler_schedule(void *coro_handle);
extern void moksha_scheduler_set_priority(void *coro_handle, int32_t priority);
extern void moksha_scheduler_run(void);
extern void *moksha_rt_alloc(size_t size, uint32_t type_id);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_rt_throw(void *payload);
extern void *moksha_rt_spawn(MokshaClosure closure);
extern void moksha_rt_reject_promise(void *promise_handle, void *ex_payload);

__thread int32_t current_spawn_priority = 1; // 1 = NORMAL

// ============================================================================
// Basic Builtins
// ============================================================================

void *moksha_builtin_yield(void) {
  return moksha_rt_make_resolved_promise(NULL);
}

void *moksha_builtin_spawn(void *closure_ptr, int32_t priority) {
  if (!closure_ptr) {
    return NULL;
  }

  // Set context before synchronous start
  current_spawn_priority = priority;
  void *promise = moksha_rt_spawn(*(MokshaClosure *)closure_ptr);
  current_spawn_priority = 1; // Reset context

  return promise;
}

void *moksha_builtin_spawn_promise(void *promise_handle, int32_t priority) {
  if (promise_handle) {
    InternalPromise *prom = (InternalPromise *)promise_handle;
    if (prom->coro_handle) {
      moksha_scheduler_set_priority(prom->coro_handle, priority);
    }
  }
  return promise_handle;
}

void moksha_builtin_cancel(void *promise_handle) {
  if (!promise_handle)
    return;
  InternalPromise *prom = (InternalPromise *)promise_handle;

  bool expected = false;
  if (!__atomic_compare_exchange_n(&prom->is_completed, &expected, true, false,
                                   __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
    return;
  }

  __atomic_store_n(&prom->is_rejected, true, __ATOMIC_RELEASE);
  __atomic_store_n(&prom->was_awaited, true, __ATOMIC_RELEASE);

  // Must use ARC allocator for exception payloads!
  // MOKSHA_TYPE_U32 = 6
  uint32_t *cancel_ex = (uint32_t *)moksha_rt_alloc(sizeof(uint32_t), 6);
  *cancel_ex = 1;
  prom->result_data = cancel_ex;

  void *waiter =
      __atomic_exchange_n(&prom->waiting_coro, NULL, __ATOMIC_ACQ_REL);
  if (waiter) {
    moksha_scheduler_schedule(waiter);
  }
}

// ============================================================================
// Sleep Builtin (Bare-Metal Compatible)
// ============================================================================

static void sleep_timer_waker(void *ctx) {
  void *master = ctx;
  moksha_rt_resolve_promise(master, NULL);
}

void *moksha_builtin_sleep(uint32_t ms) {
  void *master = moksha_rt_make_unresolved_promise();
  const sys_caps_t *caps = sys_get_caps();

  if (caps->has_async_io) {
    sys_event_register_timer(ms, sleep_timer_waker, master);
  } else {
    // Cooperative bare-metal fallback (Yields execution until time passes)
    uint64_t end_time = sys_time_now_ms() + ms;
    while (sys_time_now_ms() < end_time) {
      moksha_scheduler_run();
    }
    moksha_rt_resolve_promise(master, NULL);
  }

  return master;
}

// ============================================================================
// Timeout Builtin (Bare-Metal Compatible)
// ============================================================================

typedef struct {
  void *master;
  void *target;
  uint64_t end_time_ms;
} TimeoutCtx;

static void timeout_poll_waker(void *arg) {
  TimeoutCtx *ctx = (TimeoutCtx *)arg;
  InternalPromise *master = (InternalPromise *)ctx->master;
  InternalPromise *target = (InternalPromise *)ctx->target;

  if (master->is_completed) {
    moksha_mem_free(ctx);
    return;
  }

  if (target && target->is_completed) {
    if (target->is_rejected)
      master->is_rejected = true;
    moksha_rt_resolve_promise(master, target->result_data);
    moksha_mem_free(ctx);
    return;
  }

  if (sys_time_now_ms() >= ctx->end_time_ms) {
    int32_t *timeout_ex =
        (int32_t *)moksha_rt_alloc(4, 5); // Exception allocation
    *timeout_ex = 1;
    moksha_rt_reject_promise(master, timeout_ex);
    moksha_mem_free(ctx);
    return;
  }

  // Not complete and time remains: poll again in 1ms
  sys_event_register_timer(1, timeout_poll_waker, ctx);
}

void *moksha_builtin_timeout(void *promise_handle, uint32_t ms) {
  void *master = moksha_rt_make_unresolved_promise();
  const sys_caps_t *caps = sys_get_caps();

  if (caps->has_async_io) {
    TimeoutCtx *ctx = (TimeoutCtx *)moksha_mem_alloc(sizeof(TimeoutCtx));
    ctx->master = master;
    ctx->target = promise_handle;
    ctx->end_time_ms = sys_time_now_ms() + ms;
    sys_event_register_timer(1, timeout_poll_waker, ctx);
  } else {
    // Cooperative fallback
    InternalPromise *target = (InternalPromise *)promise_handle;
    uint64_t end_time = sys_time_now_ms() + ms;

    while (!target->is_completed && sys_time_now_ms() < end_time) {
      moksha_scheduler_run();
    }

    if (target->is_completed) {
      InternalPromise *m = (InternalPromise *)master;
      if (target->is_rejected)
        m->is_rejected = true;
      moksha_rt_resolve_promise(master, target->result_data);
    } else {
      int32_t *timeout_ex = (int32_t *)moksha_rt_alloc(4, 5);
      *timeout_ex = 1;
      moksha_rt_reject_promise(master, timeout_ex);
    }
  }

  return master;
}

// ============================================================================
// Select Builtin (Bare-Metal Compatible)
// ============================================================================

typedef struct {
  void *master;
  void *p1;
  void *p2;
} SelectCtx;

static void select_poll_waker(void *arg) {
  SelectCtx *ctx = (SelectCtx *)arg;
  InternalPromise *master = (InternalPromise *)ctx->master;
  InternalPromise *p1 = (InternalPromise *)ctx->p1;
  InternalPromise *p2 = (InternalPromise *)ctx->p2;

  if (master->is_completed) {
    moksha_mem_free(ctx);
    return;
  }

  if (p1 && p1->is_completed) {
    if (p1->is_rejected)
      master->is_rejected = true;
    moksha_rt_resolve_promise(master, p1->result_data);
    if (p2)
      moksha_builtin_cancel(p2); // Cancel the loser
    moksha_mem_free(ctx);
    return;
  }

  if (p2 && p2->is_completed) {
    if (p2->is_rejected)
      master->is_rejected = true;
    moksha_rt_resolve_promise(master, p2->result_data);
    if (p1)
      moksha_builtin_cancel(p1); // Cancel the loser
    moksha_mem_free(ctx);
    return;
  }

  // Neither complete, poll again next tick
  sys_event_register_timer(1, select_poll_waker, ctx);
}

void *moksha_builtin_select(void *p1, void *p2) {
  void *master = moksha_rt_make_unresolved_promise();
  const sys_caps_t *caps = sys_get_caps();

  if (caps->has_async_io) {
    SelectCtx *ctx = (SelectCtx *)moksha_mem_alloc(sizeof(SelectCtx));
    ctx->master = master;
    ctx->p1 = p1;
    ctx->p2 = p2;
    sys_event_register_timer(1, select_poll_waker, ctx);
  } else {
    // Cooperative polling loop fallback
    InternalPromise *pm1 = (InternalPromise *)p1;
    InternalPromise *pm2 = (InternalPromise *)p2;
    InternalPromise *m = (InternalPromise *)master;

    while (true) {
      if (pm1 && pm1->is_completed) {
        if (pm1->is_rejected)
          m->is_rejected = true;
        moksha_rt_resolve_promise(master, pm1->result_data);
        if (pm2)
          moksha_builtin_cancel(pm2);
        break;
      }
      if (pm2 && pm2->is_completed) {
        if (pm2->is_rejected)
          m->is_rejected = true;
        moksha_rt_resolve_promise(master, pm2->result_data);
        if (pm1)
          moksha_builtin_cancel(pm1);
        break;
      }
      // Give time to the rest of the system to complete these promises
      moksha_scheduler_run();
    }
  }

  return master;
}
