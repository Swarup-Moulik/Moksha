#include <stddef.h>
#include <stdint.h>

// Declare the Moksha heap functions we just wrote
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);

void *memset(void *s, int c, size_t n);

void *sys_alloc(size_t size) {
  void *ptr = kmalloc(size);
  if (ptr) {
    memset(ptr, 0, size); // Moksha expects memory to be zeroed
  }
  return ptr;
}

void sys_free(void *ptr) {
  kfree(ptr); // The memory leak is finally fixed!
}

void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);
  if (new_size == 0) {
    sys_free(ptr);
    return NULL;
  }
  // Simplest approach for now: allocate a new block.
  // ARC handles copying elements underneath.
  return kmalloc(new_size);
}
