#include "../../include/moksha_rt.h"
#include "../abi/sys_alloc.h"
#include "../abi/sys_io.h"
#include <ctype.h>
#include <errno.h> // NEW: Required for ERANGE checks
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declare your runtime allocator and panic function
extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message); // NEW

char *__moksha_input(char *prompt) {
  // 1. Print the prompt as a raw C-string
  if (prompt && prompt[0] != '\0') {
    printf("%s", prompt);
    fflush(stdout);
  }

  // 2. Read from STDIN
  char buffer[1024];
  size_t bytesRead = 0;
  sys_io_read(0, buffer, 1024, &bytesRead);

  // 3. Clean up the trailing newline (\r\n or \n)
  if (bytesRead > 0 &&
      (buffer[bytesRead - 1] == '\n' || buffer[bytesRead - 1] == '\r')) {
    bytesRead--;
    if (bytesRead > 0 && buffer[bytesRead - 1] == '\r') {
      bytesRead--;
    }
  }

  // 4. Allocate payload space using the ARC allocator
  char *result = (char *)moksha_rt_alloc(bytesRead + 1, 0);
  if (!result)
    moksha_rt_panic("OOM during input allocation");
  memcpy(result, buffer, bytesRead);
  result[bytesRead] = '\0';

  return result;
}

// --- Integer Parsers ---
int64_t __moksha_parse_int(const char *input_str) {
  if (!input_str)
    moksha_rt_panic("Input Error: Null string passed to integer parser.");

  char *endptr;
  errno = 0; // Clear errno before checking
  int64_t val = strtoll(input_str, &endptr, 10);

  if (endptr == input_str) {
    moksha_rt_panic("Parse Error: Invalid input. Expected an integer.");
  }
  if (errno == ERANGE) {
    moksha_rt_panic("Parse Error: Integer input out of range.");
  }

  return val;
}

uint64_t __moksha_parse_uint(const char *input_str) {
  if (!input_str)
    moksha_rt_panic(
        "Input Error: Null string passed to unsigned integer parser.");

  // Strict validation: Don't allow negative inputs to wrap around
  const char *check_str = input_str;
  while (isspace((unsigned char)*check_str))
    check_str++;
  if (*check_str == '-') {
    moksha_rt_panic("Parse Error: Expected an unsigned integer, but got a "
                    "negative number.");
  }

  char *endptr;
  errno = 0;
  uint64_t val = strtoull(input_str, &endptr, 10);

  if (endptr == input_str) {
    moksha_rt_panic(
        "Parse Error: Invalid input. Expected an unsigned integer.");
  }
  if (errno == ERANGE) {
    moksha_rt_panic("Parse Error: Unsigned integer input out of range.");
  }

  return val;
}

// --- Float Parser ---
double __moksha_parse_float(const char *input_str) {
  if (!input_str)
    moksha_rt_panic("Input Error: Null string passed to float parser.");

  char *endptr;
  errno = 0;
  double val = strtod(input_str, &endptr);

  if (endptr == input_str) {
    moksha_rt_panic(
        "Parse Error: Invalid input. Expected a floating-point number.");
  }
  if (errno == ERANGE) {
    moksha_rt_panic("Parse Error: Float input out of range.");
  }

  return val;
}

// --- Char Parser ---
char __moksha_parse_char(const char *input_str) {
  if (!input_str)
    moksha_rt_panic("Input Error: Null string passed to char parser.");

  if (input_str[0] == '\0') {
    moksha_rt_panic(
        "Parse Error: Expected a character, but got an empty string.");
  }

  return input_str[0];
}

// --- Bool Parser ---
bool __moksha_parse_bool(const char *input_str) {
  if (!input_str)
    moksha_rt_panic("Input Error: Null string passed to bool parser.");

  // Skip leading whitespace
  while (isspace((unsigned char)*input_str)) {
    input_str++;
  }

  if (strncmp(input_str, "true", 4) == 0 || input_str[0] == '1') {
    return true;
  }
  if (strncmp(input_str, "false", 5) == 0 || input_str[0] == '0') {
    return false;
  }

  moksha_rt_panic(
      "Parse Error: Invalid input. Expected a boolean (true, false, 1, or 0).");
  return false;
}

// --- Decimal Parser ---
MokshaDecimal __moksha_parse_decimal(const char *input_str) {
  if (!input_str)
    moksha_rt_panic("Input Error: Null string passed to decimal parser.");

  MokshaDecimal dec = {0, 0};

  // 1. Skip leading whitespace
  while (isspace((unsigned char)*input_str)) {
    input_str++;
  }

  if (*input_str == '\0') {
    moksha_rt_panic(
        "Parse Error: Expected a decimal, but got an empty string.");
  }

  // 2. Handle optional sign
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
  bool found_digits = false; // Track if we actually parsed numbers

  // 3. Parse characters
  while (*input_str != '\0') {
    char c = *input_str;

    if (c == '.') {
      if (in_fraction) {
        moksha_rt_panic(
            "Parse Error: Invalid decimal format (multiple decimal points).");
      }
      in_fraction = true;
    } else if (isdigit((unsigned char)c)) {
      found_digits = true;
      mantissa = (mantissa * 10) + (c - '0');

      if (in_fraction) {
        scale++;
      }
    } else if (!isspace((unsigned char)c)) {
      // Allow trailing spaces, but panic on trailing garbage like "12.5a"
      moksha_rt_panic("Parse Error: Invalid character found in decimal input.");
    }
    input_str++;
  }

  if (!found_digits) {
    moksha_rt_panic("Parse Error: Invalid input. Expected a decimal number.");
  }

  // 4. Apply sign
  if (is_negative) {
    mantissa = -mantissa;
  }

  dec.mantissa = mantissa;
  dec.scale = scale;

  return dec;
}
