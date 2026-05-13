#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare the system unwinder struct
struct _Unwind_Exception;

// ============================================================================
// Core Memory Model
// ============================================================================
typedef struct {
  uint32_t ref_count;  // Strong references
  uint32_t weak_count; // Weak references
  uint32_t type_id;    // Metadata
  // uint32_t _padding;   // Explicit 4-byte padding to guarantee 16-byte size!
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

// 1. Define the Virtual Method Table (VTable) for 'any'
typedef struct {
  uint32_t type_id;
  char *(*to_string)(void *); // Dynamic dispatch for stringification
  void (*retain)(void *);
  void (*drop)(void *); // Dynamic dispatch for ARC release
} AnyVTable;

// 2. Define the Fat Pointer
typedef struct {
  void *data;              // Pointer to the actual heap/stack data
  const AnyVTable *vtable; // Pointer to the type's specific VTable
} MokshaAny;

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
  MOKSHA_TYPE_PROMISE = 20,
  MOKSHA_TYPE_CLOSURE = 21,
  MOKSHA_TYPE_CLASS = 22 // Extended for internal runtime classes like Channels
} MokshaTypeID;

typedef struct AsyncMutexWaitNode {
  void *promise_handle;
  struct AsyncMutexWaitNode *next;
} AsyncMutexWaitNode;

typedef struct {
  bool is_locked;
  AsyncMutexWaitNode *waiters_head;
  AsyncMutexWaitNode *waiters_tail;
  int spin_lock;
} MokshaAsyncMutex;

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
int32_t __moksha_get_type(void *ptr);
void *__moksha_alloc(uint32_t size, uint32_t type_id);
void __moksha_free(void *ptr);

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
int32_t moksha_rt_string_len(char *str);
char moksha_rt_string_char_at(char *str, int32_t index);

void print(MokshaAny *any_val, ...);
void println(MokshaAny *any_val, ...);
MokshaString *moksha_rt_readFile(void *any_ptr);
void moksha_rt_close(void *any_ptr);

// String Allocation & Concatenation (Updated to raw char*)
char *__moksha_string_concat(char *a, char *b);
char *__moksha_template_join_strs(int32_t count, ...);

// Structural Equality Hooks
bool __moksha_string_eq(void *a_ptr, void *b_ptr);
bool __moksha_array_eq(void *a_ptr, int32_t a_len, void *b_ptr, int32_t b_len,
                       int32_t elem_size);

// Type to String Conversions (Updated to raw char*)
char *__moksha_bool_to_string(bool val);
char *__moksha_char_to_string(int8_t val);
char *__moksha_uchar_to_string(uint8_t val);
char *__moksha_short_to_string(int16_t val);
char *__moksha_ushort_to_string(uint16_t val);
char *__moksha_int_to_string(int32_t val);
char *__moksha_uint_to_string(uint32_t val);
char *__moksha_long_to_string(int64_t val);
char *__moksha_ulong_to_string(uint64_t val);
char *__moksha_isize_to_string(intptr_t val);
char *__moksha_usize_to_string(size_t val);
char *__moksha_quarter_to_string(float val); // f8
char *__moksha_half_to_string(float val);    // f16
char *__moksha_float_to_string(float val);   // f32
char *__moksha_double_to_string(double val); // f64
char *__moksha_half_to_string_abi(float val);
char *__moksha_quarter_to_string_abi(float val);
char *moksha_rt_dec_to_string(MokshaDecimal *dec);
char *__moksha_ptr_to_string(void *ptr);
char *__moksha_cstr_to_string(const char *cstr);
char *__moksha_any_to_string(MokshaAny *any_val);
void moksha_print_decimal128(__int128_t value, int scale);

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
void moksha_rt_map_insert(void *map_ptr, MokshaAny *key, MokshaAny *value);
MokshaAny *moksha_rt_map_get_key_at(void *map_ptr, int32_t index);
MokshaAny *moksha_rt_map_get_val_at(void *map_ptr, int32_t index);
MokshaAny *moksha_rt_map_get(void *map_ptr, MokshaAny *key);
int32_t moksha_rt_map_len(void *map_ptr);
void moksha_rt_map_free_internal(void *map_ptr);

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
void moksha_rt_reject_promise(void *promise_handle, void *ex_payload);
void *moksha_rt_make_resolved_promise(void *result_data);
void *moksha_rt_make_rejected_promise(void *ex_payload);
void *moksha_rt_block_on(void *promise_handle);
void moksha_scheduler_inc_active(void);
void moksha_scheduler_dec_active(void);
void *moksha_rt_coro_setup(void *coro_handle);
void moksha_rt_coro_finish(void *promise_handle, void *payload);
void *moksha_rt_make_unresolved_promise(void);
void *spawn_func(void *closure_ptr);
void *join(void *p1_handle, void *p2_handle);
bool moksha_scheduler_is_active(void);
void moksha_rt_join_all_callback(void *sub_result, void *ctx_ptr, int index);
void *moksha_builtin_yield(void);
void *moksha_builtin_spawn(void *closure_ptr, int32_t priority);
void *moksha_builtin_spawn_promise(void *promise_handle, int32_t priority);
void moksha_builtin_cancel(void *promise_handle);
void *moksha_builtin_select(void *p1, void *p2);
void *moksha_builtin_timeout(void *promise_handle, uint32_t ms);
void *moksha_builtin_sleep(uint32_t ms);
void *moksha_rt_consume_exception();

// ============================================================================
// Error Handling & Exceptions
// ============================================================================
void moksha_rt_panic(const char *message);
void moksha_rt_panic_out_of_bounds(int64_t index, int64_t length);
void moksha_rt_panic_null_deref(void);
void moksha_rt_panic_key_not_found(void);
void moksha_rt_panic_bad_cast(void);

// [FIX] Updated throw signature and payload getter for the bare-metal unwinder
void moksha_rt_throw(void *payload);
void *moksha_rt_get_exception_payload(struct _Unwind_Exception *exc_base);

// ============================================================================
// Decimal Conversions
// ============================================================================
void __moksha_f64_to_decimal(MokshaDecimal *out, double val,
                             int32_t target_scale);
double __moksha_decimal_to_f64(MokshaDecimal *dec);

// ============================================================================
// Builtin Object Bindings
// ============================================================================
void AsyncMutex_constructor(void *this_ptr) __asm__(
    "AsyncMutex_constructor_ret_void");
void *
AsyncMutex_lock(void *this_ptr) __asm__("AsyncMutex_lock_ret_promise_void");
void AsyncMutex_unlock(void *this_ptr) __asm__("AsyncMutex_unlock_ret_void");
void AsyncMutex_destructor(void *this_ptr) __asm__(
    "AsyncMutex_destructor_ret_void");

void moksha_builtin_Channel_constructor(void *this_ptr, int capacity) __asm__(
    "moksha_builtin_Channel_constructor");
void *moksha_builtin_Channel_recv(void *this_ptr) __asm__(
    "moksha_builtin_Channel_recv");
void *
moksha_builtin_Channel_send(void *this_ptr,
                            void *val) __asm__("moksha_builtin_Channel_send");
void moksha_builtin_Channel_close(void *this_ptr) __asm__(
    "moksha_builtin_Channel_close");

void Exception_constructor_string_ret_void(
    void *this_ptr,
    char *msg_str) __asm__("Exception_constructor_string_ret_void");

#ifdef __cplusplus
}
#endif
