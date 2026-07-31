#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int __gxx_personality_v0(void) { return 0; }

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_rt_release(void *ptr);

/** @brief Bare-Metal Utility Functions */

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
    return true;
  if (!a || !b)
    return false;

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
  sys_io_write(1, str, internal_strlen(str), &written);
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
  if (val != val) {
    internal_strcpy(buf, "NaN");
    return 3;
  }

  double test = val - val;
  if (test != test) {
    if (val > 0) {
      internal_strcpy(buf, "Infinity");
      return 8;
    } else {
      internal_strcpy(buf, "-Infinity");
      return 9;
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

  bool use_scientific = (val >= 1e18 || val < 1e-4);

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

  uint64_t int_part = (uint64_t)temp;
  double frac_part = temp - (double)int_part;

  j += internal_utoa64(int_part, buf + j);

  buf[j++] = '.';
  for (int i = 0; i < 6; i++) {
    frac_part *= 10;
    int digit = (int)frac_part;
    buf[j++] = digit + '0';
    frac_part -= digit;
  }

  while (buf[j - 1] == '0')
    j--;
  if (buf[j - 1] == '.')
    j--;

  if (use_scientific) {
    buf[j++] = 'e';
    if (exponent >= 0) {
      buf[j++] = '+';
      j += internal_itoa64(exponent, buf + j);
    } else {
      j += internal_itoa64(exponent, buf + j);
    }
  }

  buf[j] = '\0';
  return j;
}

// Global Spinlock
static int stdout_lock = 0;

static inline void acquire_print_lock(void) {
  while (__atomic_exchange_n(&stdout_lock, 1, __ATOMIC_ACQUIRE)) {
  }
}

static inline void release_print_lock(void) {
  __atomic_store_n(&stdout_lock, 0, __ATOMIC_RELEASE);
}

// Core Formatter

void moksha_print_decimal128(moksha_int128_t value, int scale) {
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
  if (index < 0 || index >= (int32_t)internal_strlen(str)) {
    moksha_rt_panic("String index out of bounds");
  }
  return str[index];
}

void print(MokshaAny *any_val, ...) {
  if (!sys_get_caps()->has_stdout)
    return;

  if (!any_val || !any_val->data || !any_val->vtable) {
    acquire_print_lock();
    sys_print("null");
    release_print_lock();
    return;
  }

  char *str = any_val->vtable->to_string(any_val->data);

  acquire_print_lock();
  sys_print(str);
  release_print_lock();

  moksha_rt_release(str);
}

void println(MokshaAny *any_val, ...) {
  if (!sys_get_caps()->has_stdout)
    return;

  if (!any_val || !any_val->data || !any_val->vtable) {
    acquire_print_lock();
    sys_print("null\n");
    release_print_lock();
    return;
  }

  char *str = any_val->vtable->to_string(any_val->data);

  acquire_print_lock();
  sys_print(str);
  sys_print("\n");
  release_print_lock();

  moksha_rt_release(str);
}

void moksha_rt_close(void *any_ptr) {
  if (!any_ptr)
    return;
  int32_t fd = *(int32_t *)any_ptr;
  sys_io_close(fd);
}

/** @brief Internal String Builders */

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

/** @brief Type Casting to String */

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

  moksha_int128_t value = dec->mantissa;
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

/** @brief Moksha String Builtins */

static char *internal_strstr(const char *haystack, const char *needle) {
  if (!*needle)
    return (char *)haystack;
  for (const char *p = haystack; *p; p++) {
    bool match = true;
    for (size_t i = 0; needle[i]; i++) {
      if (p[i] != needle[i]) {
        match = false;
        break;
      }
    }
    if (match)
      return (char *)p;
  }
  return NULL;
}

char *moksha_string_substring(char *str, int32_t start, int32_t end) {
  if (!str)
    return NULL;
  int32_t len = (int32_t)internal_strlen(str);
  if (start < 0)
    start = 0;
  if (end > len)
    end = len;
  if (start > end || start >= len) {
    char *empty = (char *)moksha_rt_alloc(1, MOKSHA_TYPE_STRING);
    empty[0] = '\0';
    return empty;
  }
  int32_t sub_len = end - start;
  char *sub = (char *)moksha_rt_alloc(sub_len + 1, MOKSHA_TYPE_STRING);
  for (int32_t i = 0; i < sub_len; i++)
    sub[i] = str[start + i];
  sub[sub_len] = '\0';
  return sub;
}

char *moksha_string_slice(char *str, int32_t start, int32_t end) {
  return moksha_string_substring(str, start, end);
}

bool moksha_string_contains(char *str, char *sub) {
  if (!str || !sub)
    return false;
  return internal_strstr(str, sub) != NULL;
}

int32_t moksha_string_index(char *str, char *sub) {
  if (!str || !sub)
    return -1;
  char *ptr = internal_strstr(str, sub);
  if (ptr)
    return (int32_t)(ptr - str);
  return -1;
}

bool moksha_string_starts_with(char *str, char *prefix) {
  if (!str || !prefix)
    return false;
  while (*prefix) {
    if (*prefix != *str)
      return false;
    prefix++;
    str++;
  }
  return true;
}

bool moksha_string_ends_with(char *str, char *suffix) {
  if (!str || !suffix)
    return false;
  size_t str_len = internal_strlen(str);
  size_t suf_len = internal_strlen(suffix);
  if (suf_len > str_len)
    return false;
  const char *p = str + (str_len - suf_len);
  while (*suffix) {
    if (*p++ != *suffix++)
      return false;
  }
  return true;
}

char *moksha_string_to_upper(char *str) {
  if (!str)
    return NULL;
  size_t len = internal_strlen(str);
  char *up = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  for (size_t i = 0; i < len; i++) {
    if (str[i] >= 'a' && str[i] <= 'z')
      up[i] = str[i] - 32;
    else
      up[i] = str[i];
  }
  up[len] = '\0';
  return up;
}

char *moksha_string_to_lower(char *str) {
  if (!str)
    return NULL;
  size_t len = internal_strlen(str);
  char *low = (char *)moksha_rt_alloc(len + 1, MOKSHA_TYPE_STRING);
  for (size_t i = 0; i < len; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z')
      low[i] = str[i] + 32;
    else
      low[i] = str[i];
  }
  low[len] = '\0';
  return low;
}

char *moksha_string_trim(char *str) {
  if (!str)
    return NULL;
  size_t len = internal_strlen(str);
  if (len == 0)
    return str;
  size_t start = 0;
  while (start < len && (str[start] == ' ' || str[start] == '\t' ||
                         str[start] == '\n' || str[start] == '\r'))
    start++;
  size_t end = len;
  while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' ||
                         str[end - 1] == '\n' || str[end - 1] == '\r'))
    end--;
  size_t new_len = end - start;
  char *res = (char *)moksha_rt_alloc(new_len + 1, MOKSHA_TYPE_STRING);
  for (size_t i = 0; i < new_len; i++)
    res[i] = str[start + i];
  res[new_len] = '\0';
  return res;
}

char *moksha_string_replace(char *str, char *old_str, char *new_str) {
  if (!str || !old_str || !new_str)
    return str;
  size_t old_len = internal_strlen(old_str);
  if (old_len == 0)
    return str;
  size_t new_len = internal_strlen(new_str);
  size_t count = 0;
  const char *p = str;
  while ((p = internal_strstr(p, old_str)) != NULL) {
    count++;
    p += old_len;
  }
  if (count == 0)
    return str;
  size_t str_len = internal_strlen(str);
  size_t total_len = str_len + count * (new_len - old_len);
  char *res = (char *)moksha_rt_alloc(total_len + 1, MOKSHA_TYPE_STRING);
  char *dst = res;
  p = str;
  const char *next;
  while ((next = internal_strstr(p, old_str)) != NULL) {
    size_t prefix_len = next - p;
    for (size_t i = 0; i < prefix_len; i++)
      *dst++ = p[i];
    for (size_t i = 0; i < new_len; i++)
      *dst++ = new_str[i];
    p = next + old_len;
  }
  while (*p)
    *dst++ = *p++;
  *dst = '\0';
  return res;
}

MokshaSlice *moksha_string_split(char *str, char *delim) {
  // Allocate the returning MokshaSlice via ARC (MOKSHA_TYPE_ARRAY)
  MokshaSlice *ret =
      (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), MOKSHA_TYPE_ARRAY);

  if (!str || !delim) {
    ret->data = NULL;
    ret->length = 0;
    return ret;
  }

  size_t delim_len = internal_strlen(delim);

  if (delim_len == 0) {
    char **arr = (char **)moksha_rt_alloc(sizeof(char *), MOKSHA_TYPE_ARRAY);
    arr[0] = str;
    ret->data = arr;
    ret->length = 1;
    return ret;
  }

  size_t count = 1;
  const char *p = str;
  while ((p = internal_strstr(p, delim)) != NULL) {
    count++;
    p += delim_len;
  }

  char **arr =
      (char **)moksha_rt_alloc(sizeof(char *) * count, MOKSHA_TYPE_ARRAY);

  size_t idx = 0;
  p = str;
  const char *next;
  while ((next = internal_strstr(p, delim)) != NULL) {
    size_t part_len = next - p;
    char *part = (char *)moksha_rt_alloc(part_len + 1, MOKSHA_TYPE_STRING);
    for (size_t i = 0; i < part_len; i++)
      part[i] = p[i];
    part[part_len] = '\0';
    arr[idx++] = part;
    p = next + delim_len;
  }

  size_t last_len = internal_strlen(p);
  char *last_part = (char *)moksha_rt_alloc(last_len + 1, MOKSHA_TYPE_STRING);
  internal_strcpy(last_part, p);
  arr[idx++] = last_part;

  ret->data = arr;
  ret->length = count;
  return ret;
}

char *moksha_string_join(MokshaSlice *arr, char *delim) {
  if (!arr || !arr->data || arr->length == 0) {
    char *empty = (char *)moksha_rt_alloc(1, MOKSHA_TYPE_STRING);
    empty[0] = '\0';
    return empty;
  }

  char **strs = (char **)arr->data;
  size_t delim_len = delim ? internal_strlen(delim) : 0;
  size_t total_len = 0;

  for (uint64_t i = 0; i < arr->length; i++) {
    total_len += internal_strlen(strs[i]);
    if (i < arr->length - 1)
      total_len += delim_len;
  }

  char *res = (char *)moksha_rt_alloc(total_len + 1, MOKSHA_TYPE_STRING);
  char *dst = res;

  for (uint64_t i = 0; i < arr->length; i++) {
    size_t len = internal_strlen(strs[i]);
    for (size_t j = 0; j < len; j++)
      *dst++ = strs[i][j];
    if (i < arr->length - 1 && delim_len > 0) {
      for (size_t j = 0; j < delim_len; j++)
        *dst++ = delim[j];
    }
  }

  *dst = '\0';
  return res;
}

bool moksha_string_is_digit(char ch) { return ch >= '0' && ch <= '9'; }
bool moksha_string_is_alpha(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}
bool moksha_string_is_whitespace(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}
