#include "../../include/moksha_rt.h"
#include "../abi/sys_thread.h"
#include <stddef.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_scheduler_schedule(void *coro_handle);
extern void moksha_scheduler_run(void);
extern void moksha_rt_panic(const char *message);

// Internal representation of a Promise (which is an ARC object itself)
typedef struct {
  void *coro_handle;
  bool is_completed;
  void *result_data;
  void *waiting_coro;
} InternalPromise;

// Context passed to strong OS threads so they can resolve their promises
typedef struct {
  MokshaClosure closure;
  void *promise_handle;
} ThreadContext;

// ============================================================================
// Promise Resolution
// ============================================================================

void moksha_rt_resolve_promise(void *promise_handle, void *result_data) {
  if (!promise_handle)
    return;
  InternalPromise *promise = (InternalPromise *)promise_handle;

  promise->result_data = result_data;
  promise->is_completed = true;

  // If a coroutine was suspended waiting for this promise, wake it up!
  if (promise->waiting_coro) {
    moksha_scheduler_schedule(promise->waiting_coro);
    promise->waiting_coro = NULL;
  }
}

// ============================================================================
// Thread Trampolines
// ============================================================================

static void *strong_thread_trampoline(void *arg) {
  ThreadContext *ctx = (ThreadContext *)arg;

  // 1. Execute the user's closure
  void (*func)(void *) = (void (*)(void *))ctx->closure.function_ptr;
  func(ctx->closure.environment_ptr);

  // 2. Resolve the promise to wake up anyone awaiting this thread
  moksha_rt_resolve_promise(ctx->promise_handle, NULL);

  // 3. Free the context struct and let the scheduler know we are done!
  moksha_mem_free(ctx);
  moksha_scheduler_dec_active(); // <--- Decrement here!
  return NULL;
}

static void *weak_thread_trampoline(void *arg) {
  MokshaClosure *closure = (MokshaClosure *)arg;
  void (*func)(void *) = (void (*)(void *))closure->function_ptr;
  func(closure->environment_ptr);
  return NULL;
}

// ============================================================================
// Spawners
// ============================================================================

// Spawn an async closure (Cooperative Coroutine)
void *moksha_rt_spawn(MokshaClosure closure) {
  if (!closure.function_ptr) {
    moksha_rt_panic("Cannot spawn null closure");
  }

  InternalPromise *promise =
      (InternalPromise *)moksha_rt_alloc(sizeof(InternalPromise), 20);
  promise->is_completed = false;
  promise->result_data = NULL;
  promise->waiting_coro = NULL;

  promise->coro_handle = closure.function_ptr;
  moksha_scheduler_schedule(promise->coro_handle);

  return promise;
}

void *moksha_rt_spawn_thread(void *closure_ptr) {
  if (!closure_ptr)
    moksha_rt_panic("Cannot spawn thread with null closure");

  InternalPromise *promise =
      (InternalPromise *)moksha_rt_alloc(sizeof(InternalPromise), 20);
  promise->is_completed = false;
  promise->result_data = NULL;
  promise->waiting_coro = NULL;

  // Wrap the closure and promise in a context payload
  ThreadContext *ctx = (ThreadContext *)moksha_mem_alloc(sizeof(ThreadContext));
  ctx->closure = *(MokshaClosure *)closure_ptr;
  ctx->promise_handle = promise;

  // Tell the scheduler to stay alive for this strong thread!
  moksha_scheduler_inc_active(); // <--- Increment here!

  sys_thread_t thread;
  if (sys_thread_create(&thread, (sys_thread_func_t)strong_thread_trampoline,
                        ctx) != 0) {
    moksha_scheduler_dec_active(); // Rollback if spawn fails
    moksha_rt_panic("Failed to spawn strong thread");
  }

  promise->coro_handle = (void *)thread;
  return promise;
}

void *moksha_rt_spawn_weak_thread(void *closure_ptr) {
  if (!closure_ptr)
    moksha_rt_panic("Cannot spawn weak thread with null closure");

  sys_thread_t thread;
  if (sys_thread_create(&thread, (sys_thread_func_t)weak_thread_trampoline,
                        closure_ptr) != 0) {
    moksha_rt_panic("Failed to spawn weak thread");
  }

  // Weak threads are detached and not meant to be awaited, but we return a
  // resolved promise to satisfy the type system.
  InternalPromise *promise =
      (InternalPromise *)moksha_rt_alloc(sizeof(InternalPromise), 20);
  promise->is_completed = true;
  promise->result_data = NULL;
  promise->coro_handle = (void *)thread;
  promise->waiting_coro = NULL;

  sys_thread_detach(thread);
  return promise;
}

// ============================================================================
// Await Hooks
// ============================================================================

void moksha_rt_register_await(void *promise_handle, void *waiting_coro) {
  if (!waiting_coro)
    return;

  if (!promise_handle) {
    moksha_scheduler_schedule(waiting_coro);
    return;
  }

  InternalPromise *promise = (InternalPromise *)promise_handle;
  // If the task is already done, schedule the continuation immediately
  if (promise->is_completed) {
    moksha_scheduler_schedule(waiting_coro);
  } else {
    // Otherwise, park the continuation on the promise
    promise->waiting_coro = waiting_coro;
  }
}

void *moksha_rt_await_payload(void *promise_handle) {
  if (!promise_handle)
    return NULL;

  InternalPromise *promise = (InternalPromise *)promise_handle;
  return promise->result_data;
}

void *moksha_rt_make_resolved_promise(void *result_data) {
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), 20); // 20 = Promise Type ID
  promise->is_completed = true;
  promise->result_data = result_data;
  promise->waiting_coro = NULL;
  promise->coro_handle = NULL;
  return promise;
}

void *moksha_rt_block_on(void *promise_handle) {
  if (!promise_handle)
    return NULL;

  InternalPromise *promise = (InternalPromise *)promise_handle;

  // Spin the event loop synchronously until this specific promise finishes
  while (!promise->is_completed) {
    // If the queue is empty, wait for OS events
    moksha_scheduler_run();
  }

  return promise->result_data;
}
