#include "../../include/moksha_rt.h"
#include "../abi/sys_thread.h"
#include <stddef.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_scheduler_schedule(void *coro_handle);
extern void moksha_scheduler_run(void);
extern void moksha_rt_panic(const char *message);
extern bool moksha_is_background_task;
extern void cpu_relax(void);
extern void *moksha_rt_array_alloc(size_t element_size, uint64_t capacity);
extern void moksha_scheduler_inc_active(void);
extern void moksha_scheduler_dec_active(void);
extern void moksha_rt_retain(void *ptr);
extern void moksha_scheduler_set_priority(void *coro_handle, int32_t priority);
extern void moksha_scheduler_remove_priority(void *coro_handle);
extern void moksha_rt_release(void *ptr);
extern __thread int32_t current_spawn_priority;
__thread void *__moksha_ex_payload = NULL;

// Internal representation of a Promise (which is an ARC object itself)
typedef struct {
  void *coro_handle;
  bool is_completed;
  void *result_data;
  void *waiting_coro;
  bool is_rejected;
  bool was_awaited;
} InternalPromise;

// Context passed to strong OS threads so they can resolve their promises
typedef struct {
  MokshaClosure closure;
  void *promise_handle;
} ThreadContext;

typedef struct {
  void *master_promise;
  void **results_array;
  int total;
  int completed;
  int spin_lock;
} JoinAllContext;

/** @brief Promise Resolution */

void moksha_rt_resolve_promise(void *promise_handle, void *result_data) {
  if (!promise_handle)
    return;
  InternalPromise *prom = (InternalPromise *)promise_handle;

  // 1. Atomically attempt to set is_completed to true.
  bool expected = false;
  if (!__atomic_compare_exchange_n(&prom->is_completed, &expected, true, false,
                                   __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
    return;
  }

  // 2. Safe to write the payload
  prom->result_data = result_data;

  // 3. Atomically steal the waiting coroutine and clear the pointer.
  void *waiter =
      __atomic_exchange_n(&prom->waiting_coro, NULL, __ATOMIC_ACQ_REL);
  if (waiter) {
    moksha_scheduler_schedule(waiter);
  }
}

/** @brief Thread Trampolines */

static void *strong_thread_trampoline(void *arg) {
  ThreadContext *ctx = (ThreadContext *)arg;

  // 1. Execute the user's closure
  void (*func)(void *) = (void (*)(void *))ctx->closure.function_ptr;
  func(ctx->closure.environment_ptr);

  // 2. Resolve the promise to wake up anyone awaiting this thread
  moksha_rt_resolve_promise(ctx->promise_handle, NULL);

  // 3. Free the context struct and let the scheduler know we are done!
  moksha_mem_free(ctx);
  moksha_scheduler_dec_active();
  return NULL;
}

static void *weak_thread_trampoline(void *arg) {
  MokshaClosure *closure = (MokshaClosure *)arg;
  void (*func)(void *) = (void (*)(void *))closure->function_ptr;
  func(closure->environment_ptr);
  return NULL;
}

/** @brief Spawners */

// Spawn an async closure (Cooperative Coroutine)
void *moksha_rt_spawn(MokshaClosure closure) {
  if (!closure.function_ptr) {
    moksha_rt_panic("Cannot spawn null closure");
  }

  // 1. Invoke the closure to start the coroutine and get the real promise frame
  void *(*coro_start)(void *) = (void *(*)(void *))closure.function_ptr;
  void *promise_handle = coro_start(closure.environment_ptr);

  return promise_handle;
}

void *moksha_rt_spawn_thread(void *closure_ptr) {
  if (!closure_ptr)
    moksha_rt_panic("Cannot spawn thread with null closure");

  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);
  promise->is_completed = false;
  promise->result_data = NULL;
  promise->waiting_coro = NULL;

  // Wrap the closure and promise in a context payload
  ThreadContext *ctx = (ThreadContext *)moksha_mem_alloc(sizeof(ThreadContext));
  ctx->closure = *(MokshaClosure *)closure_ptr;
  ctx->promise_handle = promise;

  // Tell the scheduler to stay alive for this strong thread!
  moksha_scheduler_inc_active();

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
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);
  promise->is_completed = true;
  promise->result_data = NULL;
  promise->coro_handle = (void *)thread;
  promise->waiting_coro = NULL;

  sys_thread_detach(thread);
  return promise;
}

/** @brief Await Hooks */

void moksha_rt_register_await(void *promise_handle, void *waiting_coro) {
  if (!promise_handle)
    return;
  InternalPromise *prom = (InternalPromise *)promise_handle;

  // Set the waiter
  __atomic_store_n(&prom->waiting_coro, waiting_coro, __ATOMIC_RELEASE);
  if (__atomic_load_n(&prom->is_completed, __ATOMIC_ACQUIRE)) {
    void *waiter =
        __atomic_exchange_n(&prom->waiting_coro, NULL, __ATOMIC_ACQ_REL);
    if (waiter) {
      moksha_scheduler_schedule(waiter);
    }
  }
}

void *moksha_rt_await_payload(void *promise_handle) {
  extern __thread void *__moksha_ex_payload;

  if (__moksha_ex_payload != NULL) {
    void *ex = __moksha_ex_payload;
    __moksha_ex_payload = NULL;
    moksha_rt_throw(ex);
  }

  InternalPromise *prom = (InternalPromise *)promise_handle;
  prom->was_awaited = true;

  if (prom->is_rejected) {
    moksha_rt_throw(prom->result_data);
  }
  return prom->result_data;
}

void *moksha_rt_make_resolved_promise(void *result_data) {
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);
  promise->is_completed = true;
  promise->result_data = result_data;
  promise->waiting_coro = NULL;
  promise->coro_handle = NULL;
  promise->is_rejected = false;
  promise->was_awaited = false;
  return promise;
}

void *moksha_rt_make_rejected_promise(void *ex_payload) {
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);

  promise->is_completed = true;      // A rejected promise is finished
  promise->result_data = ex_payload; // Store the exception here
  promise->waiting_coro = NULL;
  promise->coro_handle = NULL;
  promise->is_rejected = true;
  promise->was_awaited = false;
  if (ex_payload) {
    extern void moksha_rt_retain(void *ptr);
    moksha_rt_retain(ex_payload);
  }

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

void *moksha_rt_coro_setup(void *coro_handle) {
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);
  promise->is_completed = false;
  promise->result_data = NULL;
  promise->waiting_coro = NULL;
  promise->is_rejected = false;
  promise->was_awaited = false;
  promise->coro_handle = coro_handle;

  moksha_scheduler_set_priority(coro_handle, current_spawn_priority);
  moksha_scheduler_inc_active();
  return promise;
}

void moksha_rt_coro_finish(void *promise_handle, void *payload) {
  if (!promise_handle)
    return;
  InternalPromise *promise = (InternalPromise *)promise_handle;

  // Cleanup priority registry to prevent memory leaks
  moksha_scheduler_remove_priority(promise->coro_handle);

  if (payload) {
    uint32_t type_id = ((MokshaHeader *)payload - 1)->type_id;
    if (type_id == MOKSHA_TYPE_PROMISE) {
      InternalPromise *inner = (InternalPromise *)payload;
      if (inner->is_rejected) {
        promise->is_rejected = true;
      }
      promise->result_data = inner->result_data;
      inner->was_awaited = true;
      moksha_rt_release(inner);
    } else {
      promise->result_data = payload;
    }
  }

  promise->is_completed = true;
  if (promise->waiting_coro) {
    moksha_scheduler_schedule(promise->waiting_coro);
    promise->waiting_coro = NULL;
  }

  moksha_scheduler_dec_active();
}

void *moksha_rt_make_unresolved_promise(void) {
  InternalPromise *promise = (InternalPromise *)moksha_rt_alloc(
      sizeof(InternalPromise), MOKSHA_TYPE_PROMISE);
  promise->is_completed = false;
  promise->result_data = NULL;
  promise->waiting_coro = NULL;
  promise->coro_handle = NULL;
  promise->is_rejected = false;
  promise->was_awaited = false;
  return promise;
}

void *spawn_func(void *closure_ptr) {
  if (!closure_ptr)
    return NULL;
  return moksha_rt_spawn(*(MokshaClosure *)closure_ptr);
}

void *moksha_builtin_join(void *p1_handle, void *p2_handle) {
  if (p1_handle) {
    moksha_rt_block_on(p1_handle);
    ((InternalPromise *)p1_handle)->was_awaited = true;
  }
  if (p2_handle) {
    moksha_rt_block_on(p2_handle);
    ((InternalPromise *)p2_handle)->was_awaited = true;
  }

  void *data_buf = moksha_rt_alloc(8, 18);
  int32_t *int_data = (int32_t *)data_buf;
  if (p1_handle) {
    InternalPromise *p1 = (InternalPromise *)p1_handle;
    int_data[0] = (int32_t)(intptr_t)p1->result_data;
  } else {
    int_data[0] = 0;
  }

  if (p2_handle) {
    InternalPromise *p2 = (InternalPromise *)p2_handle;
    int_data[1] = (int32_t)(intptr_t)p2->result_data;
  } else {
    int_data[1] = 0;
  }

  MokshaSlice *slice = (MokshaSlice *)moksha_mem_alloc(sizeof(MokshaSlice));
  slice->data = data_buf;
  slice->length = 2;
  return moksha_rt_make_resolved_promise(slice);
}

void moksha_rt_join_all_callback(void *sub_result, void *ctx_ptr, int index) {
  JoinAllContext *ctx = (JoinAllContext *)ctx_ptr;

  while (__atomic_exchange_n(&ctx->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  ctx->results_array[index] = sub_result;
  ctx->completed++;

  if (ctx->completed == ctx->total) {
    void *data_buf = moksha_rt_alloc(ctx->total * 4, 18);
    int32_t *int_data = (int32_t *)data_buf;
    for (int i = 0; i < ctx->total; i++) {
      int_data[i] = (int32_t)(intptr_t)ctx->results_array[i];
    }

    MokshaSlice *final_array =
        (MokshaSlice *)moksha_mem_alloc(sizeof(MokshaSlice));
    final_array->data = data_buf;
    final_array->length = ctx->total;

    __atomic_store_n(&ctx->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_resolve_promise(ctx->master_promise, final_array);

    moksha_mem_free(ctx->results_array);
    moksha_mem_free(ctx);
  } else {
    __atomic_store_n(&ctx->spin_lock, 0, __ATOMIC_RELEASE);
  }
}

void moksha_rt_reject_promise(void *promise_handle, void *ex_payload) {
  if (!promise_handle)
    return;
  InternalPromise *prom = (InternalPromise *)promise_handle;

  bool expected = false;
  if (!__atomic_compare_exchange_n(&prom->is_completed, &expected, true, false,
                                   __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
    return;
  }

  prom->is_rejected = true;
  prom->result_data = ex_payload;

  if (ex_payload) {
    moksha_rt_retain(ex_payload);
  }

  void *waiter =
      __atomic_exchange_n(&prom->waiting_coro, NULL, __ATOMIC_ACQ_REL);
  if (waiter) {
    moksha_scheduler_schedule(waiter);
  }
}
