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
static inline void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
  __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
  __asm__ volatile("yield" ::: "memory");
#endif
}

// OS-specific Thread Yield
static inline void thread_yield() {
#if defined(_WIN32)
  Sleep(0); // Yield to another thread of equal priority
#elif defined(__linux__) || defined(__APPLE__)
  sched_yield();
#endif
}

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
