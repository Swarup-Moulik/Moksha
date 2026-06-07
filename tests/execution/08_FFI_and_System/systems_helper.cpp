#include <cfenv>
#include <cstdint>
#include <iostream>
#include <stdexcept>

// ============================================================================
// Memory Layout Definitions (Matching Moksha's AST)
// ============================================================================

struct RefTarget {
  int32_t x;
};

struct ComplexDouble {
  double re;
  double im;
};

// Explicit 32-byte alignment for AVX vector tests
struct alignas(32) Vec256 {
  float f1, f2, f3, f4, f5, f6, f7, f8;
};

struct BrokenHFA {
  float a;
  float b;
  int32_t c;
};

struct Huge {
  int64_t data[8];
};

struct MixedRet {
  int32_t a;
  float b;
};

struct Vector4 {
  float x;
};

struct Inner {
  int32_t a;
  float b;
};

struct Outer {
  Inner x;
  Inner y;
};

struct Padded {
  int8_t a;
  int32_t b;
};

struct Split {
  float a;
  int32_t b;
};

struct ThreeReg {
  int64_t a;
  int64_t b;
  int64_t c;
};

struct TwoRegStruct {
  int64_t a;
  int64_t b;
};

struct Float4 {
  float x, y, z, w;
};

struct Empty {};

struct SmallPair {
  int32_t a;
  int32_t b;
};

struct MixedStruct {
  int32_t a;
  float b;
};

struct WeirdCollision {
  double a;
  int32_t b;
  double c;
};

struct HugeData {
  int64_t data[16];
};

// ============================================================================
// Extern "C" Implementations
// ============================================================================

extern "C" {

// 1. FLOAT RETURN ABI
double return_double_identity(double x) {
  std::cout << "[C++] return_double_identity received: " << x << "\n";
  return x;
}

// 2. COMPLEX NUMBERS
ComplexDouble return_complex_pair() { return {3.14, 2.71}; }

// 3. INTEGER EXTENSION
void takes_signed_char(int8_t c) {
  std::cout << "[C++] takes_signed_char: " << static_cast<int>(c) << "\n";
}

void takes_unsigned_char(uint8_t c) {
  std::cout << "[C++] takes_unsigned_char: " << static_cast<int>(c) << "\n";
}

// 4. THE 256-BIT VECTOR (AVX)
void pass_vec256(Vec256 v) {
  std::cout << "[C++] pass_vec256 received f1: " << v.f1 << ", f8: " << v.f8
            << "\n";
}

// 5. NATIVE REFERENCE ABI
void take_by_ref(RefTarget *c) {
  if (c) {
    std::cout << "[C++] take_by_ref received target.x = " << c->x << "\n";
    c->x = 999;
  } else {
    std::cout << "[C++] take_by_ref received a null pointer!\n";
  }
}

// 7. PLT/GOT RELOCATION
void external_shared_lib_func() {
  std::cout << "[C++] external_shared_lib_func executed successfully!\n";
}

// 8. EVALUATION ORDER BARRIER
void side_effect(int32_t a, int32_t b) {
  std::cout << "[C++] side_effect called with a=" << a << ", b=" << b << "\n";
}

void check_broken_hfa(BrokenHFA s) {
  std::cout << "[C++] check_broken_hfa received: a=" << s.a << ", b=" << s.b
            << ", c=" << s.c << "\n";
}

void spill_args(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5,
                int32_t a6, int32_t a7, int32_t a8) {
  std::cout << "[C++] spill_args received a7: " << a7 << ", a8: " << a8 << "\n";
}

void set_rounding_downward() {
  std::fesetround(FE_DOWNWARD);
  std::cout << "[C++] Rounding mode set to DOWNWARD\n";
}

int32_t get_rounding_mode() { return std::fegetround(); }

int32_t tail_recursive_sum(int32_t count, int32_t acc) {
  int32_t sum = acc;
  for (int32_t i = count; i > 0; --i) {
    sum += i;
  }
  return sum;
}

Huge return_huge() {
  Huge h;
  for (int i = 0; i < 8; i++) {
    h.data[i] = i * 100;
  }
  std::cout << "[C++] return_huge populated 64-byte array\n";
  return h;
}

MixedRet return_mixed() {
  std::cout << "[C++] return_mixed returning {42, 3.14}\n";
  return {42, 3.14f};
}

void simd_func(Vector4 v) {
  std::cout << "[C++] simd_func received x: " << v.x << "\n";
}

void pass_nested_aggregate(Outer o) {
  std::cout << "[C++] pass_nested_aggregate received o.x.a=" << o.x.a << "\n";
}

Padded return_padded_struct() {
  std::cout << "[C++] return_padded_struct executing\n";
  Padded p;
  p.a = 'A';
  p.b = 999;
  return p;
}

void register_float_callback(void (*cb)(float)) {
  std::cout << "[C++] register_float_callback executing\n";
  if (cb) {
    cb(3.14159f); // Call the Moksha function from C++!
  }
}

float pass_float(float f) {
  std::cout << "[C++] pass_float received: " << f << "\n";
  return f;
}

// 1. THE 8-BYTE SPLIT
void pass_split_struct(Split s) {
  std::cout << "[C++] pass_split_struct received: a=" << s.a << ", b=" << s.b
            << "\n";
}

// 4. TAIL CALL STACK NEUTRALITY
int32_t terminal_callee(int32_t x) {
  std::cout << "[C++] terminal_callee executing with x=" << x << "\n";
  return x;
}

// 7. EXCEPTION UNWINDING ACROSS FFI
void external_c_thrower() {
  std::cout << "[C++] external_c_thrower throwing exception!\n";
  throw std::runtime_error("Exception from C++ land");
}

// 8. SIZED ENUMS & PROMOTION
void pass_huge_enum(uint64_t e) {
  std::cout << "[C++] pass_huge_enum received: " << e << "\n";
}

// 9. THE 3-REGISTER RETURN
ThreeReg return_three_reg() {
  std::cout << "[C++] return_three_reg returning {1, 2, 3}\n";
  return {1, 2, 3};
}

// 1. ABI CLASSIFICATION
TwoRegStruct return_two_reg() {
  std::cout << "[C++] return_two_reg returning {10, 20}\n";
  return {10, 20};
}

Float4 process_hfa(Float4 input) {
  std::cout << "[C++] process_hfa received x: " << input.x << "\n";
  return {input.x + 1.0f, input.y + 1.0f, input.z + 1.0f, input.w + 1.0f};
}

void pass_empty(Empty e) {
  std::cout << "[C++] pass_empty received (C++ size: " << sizeof(e) << ")\n";
}

// 7. FUNCTION POINTERS ACROSS FFI
void register_os_callback(void (*cb)(int32_t)) {
  std::cout
      << "[C++] register_os_callback triggering callback with status 200\n";
  if (cb) {
    cb(200); // Call the Moksha function!
  }
}

// (a) Mixed Integer + Float ABI Passing
void mixed_args(int32_t a, float b, double c, int32_t d) {
  std::cout << "[C++] mixed_args received: a=" << a << ", b=" << b
            << ", c=" << c << ", d=" << d << "\n";
}

// (b) Struct ABI Classification
SmallPair return_small_pair() {
  std::cout << "[C++] return_small_pair returning {100, 200}\n";
  return {100, 200};
}

void pass_mixed_struct(MixedStruct m) {
  std::cout << "[C++] pass_mixed_struct received: a=" << m.a << ", b=" << m.b
            << "\n";
}

// (9) Type System ABI Interop (Low-level representations)
void log_status(int32_t s) {
  std::cout << "[C++] log_status received: " << s << "\n";
}

void enable_feature(bool b) {
  std::cout << "[C++] enable_feature received: " << (b ? "true" : "false")
            << "\n";
}

// 1. ABI CLASSIFICATION: THE MEMORY FALLBACK TRAP
WeirdCollision return_weird() {
  std::cout << "[C++] return_weird returning {1.1, 42, 2.2}\n";
  return {1.1, 42, 2.2};
}

// 2. SRET & BYVAL
HugeData return_huge_sret() {
  HugeData h;
  for (int i = 0; i < 16; i++)
    h.data[i] = i * 11;
  return h;
}

void take_huge_byval(HugeData d) {
  std::cout << "[C++] take_huge_byval received data[15]: " << d.data[15]
            << "\n";
}

// 3. FLOATING POINT & SIMD
double pass_ld(double x) { return x; }

void pass_true_vector(Float4 v) {
  std::cout << "[C++] pass_true_vector received x: " << v.x << "\n";
}

// 4. CALLER-SAVED & RED ZONE STRESS
void clobber_registers() {
  // Trashing callee-saved registers to force the Moksha compiler
  // to prove it saved them correctly.
  asm volatile("movq $0xDEADBEEF, %%r12\n\t"
               "movq $0xDEADBEEF, %%r13\n\t"
               "movq $0xDEADBEEF, %%r14\n\t"
               "movq $0xDEADBEEF, %%r15\n\t" ::
                   : "r12", "r13", "r14", "r15");
  std::cout << "[C++] clobber_registers: R12-R15 filled with garbage\n";
}

} // extern "C"
