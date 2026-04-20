#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declare panic and memory functions
extern void moksha_rt_panic(const char *message);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);

// Fast heuristic to detect if a pointer lives on the stack.
// Stack memory is kept safe from ARC manipulation and free().
bool is_stack_ptr(void *ptr) {
  void *local_var = NULL;
  uintptr_t local_addr = (uintptr_t)&local_var;
  uintptr_t ptr_addr = (uintptr_t)ptr;
  uintptr_t diff =
      ptr_addr > local_addr ? ptr_addr - local_addr : local_addr - ptr_addr;
  return diff < 1 * 1024 * 1024;
}

void *moksha_rt_alloc(size_t payload_size, uint32_t type_id) {
  MokshaHeader *header =
      (MokshaHeader *)moksha_mem_alloc(sizeof(MokshaHeader) + payload_size);
  if (!header)
    moksha_rt_panic("OOM during ARC allocation");

  header->ref_count = 1;
  header->weak_count = 1;
  header->type_id = type_id;
  return (void *)(header + 1);
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
  if (!ptr || is_stack_ptr(ptr))
    return;

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

// Keep the original 1-argument version as a wrapper so we don't break
// internal C-runtime APIs (like map.c) that blindly release objects!
void moksha_rt_release(void *ptr) { moksha_rt_release_with_dtor(ptr, NULL); }

// This ensures that raw allocations requested by the compiler
// are compatible with the retain/release system.
void *__moksha_alloc(uint32_t size) { return moksha_rt_alloc((size_t)size, 0); }

void __moksha_free(void *ptr) {
  // Instead of raw free, use release to handle the header
  moksha_rt_release(ptr);
}

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
      return NULL; // Object is dying or dead

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
