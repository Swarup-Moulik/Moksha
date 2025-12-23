; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Dummy = type { ptr }

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [21 x i8] c"==== ENUM SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [29 x i8] c"\0A[1. C-Style Auto-Increment]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [18 x i8] c"Idle (Expect 0): \00", align 1
@5 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@6 = private unnamed_addr constant [21 x i8] c"Running (Expect 1): \00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@8 = private unnamed_addr constant [21 x i8] c"Stopped (Expect 2): \00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@10 = private unnamed_addr constant [26 x i8] c"\0A[2. Explicit Assignment]\00", align 1
@11 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@12 = private unnamed_addr constant [5 x i8] c"OK: \00", align 1
@13 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@14 = private unnamed_addr constant [13 x i8] c"BadRequest: \00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@16 = private unnamed_addr constant [12 x i8] c"ServerErr: \00", align 1
@17 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@18 = private unnamed_addr constant [23 x i8] c"\0A[3. Mixed Assignment]\00", align 1
@19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@20 = private unnamed_addr constant [11 x i8] c"Low (10): \00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [14 x i8] c"Medium (11): \00", align 1
@23 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@24 = private unnamed_addr constant [12 x i8] c"High (20): \00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [16 x i8] c"Critical (21): \00", align 1
@27 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@28 = private unnamed_addr constant [25 x i8] c"\0A[4. Enums in Switch/If]\00", align 1
@29 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@30 = private unnamed_addr constant [25 x i8] c"System is RUNNING (Pass)\00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@32 = private unnamed_addr constant [29 x i8] c"System is NOT running (Fail)\00", align 1
@33 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@34 = private unnamed_addr constant [13 x i8] c"Switch: Idle\00", align 1
@35 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@36 = private unnamed_addr constant [23 x i8] c"Switch: Running (Pass)\00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@38 = private unnamed_addr constant [16 x i8] c"Switch: Stopped\00", align 1
@39 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@40 = private unnamed_addr constant [16 x i8] c"Switch: Unknown\00", align 1
@41 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@42 = private unnamed_addr constant [29 x i8] c"\0A=== ENUM TEST COMPLETED ===\00", align 1
@43 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@44 = private unnamed_addr constant [29 x i8] c"==== MACRO SYNTAX SUITE ====\00", align 1
@45 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@46 = private unnamed_addr constant [14 x i8] c"[LOG MACRO]: \00", align 1
@47 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@48 = private unnamed_addr constant [14 x i8] c"Calc Result: \00", align 1
@49 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@50 = private unnamed_addr constant [29 x i8] c"Macros defined successfully.\00", align 1
@51 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@52 = private unnamed_addr constant [60 x i8] c"(Note: Macros do not execute at runtime in current version)\00", align 1
@53 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@54 = private unnamed_addr constant [13 x i8] c"System Start\00", align 1
@55 = private unnamed_addr constant [29 x i8] c"=== MACRO TEST COMPLETED ===\00", align 1
@56 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@57 = private unnamed_addr constant [44 x i8] c"==== PROFESSIONAL ERROR HANDLING SUITE ====\00", align 1
@58 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@59 = private unnamed_addr constant [24 x i8] c"\0A[1. Arithmetic Safety]\00", align 1
@60 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@61 = private unnamed_addr constant [53 x i8] c"  [Test 1.1] Attempting Int Div by Zero (100 / 0)...\00", align 1
@62 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@63 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@64 = private unnamed_addr constant [42 x i8] c"  FAIL: Operation proceeded with result: \00", align 1
@65 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@66 = private unnamed_addr constant [38 x i8] c"  PASS: Caught Arithmetic Exception: \00", align 1
@67 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@68 = private unnamed_addr constant [52 x i8] c"  [Test 1.2] Attempting Modulo by Zero (100 % 0)...\00", align 1
@69 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@70 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@71 = private unnamed_addr constant [42 x i8] c"  FAIL: Operation proceeded with result: \00", align 1
@72 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@73 = private unnamed_addr constant [38 x i8] c"  PASS: Caught Arithmetic Exception: \00", align 1
@74 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@75 = private unnamed_addr constant [59 x i8] c"  [Test 1.3] Attempting Double Div by Zero (10.0 / 0.0)...\00", align 1
@76 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@77 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@78 = private unnamed_addr constant [21 x i8] c"  INFO: Result was: \00", align 1
@79 = private unnamed_addr constant [38 x i8] c" (Infinity is acceptable for Doubles)\00", align 1
@80 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@81 = private unnamed_addr constant [38 x i8] c"  PASS: Caught Arithmetic Exception: \00", align 1
@82 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@83 = private unnamed_addr constant [29 x i8] c"\0A[2. Memory & Bounds Safety]\00", align 1
@84 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@85 = private unnamed_addr constant [50 x i8] c"  [Test 2.1] Accessing index 5 of size 3 array...\00", align 1
@86 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@87 = private unnamed_addr constant [26 x i8] c"  FAIL: Retrieved value: \00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@89 = private unnamed_addr constant [34 x i8] c"  PASS: Caught IndexOutOfBounds: \00", align 1
@90 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@91 = private unnamed_addr constant [35 x i8] c"  [Test 2.2] Accessing index -1...\00", align 1
@92 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@93 = private unnamed_addr constant [26 x i8] c"  FAIL: Retrieved value: \00", align 1
@94 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@95 = private unnamed_addr constant [34 x i8] c"  PASS: Caught IndexOutOfBounds: \00", align 1
@96 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@97 = private unnamed_addr constant [4 x i8] c"abc\00", align 1
@98 = private unnamed_addr constant [52 x i8] c"  [Test 2.3] Accessing char at index 10 of 'abc'...\00", align 1
@99 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@100 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@101 = private unnamed_addr constant [52 x i8] c"  WARN: Returned empty string (permissive behavior)\00", align 1
@102 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@103 = private unnamed_addr constant [25 x i8] c"  FAIL: Retrieved char: \00", align 1
@104 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@105 = private unnamed_addr constant [34 x i8] c"  PASS: Caught IndexOutOfBounds: \00", align 1
@106 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@107 = private unnamed_addr constant [21 x i8] c"\0A[3. Type Integrity]\00", align 1
@108 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@109 = private unnamed_addr constant [12 x i8] c"NotAnNumber\00", align 1
@110 = private unnamed_addr constant [45 x i8] c"  [Test 3.1] Casting 'NotAnNumber' to int...\00", align 1
@111 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@112 = private unnamed_addr constant [23 x i8] c"  INFO: Converted to: \00", align 1
@113 = private unnamed_addr constant [51 x i8] c" (0 is standard C behavior, Exception is stricter)\00", align 1
@114 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@115 = private unnamed_addr constant [33 x i8] c"  PASS: Caught FormatException: \00", align 1
@116 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Dummy = constant [1 x ptr] [ptr @Dummy_act]
@117 = private unnamed_addr constant [72 x i8] c"  [Test 3.2] Calling method on null object without optional chaining...\00", align 1
@118 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@119 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@120 = private unnamed_addr constant [38 x i8] c"  FAIL: Method call proceeded on null\00", align 1
@121 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@122 = private unnamed_addr constant [40 x i8] c"  PASS: Caught NullReferenceException: \00", align 1
@123 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@124 = private unnamed_addr constant [29 x i8] c"\0A[4. Control Flow Integrity]\00", align 1
@125 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@126 = private unnamed_addr constant [16 x i8] c"ErrorInFunction\00", align 1
@127 = private unnamed_addr constant [44 x i8] c"  [Test 4.1] Exception bubbling up stack...\00", align 1
@128 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@129 = private unnamed_addr constant [29 x i8] c"  FAIL: Stack did not unwind\00", align 1
@130 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@131 = private unnamed_addr constant [35 x i8] c"  PASS: Caught bubbled exception: \00", align 1
@132 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@133 = private unnamed_addr constant [46 x i8] c"  [Test 4.2] Verifying 'finally' execution...\00", align 1
@134 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@135 = private unnamed_addr constant [5 x i8] c"Boom\00", align 1
@136 = private unnamed_addr constant [30 x i8] c"    -> Inner finally executed\00", align 1
@137 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@138 = private unnamed_addr constant [48 x i8] c"  PASS: Finally ran before catch block finished\00", align 1
@139 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@140 = private unnamed_addr constant [30 x i8] c"  FAIL: Finally block skipped\00", align 1
@141 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@142 = private unnamed_addr constant [36 x i8] c"\0A[5. Resource Limits (Stress Test)]\00", align 1
@143 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@144 = private unnamed_addr constant [47 x i8] c"  [Test 5.1] Allocating negative array size...\00", align 1
@145 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@146 = private unnamed_addr constant [38 x i8] c"  FAIL: Allocated negative size array\00", align 1
@147 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@148 = private unnamed_addr constant [37 x i8] c"  PASS: Caught AllocationException: \00", align 1
@149 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@150 = private unnamed_addr constant [38 x i8] c"\0A=== PROFESSIONAL SUITE COMPLETED ===\00", align 1
@151 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare ptr @calloc(i64, i64)

declare void @moksha_free(ptr)

declare ptr @strcpy(ptr, ptr)

declare ptr @memcpy(ptr, ptr, i64)

declare ptr @memset(ptr, i32, i64)

declare i32 @sprintf(ptr, ptr, ...)

declare i32 @fprintf(ptr, ptr, ...)

declare i32 @strlen(ptr)

declare i32 @puts(ptr)

declare ptr @moksha_box_int(i32)

declare ptr @moksha_box_double(double)

declare ptr @moksha_box_bool(i32)

declare ptr @moksha_box_string(ptr)

declare ptr @moksha_box_char(i8)

declare ptr @moksha_box_short(i16)

declare ptr @moksha_box_long(i64)

declare ptr @moksha_box_float(float)

declare i32 @moksha_unbox_int(ptr)

declare double @moksha_unbox_double(ptr)

declare ptr @moksha_unbox_string(ptr)

declare ptr @moksha_any_to_str(ptr)

declare ptr @moksha_int_to_str(i32)

declare ptr @moksha_int_to_ascii(i32)

declare ptr @moksha_double_to_str(double)

declare ptr @moksha_alloc_array(i32)

declare ptr @moksha_alloc_table(i32)

declare ptr @moksha_alloc_array_fill(i32, ptr)

declare ptr @moksha_table_set(ptr, ptr, ptr)

declare ptr @moksha_table_get(ptr, ptr)

declare void @moksha_table_delete(ptr, ptr)

declare ptr @moksha_table_keys(ptr)

declare ptr @moksha_array_get(ptr, i32)

declare void @moksha_array_set(ptr, i32, ptr)

declare ptr @moksha_array_push_val(ptr, ptr)

declare ptr @moksha_array_spread(ptr, ptr)

declare void @moksha_array_delete(ptr, i32)

declare ptr @moksha_string_concat(ptr, ptr)

declare i32 @moksha_get_length(ptr)

declare ptr @moksha_string_get_char(ptr, i32)

declare i32 @moksha_read_int()

declare double @moksha_read_double()

declare i32 @moksha_unbox_bool(ptr)

declare i32 @moksha_read_bool()

declare ptr @moksha_read_string()

declare void @moksha_throw_exception(ptr)

declare ptr @moksha_create_closure(ptr, i32, ptr)

declare ptr @moksha_get_closure_env(ptr)

declare ptr @moksha_get_closure_func(ptr)

declare i32 @moksha_strlen(ptr)

declare i32 @strcmp(ptr, ptr)

declare ptr @moksha_ptr_to_str(ptr)

define i32 @main() {
entry:
  %e287 = alloca ptr, align 8
  %negArr281 = alloca ptr, align 8
  %negArr = alloca ptr, align 8
  %e261 = alloca ptr, align 8
  %finallyRun244 = alloca i1, align 1
  %finallyRun = alloca i1, align 1
  %e239 = alloca ptr, align 8
  %e222 = alloca ptr, align 8
  %d211 = alloca ptr, align 8
  %d = alloca ptr, align 8
  %e203 = alloca ptr, align 8
  %val197 = alloca i32, align 4
  %val196 = alloca i32, align 4
  %badStr190 = alloca ptr, align 8
  %badStr = alloca ptr, align 8
  %e181 = alloca ptr, align 8
  %c167 = alloca ptr, align 8
  %c = alloca ptr, align 8
  %s161 = alloca ptr, align 8
  %s = alloca ptr, align 8
  %e153 = alloca ptr, align 8
  %val147 = alloca i32, align 4
  %val146 = alloca i32, align 4
  %arr140 = alloca ptr, align 8
  %arr136 = alloca ptr, align 8
  %e128 = alloca ptr, align 8
  %val122 = alloca i32, align 4
  %val = alloca i32, align 4
  %arr116 = alloca ptr, align 8
  %arr = alloca ptr, align 8
  %e105 = alloca ptr, align 8
  %res99 = alloca double, align 8
  %res94 = alloca double, align 8
  %y88 = alloca double, align 8
  %y = alloca double, align 8
  %x85 = alloca double, align 8
  %x = alloca double, align 8
  %e77 = alloca ptr, align 8
  %res71 = alloca i32, align 4
  %res66 = alloca i32, align 4
  %e = alloca ptr, align 8
  %res50 = alloca i32, align 4
  %res = alloca i32, align 4
  %b44 = alloca i32, align 4
  %b = alloca i32, align 4
  %a41 = alloca i32, align 4
  %a = alloca i32, align 4
  %currentStatus15 = alloca i32, align 4
  %currentStatus = alloca i32, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  %4 = call ptr @moksha_box_string(ptr @4)
  %5 = call ptr @moksha_int_to_str(i32 0)
  %6 = call ptr @moksha_box_string(ptr %5)
  %7 = call ptr @moksha_string_concat(ptr %4, ptr %6)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @5, ptr %print_unbox2)
  %9 = call ptr @moksha_box_string(ptr @6)
  %10 = call ptr @moksha_int_to_str(i32 1)
  %11 = call ptr @moksha_box_string(ptr %10)
  %12 = call ptr @moksha_string_concat(ptr %9, ptr %11)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %12)
  %13 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox3)
  %14 = call ptr @moksha_box_string(ptr @8)
  %15 = call ptr @moksha_int_to_str(i32 2)
  %16 = call ptr @moksha_box_string(ptr %15)
  %17 = call ptr @moksha_string_concat(ptr %14, ptr %16)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %17)
  %18 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox4)
  %19 = call ptr @moksha_box_string(ptr @10)
  %print_unbox5 = call ptr @moksha_unbox_string(ptr %19)
  %20 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox5)
  %21 = call ptr @moksha_box_string(ptr @12)
  %22 = call ptr @moksha_int_to_str(i32 200)
  %23 = call ptr @moksha_box_string(ptr %22)
  %24 = call ptr @moksha_string_concat(ptr %21, ptr %23)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %24)
  %25 = call i32 (ptr, ...) @printf(ptr @13, ptr %print_unbox6)
  %26 = call ptr @moksha_box_string(ptr @14)
  %27 = call ptr @moksha_int_to_str(i32 400)
  %28 = call ptr @moksha_box_string(ptr %27)
  %29 = call ptr @moksha_string_concat(ptr %26, ptr %28)
  %print_unbox7 = call ptr @moksha_unbox_string(ptr %29)
  %30 = call i32 (ptr, ...) @printf(ptr @15, ptr %print_unbox7)
  %31 = call ptr @moksha_box_string(ptr @16)
  %32 = call ptr @moksha_int_to_str(i32 500)
  %33 = call ptr @moksha_box_string(ptr %32)
  %34 = call ptr @moksha_string_concat(ptr %31, ptr %33)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %34)
  %35 = call i32 (ptr, ...) @printf(ptr @17, ptr %print_unbox8)
  %36 = call ptr @moksha_box_string(ptr @18)
  %print_unbox9 = call ptr @moksha_unbox_string(ptr %36)
  %37 = call i32 (ptr, ...) @printf(ptr @19, ptr %print_unbox9)
  %38 = call ptr @moksha_box_string(ptr @20)
  %39 = call ptr @moksha_int_to_str(i32 10)
  %40 = call ptr @moksha_box_string(ptr %39)
  %41 = call ptr @moksha_string_concat(ptr %38, ptr %40)
  %print_unbox10 = call ptr @moksha_unbox_string(ptr %41)
  %42 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox10)
  %43 = call ptr @moksha_box_string(ptr @22)
  %44 = call ptr @moksha_int_to_str(i32 11)
  %45 = call ptr @moksha_box_string(ptr %44)
  %46 = call ptr @moksha_string_concat(ptr %43, ptr %45)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %46)
  %47 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox11)
  %48 = call ptr @moksha_box_string(ptr @24)
  %49 = call ptr @moksha_int_to_str(i32 20)
  %50 = call ptr @moksha_box_string(ptr %49)
  %51 = call ptr @moksha_string_concat(ptr %48, ptr %50)
  %print_unbox12 = call ptr @moksha_unbox_string(ptr %51)
  %52 = call i32 (ptr, ...) @printf(ptr @25, ptr %print_unbox12)
  %53 = call ptr @moksha_box_string(ptr @26)
  %54 = call ptr @moksha_int_to_str(i32 21)
  %55 = call ptr @moksha_box_string(ptr %54)
  %56 = call ptr @moksha_string_concat(ptr %53, ptr %55)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %56)
  %57 = call i32 (ptr, ...) @printf(ptr @27, ptr %print_unbox13)
  %58 = call ptr @moksha_box_string(ptr @28)
  %print_unbox14 = call ptr @moksha_unbox_string(ptr %58)
  %59 = call i32 (ptr, ...) @printf(ptr @29, ptr %print_unbox14)
  store i32 1, ptr %currentStatus, align 4
  store i32 1, ptr %currentStatus15, align 4
  %60 = load i32, ptr %currentStatus15, align 4
  %icmptmp = icmp eq i32 %60, 1
  br i1 %icmptmp, label %then, label %else

then:                                             ; preds = %entry
  %61 = call ptr @moksha_box_string(ptr @30)
  %print_unbox16 = call ptr @moksha_unbox_string(ptr %61)
  %62 = call i32 (ptr, ...) @printf(ptr @31, ptr %print_unbox16)
  %63 = load i32, ptr @__exception_flag, align 4
  %64 = icmp ne i32 %63, 0
  br i1 %64, label %unwind, label %next_stmt

else:                                             ; preds = %entry
  %65 = call ptr @moksha_box_string(ptr @32)
  %print_unbox17 = call ptr @moksha_unbox_string(ptr %65)
  %66 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox17)
  %67 = load i32, ptr @__exception_flag, align 4
  %68 = icmp ne i32 %67, 0
  br i1 %68, label %unwind19, label %next_stmt18

ifcont:                                           ; preds = %next_stmt18, %next_stmt
  %69 = load i32, ptr %currentStatus15, align 4
  switch i32 %69, label %default [
    i32 0, label %case
    i32 1, label %case20
    i32 2, label %case21
  ]

next_stmt:                                        ; preds = %then
  br label %ifcont

unwind:                                           ; preds = %then
  ret i32 0

next_stmt18:                                      ; preds = %else
  br label %ifcont

unwind19:                                         ; preds = %else
  ret i32 0

switchend:                                        ; preds = %next_stmt32, %next_stmt29, %next_stmt26, %next_stmt23
  %70 = call ptr @moksha_box_string(ptr @42)
  %print_unbox34 = call ptr @moksha_unbox_string(ptr %70)
  %71 = call i32 (ptr, ...) @printf(ptr @43, ptr %print_unbox34)
  %72 = call ptr @moksha_box_string(ptr @44)
  %print_unbox35 = call ptr @moksha_unbox_string(ptr %72)
  %73 = call i32 (ptr, ...) @printf(ptr @45, ptr %print_unbox35)
  %74 = call ptr @moksha_box_string(ptr @50)
  %print_unbox36 = call ptr @moksha_unbox_string(ptr %74)
  %75 = call i32 (ptr, ...) @printf(ptr @51, ptr %print_unbox36)
  %76 = call ptr @moksha_box_string(ptr @52)
  %print_unbox37 = call ptr @moksha_unbox_string(ptr %76)
  %77 = call i32 (ptr, ...) @printf(ptr @53, ptr %print_unbox37)
  %78 = call ptr @moksha_box_string(ptr @54)
  call void @log(ptr %78)
  %79 = call ptr @moksha_box_int(i32 10)
  %80 = call ptr @moksha_box_int(i32 20)
  call void @performCalc(ptr %79, ptr %80)
  %81 = call ptr @moksha_box_string(ptr @55)
  %print_unbox38 = call ptr @moksha_unbox_string(ptr %81)
  %82 = call i32 (ptr, ...) @printf(ptr @56, ptr %print_unbox38)
  %83 = call ptr @moksha_box_string(ptr @57)
  %print_unbox39 = call ptr @moksha_unbox_string(ptr %83)
  %84 = call i32 (ptr, ...) @printf(ptr @58, ptr %print_unbox39)
  %85 = call ptr @moksha_box_string(ptr @59)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %85)
  %86 = call i32 (ptr, ...) @printf(ptr @60, ptr %print_unbox40)
  br label %try_block

default:                                          ; preds = %ifcont
  %87 = call ptr @moksha_box_string(ptr @40)
  %print_unbox31 = call ptr @moksha_unbox_string(ptr %87)
  %88 = call i32 (ptr, ...) @printf(ptr @41, ptr %print_unbox31)
  %89 = load i32, ptr @__exception_flag, align 4
  %90 = icmp ne i32 %89, 0
  br i1 %90, label %unwind33, label %next_stmt32

case:                                             ; preds = %ifcont
  %91 = call ptr @moksha_box_string(ptr @34)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %91)
  %92 = call i32 (ptr, ...) @printf(ptr @35, ptr %print_unbox22)
  %93 = load i32, ptr @__exception_flag, align 4
  %94 = icmp ne i32 %93, 0
  br i1 %94, label %unwind24, label %next_stmt23

case20:                                           ; preds = %ifcont
  %95 = call ptr @moksha_box_string(ptr @36)
  %print_unbox25 = call ptr @moksha_unbox_string(ptr %95)
  %96 = call i32 (ptr, ...) @printf(ptr @37, ptr %print_unbox25)
  %97 = load i32, ptr @__exception_flag, align 4
  %98 = icmp ne i32 %97, 0
  br i1 %98, label %unwind27, label %next_stmt26

case21:                                           ; preds = %ifcont
  %99 = call ptr @moksha_box_string(ptr @38)
  %print_unbox28 = call ptr @moksha_unbox_string(ptr %99)
  %100 = call i32 (ptr, ...) @printf(ptr @39, ptr %print_unbox28)
  %101 = load i32, ptr @__exception_flag, align 4
  %102 = icmp ne i32 %101, 0
  br i1 %102, label %unwind30, label %next_stmt29

next_stmt23:                                      ; preds = %case
  br label %switchend

unwind24:                                         ; preds = %case
  ret i32 0

next_stmt26:                                      ; preds = %case20
  br label %switchend

unwind27:                                         ; preds = %case20
  ret i32 0

next_stmt29:                                      ; preds = %case21
  br label %switchend

unwind30:                                         ; preds = %case21
  ret i32 0

next_stmt32:                                      ; preds = %default
  br label %switchend

unwind33:                                         ; preds = %default
  ret i32 0

try_block:                                        ; preds = %switchend
  store i32 100, ptr %a, align 4
  store i32 100, ptr %a41, align 4
  %103 = load i32, ptr @__exception_flag, align 4
  %104 = icmp ne i32 %103, 0
  br i1 %104, label %unwind43, label %next_stmt42

exception_check:                                  ; preds = %next_stmt54
  %105 = load i32, ptr @__exception_flag, align 4
  %106 = icmp ne i32 %105, 0
  br i1 %106, label %catch_block, label %try_cont

catch_block:                                      ; preds = %exception_check, %unwind55, %unwind52, %unwind49, %unwind46, %unwind43
  store i32 0, ptr @__exception_flag, align 4
  %107 = load ptr, ptr @__exception_val, align 8
  store ptr %107, ptr %e, align 8
  %108 = call ptr @moksha_box_string(ptr @66)
  %109 = load ptr, ptr %e, align 8
  %110 = call ptr @moksha_any_to_str(ptr %109)
  %111 = call ptr @moksha_box_string(ptr %110)
  %112 = call ptr @moksha_string_concat(ptr %108, ptr %111)
  %print_unbox56 = call ptr @moksha_unbox_string(ptr %112)
  %113 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox56)
  %114 = load i32, ptr @__exception_flag, align 4
  %115 = icmp ne i32 %114, 0
  br i1 %115, label %unwind58, label %next_stmt57

try_cont:                                         ; preds = %next_stmt57, %exception_check
  br label %try_block59

next_stmt42:                                      ; preds = %try_block
  store i32 0, ptr %b, align 4
  store i32 0, ptr %b44, align 4
  %116 = load i32, ptr @__exception_flag, align 4
  %117 = icmp ne i32 %116, 0
  br i1 %117, label %unwind46, label %next_stmt45

unwind43:                                         ; preds = %try_block
  br label %catch_block

next_stmt45:                                      ; preds = %next_stmt42
  %118 = call ptr @moksha_box_string(ptr @61)
  %print_unbox47 = call ptr @moksha_unbox_string(ptr %118)
  %119 = call i32 (ptr, ...) @printf(ptr @62, ptr %print_unbox47)
  %120 = load i32, ptr @__exception_flag, align 4
  %121 = icmp ne i32 %120, 0
  br i1 %121, label %unwind49, label %next_stmt48

unwind46:                                         ; preds = %next_stmt42
  br label %catch_block

next_stmt48:                                      ; preds = %next_stmt45
  %122 = load i32, ptr %a41, align 4
  %123 = load i32, ptr %b44, align 4
  %124 = icmp eq i32 %123, 0
  br i1 %124, label %math_error, label %math_safe

unwind49:                                         ; preds = %next_stmt45
  br label %catch_block

math_error:                                       ; preds = %next_stmt48
  %125 = call ptr @moksha_box_string(ptr @63)
  call void @moksha_throw_exception(ptr %125)
  store ptr %125, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge

math_safe:                                        ; preds = %next_stmt48
  %sdivtmp = sdiv i32 %122, %123
  br label %math_merge

math_merge:                                       ; preds = %math_safe, %math_error
  %math_res = phi i32 [ 0, %math_error ], [ %sdivtmp, %math_safe ]
  store i32 %math_res, ptr %res, align 4
  store i32 %math_res, ptr %res50, align 4
  %126 = load i32, ptr @__exception_flag, align 4
  %127 = icmp ne i32 %126, 0
  br i1 %127, label %unwind52, label %next_stmt51

next_stmt51:                                      ; preds = %math_merge
  %128 = call ptr @moksha_box_string(ptr @64)
  %129 = load i32, ptr %res50, align 4
  %130 = call ptr @moksha_int_to_str(i32 %129)
  %131 = call ptr @moksha_box_string(ptr %130)
  %132 = call ptr @moksha_string_concat(ptr %128, ptr %131)
  %print_unbox53 = call ptr @moksha_unbox_string(ptr %132)
  %133 = call i32 (ptr, ...) @printf(ptr @65, ptr %print_unbox53)
  %134 = load i32, ptr @__exception_flag, align 4
  %135 = icmp ne i32 %134, 0
  br i1 %135, label %unwind55, label %next_stmt54

unwind52:                                         ; preds = %math_merge
  br label %catch_block

next_stmt54:                                      ; preds = %next_stmt51
  br label %exception_check

unwind55:                                         ; preds = %next_stmt51
  br label %catch_block

next_stmt57:                                      ; preds = %catch_block
  br label %try_cont

unwind58:                                         ; preds = %catch_block
  ret i32 0

try_block59:                                      ; preds = %try_cont
  %136 = call ptr @moksha_box_string(ptr @68)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %136)
  %137 = call i32 (ptr, ...) @printf(ptr @69, ptr %print_unbox63)
  %138 = load i32, ptr @__exception_flag, align 4
  %139 = icmp ne i32 %138, 0
  br i1 %139, label %unwind65, label %next_stmt64

exception_check60:                                ; preds = %next_stmt75
  %140 = load i32, ptr @__exception_flag, align 4
  %141 = icmp ne i32 %140, 0
  br i1 %141, label %catch_block61, label %try_cont62

catch_block61:                                    ; preds = %exception_check60, %unwind76, %unwind73, %unwind65
  store i32 0, ptr @__exception_flag, align 4
  %142 = load ptr, ptr @__exception_val, align 8
  store ptr %142, ptr %e77, align 8
  %143 = call ptr @moksha_box_string(ptr @73)
  %144 = load ptr, ptr %e77, align 8
  %145 = call ptr @moksha_any_to_str(ptr %144)
  %146 = call ptr @moksha_box_string(ptr %145)
  %147 = call ptr @moksha_string_concat(ptr %143, ptr %146)
  %print_unbox78 = call ptr @moksha_unbox_string(ptr %147)
  %148 = call i32 (ptr, ...) @printf(ptr @74, ptr %print_unbox78)
  %149 = load i32, ptr @__exception_flag, align 4
  %150 = icmp ne i32 %149, 0
  br i1 %150, label %unwind80, label %next_stmt79

try_cont62:                                       ; preds = %next_stmt79, %exception_check60
  br label %try_block81

next_stmt64:                                      ; preds = %try_block59
  br i1 true, label %math_error67, label %math_safe68

unwind65:                                         ; preds = %try_block59
  br label %catch_block61

math_error67:                                     ; preds = %next_stmt64
  %151 = call ptr @moksha_box_string(ptr @70)
  call void @moksha_throw_exception(ptr %151)
  store ptr %151, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge69

math_safe68:                                      ; preds = %next_stmt64
  br label %math_merge69

math_merge69:                                     ; preds = %math_safe68, %math_error67
  %math_res70 = phi i32 [ 0, %math_error67 ], [ poison, %math_safe68 ]
  store i32 %math_res70, ptr %res66, align 4
  store i32 %math_res70, ptr %res71, align 4
  %152 = load i32, ptr @__exception_flag, align 4
  %153 = icmp ne i32 %152, 0
  br i1 %153, label %unwind73, label %next_stmt72

next_stmt72:                                      ; preds = %math_merge69
  %154 = call ptr @moksha_box_string(ptr @71)
  %155 = load i32, ptr %res71, align 4
  %156 = call ptr @moksha_int_to_str(i32 %155)
  %157 = call ptr @moksha_box_string(ptr %156)
  %158 = call ptr @moksha_string_concat(ptr %154, ptr %157)
  %print_unbox74 = call ptr @moksha_unbox_string(ptr %158)
  %159 = call i32 (ptr, ...) @printf(ptr @72, ptr %print_unbox74)
  %160 = load i32, ptr @__exception_flag, align 4
  %161 = icmp ne i32 %160, 0
  br i1 %161, label %unwind76, label %next_stmt75

unwind73:                                         ; preds = %math_merge69
  br label %catch_block61

next_stmt75:                                      ; preds = %next_stmt72
  br label %exception_check60

unwind76:                                         ; preds = %next_stmt72
  br label %catch_block61

next_stmt79:                                      ; preds = %catch_block61
  br label %try_cont62

unwind80:                                         ; preds = %catch_block61
  ret i32 0

try_block81:                                      ; preds = %try_cont62
  store double 1.000000e+01, ptr %x, align 8
  store double 1.000000e+01, ptr %x85, align 8
  %162 = load i32, ptr @__exception_flag, align 4
  %163 = icmp ne i32 %162, 0
  br i1 %163, label %unwind87, label %next_stmt86

exception_check82:                                ; preds = %next_stmt103
  %164 = load i32, ptr @__exception_flag, align 4
  %165 = icmp ne i32 %164, 0
  br i1 %165, label %catch_block83, label %try_cont84

catch_block83:                                    ; preds = %exception_check82, %unwind104, %unwind101, %unwind93, %unwind90, %unwind87
  store i32 0, ptr @__exception_flag, align 4
  %166 = load ptr, ptr @__exception_val, align 8
  store ptr %166, ptr %e105, align 8
  %167 = call ptr @moksha_box_string(ptr @81)
  %168 = load ptr, ptr %e105, align 8
  %169 = call ptr @moksha_any_to_str(ptr %168)
  %170 = call ptr @moksha_box_string(ptr %169)
  %171 = call ptr @moksha_string_concat(ptr %167, ptr %170)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %171)
  %172 = call i32 (ptr, ...) @printf(ptr @82, ptr %print_unbox106)
  %173 = load i32, ptr @__exception_flag, align 4
  %174 = icmp ne i32 %173, 0
  br i1 %174, label %unwind108, label %next_stmt107

try_cont84:                                       ; preds = %next_stmt107, %exception_check82
  %175 = call ptr @moksha_box_string(ptr @83)
  %print_unbox109 = call ptr @moksha_unbox_string(ptr %175)
  %176 = call i32 (ptr, ...) @printf(ptr @84, ptr %print_unbox109)
  br label %try_block110

next_stmt86:                                      ; preds = %try_block81
  store double 0.000000e+00, ptr %y, align 8
  store double 0.000000e+00, ptr %y88, align 8
  %177 = load i32, ptr @__exception_flag, align 4
  %178 = icmp ne i32 %177, 0
  br i1 %178, label %unwind90, label %next_stmt89

unwind87:                                         ; preds = %try_block81
  br label %catch_block83

next_stmt89:                                      ; preds = %next_stmt86
  %179 = call ptr @moksha_box_string(ptr @75)
  %print_unbox91 = call ptr @moksha_unbox_string(ptr %179)
  %180 = call i32 (ptr, ...) @printf(ptr @76, ptr %print_unbox91)
  %181 = load i32, ptr @__exception_flag, align 4
  %182 = icmp ne i32 %181, 0
  br i1 %182, label %unwind93, label %next_stmt92

unwind90:                                         ; preds = %next_stmt86
  br label %catch_block83

next_stmt92:                                      ; preds = %next_stmt89
  %183 = load double, ptr %x85, align 8
  %184 = load double, ptr %y88, align 8
  %185 = fcmp oeq double %184, 0.000000e+00
  br i1 %185, label %math_error95, label %math_safe96

unwind93:                                         ; preds = %next_stmt89
  br label %catch_block83

math_error95:                                     ; preds = %next_stmt92
  %186 = call ptr @moksha_box_string(ptr @77)
  call void @moksha_throw_exception(ptr %186)
  store ptr %186, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge97

math_safe96:                                      ; preds = %next_stmt92
  %fdivtmp = fdiv double %183, %184
  br label %math_merge97

math_merge97:                                     ; preds = %math_safe96, %math_error95
  %math_res98 = phi double [ 0.000000e+00, %math_error95 ], [ %fdivtmp, %math_safe96 ]
  store double %math_res98, ptr %res94, align 8
  store double %math_res98, ptr %res99, align 8
  %187 = load i32, ptr @__exception_flag, align 4
  %188 = icmp ne i32 %187, 0
  br i1 %188, label %unwind101, label %next_stmt100

next_stmt100:                                     ; preds = %math_merge97
  %189 = call ptr @moksha_box_string(ptr @78)
  %190 = load double, ptr %res99, align 8
  %191 = call ptr @moksha_double_to_str(double %190)
  %192 = call ptr @moksha_box_string(ptr %191)
  %193 = call ptr @moksha_string_concat(ptr %189, ptr %192)
  %194 = call ptr @moksha_box_string(ptr @79)
  %195 = call ptr @moksha_string_concat(ptr %193, ptr %194)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %195)
  %196 = call i32 (ptr, ...) @printf(ptr @80, ptr %print_unbox102)
  %197 = load i32, ptr @__exception_flag, align 4
  %198 = icmp ne i32 %197, 0
  br i1 %198, label %unwind104, label %next_stmt103

unwind101:                                        ; preds = %math_merge97
  br label %catch_block83

next_stmt103:                                     ; preds = %next_stmt100
  br label %exception_check82

unwind104:                                        ; preds = %next_stmt100
  br label %catch_block83

next_stmt107:                                     ; preds = %catch_block83
  br label %try_cont84

unwind108:                                        ; preds = %catch_block83
  ret i32 0

try_block110:                                     ; preds = %try_cont84
  %199 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 1)
  %200 = call ptr @moksha_array_push_val(ptr %199, ptr %box_i)
  %box_i114 = call ptr @moksha_box_int(i32 2)
  %201 = call ptr @moksha_array_push_val(ptr %199, ptr %box_i114)
  %box_i115 = call ptr @moksha_box_int(i32 3)
  %202 = call ptr @moksha_array_push_val(ptr %199, ptr %box_i115)
  store ptr %199, ptr %arr, align 8
  store ptr %199, ptr %arr116, align 8
  %203 = load i32, ptr @__exception_flag, align 4
  %204 = icmp ne i32 %203, 0
  br i1 %204, label %unwind118, label %next_stmt117

exception_check111:                               ; preds = %next_stmt126
  %205 = load i32, ptr @__exception_flag, align 4
  %206 = icmp ne i32 %205, 0
  br i1 %206, label %catch_block112, label %try_cont113

catch_block112:                                   ; preds = %exception_check111, %unwind127, %unwind124, %unwind121, %unwind118
  store i32 0, ptr @__exception_flag, align 4
  %207 = load ptr, ptr @__exception_val, align 8
  store ptr %207, ptr %e128, align 8
  %208 = call ptr @moksha_box_string(ptr @89)
  %209 = load ptr, ptr %e128, align 8
  %210 = call ptr @moksha_any_to_str(ptr %209)
  %211 = call ptr @moksha_box_string(ptr %210)
  %212 = call ptr @moksha_string_concat(ptr %208, ptr %211)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %212)
  %213 = call i32 (ptr, ...) @printf(ptr @90, ptr %print_unbox129)
  %214 = load i32, ptr @__exception_flag, align 4
  %215 = icmp ne i32 %214, 0
  br i1 %215, label %unwind131, label %next_stmt130

try_cont113:                                      ; preds = %next_stmt130, %exception_check111
  br label %try_block132

next_stmt117:                                     ; preds = %try_block110
  %216 = call ptr @moksha_box_string(ptr @85)
  %print_unbox119 = call ptr @moksha_unbox_string(ptr %216)
  %217 = call i32 (ptr, ...) @printf(ptr @86, ptr %print_unbox119)
  %218 = load i32, ptr @__exception_flag, align 4
  %219 = icmp ne i32 %218, 0
  br i1 %219, label %unwind121, label %next_stmt120

unwind118:                                        ; preds = %try_block110
  br label %catch_block112

next_stmt120:                                     ; preds = %next_stmt117
  %220 = load ptr, ptr %arr116, align 8
  %221 = call ptr @moksha_array_get(ptr %220, i32 5)
  store ptr %221, ptr %val, align 8
  store ptr %221, ptr %val122, align 8
  %222 = load i32, ptr @__exception_flag, align 4
  %223 = icmp ne i32 %222, 0
  br i1 %223, label %unwind124, label %next_stmt123

unwind121:                                        ; preds = %next_stmt117
  br label %catch_block112

next_stmt123:                                     ; preds = %next_stmt120
  %224 = call ptr @moksha_box_string(ptr @87)
  %225 = load i32, ptr %val122, align 4
  %226 = call ptr @moksha_int_to_str(i32 %225)
  %227 = call ptr @moksha_box_string(ptr %226)
  %228 = call ptr @moksha_string_concat(ptr %224, ptr %227)
  %print_unbox125 = call ptr @moksha_unbox_string(ptr %228)
  %229 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox125)
  %230 = load i32, ptr @__exception_flag, align 4
  %231 = icmp ne i32 %230, 0
  br i1 %231, label %unwind127, label %next_stmt126

unwind124:                                        ; preds = %next_stmt120
  br label %catch_block112

next_stmt126:                                     ; preds = %next_stmt123
  br label %exception_check111

unwind127:                                        ; preds = %next_stmt123
  br label %catch_block112

next_stmt130:                                     ; preds = %catch_block112
  br label %try_cont113

unwind131:                                        ; preds = %catch_block112
  ret i32 0

try_block132:                                     ; preds = %try_cont113
  %232 = call ptr @moksha_alloc_array(i32 3)
  %box_i137 = call ptr @moksha_box_int(i32 1)
  %233 = call ptr @moksha_array_push_val(ptr %232, ptr %box_i137)
  %box_i138 = call ptr @moksha_box_int(i32 2)
  %234 = call ptr @moksha_array_push_val(ptr %232, ptr %box_i138)
  %box_i139 = call ptr @moksha_box_int(i32 3)
  %235 = call ptr @moksha_array_push_val(ptr %232, ptr %box_i139)
  store ptr %232, ptr %arr136, align 8
  store ptr %232, ptr %arr140, align 8
  %236 = load i32, ptr @__exception_flag, align 4
  %237 = icmp ne i32 %236, 0
  br i1 %237, label %unwind142, label %next_stmt141

exception_check133:                               ; preds = %next_stmt151
  %238 = load i32, ptr @__exception_flag, align 4
  %239 = icmp ne i32 %238, 0
  br i1 %239, label %catch_block134, label %try_cont135

catch_block134:                                   ; preds = %exception_check133, %unwind152, %unwind149, %unwind145, %unwind142
  store i32 0, ptr @__exception_flag, align 4
  %240 = load ptr, ptr @__exception_val, align 8
  store ptr %240, ptr %e153, align 8
  %241 = call ptr @moksha_box_string(ptr @95)
  %242 = load ptr, ptr %e153, align 8
  %243 = call ptr @moksha_any_to_str(ptr %242)
  %244 = call ptr @moksha_box_string(ptr %243)
  %245 = call ptr @moksha_string_concat(ptr %241, ptr %244)
  %print_unbox154 = call ptr @moksha_unbox_string(ptr %245)
  %246 = call i32 (ptr, ...) @printf(ptr @96, ptr %print_unbox154)
  %247 = load i32, ptr @__exception_flag, align 4
  %248 = icmp ne i32 %247, 0
  br i1 %248, label %unwind156, label %next_stmt155

try_cont135:                                      ; preds = %next_stmt155, %exception_check133
  br label %try_block157

next_stmt141:                                     ; preds = %try_block132
  %249 = call ptr @moksha_box_string(ptr @91)
  %print_unbox143 = call ptr @moksha_unbox_string(ptr %249)
  %250 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox143)
  %251 = load i32, ptr @__exception_flag, align 4
  %252 = icmp ne i32 %251, 0
  br i1 %252, label %unwind145, label %next_stmt144

unwind142:                                        ; preds = %try_block132
  br label %catch_block134

next_stmt144:                                     ; preds = %next_stmt141
  %253 = load ptr, ptr %arr140, align 8
  %254 = call ptr @moksha_array_get(ptr %253, i32 -1)
  store ptr %254, ptr %val146, align 8
  store ptr %254, ptr %val147, align 8
  %255 = load i32, ptr @__exception_flag, align 4
  %256 = icmp ne i32 %255, 0
  br i1 %256, label %unwind149, label %next_stmt148

unwind145:                                        ; preds = %next_stmt141
  br label %catch_block134

next_stmt148:                                     ; preds = %next_stmt144
  %257 = call ptr @moksha_box_string(ptr @93)
  %258 = load i32, ptr %val147, align 4
  %259 = call ptr @moksha_int_to_str(i32 %258)
  %260 = call ptr @moksha_box_string(ptr %259)
  %261 = call ptr @moksha_string_concat(ptr %257, ptr %260)
  %print_unbox150 = call ptr @moksha_unbox_string(ptr %261)
  %262 = call i32 (ptr, ...) @printf(ptr @94, ptr %print_unbox150)
  %263 = load i32, ptr @__exception_flag, align 4
  %264 = icmp ne i32 %263, 0
  br i1 %264, label %unwind152, label %next_stmt151

unwind149:                                        ; preds = %next_stmt144
  br label %catch_block134

next_stmt151:                                     ; preds = %next_stmt148
  br label %exception_check133

unwind152:                                        ; preds = %next_stmt148
  br label %catch_block134

next_stmt155:                                     ; preds = %catch_block134
  br label %try_cont135

unwind156:                                        ; preds = %catch_block134
  ret i32 0

try_block157:                                     ; preds = %try_cont135
  %265 = call ptr @moksha_box_string(ptr @97)
  store ptr %265, ptr %s, align 8
  store ptr %265, ptr %s161, align 8
  %266 = load i32, ptr @__exception_flag, align 4
  %267 = icmp ne i32 %266, 0
  br i1 %267, label %unwind163, label %next_stmt162

exception_check158:                               ; preds = %next_stmt179
  %268 = load i32, ptr @__exception_flag, align 4
  %269 = icmp ne i32 %268, 0
  br i1 %269, label %catch_block159, label %try_cont160

catch_block159:                                   ; preds = %exception_check158, %unwind180, %unwind178, %unwind175, %unwind169, %unwind166, %unwind163
  store i32 0, ptr @__exception_flag, align 4
  %270 = load ptr, ptr @__exception_val, align 8
  store ptr %270, ptr %e181, align 8
  %271 = call ptr @moksha_box_string(ptr @105)
  %272 = load ptr, ptr %e181, align 8
  %273 = call ptr @moksha_any_to_str(ptr %272)
  %274 = call ptr @moksha_box_string(ptr %273)
  %275 = call ptr @moksha_string_concat(ptr %271, ptr %274)
  %print_unbox182 = call ptr @moksha_unbox_string(ptr %275)
  %276 = call i32 (ptr, ...) @printf(ptr @106, ptr %print_unbox182)
  %277 = load i32, ptr @__exception_flag, align 4
  %278 = icmp ne i32 %277, 0
  br i1 %278, label %unwind184, label %next_stmt183

try_cont160:                                      ; preds = %next_stmt183, %exception_check158
  %279 = call ptr @moksha_box_string(ptr @107)
  %print_unbox185 = call ptr @moksha_unbox_string(ptr %279)
  %280 = call i32 (ptr, ...) @printf(ptr @108, ptr %print_unbox185)
  br label %try_block186

next_stmt162:                                     ; preds = %try_block157
  %281 = call ptr @moksha_box_string(ptr @98)
  %print_unbox164 = call ptr @moksha_unbox_string(ptr %281)
  %282 = call i32 (ptr, ...) @printf(ptr @99, ptr %print_unbox164)
  %283 = load i32, ptr @__exception_flag, align 4
  %284 = icmp ne i32 %283, 0
  br i1 %284, label %unwind166, label %next_stmt165

unwind163:                                        ; preds = %try_block157
  br label %catch_block159

next_stmt165:                                     ; preds = %next_stmt162
  %285 = load ptr, ptr %s161, align 8
  %286 = call ptr @moksha_string_get_char(ptr %285, i32 10)
  %287 = call ptr @moksha_any_to_str(ptr %286)
  %288 = call ptr @moksha_box_string(ptr %287)
  store ptr %288, ptr %c, align 8
  store ptr %288, ptr %c167, align 8
  %289 = load i32, ptr @__exception_flag, align 4
  %290 = icmp ne i32 %289, 0
  br i1 %290, label %unwind169, label %next_stmt168

unwind166:                                        ; preds = %next_stmt162
  br label %catch_block159

next_stmt168:                                     ; preds = %next_stmt165
  %291 = load ptr, ptr %c167, align 8
  %292 = call ptr @moksha_box_string(ptr @100)
  %unbox_L_f = call double @moksha_unbox_double(ptr %291)
  %unbox_R_f = call double @moksha_unbox_double(ptr %292)
  %fcmptmp = fcmp oeq double %unbox_L_f, %unbox_R_f
  br i1 %fcmptmp, label %then170, label %else171

unwind169:                                        ; preds = %next_stmt165
  br label %catch_block159

then170:                                          ; preds = %next_stmt168
  %293 = call ptr @moksha_box_string(ptr @101)
  %print_unbox173 = call ptr @moksha_unbox_string(ptr %293)
  %294 = call i32 (ptr, ...) @printf(ptr @102, ptr %print_unbox173)
  %295 = load i32, ptr @__exception_flag, align 4
  %296 = icmp ne i32 %295, 0
  br i1 %296, label %unwind175, label %next_stmt174

else171:                                          ; preds = %next_stmt168
  %297 = call ptr @moksha_box_string(ptr @103)
  %298 = load ptr, ptr %c167, align 8
  %299 = call ptr @moksha_string_concat(ptr %297, ptr %298)
  %print_unbox176 = call ptr @moksha_unbox_string(ptr %299)
  %300 = call i32 (ptr, ...) @printf(ptr @104, ptr %print_unbox176)
  %301 = load i32, ptr @__exception_flag, align 4
  %302 = icmp ne i32 %301, 0
  br i1 %302, label %unwind178, label %next_stmt177

ifcont172:                                        ; preds = %next_stmt177, %next_stmt174
  %303 = load i32, ptr @__exception_flag, align 4
  %304 = icmp ne i32 %303, 0
  br i1 %304, label %unwind180, label %next_stmt179

next_stmt174:                                     ; preds = %then170
  br label %ifcont172

unwind175:                                        ; preds = %then170
  br label %catch_block159

next_stmt177:                                     ; preds = %else171
  br label %ifcont172

unwind178:                                        ; preds = %else171
  br label %catch_block159

next_stmt179:                                     ; preds = %ifcont172
  br label %exception_check158

unwind180:                                        ; preds = %ifcont172
  br label %catch_block159

next_stmt183:                                     ; preds = %catch_block159
  br label %try_cont160

unwind184:                                        ; preds = %catch_block159
  ret i32 0

try_block186:                                     ; preds = %try_cont160
  %305 = call ptr @moksha_box_string(ptr @109)
  store ptr %305, ptr %badStr, align 8
  store ptr %305, ptr %badStr190, align 8
  %306 = load i32, ptr @__exception_flag, align 4
  %307 = icmp ne i32 %306, 0
  br i1 %307, label %unwind192, label %next_stmt191

exception_check187:                               ; preds = %next_stmt201
  %308 = load i32, ptr @__exception_flag, align 4
  %309 = icmp ne i32 %308, 0
  br i1 %309, label %catch_block188, label %try_cont189

catch_block188:                                   ; preds = %exception_check187, %unwind202, %unwind199, %unwind195, %unwind192
  store i32 0, ptr @__exception_flag, align 4
  %310 = load ptr, ptr @__exception_val, align 8
  store ptr %310, ptr %e203, align 8
  %311 = call ptr @moksha_box_string(ptr @115)
  %312 = load ptr, ptr %e203, align 8
  %313 = call ptr @moksha_any_to_str(ptr %312)
  %314 = call ptr @moksha_box_string(ptr %313)
  %315 = call ptr @moksha_string_concat(ptr %311, ptr %314)
  %print_unbox204 = call ptr @moksha_unbox_string(ptr %315)
  %316 = call i32 (ptr, ...) @printf(ptr @116, ptr %print_unbox204)
  %317 = load i32, ptr @__exception_flag, align 4
  %318 = icmp ne i32 %317, 0
  br i1 %318, label %unwind206, label %next_stmt205

try_cont189:                                      ; preds = %next_stmt205, %exception_check187
  br label %try_block207

next_stmt191:                                     ; preds = %try_block186
  %319 = call ptr @moksha_box_string(ptr @110)
  %print_unbox193 = call ptr @moksha_unbox_string(ptr %319)
  %320 = call i32 (ptr, ...) @printf(ptr @111, ptr %print_unbox193)
  %321 = load i32, ptr @__exception_flag, align 4
  %322 = icmp ne i32 %321, 0
  br i1 %322, label %unwind195, label %next_stmt194

unwind192:                                        ; preds = %try_block186
  br label %catch_block188

next_stmt194:                                     ; preds = %next_stmt191
  %323 = load ptr, ptr %badStr190, align 8
  %unbox_i = call i32 @moksha_unbox_int(ptr %323)
  store i32 %unbox_i, ptr %val196, align 4
  store i32 %unbox_i, ptr %val197, align 4
  %324 = load i32, ptr @__exception_flag, align 4
  %325 = icmp ne i32 %324, 0
  br i1 %325, label %unwind199, label %next_stmt198

unwind195:                                        ; preds = %next_stmt191
  br label %catch_block188

next_stmt198:                                     ; preds = %next_stmt194
  %326 = call ptr @moksha_box_string(ptr @112)
  %327 = load i32, ptr %val197, align 4
  %328 = call ptr @moksha_int_to_str(i32 %327)
  %329 = call ptr @moksha_box_string(ptr %328)
  %330 = call ptr @moksha_string_concat(ptr %326, ptr %329)
  %331 = call ptr @moksha_box_string(ptr @113)
  %332 = call ptr @moksha_string_concat(ptr %330, ptr %331)
  %print_unbox200 = call ptr @moksha_unbox_string(ptr %332)
  %333 = call i32 (ptr, ...) @printf(ptr @114, ptr %print_unbox200)
  %334 = load i32, ptr @__exception_flag, align 4
  %335 = icmp ne i32 %334, 0
  br i1 %335, label %unwind202, label %next_stmt201

unwind199:                                        ; preds = %next_stmt194
  br label %catch_block188

next_stmt201:                                     ; preds = %next_stmt198
  br label %exception_check187

unwind202:                                        ; preds = %next_stmt198
  br label %catch_block188

next_stmt205:                                     ; preds = %catch_block188
  br label %try_cont189

unwind206:                                        ; preds = %catch_block188
  ret i32 0

try_block207:                                     ; preds = %try_cont189
  br i1 true, label %clone_skip, label %clone_do

exception_check208:                               ; preds = %next_stmt220
  %336 = load i32, ptr @__exception_flag, align 4
  %337 = icmp ne i32 %336, 0
  br i1 %337, label %catch_block209, label %try_cont210

catch_block209:                                   ; preds = %exception_check208, %unwind221, %unwind218, %call_error, %unwind216, %unwind213
  store i32 0, ptr @__exception_flag, align 4
  %338 = load ptr, ptr @__exception_val, align 8
  store ptr %338, ptr %e222, align 8
  %339 = call ptr @moksha_box_string(ptr @122)
  %340 = load ptr, ptr %e222, align 8
  %341 = call ptr @moksha_any_to_str(ptr %340)
  %342 = call ptr @moksha_box_string(ptr %341)
  %343 = call ptr @moksha_string_concat(ptr %339, ptr %342)
  %print_unbox223 = call ptr @moksha_unbox_string(ptr %343)
  %344 = call i32 (ptr, ...) @printf(ptr @123, ptr %print_unbox223)
  %345 = load i32, ptr @__exception_flag, align 4
  %346 = icmp ne i32 %345, 0
  br i1 %346, label %unwind225, label %next_stmt224

try_cont210:                                      ; preds = %next_stmt224, %exception_check208
  %347 = call ptr @moksha_box_string(ptr @124)
  %print_unbox226 = call ptr @moksha_unbox_string(ptr %347)
  %348 = call i32 (ptr, ...) @printf(ptr @125, ptr %print_unbox226)
  br label %try_block227

clone_do:                                         ; preds = %try_block207
  %clone_alloc = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 null, i64 8, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %try_block207
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %d, align 8
  store ptr %clone_res, ptr %d211, align 8
  %349 = load i32, ptr @__exception_flag, align 4
  %350 = icmp ne i32 %349, 0
  br i1 %350, label %unwind213, label %next_stmt212

next_stmt212:                                     ; preds = %clone_merge
  %351 = call ptr @moksha_box_string(ptr @117)
  %print_unbox214 = call ptr @moksha_unbox_string(ptr %351)
  %352 = call i32 (ptr, ...) @printf(ptr @118, ptr %print_unbox214)
  %353 = load i32, ptr @__exception_flag, align 4
  %354 = icmp ne i32 %353, 0
  br i1 %354, label %unwind216, label %next_stmt215

unwind213:                                        ; preds = %clone_merge
  br label %catch_block209

next_stmt215:                                     ; preds = %next_stmt212
  %355 = load ptr, ptr %d211, align 8
  %356 = icmp eq ptr %355, null
  br i1 %356, label %call_error, label %call_safe

unwind216:                                        ; preds = %next_stmt212
  br label %catch_block209

call_error:                                       ; preds = %next_stmt215
  %357 = call ptr @moksha_box_string(ptr @119)
  store ptr %357, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %catch_block209

call_safe:                                        ; preds = %next_stmt215
  %358 = getelementptr inbounds nuw %Dummy, ptr %355, i32 0, i32 0
  %vptr = load ptr, ptr %358, align 8
  %359 = getelementptr inbounds ptr, ptr %vptr, i32 0
  %func_ptr = load ptr, ptr %359, align 8
  call void %func_ptr(ptr %355)
  %360 = load i32, ptr @__exception_flag, align 4
  %361 = icmp ne i32 %360, 0
  br i1 %361, label %unwind218, label %next_stmt217

next_stmt217:                                     ; preds = %call_safe
  %362 = call ptr @moksha_box_string(ptr @120)
  %print_unbox219 = call ptr @moksha_unbox_string(ptr %362)
  %363 = call i32 (ptr, ...) @printf(ptr @121, ptr %print_unbox219)
  %364 = load i32, ptr @__exception_flag, align 4
  %365 = icmp ne i32 %364, 0
  br i1 %365, label %unwind221, label %next_stmt220

unwind218:                                        ; preds = %call_safe
  br label %catch_block209

next_stmt220:                                     ; preds = %next_stmt217
  br label %exception_check208

unwind221:                                        ; preds = %next_stmt217
  br label %catch_block209

next_stmt224:                                     ; preds = %catch_block209
  br label %try_cont210

unwind225:                                        ; preds = %catch_block209
  ret i32 0

try_block227:                                     ; preds = %try_cont210
  %366 = call ptr @moksha_box_string(ptr @127)
  %print_unbox231 = call ptr @moksha_unbox_string(ptr %366)
  %367 = call i32 (ptr, ...) @printf(ptr @128, ptr %print_unbox231)
  %368 = load i32, ptr @__exception_flag, align 4
  %369 = icmp ne i32 %368, 0
  br i1 %369, label %unwind233, label %next_stmt232

exception_check228:                               ; preds = %next_stmt237
  %370 = load i32, ptr @__exception_flag, align 4
  %371 = icmp ne i32 %370, 0
  br i1 %371, label %catch_block229, label %try_cont230

catch_block229:                                   ; preds = %exception_check228, %unwind238, %unwind235, %unwind233
  store i32 0, ptr @__exception_flag, align 4
  %372 = load ptr, ptr @__exception_val, align 8
  store ptr %372, ptr %e239, align 8
  %373 = call ptr @moksha_box_string(ptr @131)
  %374 = load ptr, ptr %e239, align 8
  %375 = call ptr @moksha_any_to_str(ptr %374)
  %376 = call ptr @moksha_box_string(ptr %375)
  %377 = call ptr @moksha_string_concat(ptr %373, ptr %376)
  %print_unbox240 = call ptr @moksha_unbox_string(ptr %377)
  %378 = call i32 (ptr, ...) @printf(ptr @132, ptr %print_unbox240)
  %379 = load i32, ptr @__exception_flag, align 4
  %380 = icmp ne i32 %379, 0
  br i1 %380, label %unwind242, label %next_stmt241

try_cont230:                                      ; preds = %next_stmt241, %exception_check228
  %381 = call ptr @moksha_box_string(ptr @133)
  %print_unbox243 = call ptr @moksha_unbox_string(ptr %381)
  %382 = call i32 (ptr, ...) @printf(ptr @134, ptr %print_unbox243)
  store i1 false, ptr %finallyRun, align 1
  store i1 false, ptr %finallyRun244, align 1
  br label %try_block245

next_stmt232:                                     ; preds = %try_block227
  call void @caller()
  %383 = load i32, ptr @__exception_flag, align 4
  %384 = icmp ne i32 %383, 0
  br i1 %384, label %unwind235, label %next_stmt234

unwind233:                                        ; preds = %try_block227
  br label %catch_block229

next_stmt234:                                     ; preds = %next_stmt232
  %385 = call ptr @moksha_box_string(ptr @129)
  %print_unbox236 = call ptr @moksha_unbox_string(ptr %385)
  %386 = call i32 (ptr, ...) @printf(ptr @130, ptr %print_unbox236)
  %387 = load i32, ptr @__exception_flag, align 4
  %388 = icmp ne i32 %387, 0
  br i1 %388, label %unwind238, label %next_stmt237

unwind235:                                        ; preds = %next_stmt232
  br label %catch_block229

next_stmt237:                                     ; preds = %next_stmt234
  br label %exception_check228

unwind238:                                        ; preds = %next_stmt234
  br label %catch_block229

next_stmt241:                                     ; preds = %catch_block229
  br label %try_cont230

unwind242:                                        ; preds = %catch_block229
  ret i32 0

try_block245:                                     ; preds = %try_cont230
  br label %try_block249

exception_check246:                               ; preds = %next_stmt259
  %389 = load i32, ptr @__exception_flag, align 4
  %390 = icmp ne i32 %389, 0
  br i1 %390, label %catch_block247, label %try_cont248

catch_block247:                                   ; preds = %exception_check246, %unwind260, %unwind258, %unwind255
  store i32 0, ptr @__exception_flag, align 4
  %391 = load ptr, ptr @__exception_val, align 8
  store ptr %391, ptr %e261, align 8
  %392 = load i1, ptr %finallyRun244, align 1
  br i1 %392, label %then262, label %else263

try_cont248:                                      ; preds = %next_stmt271, %exception_check246
  %393 = call ptr @moksha_box_string(ptr @142)
  %print_unbox273 = call ptr @moksha_unbox_string(ptr %393)
  %394 = call i32 (ptr, ...) @printf(ptr @143, ptr %print_unbox273)
  br label %try_block274

try_block249:                                     ; preds = %try_block245
  %395 = call ptr @moksha_box_string(ptr @135)
  call void @moksha_throw_exception(ptr %395)
  store ptr %395, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %finally_block

exception_check250:                               ; preds = %next_stmt252
  %396 = load i32, ptr @__exception_flag, align 4
  %397 = icmp ne i32 %396, 0
  br i1 %397, label %finally_block, label %finally_block

finally_block:                                    ; preds = %exception_check250, %exception_check250, %unwind253, %try_block249
  store i1 true, ptr %finallyRun244, align 1
  %398 = load i32, ptr @__exception_flag, align 4
  %399 = icmp ne i32 %398, 0
  br i1 %399, label %unwind255, label %next_stmt254

try_cont251:                                      ; preds = %next_stmt257
  %400 = load i32, ptr @__exception_flag, align 4
  %401 = icmp ne i32 %400, 0
  br i1 %401, label %unwind260, label %next_stmt259

dead_code:                                        ; No predecessors!
  %402 = load i32, ptr @__exception_flag, align 4
  %403 = icmp ne i32 %402, 0
  br i1 %403, label %unwind253, label %next_stmt252

next_stmt252:                                     ; preds = %dead_code
  br label %exception_check250

unwind253:                                        ; preds = %dead_code
  br label %finally_block

next_stmt254:                                     ; preds = %finally_block
  %404 = call ptr @moksha_box_string(ptr @136)
  %print_unbox256 = call ptr @moksha_unbox_string(ptr %404)
  %405 = call i32 (ptr, ...) @printf(ptr @137, ptr %print_unbox256)
  %406 = load i32, ptr @__exception_flag, align 4
  %407 = icmp ne i32 %406, 0
  br i1 %407, label %unwind258, label %next_stmt257

unwind255:                                        ; preds = %finally_block
  br label %catch_block247

next_stmt257:                                     ; preds = %next_stmt254
  br label %try_cont251

unwind258:                                        ; preds = %next_stmt254
  br label %catch_block247

next_stmt259:                                     ; preds = %try_cont251
  br label %exception_check246

unwind260:                                        ; preds = %try_cont251
  br label %catch_block247

then262:                                          ; preds = %catch_block247
  %408 = call ptr @moksha_box_string(ptr @138)
  %print_unbox265 = call ptr @moksha_unbox_string(ptr %408)
  %409 = call i32 (ptr, ...) @printf(ptr @139, ptr %print_unbox265)
  %410 = load i32, ptr @__exception_flag, align 4
  %411 = icmp ne i32 %410, 0
  br i1 %411, label %unwind267, label %next_stmt266

else263:                                          ; preds = %catch_block247
  %412 = call ptr @moksha_box_string(ptr @140)
  %print_unbox268 = call ptr @moksha_unbox_string(ptr %412)
  %413 = call i32 (ptr, ...) @printf(ptr @141, ptr %print_unbox268)
  %414 = load i32, ptr @__exception_flag, align 4
  %415 = icmp ne i32 %414, 0
  br i1 %415, label %unwind270, label %next_stmt269

ifcont264:                                        ; preds = %next_stmt269, %next_stmt266
  %416 = load i32, ptr @__exception_flag, align 4
  %417 = icmp ne i32 %416, 0
  br i1 %417, label %unwind272, label %next_stmt271

next_stmt266:                                     ; preds = %then262
  br label %ifcont264

unwind267:                                        ; preds = %then262
  ret i32 0

next_stmt269:                                     ; preds = %else263
  br label %ifcont264

unwind270:                                        ; preds = %else263
  ret i32 0

next_stmt271:                                     ; preds = %ifcont264
  br label %try_cont248

unwind272:                                        ; preds = %ifcont264
  ret i32 0

try_block274:                                     ; preds = %try_cont248
  %418 = call ptr @moksha_box_string(ptr @144)
  %print_unbox278 = call ptr @moksha_unbox_string(ptr %418)
  %419 = call i32 (ptr, ...) @printf(ptr @145, ptr %print_unbox278)
  %420 = load i32, ptr @__exception_flag, align 4
  %421 = icmp ne i32 %420, 0
  br i1 %421, label %unwind280, label %next_stmt279

exception_check275:                               ; preds = %next_stmt285
  %422 = load i32, ptr @__exception_flag, align 4
  %423 = icmp ne i32 %422, 0
  br i1 %423, label %catch_block276, label %try_cont277

catch_block276:                                   ; preds = %exception_check275, %unwind286, %unwind283, %unwind280
  store i32 0, ptr @__exception_flag, align 4
  %424 = load ptr, ptr @__exception_val, align 8
  store ptr %424, ptr %e287, align 8
  %425 = call ptr @moksha_box_string(ptr @148)
  %426 = load ptr, ptr %e287, align 8
  %427 = call ptr @moksha_any_to_str(ptr %426)
  %428 = call ptr @moksha_box_string(ptr %427)
  %429 = call ptr @moksha_string_concat(ptr %425, ptr %428)
  %print_unbox288 = call ptr @moksha_unbox_string(ptr %429)
  %430 = call i32 (ptr, ...) @printf(ptr @149, ptr %print_unbox288)
  %431 = load i32, ptr @__exception_flag, align 4
  %432 = icmp ne i32 %431, 0
  br i1 %432, label %unwind290, label %next_stmt289

try_cont277:                                      ; preds = %next_stmt289, %exception_check275
  %433 = call ptr @moksha_box_string(ptr @150)
  %print_unbox291 = call ptr @moksha_unbox_string(ptr %433)
  %434 = call i32 (ptr, ...) @printf(ptr @151, ptr %print_unbox291)
  ret i32 0

next_stmt279:                                     ; preds = %try_block274
  %435 = call ptr @moksha_box_int(i32 0)
  %arr_leaf = call ptr @moksha_alloc_array_fill(i32 -5, ptr %435)
  store ptr %arr_leaf, ptr %negArr, align 8
  store ptr %arr_leaf, ptr %negArr281, align 8
  %436 = load i32, ptr @__exception_flag, align 4
  %437 = icmp ne i32 %436, 0
  br i1 %437, label %unwind283, label %next_stmt282

unwind280:                                        ; preds = %try_block274
  br label %catch_block276

next_stmt282:                                     ; preds = %next_stmt279
  %438 = call ptr @moksha_box_string(ptr @146)
  %print_unbox284 = call ptr @moksha_unbox_string(ptr %438)
  %439 = call i32 (ptr, ...) @printf(ptr @147, ptr %print_unbox284)
  %440 = load i32, ptr @__exception_flag, align 4
  %441 = icmp ne i32 %440, 0
  br i1 %441, label %unwind286, label %next_stmt285

unwind283:                                        ; preds = %next_stmt279
  br label %catch_block276

next_stmt285:                                     ; preds = %next_stmt282
  br label %exception_check275

unwind286:                                        ; preds = %next_stmt282
  br label %catch_block276

next_stmt289:                                     ; preds = %catch_block276
  br label %try_cont277

unwind290:                                        ; preds = %catch_block276
  ret i32 0
}

; Function Attrs: alwaysinline
define void @log(ptr %msg) #0 {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %0 = call ptr @moksha_box_string(ptr @46)
  %1 = load ptr, ptr %msg1, align 8
  %2 = call ptr @moksha_any_to_str(ptr %1)
  %3 = call ptr @moksha_box_string(ptr %2)
  %4 = call ptr @moksha_string_concat(ptr %0, ptr %3)
  %print_unbox = call ptr @moksha_unbox_string(ptr %4)
  %5 = call i32 (ptr, ...) @printf(ptr @47, ptr %print_unbox)
  %6 = load i32, ptr @__exception_flag, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

; Function Attrs: alwaysinline
define void @performCalc(ptr %x, ptr %y) #0 {
entry:
  %result3 = alloca i32, align 4
  %result = alloca i32, align 4
  %y2 = alloca ptr, align 8
  %x1 = alloca ptr, align 8
  store ptr %x, ptr %x1, align 8
  store ptr %y, ptr %y2, align 8
  %0 = load ptr, ptr %x1, align 8
  %1 = load ptr, ptr %y2, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %0)
  %unbox_R_f = call double @moksha_unbox_double(ptr %1)
  %faddtmp = fadd double %unbox_L_f, %unbox_R_f
  %2 = call ptr @moksha_box_double(double %faddtmp)
  %unbox_i = call i32 @moksha_unbox_int(ptr %2)
  store i32 %unbox_i, ptr %result, align 4
  store i32 %unbox_i, ptr %result3, align 4
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %5 = call ptr @moksha_box_string(ptr @48)
  %6 = load i32, ptr %result3, align 4
  %7 = call ptr @moksha_int_to_str(i32 %6)
  %8 = call ptr @moksha_box_string(ptr %7)
  %9 = call ptr @moksha_string_concat(ptr %5, ptr %8)
  %print_unbox = call ptr @moksha_unbox_string(ptr %9)
  %10 = call i32 (ptr, ...) @printf(ptr @49, ptr %print_unbox)
  %11 = load i32, ptr @__exception_flag, align 4
  %12 = icmp ne i32 %11, 0
  br i1 %12, label %unwind5, label %next_stmt4

unwind:                                           ; preds = %entry
  ret void

next_stmt4:                                       ; preds = %next_stmt
  ret void

unwind5:                                          ; preds = %next_stmt
  ret void
}

define void @Dummy_act(ptr %this) {
entry:
  ret void
}

define void @Dummy_constructor(ptr %this) {
entry:
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #1

define void @thrower() {
entry:
  %0 = call ptr @moksha_box_string(ptr @126)
  call void @moksha_throw_exception(ptr %0)
  store ptr %0, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

dead_code:                                        ; No predecessors!
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %dead_code
  ret void

unwind:                                           ; preds = %dead_code
  ret void
}

define void @caller() {
entry:
  call void @thrower()
  %0 = load i32, ptr @__exception_flag, align 4
  %1 = icmp ne i32 %0, 0
  br i1 %1, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

attributes #0 = { alwaysinline }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
