#include "../../abi/sys_alloc.h"
#include <stdlib.h>

void *sys_alloc(size_t size) { return calloc(1, size); }
void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);
  return realloc(ptr, new_size);
}
void sys_free(void *ptr) {
  if (ptr)
    free(ptr);
}
