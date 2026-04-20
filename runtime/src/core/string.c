#include "../../include/moksha_rt.h"
#include "../abi/sys_caps.h"
#include "../abi/sys_io.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);
extern void *__moksha_alloc(uint32_t size);

void moksha_print_decimal128(__int128_t value, int scale) {
  if (value == 0) {
    printf("0");
    return;
  }

  bool is_negative = value < 0;
  if (is_negative) {
    value = -value;
    printf("-");
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
  printf("%s", &buffer[pos + 1]);
}

static float half_to_float(uint16_t h) {
  uint32_t sign = (h >> 15) & 1;
  uint32_t exp = (h >> 10) & 0x1F;
  uint32_t mant = h & 0x3FF;

  if (exp == 0) {
    if (mant == 0) {
      uint32_t res = sign << 31;
      float f;
      memcpy(&f, &res, 4);
      return f;
    } else {
      while (!(mant & 0x400)) {
        mant <<= 1;
        exp -= 1;
      }
      exp += 1;
      mant &= ~0x400;
    }
  } else if (exp == 31) {
    if (mant == 0) {
      uint32_t res = (sign << 31) | 0x7f800000;
      float f;
      memcpy(&f, &res, 4);
      return f;
    } else {
      uint32_t res = (sign << 31) | 0x7f800000 | (mant << 13);
      float f;
      memcpy(&f, &res, 4);
      return f;
    }
  }

  exp = exp + (127 - 15);
  uint32_t res = (sign << 31) | (exp << 23) | (mant << 13);
  float f;
  memcpy(&f, &res, 4);
  return f;
}

int32_t moksha_rt_string_length(MokshaString *str) {
  if (!str)
    return 0;
  return (int32_t)str->length;
}

char moksha_rt_string_at(MokshaString *str, int32_t index) {
  if (!str || index < 0 || (uint64_t)index >= str->length) {
    moksha_rt_panic("String index out of bounds");
  }
  return str->chars[index];
}

// Dynamically unpacks the data pointer using the hidden ARC header
void print(void *any_ptr, ...) {
  if (!sys_get_caps()->has_stdout)
    return;

  if (!any_ptr) {
    printf("null");
    return;
  }

  // Extract the true type from the ARC allocator's memory header!
  MokshaHeader *header = ((MokshaHeader *)any_ptr) - 1;
  uint32_t type_id = header->type_id;
  void *data = any_ptr;

  switch (type_id) {
  case MOKSHA_TYPE_BOOL: {
    bool val = *(bool *)data;
    printf("%s", val ? "true" : "false");
    break;
  }
  case MOKSHA_TYPE_I8: {
    int8_t val = *(int8_t *)data;
    printf("%d", val);
    break;
  }
  case MOKSHA_TYPE_U8: {
    uint8_t val = *(uint8_t *)data;
    printf("%u", val);
    break;
  }
  case MOKSHA_TYPE_I16: {
    int16_t val = *(int16_t *)data;
    printf("%d", val);
    break;
  }
  case MOKSHA_TYPE_U16: {
    uint16_t val = *(uint16_t *)data;
    printf("%u", val);
    break;
  }
  case MOKSHA_TYPE_I32: {
    int32_t val = *(int32_t *)data;
    printf("%d", val);
    break;
  }
  case MOKSHA_TYPE_U32: {
    uint32_t val = *(uint32_t *)data;
    printf("%u", val);
    break;
  }
  case MOKSHA_TYPE_I64: {
    int64_t val = *(int64_t *)data;
    printf("%" PRId64, val);
    break;
  }
  case MOKSHA_TYPE_U64: {
    uint64_t val = *(uint64_t *)data;
    printf("%" PRIu64, val);
    break;
  }
  case MOKSHA_TYPE_ISIZE: {
    intptr_t val = *(intptr_t *)data;
    printf("%" PRIdPTR, val);
    break;
  }
  case MOKSHA_TYPE_USIZE: {
    size_t val = *(size_t *)data;
    printf("%zu", val);
    break;
  }
  case MOKSHA_TYPE_F8:
  case MOKSHA_TYPE_F16: {
    uint16_t val = *(uint16_t *)data;
    printf("%g", half_to_float(val));
    break;
  }
  case MOKSHA_TYPE_F32: {
    float val = *(float *)data;
    printf("%g", val);
    break;
  }
  case MOKSHA_TYPE_F64: {
    double val = *(double *)data;
    printf("%g", val);
    break;
  }
  case MOKSHA_TYPE_DECIMAL: {
    struct {
      __int128_t mantissa;
      int32_t scale;
    } *dec = data;
    moksha_print_decimal128(dec->mantissa, dec->scale);
    break;
  }
  case MOKSHA_TYPE_STRING: {
    char *str = *(char **)data;
    if (str) {
      printf("%s", str);
    } else {
      printf("null");
    }
    break;
  }
  case MOKSHA_TYPE_POINTER: {
    // Unbox the actual memory address from the heap allocation
    void *actual_ptr = *(void **)data;
    if (actual_ptr == NULL) {
      printf("null");
    } else {
      printf("%p", actual_ptr);
    }
    break;
  }
  case MOKSHA_TYPE_TABLE:
  default: {
    printf("<type_id: %d, ptr: %p>", type_id, data);
    break;
  }
  }
  fflush(stdout);
}

void println(void *any_ptr, ...) {
  print(any_ptr);
  printf("\n");
  fflush(stdout);
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
  size_t len_a = a ? strlen(a) : 0;
  size_t len_b = b ? strlen(b) : 0;
  char *str = (char *)__moksha_alloc(len_a + len_b + 1);
  if (len_a > 0)
    strcpy(str, a);
  if (len_b > 0)
    strcpy(str + len_a, b);
  str[len_a + len_b] = '\0';
  return str;
}

char *__moksha_template_join_strs(int32_t count, ...) {
  if (count <= 0) {
    char *empty = (char *)__moksha_alloc(1);
    empty[0] = '\0';
    return empty;
  }

  va_list args;
  size_t total_len = 0;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    char *str = va_arg(args, char *);
    if (str)
      total_len += strlen(str);
  }
  va_end(args);

  char *result = (char *)__moksha_alloc(total_len + 1);
  result[0] = '\0';

  va_start(args, count);
  for (int i = 0; i < count; i++) {
    char *str = va_arg(args, char *);
    if (str)
      strcat(result, str);
  }
  va_end(args);

  return result;
}

// ============================================================================
// Type Casting to String
// ============================================================================

char *__moksha_bool_to_string(bool val) {
  char *str = (char *)__moksha_alloc(6);
  strcpy(str, val ? "true" : "false");
  return str;
}

char *__moksha_char_to_string(int8_t val) {
  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%d", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_uchar_to_string(uint8_t val) {
  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%u", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_short_to_string(int16_t val) {
  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%d", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_ushort_to_string(uint16_t val) {
  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%u", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_int_to_string(int32_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%d", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_uint_to_string(uint32_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%u", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_long_to_string(int64_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%" PRId64, val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_ulong_to_string(uint64_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%" PRIu64, val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_isize_to_string(intptr_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%" PRIdPTR, val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_usize_to_string(size_t val) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%zu", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_quarter_to_string(uint16_t val) {
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "%g", half_to_float(val));
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_half_to_string(uint16_t val) {
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "%g", half_to_float(val));
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_float_to_string(float val) {
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "%g", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_double_to_string(double val) {
  char buf[64];
  int len = snprintf(buf, sizeof(buf), "%g", val);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);
  return str;
}

char *__moksha_decimal_to_string(__int128_t value, int32_t scale) {
  if (value == 0) {
    char *str = (char *)__moksha_alloc(2);
    strcpy(str, "0");
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

  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, final_str);
  return str;
}

char *__moksha_ptr_to_string(void *ptr) {
  char buf[32];
  // Format the pointer as a hex string (e.g., 0x000002A5B)
  int len = snprintf(buf, sizeof(buf), "%p", ptr);

  // Allocate memory using Moksha's allocator
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, buf);

  return str;
}

char *__moksha_cstr_to_string(char *cstr) {
  if (!cstr) {
    char *str = (char *)__moksha_alloc(5);
    strcpy(str, "null");
    return str;
  }

  size_t len = strlen(cstr);
  char *str = (char *)__moksha_alloc(len + 1);
  strcpy(str, cstr);

  return str;
}
