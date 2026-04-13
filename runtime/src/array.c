#include "../include/moksha_rt.h"
#include <stdlib.h>
#include <string.h>

void *moksha_rt_array_alloc(size_t element_size, uint64_t capacity) {
  size_t total_size = sizeof(MokshaHeader) + (element_size * capacity);
  MokshaHeader *obj = (MokshaHeader *)malloc(total_size);
  if (!obj)
    moksha_rt_panic("Out of memory during array allocation", __FILE__,
                    __LINE__);

  obj->ref_count = 1;
  obj->type_id = 0;
  return (void *)(obj + 1);
}

int32_t moksha_rt_array_length(MokshaSlice *slice) {
  if (!slice)
    return 0;
  return (int32_t)slice->length;
}

void *moksha_rt_array_at(MokshaSlice *slice, int32_t index,
                         size_t element_size) {
  if (!slice || !slice->data)
    moksha_rt_panic("Null pointer exception: Array is uninitialized", __FILE__,
                    __LINE__);
  if (index < 0 || (uint64_t)index >= slice->length)
    moksha_rt_panic("Index out of bounds exception", __FILE__, __LINE__);

  char *byte_ptr = (char *)slice->data;
  return (void *)(byte_ptr + (index * element_size));
}

void moksha_rt_array_push(MokshaSlice *slice, void *value_ptr,
                          size_t element_size) {
  if (!slice)
    moksha_rt_panic("Cannot push to null slice", __FILE__, __LINE__);

  void *new_data = realloc(slice->data, (slice->length + 1) * element_size);
  if (!new_data)
    moksha_rt_panic("Out of memory during array push", __FILE__, __LINE__);

  char *dest = (char *)new_data + (slice->length * element_size);
  memcpy(dest, value_ptr, element_size);

  slice->data = new_data;
  slice->length += 1;
}

void *moksha_rt_array_pop(MokshaSlice *slice, size_t element_size) {
  if (!slice || slice->length == 0)
    moksha_rt_panic("Cannot pop from empty array", __FILE__, __LINE__);

  slice->length -= 1;
  char *byte_ptr = (char *)slice->data;
  return (void *)(byte_ptr + (slice->length * element_size));
}
