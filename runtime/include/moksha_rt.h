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
  uint32_t ref_count;  // Strong references
  uint32_t weak_count; // Weak references
  uint32_t type_id;    // Metadata
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

// typedef struct {
//   void *data;
//   uint32_t type_id;
// } MokshaAny;

typedef struct {
  __int128 mantissa; // 128-bit signed integer
  int32_t scale;     // 32-bit signed scale
} MokshaDecimal;

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
  MOKSHA_TYPE_F64 = 14, // double
  MOKSHA_TYPE_DECIMAL = 15,
  MOKSHA_TYPE_STRING = 16,
  MOKSHA_TYPE_TABLE = 17,
  MOKSHA_TYPE_ARRAY = 18,
  MOKSHA_TYPE_POINTER = 19,
  MOKSHA_TYPE_PROMISE = 20
} MokshaTypeID;

// ============================================================================
// I/O & Parsing Intrinsics
// ============================================================================
char *__moksha_input(char *prompt);

// New Type-Specific Parsers
int64_t __moksha_parse_int(const char *input_str);
uint64_t __moksha_parse_uint(const char *input_str);
double __moksha_parse_float(const char *input_str);
bool __moksha_parse_bool(const char *input_str);
char __moksha_parse_char(const char *input_str);
MokshaDecimal __moksha_parse_decimal(const char *input_str);

// ============================================================================
// ARC (Automatic Reference Counting)
// ============================================================================
void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
void moksha_rt_retain(void *ptr);
void moksha_rt_release_with_dtor(void *ptr, void (*dtor)(void *));
void moksha_rt_release(void *ptr);
void moksha_rt_store_weak(void **dest, void *obj);
void *moksha_rt_load_weak(void **src);

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

void print(void *any_ptr, ...);
void println(void *any_ptr, ...);
MokshaString *moksha_rt_readFile(void *any_ptr);
void moksha_rt_close(void *any_ptr);

// String Allocation & Concatenation (Updated to raw char*)
char *__moksha_string_concat(char *a, char *b);
char *__moksha_template_join_strs(int32_t count, ...);

// Type to String Conversions (Updated to raw char*)
char *__moksha_bool_to_string(bool val);
char *__moksha_i8_to_string(int8_t val);
char *__moksha_u8_to_string(uint8_t val);
char *__moksha_i16_to_string(int16_t val);
char *__moksha_u16_to_string(uint16_t val);
char *__moksha_i32_to_string(int32_t val);
char *__moksha_u32_to_string(uint32_t val);
char *__moksha_i64_to_string(int64_t val);
char *__moksha_u64_to_string(uint64_t val);
char *__moksha_isize_to_string(intptr_t val);
char *__moksha_usize_to_string(size_t val);
char *__moksha_f32_to_string(float val);
char *__moksha_f64_to_string(double val);
char *__moksha_f8_to_string(uint16_t val);
char *__moksha_f16_to_string(uint16_t val);
char *__moksha_decimal_to_string(__int128_t value, int32_t scale);
char *__moksha_ptr_to_string(void *ptr);
char *__moksha_cstr_to_string(char *cstr);

// ============================================================================
// Error Handling
// ============================================================================
void moksha_rt_panic(const char *message);

// ============================================================================
// Decimal Math Functions
// ============================================================================
void __moksha_dec_scale(MokshaDecimal *out, MokshaDecimal *dec,
                        int32_t target_scale);
void __moksha_dec_add(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b);
void __moksha_dec_sub(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b);
void __moksha_dec_mul(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b);
void __moksha_dec_div(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b);
int32_t __moksha_dec_cmp(MokshaDecimal *a, MokshaDecimal *b);
void __moksha_dec_mod(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b);

// ============================================================================
// Tables & Maps
// ============================================================================
void *moksha_rt_map_new(void);
void moksha_rt_map_insert(void *map_ptr, void *key, void *value);
void *moksha_rt_map_get(void *map_ptr, void *key);

// ============================================================================
// Synchronization & Concurrency
// ============================================================================
void __moksha_lock(void *ptr);
void __moksha_unlock(void *ptr);

void *moksha_rt_spawn(MokshaClosure closure);
void *moksha_rt_await(void *promise_handle);
void *moksha_rt_spawn_thread(void *closure_ptr);
void *moksha_rt_spawn_weak_thread(void *closure_ptr);
void moksha_rt_register_await(void *promise_handle, void *waiting_coro);
void *moksha_rt_await_payload(void *promise_handle);
void moksha_rt_resolve_promise(void *promise_handle, void *result_data);
void *moksha_rt_make_resolved_promise(void *result_data);
void *moksha_rt_block_on(void *promise_handle);
void moksha_scheduler_inc_active(void);
void moksha_scheduler_dec_active(void);

#ifdef __cplusplus
}
#endif
