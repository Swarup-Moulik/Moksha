#include "../../include/moksha_rt.h"
#include <stdbool.h>
#include <stdint.h>

#if !defined(__MOKSHA_BAREMETAL__)
#include <math.h>
#include <stdlib.h>
#endif

/** @brief Math Constants */
const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;
const double TAU = 6.28318530717958647692;

#if defined(__MOKSHA_BAREMETAL__)
const double INF = 1.0 / 0.0;
const double NAN = 0.0 / 0.0;
#else
const double INF = INFINITY;
#undef NAN
const double NAN = __builtin_nan("");
#endif

// 32-bit integer exponentiation
int32_t __moksha_powi32(int32_t base, int32_t exp) {
  if (exp < 0) {
    return 0;
  }
  int32_t result = 1;
  while (exp > 0) {
    if (exp % 2 == 1) {
      result *= base;
    }
    base *= base;
    exp /= 2;
  }
  return result;
}

// 64-bit integer exponentiation
int64_t __moksha_powi64(int64_t base, int64_t exp) {
  if (exp < 0) {
    return 0;
  }
  int64_t result = 1;
  while (exp > 0) {
    if (exp % 2 == 1) {
      result *= base;
    }
    base *= base;
    exp /= 2;
  }
  return result;
}

// Helper for 128-bit powers of 10
static moksha_int128_t moksha_pow10_128(int32_t exp) {
  if (exp < 0)
    return 0;
  moksha_int128_t res = 1;
  moksha_int128_t base = 10;
  while (exp > 0) {
    if (exp % 2 == 1)
      res *= base;
    base *= base;
    exp /= 2;
  }
  return res;
}

// Decimal Runtime Operations

void __moksha_dec_scale(MokshaDecimal *out, MokshaDecimal *dec,
                        int32_t target_scale) {
  if (dec->scale == target_scale) {
    *out = *dec;
    return;
  }
  out->scale = target_scale;
  if (target_scale > dec->scale) {
    out->mantissa = dec->mantissa * moksha_pow10_128(target_scale - dec->scale);
  } else {
    out->mantissa = dec->mantissa / moksha_pow10_128(dec->scale - target_scale);
  }
}

void __moksha_dec_add(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  out->mantissa = a_scaled.mantissa + b_scaled.mantissa;
  out->scale = max_scale;
}

void __moksha_dec_sub(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  out->mantissa = a_scaled.mantissa - b_scaled.mantissa;
  out->scale = max_scale;
}

void __moksha_dec_mul(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  out->mantissa = a->mantissa * b->mantissa;
  out->scale = a->scale + b->scale;
}

void __moksha_dec_div(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  out->scale = max_scale;

  if (b->mantissa == 0) {
    moksha_rt_panic("Math Error: Division by zero.");
    return;
  }

  int32_t pow_exp = b->scale + (max_scale - a->scale);
  moksha_int128_t scaled_ma = a->mantissa * moksha_pow10_128(pow_exp);
  out->mantissa = scaled_ma / b->mantissa;
}

void __moksha_dec_mod(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  if (b_scaled.mantissa == 0) {
    moksha_rt_panic("Math Error: Modulo by zero.");
    return;
  }

  out->mantissa = a_scaled.mantissa % b_scaled.mantissa;
  out->scale = max_scale;
}

int32_t __moksha_dec_cmp(MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  if (a_scaled.mantissa < b_scaled.mantissa)
    return -1;
  if (a_scaled.mantissa > b_scaled.mantissa)
    return 1;
  return 0;
}

static double internal_pow10(int32_t exp) {
  double res = 1.0;
  double base = 10.0;
  int32_t p = exp < 0 ? -exp : exp;
  while (p > 0) {
    if (p % 2 == 1)
      res *= base;
    base *= base;
    p /= 2;
  }
  return exp < 0 ? (1.0 / res) : res;
}

void __moksha_f64_to_decimal(MokshaDecimal *out, double val,
                             int32_t target_scale) {
  out->scale = target_scale;
  double multiplier = internal_pow10(target_scale);
  out->mantissa = (moksha_int128_t)(val * multiplier);
}

double __moksha_decimal_to_f64(MokshaDecimal *dec) {
  double val = (double)dec->mantissa;
  double divisor = internal_pow10(dec->scale);
  return val / divisor;
}

/** @brief Math Functions with Freestanding Fallbacks */

#if defined(__MOKSHA_BAREMETAL__)

static uint32_t bm_rand_seed = 123456789;

double moksha_rt_math_fmod(double a, double b) {
  if (b == 0.0)
    return NAN;
  int64_t q = (int64_t)(a / b);
  return a - ((double)q * b);
}

double moksha_rt_math_tan(double x) {
  // Polynomial approximation fallback
  double x2 = x * x;
  return x * (1.0 + x2 * (1.0 / 3.0 + x2 * (2.0 / 15.0)));
}

double moksha_rt_math_asin(double x) { return x; }
double moksha_rt_math_acos(double x) { return (PI / 2.0) - x; }
double moksha_rt_math_atan(double x) { return x / (1.0 + 0.28 * x * x); }
double moksha_rt_math_atan2(double y, double x) {
  return moksha_rt_math_atan(y / x);
}
double moksha_rt_math_cbrt(double x) { return x; }
double moksha_rt_math_hypot(double x, double y) {
  return (x * x + y * y) > 0 ? (x + y) * 0.7071067811865476 : 0.0;
}

void moksha_rt_math_seed(int32_t val) { bm_rand_seed = (uint32_t)val; }
double moksha_rt_math_random() {
  bm_rand_seed = bm_rand_seed * 1103515245 + 12345;
  return (double)(bm_rand_seed & 0x7FFFFFFF) / 2147483647.0;
}

#else

double moksha_rt_math_tan(double x) { return tan(x); }
double moksha_rt_math_asin(double x) { return asin(x); }
double moksha_rt_math_acos(double x) { return acos(x); }
double moksha_rt_math_atan(double x) { return atan(x); }
double moksha_rt_math_atan2(double y, double x) { return atan2(y, x); }
double moksha_rt_math_cbrt(double x) { return cbrt(x); }
double moksha_rt_math_hypot(double x, double y) { return hypot(x, y); }
double moksha_rt_math_fmod(double a, double b) { return fmod(a, b); }

void moksha_rt_math_seed(int32_t val) { srand((unsigned int)val); }
double moksha_rt_math_random() { return (double)rand() / (double)RAND_MAX; }

#endif

double moksha_rt_math_mod(double a, double b) {
  double r = moksha_rt_math_fmod(a, b);
  return r < 0 ? r + b : r;
}

static double internal_fmin(double a, double b) { return a < b ? a : b; }
static double internal_fmax(double a, double b) { return a > b ? a : b; }

double moksha_rt_math_min(double a, double b) { return internal_fmin(a, b); }
double moksha_rt_math_max(double a, double b) { return internal_fmax(a, b); }
double moksha_rt_math_clamp(double x, double low, double high) {
  return internal_fmax(low, internal_fmin(x, high));
}
double moksha_rt_math_lerp(double a, double b, double t) {
  return a + t * (b - a);
}
double moksha_rt_math_sign(double x) {
  if (x > 0.0)
    return 1.0;
  if (x < 0.0)
    return -1.0;
  return 0.0;
}

int32_t moksha_rt_math_randint(int32_t min, int32_t max) {
  if (min >= max)
    return min;
  return min + (int32_t)(moksha_rt_math_random() * (double)(max - min + 1));
}

bool moksha_rt_math_isPowerOf2(int32_t x) {
  return (x > 0) && ((x & (x - 1)) == 0);
}

bool moksha_rt_math_isnan(double x) { return x != x; }
bool moksha_rt_math_isinf(double x) { return (x == INF || x == -INF); }
bool moksha_rt_math_isfinite(double x) {
  return !moksha_rt_math_isnan(x) && !moksha_rt_math_isinf(x);
}

bool moksha_rt_math_is_close(double a, double b, double epsilon) {
  double diff = a - b;
  if (diff < 0)
    diff = -diff;
  return diff < epsilon;
}

/* @brief LLVM COMPILER-RT FALLBACKS FOR 32-BIT ARCHITECTURES (i686, arm, rv32)
 */

// Core unsigned 64-bit division and modulo algorithm
static uint64_t __udivmoddi4(uint64_t a, uint64_t b, uint64_t *rem) {
  uint64_t res = 0;
  uint64_t r = 0;

  if (b == 0) {
    // Hardware division by zero trap fallback
    if (rem)
      *rem = 0;
    return 0;
  }

  for (int i = 63; i >= 0; i--) {
    r = (r << 1) | ((a >> i) & 1);
    if (r >= b) {
      r -= b;
      res |= (1ULL << i);
    }
  }
  if (rem)
    *rem = r;
  return res;
}

// Signed 64-bit division (Required by LLVM on 32-bit targets)
int64_t __divdi3(int64_t a, int64_t b) {
  int minus = 0;
  if (a < 0) {
    a = -a;
    minus = 1;
  }
  if (b < 0) {
    b = -b;
    minus ^= 1;
  }

  uint64_t res = __udivmoddi4((uint64_t)a, (uint64_t)b, 0);
  return minus ? -(int64_t)res : (int64_t)res;
}

// Signed 64-bit modulo (Required by LLVM on 32-bit targets)
int64_t __moddi3(int64_t a, int64_t b) {
  int minus = 0;
  uint64_t rem;
  if (a < 0) {
    a = -a;
    minus = 1;
  }
  if (b < 0) {
    b = -b;
  }

  __udivmoddi4((uint64_t)a, (uint64_t)b, &rem);
  return minus ? -(int64_t)rem : (int64_t)rem;
}

// Unsigned 64-bit division
uint64_t __udivdi3(uint64_t a, uint64_t b) { return __udivmoddi4(a, b, 0); }

// Unsigned 64-bit modulo
uint64_t __umoddi3(uint64_t a, uint64_t b) {
  uint64_t rem;
  __udivmoddi4(a, b, &rem);
  return rem;
}

// ============================================================================
// 64-bit Float-to-Int Conversions for 32-bit targets (RV32, ARM32, i686)
// ============================================================================

// Convert uint64_t to double
double __floatundidf(uint64_t a) {
    // Break into 32-bit chunks so the compiler doesn't recursively call this function
    uint32_t high = (uint32_t)(a >> 32);
    uint32_t low = (uint32_t)(a & 0xFFFFFFFF);
    return ((double)high * 4294967296.0) + (double)low;
}

// Convert int64_t to double
double __floatdidf(int64_t a) {
    if (a < 0) {
        return -__floatundidf((uint64_t)-a);
    }
    return __floatundidf((uint64_t)a);
}

// Convert double to uint64_t
uint64_t __fixunsdfdi(double a) {
    if (a <= 0.0) return 0;
    if (a >= 18446744073709551615.0) return 18446744073709551615ULL;
    uint32_t high = (uint32_t)(a / 4294967296.0);
    uint32_t low = (uint32_t)(a - ((double)high * 4294967296.0));
    return ((uint64_t)high << 32) | low;
}

// Convert double to int64_t
int64_t __fixdfdi(double a) {
    if (a < 0.0) {
        return -(int64_t)__fixunsdfdi(-a);
    }
    return (int64_t)__fixunsdfdi(a);
}
