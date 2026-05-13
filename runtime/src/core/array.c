#include "../../include/moksha_rt.h"
#include <stdbool.h>
#include <stdint.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);
extern void *moksha_mem_alloc(size_t size);
extern void *moksha_mem_realloc(void *ptr, size_t new_size);

// Bare-metal memory copy
static void *internal_memcpy(void *dest, const void *src, size_t n) {
  char *d = (char *)dest;
  const char *s = (const char *)src;
  for (size_t i = 0; i < n; i++)
    d[i] = s[i];
  return dest;
}

// Hidden buffer structure to track capacity separate from the Slice's length
typedef struct {
  uint64_t capacity;
  uint8_t data[];
} ArrayBuffer;

void *moksha_rt_array_alloc(size_t element_size, uint64_t capacity) {
  MokshaSlice *slice =
      (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), MOKSHA_TYPE_ARRAY);

  size_t buf_size = sizeof(ArrayBuffer) + (element_size * capacity);
  ArrayBuffer *buffer = (ArrayBuffer *)moksha_mem_alloc(buf_size);

  if (!buffer)
    moksha_rt_panic("OOM: Failed to allocate array buffer");

  buffer->capacity = capacity;
  slice->data = buffer->data;
  slice->length = 0;

  return slice;
}

void *moksha_rt_array_at(MokshaSlice *slice, int32_t index,
                         size_t element_size) {
  if (!slice || index < 0 || (uint64_t)index >= slice->length) {
    moksha_rt_panic("Array index out of bounds");
  }

  uint8_t *raw_data = (uint8_t *)slice->data;
  return raw_data + (index * element_size);
}

int32_t moksha_rt_array_length(MokshaSlice *slice) {
  if (!slice)
    return 0;
  return (int32_t)slice->length;
}

// Used by the spread operator (...) to bulk-copy array chunks
void __moksha_array_copy(void *dest, void *src, uint32_t bytes) {
  if (dest && src && bytes > 0) {
    internal_memcpy(dest, src, bytes); // <-- Used here!
  }
}

// Structural Equality for Arrays and Slices
bool __moksha_array_eq(void *a_ptr, int32_t a_len, void *b_ptr, int32_t b_len,
                       int32_t elem_size) {
  // 1. O(1) Fast-path: Check lengths
  if (a_len != b_len)
    return false;
  if (a_len == 0)
    return true;

  // 2. O(1) Fast-path: Check identity
  if (a_ptr == b_ptr)
    return true;
  if (!a_ptr || !b_ptr)
    return false;

  // 3. O(N) Deep memory comparison
  char *a = (char *)a_ptr;
  char *b = (char *)b_ptr;
  size_t total_bytes = (size_t)a_len * (size_t)elem_size;

  for (size_t i = 0; i < total_bytes; i++) {
    if (a[i] != b[i])
      return false;
  }

  return true;
}
