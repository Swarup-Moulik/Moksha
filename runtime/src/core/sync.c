#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sched.h>
#endif

// 1. INCREASE TABLE SIZE to reduce collision probability
#define LOCK_TABLE_SIZE 4096

// 2. CACHE LINE PADDING to prevent False Sharing (64 bytes is standard)
typedef struct __attribute__((aligned(64))) {
  int locked;
  // The remaining 60 bytes are padding to fill the cache line
} PaddedLock;

static PaddedLock lock_table[LOCK_TABLE_SIZE] = {0};

// CPU-specific Pause instruction to prevent pipeline starvation
void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
  __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
  __asm__ volatile("yield" ::: "memory");
#elif defined(__riscv)
  __asm__ volatile("pause" ::: "memory");
#endif
}

// OS-specific Thread Yield
static inline void thread_yield() {
#if defined(_WIN32)
  Sleep(0); // Yield to another thread of equal priority
#elif defined(__linux__) || defined(__APPLE__)
  sched_yield();
#else
  // Bare-metal fallback: just relax the CPU, no OS scheduler exists
  cpu_relax();
#endif
}

extern void *moksha_rt_make_unresolved_promise(void);
extern void moksha_rt_resolve_promise(void *promise_handle, void *result_data);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);

static inline uint32_t hash_ptr(void *ptr) {
  uintptr_t val = (uintptr_t)ptr;
  val ^= val >> 16;
  val *= 0x85ebca6b;
  val ^= val >> 13;
  val *= 0xc2b2ae35;
  val ^= val >> 16;
  return val % LOCK_TABLE_SIZE;
}

void __moksha_lock(void *ptr) {
  if (!ptr)
    return;
  if (!sys_get_caps()->has_threads)
    return;

  uint32_t idx = hash_ptr(ptr);
  int spin_count = 0;

  // 3. EXPONENTIAL BACKOFF SPINLOCK
  while (__atomic_exchange_n(&lock_table[idx].locked, 1, __ATOMIC_ACQUIRE)) {
    if (spin_count < 100) {
      // First 100 tries: just pause the CPU lightly
      cpu_relax();
      spin_count++;
    } else {
      // If it's taking too long, the lock-holder was likely preempted by the
      // OS. Stop burning CPU and yield our timeslice to the OS.
      thread_yield();
    }
  }
}

void __moksha_unlock(void *ptr) {
  if (!ptr)
    return;
  if (!sys_get_caps()->has_threads)
    return;

  uint32_t idx = hash_ptr(ptr);
  __atomic_store_n(&lock_table[idx].locked, 0, __ATOMIC_RELEASE);
}

void *AsyncMutex_new(void) {
  void *ptr = moksha_rt_alloc(sizeof(MokshaAsyncMutex), MOKSHA_TYPE_MUTEX);
  MokshaAsyncMutex *mtx = (MokshaAsyncMutex *)ptr;
  mtx->spin_lock = 0;
  mtx->waiters_head = NULL;
  mtx->waiters_tail = NULL;
  mtx->is_locked = false;
  return ptr;
}

void *AsyncMutex_lock(void *this_ptr) {
  MokshaAsyncMutex *mtx = (MokshaAsyncMutex *)this_ptr;
  void *promise = moksha_rt_make_unresolved_promise();

  while (__atomic_exchange_n(&mtx->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  if (!mtx->is_locked) {
    mtx->is_locked = true;
    moksha_rt_resolve_promise(promise, NULL); // Available immediately
  } else {
    // Lock is taken. Enqueue the promise to be woken up later!
    AsyncMutexWaitNode *node =
        (AsyncMutexWaitNode *)moksha_mem_alloc(sizeof(AsyncMutexWaitNode));
    node->promise_handle = promise;
    node->next = NULL;
    if (!mtx->waiters_head) {
      mtx->waiters_head = node;
      mtx->waiters_tail = node;
    } else {
      mtx->waiters_tail->next = node;
      mtx->waiters_tail = node;
    }
  }

  __atomic_store_n(&mtx->spin_lock, 0, __ATOMIC_RELEASE);
  return promise;
}

void AsyncMutex_unlock(void *this_ptr) {
  MokshaAsyncMutex *mtx = (MokshaAsyncMutex *)this_ptr;

  while (__atomic_exchange_n(&mtx->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  if (mtx->waiters_head) {
    // Transfer the lock directly to the next coroutine in the queue
    AsyncMutexWaitNode *node = mtx->waiters_head;
    mtx->waiters_head = node->next;
    if (!mtx->waiters_head)
      mtx->waiters_tail = NULL;

    // Resolve their promise, notifying the scheduler to wake them up
    moksha_rt_resolve_promise(node->promise_handle, NULL);
    moksha_mem_free(node);
  } else {
    mtx->is_locked = false;
  }

  __atomic_store_n(&mtx->spin_lock, 0, __ATOMIC_RELEASE);
}

void AsyncMutex_destructor(void *this_ptr) {
  MokshaAsyncMutex *mtx = (MokshaAsyncMutex *)this_ptr;
  while (__atomic_exchange_n(&mtx->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  // Free any dangling memory if the mutex is destroyed while tasks are waiting
  AsyncMutexWaitNode *curr = mtx->waiters_head;
  while (curr) {
    AsyncMutexWaitNode *next = curr->next;
    moksha_mem_free(curr);
    curr = next;
  }
  __atomic_store_n(&mtx->spin_lock, 0, __ATOMIC_RELEASE);
}
