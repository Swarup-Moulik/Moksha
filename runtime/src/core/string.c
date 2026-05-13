#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);

// ============================================================================
// Bare-Metal Utility Functions (Replaces <string.h> and <stdio.h>)
// ============================================================================

static size_t internal_strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

static void internal_strcpy(char *dest, const char *src) {
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
}

static void internal_strcat(char *dest, const char *src) {
  while (*dest)
    dest++;
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
}

bool __moksha_string_eq(void *a_ptr, void *b_ptr) {
  char *a = (char *)a_ptr;
  char *b = (char *)b_ptr;

  if (a == b)
    return true; // Same identity
  if (!a || !b)
    return false; // One is null

  while (*a && (*a == *b)) {
    a++;
    b++;
  }
  return (*(const unsigned char *)a - *(const unsigned char *)b) == 0;
}

static void sys_print(const char *str) {
  if (!str)
    return;
  size_t written;
  sys_io_write(1, str, internal_strlen(str), &written); // 1 = STDOUT
}

static int internal_utoa64(uint64_t val, char *buf) {
  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return 1;
  }
  char temp[32];
  int i = 0, j = 0;
  while (val > 0) {
    temp[i++] = (val % 10) + '0';
    val /= 10;
  }
  while (i > 0)
    buf[j++] = temp[--i];
  buf[j] = '\0';
  return j;
}

static int internal_itoa64(int64_t val, char *buf) {
  int j = 0;
  if (val < 0) {
    buf[j++] = '-';
  }
  uint64_t uval = (val < 0) ? ((uint64_t)(-(val + 1)) + 1) : (uint64_t)val;
  j += internal_utoa64(uval, buf + j);
  return j;
}

static int internal_ptrtoa(void *ptr, char *buf) {
  uintptr_t val = (uintptr_t)ptr;
  buf[0] = '0';
  buf[1] = 'x';
  if (val == 0) {
    buf[2] = '0';
    buf[3] = '\0';
    return 3;
  }
  int i = 0, j = 2;
  char temp[32];
  while (val > 0) {
    int rem = val % 16;
    temp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
    val /= 16;
  }
  while (i > 0)
    buf[j++] = temp[--i];
  buf[j] = '\0';
  return j;
}

static int internal_ftoa(double val, char *buf) {
  // 1. Check for NaN
  if (val != val) {
    internal_strcpy(buf, "NaN");
    return 3;
  }

  // 2. Check for Infinity (Bare-metal trick: Infinity - Infinity = NaN)
  double test = val - val;
  if (test != test) {
    if (val > 0) {
      internal_strcpy(buf, "Infinity");
      return 8; // Length of "Infinity"
    } else {
      internal_strcpy(buf, "-Infinity");
      return 9; // Length of "-Infinity"
    }
  }

  int j = 0;
  if (val < 0) {
    buf[j++] = '-';
    val = -val;
  }

  if (val == 0.0) {
    buf[j++] = '0';
    buf[j] = '\0';
    return j;
  }

  int exponent = 0;
  double temp = val;

  // 1. Detect if we need Scientific Notation
  bool use_scientific = (val >= 1e6 || val < 1e-4);

  if (use_scientific) {
    if (temp >= 10.0) {
      while (temp >= 10.0) {
        temp /= 10.0;
        exponent++;
      }
    } else if (temp < 1.0) {
      while (temp < 1.0) {
        temp *= 10.0;
        exponent--;
      }
    }
  }

  temp += 0.0000005;

  // 2. Extract Integer and Fractional parts
  uint64_t int_part = (uint64_t)temp;
  double frac_part = temp - (double)int_part;

  j += internal_utoa64(int_part, buf + j);

  // 3. Print up to 6 decimal places
  buf[j++] = '.';
  for (int i = 0; i < 6; i++) {
    frac_part *= 10;
    int digit = (int)frac_part;
    buf[j++] = digit + '0';
    frac_part -= digit;
  }

  // 4. Strip trailing zeros and the decimal point if it's clean
  while (buf[j - 1] == '0')
    j--;
  if (buf[j - 1] == '.')
    j--;

  // 5. Append the 'e' exponent if we used scientific notation
  if (use_scientific) {
    buf[j++] = 'e';
    if (exponent >= 0) {
      buf[j++] = '+';
      j += internal_itoa64(exponent, buf + j);
    } else {
      // internal_itoa64 automatically adds the '-' for negative numbers
      j += internal_itoa64(exponent, buf + j);
    }
  }

  buf[j] = '\0';
  return j;
}

// 1. Define the Global Spinlock
static int stdout_lock = 0;

static inline void acquire_print_lock(void) {
  while (__atomic_exchange_n(&stdout_lock, 1, __ATOMIC_ACQUIRE)) {
    // Spin lightly.
  }
}

static inline void release_print_lock(void) {
  __atomic_store_n(&stdout_lock, 0, __ATOMIC_RELEASE);
}

// ============================================================================
// Core Formatter
// ============================================================================

void moksha_print_decimal128(__int128_t value, int scale) {
  if (value == 0) {
    sys_print("0");
    return;
  }

  bool is_negative = value < 0;
  if (is_negative) {
    value = -value;
    sys_print("-");
  }

  char buffer[45];
  int pos = sizeof(buffer) - 1;
  buffer[pos] = '\0';
  pos--;

  int digits_printed = 0;
  while (value > 0 || digits_printed < scale + 1) {
    if (digits_printed == scale && scale > 0) {
      buffer[pos--] = '.';
    }
    int digit = (int)(value % 10);
    buffer[pos--] = '0' + digit;
    value /= 10;
    digits_printed++;
  }

  if (buffer[pos + 1] == '.') {
    buffer[pos--] = '0';
  }
  sys_print(&buffer[pos + 1]);
}

int32_t moksha_rt_string_len(char *str) {
  if (!str)
    return 0;
  return (int32_t)internal_strlen(str);
}

char moksha_rt_string_char_at(char *str, int32_t index) {
  if (!str)
    moksha_rt_panic("Null string access");
  // Simple bounds check
  if (index < 0 || index >= (int32_t)internal_strlen(str)) {
    moksha_rt_panic("String index out of bounds");
  }
  return str[index];
}

void print(MokshaAny *any_val, ...) {
  if (!sys_get_caps()->has_stdout)
    return;

  // Handle nulls safely
  if (!any_val || !any_val->data || !any_val->vtable) {
    acquire_print_lock();
    sys_print("null");
    release_print_lock();
    return;
  }

  // 1. Resolve the string BEFORE taking the lock
  char *str = any_val->vtable->to_string(any_val->data);

  // 2. Lock and write
  acquire_print_lock();
  sys_print(str);
  release_print_lock();

  // 3. Cleanup after the lock is released
  moksha_rt_release(str);
}

void println(MokshaAny *any_val, ...) {
  if (!sys_get_caps()->has_stdout)
    return;

  // Handle nulls safely with a built-in newline
  if (!any_val || !any_val->data || !any_val->vtable) {
    acquire_print_lock();
    sys_print("null\n");
    release_print_lock();
    return;
  }

  // 1. Resolve the string BEFORE taking the lock
  char *str = any_val->vtable->to_string(any_val->data);

  // 2. Lock, write the string, AND write the newline atomically!
  acquire_print_lock();
  sys_print(str);
  sys_print("\n");
  release_print_lock();

  // 3. Cleanup after the lock is released
  moksha_rt_release(str);
}

void moksha_rt_close(void *any_ptr) {
  if (!any_ptr)
    return;
  int32_t fd = *(int32_t *)any_ptr;
  sys_io_close(fd);
}

// ============================================================================
// Internal String Builders
// ============================================================================

char *__moksha_string_concat(char *a, char *b) {
  size_t len_a = a ? internal_strlen(a) : 0;
  size_t len_b = b ? internal_strlen(b) : 0;
  char *str = (char *)moksha_rt_alloc(len_a + len_b + 1, MOKSHA_TYPE_STRING);
  if (len_a > 0)
    internal_strcpy(str, a);
  if (len_b > 0)
    internal_strcpy(str + len_a, b);
  str[len_a + len_b] = '\0';
  return str;
}

char *__moksha_template_join_strs(int32_t count, ...) {
  if (count <= 0) {
    char *empty = (char *)moksha_rt_alloc(1, MOKSHA_TYPE_STRING);
    empty[0] = '\0';
    return empty;
  }

  va_list args;
  size_t total_len = 0;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    char *str = va_arg(args, char *);
    if (str)
      total_len += internal_strlen(str);
  }
  va_end(args);

  char *result = (char *)moksha_rt_alloc(total_len + 1, MOKSHA_TYPE_STRING);
  result[0] = '\0';

  va_start(args, count);
  for (int i = 0; i < count; i++) {
    char *str = va_arg(args, char *);
    if (str)
      internal_strcat(result, str);
  }
  va_end(args);

  return result;
}

// ============================================================================
// Type Casting to String
// ============================================================================

char *__moksha_bool_to_string(bool val) {
  char *str = (char *)moksha_rt_alloc(6, MOKSHA_TYPE_STRING);
  internal_strcpy(str, val ? "true" : "false");
  return str;
}

char *__moksha_char_to_string(int8_t val) {
  char *str = (char *)moksha_rt_alloc(2, MOKSHA_TYPE_STRING);
  str[0] = (char)val;
  str[1] = '\0';
  return str;
}

char *__moksha_uchar_to_string(uint8_t val) {
  char buf[32];
  int len = internal_utoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_short_to_string(int16_t val) {
  char buf[32];
  int len = internal_itoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_ushort_to_string(uint16_t val) {
  char buf[32];
  int len = internal_utoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_int_to_string(int32_t val) {
  char buf[32];
  int len = internal_itoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_uint_to_string(uint32_t val) {
  char buf[32];
  int len = internal_utoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_long_to_string(int64_t val) {
  char buf[32];
  int len = internal_itoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_ulong_to_string(uint64_t val) {
  char buf[32];
  int len = internal_utoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_isize_to_string(intptr_t val) {
  char buf[32];
  int len = internal_itoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_usize_to_string(size_t val) {
  char buf[32];
  int len = internal_utoa64(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_quarter_to_string(float val) {
  char buf[64];
  int len = internal_ftoa(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_half_to_string(float val) {
  char buf[64];
  int len = internal_ftoa(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_float_to_string(float val) {
  char buf[64];
  int len = internal_ftoa(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_double_to_string(double val) {
  char buf[64];
  int len = internal_ftoa(val, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *moksha_rt_dec_to_string(MokshaDecimal *dec) {
  if (!dec)
    return NULL;

  // Extract the values locally
  __int128_t value = dec->mantissa;
  int32_t scale = dec->scale;

  if (value == 0) {
    char *str = (char *)moksha_rt_alloc(2, MOKSHA_TYPE_STRING);
    internal_strcpy(str, "0");
    return str;
  }

  bool is_negative = value < 0;
  if (is_negative)
    value = -value;

  char buffer[64];
  int pos = sizeof(buffer) - 1;
  buffer[pos] = '\0';
  pos--;

  int digits_printed = 0;
  while (value > 0 || digits_printed < scale + 1) {
    if (digits_printed == scale && scale > 0) {
      buffer[pos--] = '.';
    }
    int digit = (int)(value % 10);
    buffer[pos--] = '0' + digit;
    value /= 10;
    digits_printed++;
  }

  if (buffer[pos + 1] == '.')
    buffer[pos--] = '0';
  if (is_negative)
    buffer[pos--] = '-';

  const char *final_str = &buffer[pos + 1];
  size_t len = (sizeof(buffer) - 1) - (pos + 1);

  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, final_str);
  return str;
}

char *__moksha_ptr_to_string(void *ptr) {
  char buf[32];
  int len = internal_ptrtoa(ptr, buf);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, buf);
  return str;
}

char *__moksha_cstr_to_string(const char *cstr) {
  if (!cstr) {
    char *str = (char *)moksha_rt_alloc(5, MOKSHA_TYPE_STRING);
    internal_strcpy(str, "null");
    return str;
  }

  size_t len = internal_strlen(cstr);
  char *str = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(str, cstr);
  return str;
}

char *__moksha_half_to_string_abi(float val) {
  return __moksha_float_to_string(val);
}

char *__moksha_quarter_to_string_abi(float val) {
  return __moksha_float_to_string(val);
}

char *__moksha_any_to_string(MokshaAny *any_val) {
  if (!any_val || !any_val->data || !any_val->vtable) {
    return __moksha_cstr_to_string("null");
  }
  return any_val->vtable->to_string(any_val->data);
}
