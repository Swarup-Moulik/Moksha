#include "../../include/moksha_rt.h"
#include "../abi/sys_alloc.h"
#include "../abi/sys_io.h"
#include <stdbool.h>

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);

static size_t internal_strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

static bool internal_isspace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
         c == '\f';
}

static bool internal_isdigit(char c) { return c >= '0' && c <= '9'; }

char *__moksha_input(char *prompt) {
  size_t written;
  if (prompt && prompt[0] != '\0') {
    sys_io_write(1, prompt, internal_strlen(prompt), &written);
  }

  char buffer[1024];
  size_t bytesRead = 0;
  // FIXED: Read up to 1023 bytes to prevent stack overflow
  sys_io_read(0, buffer, sizeof(buffer) - 1, &bytesRead);

  if (bytesRead > 0 &&
      (buffer[bytesRead - 1] == '\n' || buffer[bytesRead - 1] == '\r')) {
    bytesRead--;
    if (bytesRead > 0 && buffer[bytesRead - 1] == '\r') {
      bytesRead--;
    }
  }

  buffer[bytesRead] = '\0';
  char *str = (char *)moksha_rt_alloc(bytesRead + 1, MOKSHA_TYPE_STRING);
  for (size_t i = 0; i <= bytesRead; i++)
    str[i] = buffer[i];

  return str;
}

void __moksha_string_to_decimal(MokshaDecimal *out, MokshaString *str) {
  const char *input_str = str->chars;
  while (internal_isspace(*input_str))
    input_str++;

  if (*input_str == '\0') {
    moksha_rt_panic(
        "Parse Error: Expected a decimal, but got an empty string.");
  }

  bool is_negative = false;
  if (*input_str == '-') {
    is_negative = true;
    input_str++;
  } else if (*input_str == '+') {
    input_str++;
  }

  __int128 mantissa = 0;
  int32_t scale = 0;
  bool in_fraction = false;
  bool found_digits = false;

  while (*input_str != '\0') {
    char c = *input_str;

    if (c == '.') {
      if (in_fraction) {
        moksha_rt_panic(
            "Parse Error: Invalid decimal format (multiple decimal points).");
      }
      in_fraction = true;
    } else if (internal_isdigit(c)) {
      found_digits = true;
      mantissa = (mantissa * 10) + (c - '0');
      if (in_fraction)
        scale++;
    } else if (!internal_isspace(c)) {
      moksha_rt_panic("Parse Error: Invalid characters in decimal string.");
    }
    input_str++;
  }

  if (!found_digits)
    moksha_rt_panic("Parse Error: No digits found in decimal string.");

  out->mantissa = is_negative ? -mantissa : mantissa;
  out->scale = scale;
}
