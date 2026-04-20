#include "../abi/sys_alloc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// External panic handler
extern void moksha_rt_panic(const char *message);

// Bring in the ARC allocator
extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern bool is_stack_ptr(void *ptr);

void *moksha_mem_alloc(size_t size) {
  if (size == 0)
    return NULL;

  void *ptr = sys_alloc(size);
  if (!ptr) {
    moksha_rt_panic("Fatal: Out of Memory");
  }

  return ptr;
}

void *moksha_mem_realloc(void *ptr, size_t new_size) {
  if (new_size == 0) {
    sys_free(ptr);
    return NULL;
  }

  void *new_ptr = sys_realloc(ptr, new_size);
  if (!new_ptr) {
    moksha_rt_panic("Fatal: Out of Memory during realloc");
  }

  return new_ptr;
}

void moksha_mem_free(void *ptr) {
  // Discard nulls AND prevent fatal crashes from stack-allocated slice literals
  if (!ptr || is_stack_ptr(ptr)) {
    return;
  }
  sys_free(ptr);
}
