#include "../../abi/sys_alloc.h"
#include <windows.h>

void *sys_alloc(size_t size) {
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);
  return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, new_size);
}

void sys_free(void *ptr) {
  if (ptr)
    HeapFree(GetProcessHeap(), 0, ptr);
}
