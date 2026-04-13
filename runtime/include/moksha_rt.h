#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core Memory Model
// ============================================================================
typedef struct {
  uint32_t ref_count;
  uint32_t type_id;
} MokshaHeader;

typedef struct {
  void *data;
  uint64_t length;
} MokshaSlice;

typedef struct {
  char *chars;
  uint64_t length;
} MokshaString;

typedef struct {
  void *function_ptr;
  void *environment_ptr;
} MokshaClosure;

typedef struct {
  void *data;
  uint32_t type_id;
} MokshaAny;

typedef enum {
  MOKSHA_TYPE_BOOL = 0,
  MOKSHA_TYPE_I8 = 1,  // signed char
  MOKSHA_TYPE_U8 = 2,  // unsigned char
  MOKSHA_TYPE_I16 = 3, // short
  MOKSHA_TYPE_U16 = 4, // unsigned short
  MOKSHA_TYPE_I32 = 5, // int
  MOKSHA_TYPE_U32 = 6, // unsigned int
  MOKSHA_TYPE_I64 = 7, // long
  MOKSHA_TYPE_U64 = 8, // unsigned long
  MOKSHA_TYPE_ISIZE = 9,
  MOKSHA_TYPE_USIZE = 10,
  MOKSHA_TYPE_F8 = 11,  // quarter
  MOKSHA_TYPE_F16 = 12, // half
  MOKSHA_TYPE_F32 = 13, // float
  MOKSHA_TYPE_F64 = 14, // long float
  MOKSHA_TYPE_DECIMAL = 15,
  MOKSHA_TYPE_STRING = 16,
  MOKSHA_TYPE_TABLE = 17
} MokshaTypeID;

// ============================================================================
// ARC (Automatic Reference Counting)
// ============================================================================
void moksha_rt_retain(void *ptr);
void moksha_rt_release(void *ptr);

// ============================================================================
// Arrays & Slices
// ============================================================================
void *moksha_rt_array_alloc(size_t element_size, uint64_t capacity);
void moksha_rt_array_push(MokshaSlice *slice, void *value_ptr,
                          size_t element_size);
void *moksha_rt_array_pop(MokshaSlice *slice, size_t element_size);
int32_t moksha_rt_array_length(MokshaSlice *slice);
void *moksha_rt_array_at(MokshaSlice *slice, int32_t index,
                         size_t element_size);

// ============================================================================
// Strings & I/O
// ============================================================================
int32_t moksha_rt_string_length(MokshaString *str);
char moksha_rt_string_at(MokshaString *str, int32_t index);
void moksha_rt_print(MokshaAny *any_val);
void moksha_rt_println(MokshaAny *any_val);
MokshaString *moksha_rt_readFile(MokshaAny *file_val);
void moksha_rt_close(MokshaAny *file_val);

// ============================================================================
// Concurrency
// ============================================================================
void *moksha_rt_spawn(MokshaClosure closure);
void *moksha_rt_await(void *promise_handle);

// ============================================================================
// Error Handling
// ============================================================================
void moksha_rt_panic(const char *message, const char *file, uint32_t line);

#ifdef __cplusplus
}
#endif
