#include "../../include/moksha_rt.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

// ============================================================================
// Internal Resizing Helper
// ============================================================================
static ArrayBuffer *get_buffer(MokshaSlice *slice) {
  return (ArrayBuffer *)((char *)slice->data - sizeof(uint64_t));
}

static void ensure_capacity(MokshaSlice *slice, uint64_t required_cap,
                            size_t element_size) {
  if (!slice)
    return;
  ArrayBuffer *buffer = get_buffer(slice);
  if (buffer->capacity >= required_cap)
    return;

  uint64_t new_cap = buffer->capacity == 0 ? 4 : buffer->capacity * 2;
  if (new_cap < required_cap)
    new_cap = required_cap;

  size_t new_size = sizeof(ArrayBuffer) + (element_size * new_cap);
  ArrayBuffer *new_buffer = (ArrayBuffer *)moksha_mem_realloc(buffer, new_size);
  if (!new_buffer)
    moksha_rt_panic("OOM: Failed to resize array buffer");

  new_buffer->capacity = new_cap;
  slice->data = new_buffer->data;
}

// ============================================================================
// Extended Array Operations
// ============================================================================

bool moksha_rt_array_is_empty(MokshaSlice *slice) {
  return !slice || slice->length == 0;
}

int32_t moksha_rt_array_capacity(MokshaSlice *slice) {
  if (!slice)
    return 0;
  return (int32_t)get_buffer(slice)->capacity;
}

void moksha_rt_array_clear(MokshaSlice *slice) {
  if (slice)
    slice->length = 0;
}

void moksha_rt_array_push(MokshaSlice *slice, void *value_ptr,
                          size_t element_size) {
  ensure_capacity(slice, slice->length + 1, element_size);
  uint8_t *dest = (uint8_t *)slice->data + (slice->length * element_size);
  internal_memcpy(dest, value_ptr, element_size);
  slice->length++;
}

void *moksha_rt_array_pop(MokshaSlice *slice, size_t element_size) {
  if (!slice || slice->length == 0)
    moksha_rt_panic("Cannot pop from an empty array");

  slice->length--;
  uint8_t *src = (uint8_t *)slice->data + (slice->length * element_size);

  // Allocate a heap copy to safely return it to the Any unboxer
  void *copy = moksha_mem_alloc(element_size);
  internal_memcpy(copy, src, element_size);
  return copy;
}

void moksha_rt_array_insert(MokshaSlice *slice, int32_t index, void *value_ptr,
                            size_t element_size) {
  if (!slice || index < 0 || (uint64_t)index > slice->length)
    moksha_rt_panic("Insert index out of bounds");

  ensure_capacity(slice, slice->length + 1, element_size);
  uint8_t *raw_data = (uint8_t *)slice->data;

  // Shift elements right
  size_t bytes_to_move = (slice->length - index) * element_size;
  uint8_t *insert_pos = raw_data + (index * element_size);

  if (bytes_to_move > 0) {
    // Must use a safe memmove-like shift to handle overlapping regions natively
    for (int i = bytes_to_move - 1; i >= 0; i--) {
      insert_pos[i + element_size] = insert_pos[i];
    }
  }

  internal_memcpy(insert_pos, value_ptr, element_size);
  slice->length++;
}

void *moksha_rt_array_remove(MokshaSlice *slice, int32_t index,
                             size_t element_size) {
  if (!slice || index < 0 || (uint64_t)index >= slice->length)
    moksha_rt_panic("Remove index out of bounds");

  uint8_t *raw_data = (uint8_t *)slice->data;
  uint8_t *remove_pos = raw_data + (index * element_size);

  void *copy = moksha_mem_alloc(element_size);
  internal_memcpy(copy, remove_pos, element_size);

  size_t bytes_to_move = (slice->length - index - 1) * element_size;
  if (bytes_to_move > 0) {
    internal_memcpy(remove_pos, remove_pos + element_size, bytes_to_move);
  }

  slice->length--;
  return copy;
}

void moksha_rt_array_extend(MokshaSlice *dest, MokshaSlice *src,
                            size_t element_size) {
  if (!dest || !src || src->length == 0)
    return;

  ensure_capacity(dest, dest->length + src->length, element_size);
  uint8_t *target = (uint8_t *)dest->data + (dest->length * element_size);
  internal_memcpy(target, src->data, src->length * element_size);
  dest->length += src->length;
}

// ============================================================================
// Array Copy (Raw Blit)
// ============================================================================
void moksha_rt_array_copy(MokshaSlice *dest, MokshaSlice *src,
                          size_t element_size) {
  if (!dest || !src || dest->length == 0 || src->length == 0) {
    return;
  }

  // Safety: Only copy up to the capacity of the smallest slice to prevent
  // buffer overflows
  uint64_t copy_len = dest->length < src->length ? dest->length : src->length;

  internal_memcpy(dest->data, src->data, copy_len * element_size);
}

// ============================================================================
// Array Clone (Deep Copy Allocation)
// ============================================================================
void *moksha_rt_array_clone(MokshaSlice *src, size_t element_size) {
  if (!src) {
    return NULL;
  }

  // Allocate a brand new array using your existing runtime allocator
  MokshaSlice *new_slice =
      (MokshaSlice *)moksha_rt_array_alloc(element_size, src->length);
  new_slice->length = src->length;

  // Deep copy the data over
  if (src->length > 0) {
    internal_memcpy(new_slice->data, src->data, src->length * element_size);
  }

  return new_slice;
}

void *moksha_rt_array_slice(MokshaSlice *slice, int32_t start, int32_t end,
                            size_t element_size) {
  if (!slice)
    return NULL;
  if (start < 0)
    start = 0;
  if ((uint64_t)end > slice->length)
    end = slice->length;
  if (start >= end)
    return moksha_rt_array_alloc(element_size, 0); // Empty

  uint64_t new_len = end - start;
  MokshaSlice *new_slice =
      (MokshaSlice *)moksha_rt_array_alloc(element_size, new_len);

  uint8_t *src_start = (uint8_t *)slice->data + (start * element_size);
  internal_memcpy(new_slice->data, src_start, new_len * element_size);
  new_slice->length = new_len;
  return new_slice;
}

void moksha_rt_array_fill(MokshaSlice *slice, void *value_ptr,
                          size_t element_size) {
  if (!slice || slice->length == 0)
    return;
  uint8_t *target = (uint8_t *)slice->data;
  for (uint64_t i = 0; i < slice->length; i++) {
    internal_memcpy(target + (i * element_size), value_ptr, element_size);
  }
}

void moksha_rt_array_reverse(MokshaSlice *slice, size_t element_size) {
  if (!slice || slice->length <= 1)
    return;
  uint8_t *raw = (uint8_t *)slice->data;
  uint8_t *temp = (uint8_t *)moksha_mem_alloc(element_size);

  uint64_t left = 0;
  uint64_t right = slice->length - 1;

  while (left < right) {
    uint8_t *l_ptr = raw + (left * element_size);
    uint8_t *r_ptr = raw + (right * element_size);

    internal_memcpy(temp, l_ptr, element_size);
    internal_memcpy(l_ptr, r_ptr, element_size);
    internal_memcpy(r_ptr, temp, element_size);

    left++;
    right--;
  }
  // Free temp if you have a free function, otherwise GC will catch it depending
  // on your alloc config
}

int32_t moksha_rt_array_index(MokshaSlice *slice, void *value_ptr,
                              size_t element_size) {
  if (!slice || slice->length == 0)
    return -1;
  uint8_t *raw = (uint8_t *)slice->data;

  for (uint64_t i = 0; i < slice->length; i++) {
    uint8_t *curr = raw + (i * element_size);
    bool match = true;
    for (size_t b = 0; b < element_size; b++) {
      if (curr[b] != ((uint8_t *)value_ptr)[b]) {
        match = false;
        break;
      }
    }
    if (match)
      return (int32_t)i;
  }
  return -1;
}

bool moksha_rt_array_contains(MokshaSlice *slice, void *value_ptr,
                              size_t element_size) {
  return moksha_rt_array_index(slice, value_ptr, element_size) >= 0;
}

void moksha_rt_array_resize(MokshaSlice *slice, int32_t new_len,
                            size_t element_size) {
  if (!slice || new_len < 0)
    return;

  uint64_t old_len = slice->length;

  if ((uint64_t)new_len > old_len) {
    ensure_capacity(slice, new_len, element_size);

    // Explicitly zero out the newly allocated space to prevent garbage values
    uint8_t *raw = (uint8_t *)slice->data;
    uint8_t *start_ptr = raw + (old_len * element_size);
    size_t bytes_to_clear = (new_len - old_len) * element_size;

    for (size_t i = 0; i < bytes_to_clear; i++) {
      start_ptr[i] = 0;
    }
  }

  slice->length = new_len;
}

void moksha_rt_array_sort(MokshaSlice *slice, size_t element_size) {
  if (!slice || slice->length <= 1)
    return;

  uint8_t *base = (uint8_t *)slice->data;
  // Temporary buffer for swapping
  uint8_t *temp = (uint8_t *)moksha_mem_alloc(element_size);

  for (uint64_t i = 1; i < slice->length; i++) {
    // Copy current element to temp
    internal_memcpy(temp, base + (i * element_size), element_size);

    int64_t j = (int64_t)i - 1;

    // Insertion sort: Move elements that are greater than temp to one position
    // ahead
    while (j >= 0 &&
           memcmp(base + (j * element_size), temp, element_size) > 0) {
      internal_memcpy(base + ((j + 1) * element_size),
                      base + (j * element_size), element_size);
      j--;
    }

    // Place temp in its correct position
    internal_memcpy(base + ((j + 1) * element_size), temp, element_size);
  }
}
