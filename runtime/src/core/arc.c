#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declare panic and memory functions
extern void moksha_rt_panic(const char *message);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);

#if defined(__MOKSHA_BAREMETAL__)
extern char _sstack[];
extern char _estack[];

bool is_stack_ptr(void *ptr) {
  char *p = (char *)ptr;
  /** @brief Stack typically grows downwards, so _sstack is the lowest address
   * and _estack is the highest address. */
  return (p >= _sstack && p <= _estack);
}

#else
// On a Host OS (Windows/Linux/Darwin), dynamic stacks and coroutines make
// absolute address bounds impossible. We use a 1MB proximity heuristic. If the
// pointer is within 1MB of the current stack frame, we assume it is a stack
// allocation.
#include <stddef.h>

bool is_stack_ptr(void *ptr) {
  if (!ptr)
    return false;

  // 1. Take the address of a local variable to get the current stack pointer
  int local_sp_marker = 0;
  char *current_sp = (char *)&local_sp_marker;
  char *target_ptr = (char *)ptr;

  // 2. Calculate the absolute distance between the pointer and our current
  // stack frame
  ptrdiff_t distance = (current_sp > target_ptr) ? (current_sp - target_ptr)
                                                 : (target_ptr - current_sp);

  // 3. Return true if it is within a 1MB (1 * 1024 * 1024) threshold
  return distance < (1024 * 1024);
}
#endif

void *moksha_rt_alloc(size_t payload_size, uint32_t type_id) {
  MokshaHeader *header =
      (MokshaHeader *)moksha_mem_alloc(sizeof(MokshaHeader) + payload_size);
  if (!header)
    moksha_rt_panic("OOM during ARC allocation");

  header->ref_count = 1;
  header->weak_count = 1;
  header->type_id = type_id;
  header->capacity_bytes = (uint32_t)payload_size;

  void *payload = (void *)(header + 1);

  char *p = (char *)payload;
  for (size_t i = 0; i < payload_size; i++) {
    p[i] = 0;
  }

  return payload;
}

void moksha_rt_retain(void *ptr) {
  if (!ptr || is_stack_ptr(ptr))
    return;

  MokshaHeader *header = ((MokshaHeader *)ptr) - 1;

  if (sys_get_caps()->has_threads) {
    __atomic_add_fetch(&header->ref_count, 1, __ATOMIC_RELAXED);
  } else {
    header->ref_count += 1;
  }
}

void moksha_rt_release_with_dtor(void *ptr, void (*dtor)(void *)) {
  if (!ptr)
    return;

  if (is_stack_ptr(ptr)) {
    if (dtor)
      dtor(ptr);
    return;
  }

  MokshaHeader *header = ((MokshaHeader *)ptr) - 1;
  uint32_t new_strong;

  if (sys_get_caps()->has_threads) {
    new_strong = __atomic_sub_fetch(&header->ref_count, 1, __ATOMIC_ACQ_REL);
  } else {
    if (header->ref_count == 0)
      moksha_rt_panic("ARC double free!");
    new_strong = --header->ref_count;
  }

  if (new_strong == (uint32_t)-1)
    moksha_rt_panic("ARC underflow!");

  if (new_strong == 0) {
    if (dtor)
      dtor(ptr);

    // Built-in Type Destructors
    if (header->type_id == MOKSHA_TYPE_PROMISE) {
      typedef struct {
        void *coro_handle;
        bool is_completed;
        void *result_data;
        void *waiting_coro;
        bool is_rejected;
        bool was_awaited;
      } PromiseLayout;

      PromiseLayout *prom = (PromiseLayout *)ptr;

      // THE TICKING TIMEBOMB DETONATOR
      if (prom->is_rejected && !prom->was_awaited) {
        moksha_rt_panic("Unhandled Promise Rejection: An async function threw "
                        "an exception that was never awaited!");
      }
    } else if (header->type_id == MOKSHA_TYPE_ARRAY) {
    } else if (header->type_id == MOKSHA_TYPE_TABLE) {
      extern void moksha_rt_map_free_internal(void *map_ptr);
      moksha_rt_map_free_internal(ptr);
    }

    uint32_t new_weak;
    if (sys_get_caps()->has_threads) {
      new_weak = __atomic_sub_fetch(&header->weak_count, 1, __ATOMIC_ACQ_REL);
    } else {
      new_weak = --header->weak_count;
    }

    if (new_weak == 0) {
      moksha_mem_free(header);
    }
  }
}

void moksha_rt_release(void *ptr) { moksha_rt_release_with_dtor(ptr, NULL); }

void *__moksha_alloc(uint32_t size, uint32_t type_id) {
  return moksha_rt_alloc((size_t)size, type_id);
}

void __moksha_free(void *ptr) { moksha_rt_release(ptr); }

void moksha_rt_store_weak(void **dest, void *obj) {
  if (!dest)
    return;

  if (obj) {
    MokshaHeader *new_header = ((MokshaHeader *)obj) - 1;
    if (sys_get_caps()->has_threads) {
      __atomic_add_fetch(&new_header->weak_count, 1, __ATOMIC_RELAXED);
    } else {
      new_header->weak_count++;
    }
  }

  void *old_obj;
  if (sys_get_caps()->has_threads) {
    old_obj = __atomic_exchange_n(dest, obj, __ATOMIC_SEQ_CST);
  } else {
    old_obj = *dest;
    *dest = obj;
  }

  if (old_obj) {
    MokshaHeader *old_header = ((MokshaHeader *)old_obj) - 1;
    uint32_t remaining_weak;
    if (sys_get_caps()->has_threads) {
      remaining_weak =
          __atomic_sub_fetch(&old_header->weak_count, 1, __ATOMIC_ACQ_REL);
    } else {
      remaining_weak = --old_header->weak_count;
    }

    if (remaining_weak == 0) {
      moksha_mem_free(old_header);
    }
  }
}

void *moksha_rt_load_weak(void **src) {
  if (!src)
    return NULL;

  while (true) {
    void *obj;
    if (sys_get_caps()->has_threads) {
      obj = __atomic_load_n(src, __ATOMIC_SEQ_CST);
    } else {
      obj = *src;
    }

    if (!obj)
      return NULL;

    MokshaHeader *header = ((MokshaHeader *)obj) - 1;
    uint32_t count;
    if (sys_get_caps()->has_threads) {
      count = __atomic_load_n(&header->ref_count, __ATOMIC_RELAXED);
    } else {
      count = header->ref_count;
    }

    if (count == 0)
      return NULL;

    if (sys_get_caps()->has_threads) {
      if (__atomic_compare_exchange_n(&header->ref_count, &count, count + 1,
                                      false, __ATOMIC_ACQ_REL,
                                      __ATOMIC_RELAXED)) {
        return obj;
      }
    } else {
      header->ref_count++;
      return obj;
    }
  }
}

int32_t __moksha_get_type(void *ptr) {
  if (!ptr) {
    return 19;
  }

  MokshaHeader *header = ((MokshaHeader *)ptr) - 1;
  return (int32_t)header->type_id;
}
