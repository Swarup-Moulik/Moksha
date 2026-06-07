#include "../../abi/sys_alloc.h"
#include <stdlib.h>

void *sys_alloc(size_t size) {
  // calloc guarantees zero-initialized memory, matching HEAP_ZERO_MEMORY
  return calloc(1, size);
}

void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);

  // Note: Standard realloc does not zero out newly allocated space
  // if the block is expanded. If your runtime strictly depends on
  // that, you will need to manually memset the new bytes here.
  return realloc(ptr, new_size);
}

void sys_free(void *ptr) {
  if (ptr)
    free(ptr);
}
