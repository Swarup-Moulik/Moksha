#include "abi/sys_alloc.h"
#include <stdint.h>

// These symbols are provided by the platform-specific linker script (linker.ld)
extern char _heap_start;
extern char _heap_end;

static char *heap_curr = &_heap_start;

void *sys_alloc(size_t size) {
  // Align allocations to 8-byte boundaries
  size = (size + 7) & ~7;

  if (heap_curr + size > &_heap_end) {
    return NULL; // Out of memory
  }

  void *ptr = heap_curr;
  heap_curr += size;

  // Zero-initialize the memory
  char *p = (char *)ptr;
  for (size_t i = 0; i < size; i++) {
    p[i] = 0;
  }

  return ptr;
}

void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);
  if (new_size == 0)
    return NULL;
  return sys_alloc(new_size);
}

void sys_free(void *ptr) { (void)ptr; }
