#include "../../include/moksha_rt.h"
#include <math.h>
#include <stdint.h>

// 32-bit integer exponentiation
int32_t __moksha_powi32(int32_t base, int32_t exp) {
  if (exp < 0) {
    return 0; // Integer division behavior for negative exponents
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

// 64-bit integer exponentiation (for future-proofing)
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
static __int128 moksha_pow10_128(int32_t exp) {
  if (exp < 0)
    return 0;
  __int128 res = 1;
  __int128 base = 10;
  while (exp > 0) {
    if (exp % 2 == 1)
      res *= base;
    base *= base;
    exp /= 2;
  }
  return res;
}

// --- Decimal Runtime Operations ---

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
    // FIXED: Do not silently absorb division by zero!
    moksha_rt_panic("Math Error: Division by zero.");
    return;
  }

  int32_t pow_exp = b->scale + (max_scale - a->scale);
  __int128 scaled_ma = a->mantissa * moksha_pow10_128(pow_exp);
  out->mantissa = scaled_ma / b->mantissa;
}

void __moksha_dec_mod(MokshaDecimal *out, MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  if (b_scaled.mantissa == 0) {
    // FIXED: Do not silently absorb modulo by zero!
    moksha_rt_panic("Math Error: Modulo by zero.");
    return;
  }

  out->mantissa = a_scaled.mantissa % b_scaled.mantissa;
  out->scale = max_scale;
}

// Decimal Comparison (-1 for less, 0 for equal, 1 for greater)
int32_t __moksha_dec_cmp(MokshaDecimal *a, MokshaDecimal *b) {
  int32_t max_scale = a->scale > b->scale ? a->scale : b->scale;
  MokshaDecimal a_scaled, b_scaled;
  __moksha_dec_scale(&a_scaled, a, max_scale);
  __moksha_dec_scale(&b_scaled, b, max_scale);

  if (a_scaled.mantissa < b_scaled.mantissa)
    return -1;
  if (a_scaled.mantissa > b_scaled.mantissa)
    return 1;
  return 0; // Equal
}

// --- Float to Decimal ---
void __moksha_f64_to_decimal(MokshaDecimal *out, double val,
                             int32_t target_scale) {
  out->scale = target_scale;
  // Shift the fractional part into the integer part
  double multiplier = pow(10.0, target_scale);
  out->mantissa = (__int128)(val * multiplier);
}

// --- Decimal to Float ---
double __moksha_decimal_to_f64(MokshaDecimal *dec) {
  double val = (double)dec->mantissa;
  double divisor = pow(10.0, dec->scale);
  return val / divisor;
}
