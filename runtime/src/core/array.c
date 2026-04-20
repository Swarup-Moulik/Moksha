#include "../../include/moksha_rt.h"
#include <string.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);
extern void *moksha_mem_alloc(size_t size);
extern void *moksha_mem_realloc(void *ptr, size_t new_size);

// Hidden buffer structure to track capacity separate from the Slice's length
typedef struct {
  uint64_t capacity;
  uint8_t data[]; // Flexible array member
} ArrayBuffer;

void *moksha_rt_array_alloc(size_t element_size, uint64_t capacity) {
  MokshaSlice *slice =
      (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), MOKSHA_TYPE_ARRAY);

  size_t buf_size = sizeof(ArrayBuffer) + (element_size * capacity);
  ArrayBuffer *buffer =
      (ArrayBuffer *)moksha_mem_alloc(buf_size); // Use wrapper!

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
    memcpy(dest, src, bytes);
  }
}

// Push/Pop would involve checking `buffer->capacity` (recovered via pointer
// math: ArrayBuffer* buffer = (ArrayBuffer*)slice->data - 1;) and calling
// sys_realloc if needed.
