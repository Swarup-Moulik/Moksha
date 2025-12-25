; ModuleID = 'moksha_module'
source_filename = "moksha_module"

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

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) #0

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

declare ptr @moksha_array_get_unsafe(ptr, i32)

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
  %val147 = alloca i32, align 4
  %val122 = alloca i32, align 4
  %0 = call ptr @moksha_box_string(ptr nonnull @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %puts = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox)
  %1 = call ptr @moksha_box_string(ptr nonnull @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %1)
  %puts292 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox1)
  %2 = call ptr @moksha_box_string(ptr nonnull @4)
  %3 = call ptr @moksha_int_to_str(i32 0)
  %4 = call ptr @moksha_box_string(ptr %3)
  %5 = call ptr @moksha_string_concat(ptr %2, ptr %4)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %5)
  %puts293 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox2)
  %6 = call ptr @moksha_box_string(ptr nonnull @6)
  %7 = call ptr @moksha_int_to_str(i32 1)
  %8 = call ptr @moksha_box_string(ptr %7)
  %9 = call ptr @moksha_string_concat(ptr %6, ptr %8)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %9)
  %puts294 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox3)
  %10 = call ptr @moksha_box_string(ptr nonnull @8)
  %11 = call ptr @moksha_int_to_str(i32 2)
  %12 = call ptr @moksha_box_string(ptr %11)
  %13 = call ptr @moksha_string_concat(ptr %10, ptr %12)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %13)
  %puts295 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox4)
  %14 = call ptr @moksha_box_string(ptr nonnull @10)
  %print_unbox5 = call ptr @moksha_unbox_string(ptr %14)
  %puts296 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox5)
  %15 = call ptr @moksha_box_string(ptr nonnull @12)
  %16 = call ptr @moksha_int_to_str(i32 200)
  %17 = call ptr @moksha_box_string(ptr %16)
  %18 = call ptr @moksha_string_concat(ptr %15, ptr %17)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %18)
  %puts297 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox6)
  %19 = call ptr @moksha_box_string(ptr nonnull @14)
  %20 = call ptr @moksha_int_to_str(i32 400)
  %21 = call ptr @moksha_box_string(ptr %20)
  %22 = call ptr @moksha_string_concat(ptr %19, ptr %21)
  %print_unbox7 = call ptr @moksha_unbox_string(ptr %22)
  %puts298 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox7)
  %23 = call ptr @moksha_box_string(ptr nonnull @16)
  %24 = call ptr @moksha_int_to_str(i32 500)
  %25 = call ptr @moksha_box_string(ptr %24)
  %26 = call ptr @moksha_string_concat(ptr %23, ptr %25)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %26)
  %puts299 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox8)
  %27 = call ptr @moksha_box_string(ptr nonnull @18)
  %print_unbox9 = call ptr @moksha_unbox_string(ptr %27)
  %puts300 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox9)
  %28 = call ptr @moksha_box_string(ptr nonnull @20)
  %29 = call ptr @moksha_int_to_str(i32 10)
  %30 = call ptr @moksha_box_string(ptr %29)
  %31 = call ptr @moksha_string_concat(ptr %28, ptr %30)
  %print_unbox10 = call ptr @moksha_unbox_string(ptr %31)
  %puts301 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox10)
  %32 = call ptr @moksha_box_string(ptr nonnull @22)
  %33 = call ptr @moksha_int_to_str(i32 11)
  %34 = call ptr @moksha_box_string(ptr %33)
  %35 = call ptr @moksha_string_concat(ptr %32, ptr %34)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %35)
  %puts302 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox11)
  %36 = call ptr @moksha_box_string(ptr nonnull @24)
  %37 = call ptr @moksha_int_to_str(i32 20)
  %38 = call ptr @moksha_box_string(ptr %37)
  %39 = call ptr @moksha_string_concat(ptr %36, ptr %38)
  %print_unbox12 = call ptr @moksha_unbox_string(ptr %39)
  %puts303 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox12)
  %40 = call ptr @moksha_box_string(ptr nonnull @26)
  %41 = call ptr @moksha_int_to_str(i32 21)
  %42 = call ptr @moksha_box_string(ptr %41)
  %43 = call ptr @moksha_string_concat(ptr %40, ptr %42)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %43)
  %puts304 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox13)
  %44 = call ptr @moksha_box_string(ptr nonnull @28)
  %print_unbox14 = call ptr @moksha_unbox_string(ptr %44)
  %puts305 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox14)
  %45 = call ptr @moksha_box_string(ptr nonnull @30)
  %print_unbox16 = call ptr @moksha_unbox_string(ptr %45)
  %puts306 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox16)
  %46 = load i32, ptr @__exception_flag, align 4
  %.not = icmp eq i32 %46, 0
  br i1 %.not, label %case20, label %common.ret

common.ret:                                       ; preds = %catch_block276, %catch_block247, %catch_block229, %catch_block209, %catch_block188, %catch_block159, %catch_block134, %catch_block112, %catch_block83, %catch_block61, %catch_block, %case20, %entry, %try_cont277
  ret i32 0

case20:                                           ; preds = %entry
  %47 = call ptr @moksha_box_string(ptr nonnull @36)
  %print_unbox25 = call ptr @moksha_unbox_string(ptr %47)
  %puts307 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox25)
  %48 = load i32, ptr @__exception_flag, align 4
  %.not308 = icmp eq i32 %48, 0
  br i1 %.not308, label %next_stmt26, label %common.ret

next_stmt26:                                      ; preds = %case20
  %49 = call ptr @moksha_box_string(ptr nonnull @42)
  %print_unbox34 = call ptr @moksha_unbox_string(ptr %49)
  %puts309 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox34)
  %50 = call ptr @moksha_box_string(ptr nonnull @44)
  %print_unbox35 = call ptr @moksha_unbox_string(ptr %50)
  %puts310 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox35)
  %51 = call ptr @moksha_box_string(ptr nonnull @50)
  %print_unbox36 = call ptr @moksha_unbox_string(ptr %51)
  %puts311 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox36)
  %52 = call ptr @moksha_box_string(ptr nonnull @52)
  %print_unbox37 = call ptr @moksha_unbox_string(ptr %52)
  %puts312 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox37)
  %53 = call ptr @moksha_box_string(ptr nonnull @54)
  call void @log(ptr %53)
  %54 = call ptr @moksha_box_int(i32 10)
  %55 = call ptr @moksha_box_int(i32 20)
  call void @performCalc(ptr %54, ptr %55)
  %56 = call ptr @moksha_box_string(ptr nonnull @55)
  %print_unbox38 = call ptr @moksha_unbox_string(ptr %56)
  %puts313 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox38)
  %57 = call ptr @moksha_box_string(ptr nonnull @57)
  %print_unbox39 = call ptr @moksha_unbox_string(ptr %57)
  %puts314 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox39)
  %58 = call ptr @moksha_box_string(ptr nonnull @59)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %58)
  %puts315 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox40)
  %59 = load i32, ptr @__exception_flag, align 4
  %.not316 = icmp eq i32 %59, 0
  br i1 %.not316, label %next_stmt45, label %catch_block

catch_block:                                      ; preds = %next_stmt51, %math_error, %next_stmt45, %next_stmt26
  store i32 0, ptr @__exception_flag, align 4
  %60 = load ptr, ptr @__exception_val, align 8
  %61 = call ptr @moksha_box_string(ptr nonnull @66)
  %62 = call ptr @moksha_any_to_str(ptr %60)
  %63 = call ptr @moksha_box_string(ptr %62)
  %64 = call ptr @moksha_string_concat(ptr %61, ptr %63)
  %print_unbox56 = call ptr @moksha_unbox_string(ptr %64)
  %puts324 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox56)
  %65 = load i32, ptr @__exception_flag, align 4
  %.not325 = icmp eq i32 %65, 0
  br i1 %.not325, label %try_block59, label %common.ret

next_stmt45:                                      ; preds = %next_stmt26
  %66 = call ptr @moksha_box_string(ptr nonnull @61)
  %print_unbox47 = call ptr @moksha_unbox_string(ptr %66)
  %puts318 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox47)
  %67 = load i32, ptr @__exception_flag, align 4
  %.not319 = icmp eq i32 %67, 0
  br i1 %.not319, label %math_error, label %catch_block

math_error:                                       ; preds = %next_stmt45
  %68 = call ptr @moksha_box_string(ptr nonnull @63)
  call void @moksha_throw_exception(ptr %68)
  store ptr %68, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  %69 = load i32, ptr @__exception_flag, align 4
  %.not320 = icmp eq i32 %69, 0
  br i1 %.not320, label %next_stmt51, label %catch_block

next_stmt51:                                      ; preds = %math_error
  %70 = call ptr @moksha_box_string(ptr nonnull @64)
  %71 = call ptr @moksha_int_to_str(i32 0)
  %72 = call ptr @moksha_box_string(ptr %71)
  %73 = call ptr @moksha_string_concat(ptr %70, ptr %72)
  %print_unbox53 = call ptr @moksha_unbox_string(ptr %73)
  %puts321 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox53)
  %74 = load i32, ptr @__exception_flag, align 4
  %.not322 = icmp eq i32 %74, 0
  br i1 %.not322, label %try_block59, label %catch_block

try_block59:                                      ; preds = %catch_block, %next_stmt51
  %75 = call ptr @moksha_box_string(ptr nonnull @68)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %75)
  %puts326 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox63)
  %76 = load i32, ptr @__exception_flag, align 4
  %.not327 = icmp eq i32 %76, 0
  br i1 %.not327, label %math_error67, label %catch_block61

catch_block61:                                    ; preds = %next_stmt72, %math_error67, %try_block59
  store i32 0, ptr @__exception_flag, align 4
  %77 = load ptr, ptr @__exception_val, align 8
  %78 = call ptr @moksha_box_string(ptr nonnull @73)
  %79 = call ptr @moksha_any_to_str(ptr %77)
  %80 = call ptr @moksha_box_string(ptr %79)
  %81 = call ptr @moksha_string_concat(ptr %78, ptr %80)
  %print_unbox78 = call ptr @moksha_unbox_string(ptr %81)
  %puts332 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox78)
  %82 = load i32, ptr @__exception_flag, align 4
  %.not333 = icmp eq i32 %82, 0
  br i1 %.not333, label %try_block81, label %common.ret

math_error67:                                     ; preds = %try_block59
  %83 = call ptr @moksha_box_string(ptr nonnull @70)
  call void @moksha_throw_exception(ptr %83)
  store ptr %83, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  %84 = load i32, ptr @__exception_flag, align 4
  %.not328 = icmp eq i32 %84, 0
  br i1 %.not328, label %next_stmt72, label %catch_block61

next_stmt72:                                      ; preds = %math_error67
  %85 = call ptr @moksha_box_string(ptr nonnull @71)
  %86 = call ptr @moksha_int_to_str(i32 0)
  %87 = call ptr @moksha_box_string(ptr %86)
  %88 = call ptr @moksha_string_concat(ptr %85, ptr %87)
  %print_unbox74 = call ptr @moksha_unbox_string(ptr %88)
  %puts329 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox74)
  %89 = load i32, ptr @__exception_flag, align 4
  %.not330 = icmp eq i32 %89, 0
  br i1 %.not330, label %try_block81, label %catch_block61

try_block81:                                      ; preds = %catch_block61, %next_stmt72
  %90 = load i32, ptr @__exception_flag, align 4
  %.not334 = icmp eq i32 %90, 0
  br i1 %.not334, label %next_stmt89, label %catch_block83

catch_block83:                                    ; preds = %next_stmt100, %math_error95, %next_stmt89, %try_block81
  store i32 0, ptr @__exception_flag, align 4
  %91 = load ptr, ptr @__exception_val, align 8
  %92 = call ptr @moksha_box_string(ptr nonnull @81)
  %93 = call ptr @moksha_any_to_str(ptr %91)
  %94 = call ptr @moksha_box_string(ptr %93)
  %95 = call ptr @moksha_string_concat(ptr %92, ptr %94)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %95)
  %puts342 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox106)
  %96 = load i32, ptr @__exception_flag, align 4
  %.not343 = icmp eq i32 %96, 0
  br i1 %.not343, label %try_cont84, label %common.ret

try_cont84:                                       ; preds = %catch_block83, %next_stmt100
  %97 = call ptr @moksha_box_string(ptr nonnull @83)
  %print_unbox109 = call ptr @moksha_unbox_string(ptr %97)
  %puts344 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox109)
  %98 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 1)
  %99 = call ptr @moksha_array_push_val(ptr %98, ptr %box_i)
  %box_i114 = call ptr @moksha_box_int(i32 2)
  %100 = call ptr @moksha_array_push_val(ptr %98, ptr %box_i114)
  %box_i115 = call ptr @moksha_box_int(i32 3)
  %101 = call ptr @moksha_array_push_val(ptr %98, ptr %box_i115)
  %102 = load i32, ptr @__exception_flag, align 4
  %.not345 = icmp eq i32 %102, 0
  br i1 %.not345, label %next_stmt117, label %catch_block112

next_stmt89:                                      ; preds = %try_block81
  %103 = call ptr @moksha_box_string(ptr nonnull @75)
  %print_unbox91 = call ptr @moksha_unbox_string(ptr %103)
  %puts336 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox91)
  %104 = load i32, ptr @__exception_flag, align 4
  %.not337 = icmp eq i32 %104, 0
  br i1 %.not337, label %math_error95, label %catch_block83

math_error95:                                     ; preds = %next_stmt89
  %105 = call ptr @moksha_box_string(ptr nonnull @77)
  call void @moksha_throw_exception(ptr %105)
  store ptr %105, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  %106 = load i32, ptr @__exception_flag, align 4
  %.not338 = icmp eq i32 %106, 0
  br i1 %.not338, label %next_stmt100, label %catch_block83

next_stmt100:                                     ; preds = %math_error95
  %107 = call ptr @moksha_box_string(ptr nonnull @78)
  %108 = call ptr @moksha_double_to_str(double 0.000000e+00)
  %109 = call ptr @moksha_box_string(ptr %108)
  %110 = call ptr @moksha_string_concat(ptr %107, ptr %109)
  %111 = call ptr @moksha_box_string(ptr nonnull @79)
  %112 = call ptr @moksha_string_concat(ptr %110, ptr %111)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %112)
  %puts339 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox102)
  %113 = load i32, ptr @__exception_flag, align 4
  %.not340 = icmp eq i32 %113, 0
  br i1 %.not340, label %try_cont84, label %catch_block83

catch_block112:                                   ; preds = %next_stmt123, %next_stmt120, %next_stmt117, %try_cont84, %next_stmt126
  store i32 0, ptr @__exception_flag, align 4
  %114 = load ptr, ptr @__exception_val, align 8
  %115 = call ptr @moksha_box_string(ptr nonnull @89)
  %116 = call ptr @moksha_any_to_str(ptr %114)
  %117 = call ptr @moksha_box_string(ptr %116)
  %118 = call ptr @moksha_string_concat(ptr %115, ptr %117)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %118)
  %puts352 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox129)
  %119 = load i32, ptr @__exception_flag, align 4
  %.not353 = icmp eq i32 %119, 0
  br i1 %.not353, label %try_block132, label %common.ret

next_stmt117:                                     ; preds = %try_cont84
  %120 = call ptr @moksha_box_string(ptr nonnull @85)
  %print_unbox119 = call ptr @moksha_unbox_string(ptr %120)
  %puts346 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox119)
  %121 = load i32, ptr @__exception_flag, align 4
  %.not347 = icmp eq i32 %121, 0
  br i1 %.not347, label %next_stmt120, label %catch_block112

next_stmt120:                                     ; preds = %next_stmt117
  %122 = call ptr @moksha_array_get(ptr %98, i32 5)
  store ptr %122, ptr %val122, align 8
  %123 = load i32, ptr @__exception_flag, align 4
  %.not348 = icmp eq i32 %123, 0
  br i1 %.not348, label %next_stmt123, label %catch_block112

next_stmt123:                                     ; preds = %next_stmt120
  %124 = call ptr @moksha_box_string(ptr nonnull @87)
  %125 = load i32, ptr %val122, align 4
  %126 = call ptr @moksha_int_to_str(i32 %125)
  %127 = call ptr @moksha_box_string(ptr %126)
  %128 = call ptr @moksha_string_concat(ptr %124, ptr %127)
  %print_unbox125 = call ptr @moksha_unbox_string(ptr %128)
  %puts349 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox125)
  %129 = load i32, ptr @__exception_flag, align 4
  %.not350 = icmp eq i32 %129, 0
  br i1 %.not350, label %next_stmt126, label %catch_block112

next_stmt126:                                     ; preds = %next_stmt123
  call void @moksha_free(ptr %98)
  %130 = load i32, ptr @__exception_flag, align 4
  %.not351 = icmp eq i32 %130, 0
  br i1 %.not351, label %try_block132, label %catch_block112

try_block132:                                     ; preds = %catch_block112, %next_stmt126
  %131 = call ptr @moksha_alloc_array(i32 3)
  %box_i137 = call ptr @moksha_box_int(i32 1)
  %132 = call ptr @moksha_array_push_val(ptr %131, ptr %box_i137)
  %box_i138 = call ptr @moksha_box_int(i32 2)
  %133 = call ptr @moksha_array_push_val(ptr %131, ptr %box_i138)
  %box_i139 = call ptr @moksha_box_int(i32 3)
  %134 = call ptr @moksha_array_push_val(ptr %131, ptr %box_i139)
  %135 = load i32, ptr @__exception_flag, align 4
  %.not354 = icmp eq i32 %135, 0
  br i1 %.not354, label %next_stmt141, label %catch_block134

catch_block134:                                   ; preds = %next_stmt148, %next_stmt144, %next_stmt141, %try_block132, %next_stmt151
  store i32 0, ptr @__exception_flag, align 4
  %136 = load ptr, ptr @__exception_val, align 8
  %137 = call ptr @moksha_box_string(ptr nonnull @95)
  %138 = call ptr @moksha_any_to_str(ptr %136)
  %139 = call ptr @moksha_box_string(ptr %138)
  %140 = call ptr @moksha_string_concat(ptr %137, ptr %139)
  %print_unbox154 = call ptr @moksha_unbox_string(ptr %140)
  %puts361 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox154)
  %141 = load i32, ptr @__exception_flag, align 4
  %.not362 = icmp eq i32 %141, 0
  br i1 %.not362, label %try_block157, label %common.ret

next_stmt141:                                     ; preds = %try_block132
  %142 = call ptr @moksha_box_string(ptr nonnull @91)
  %print_unbox143 = call ptr @moksha_unbox_string(ptr %142)
  %puts355 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox143)
  %143 = load i32, ptr @__exception_flag, align 4
  %.not356 = icmp eq i32 %143, 0
  br i1 %.not356, label %next_stmt144, label %catch_block134

next_stmt144:                                     ; preds = %next_stmt141
  %144 = call ptr @moksha_array_get(ptr %131, i32 -1)
  store ptr %144, ptr %val147, align 8
  %145 = load i32, ptr @__exception_flag, align 4
  %.not357 = icmp eq i32 %145, 0
  br i1 %.not357, label %next_stmt148, label %catch_block134

next_stmt148:                                     ; preds = %next_stmt144
  %146 = call ptr @moksha_box_string(ptr nonnull @93)
  %147 = load i32, ptr %val147, align 4
  %148 = call ptr @moksha_int_to_str(i32 %147)
  %149 = call ptr @moksha_box_string(ptr %148)
  %150 = call ptr @moksha_string_concat(ptr %146, ptr %149)
  %print_unbox150 = call ptr @moksha_unbox_string(ptr %150)
  %puts358 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox150)
  %151 = load i32, ptr @__exception_flag, align 4
  %.not359 = icmp eq i32 %151, 0
  br i1 %.not359, label %next_stmt151, label %catch_block134

next_stmt151:                                     ; preds = %next_stmt148
  call void @moksha_free(ptr %131)
  %152 = load i32, ptr @__exception_flag, align 4
  %.not360 = icmp eq i32 %152, 0
  br i1 %.not360, label %try_block157, label %catch_block134

try_block157:                                     ; preds = %catch_block134, %next_stmt151
  %153 = call ptr @moksha_box_string(ptr nonnull @97)
  %154 = load i32, ptr @__exception_flag, align 4
  %.not363 = icmp eq i32 %154, 0
  br i1 %.not363, label %next_stmt162, label %catch_block159

catch_block159:                                   ; preds = %ifcont172, %else171, %then170, %next_stmt165, %next_stmt162, %try_block157, %next_stmt179
  store i32 0, ptr @__exception_flag, align 4
  %155 = load ptr, ptr @__exception_val, align 8
  %156 = call ptr @moksha_box_string(ptr nonnull @105)
  %157 = call ptr @moksha_any_to_str(ptr %155)
  %158 = call ptr @moksha_box_string(ptr %157)
  %159 = call ptr @moksha_string_concat(ptr %156, ptr %158)
  %print_unbox182 = call ptr @moksha_unbox_string(ptr %159)
  %puts373 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox182)
  %160 = load i32, ptr @__exception_flag, align 4
  %.not374 = icmp eq i32 %160, 0
  br i1 %.not374, label %try_cont160, label %common.ret

try_cont160:                                      ; preds = %catch_block159, %next_stmt179
  %161 = call ptr @moksha_box_string(ptr nonnull @107)
  %print_unbox185 = call ptr @moksha_unbox_string(ptr %161)
  %puts375 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox185)
  %162 = call ptr @moksha_box_string(ptr nonnull @109)
  %163 = load i32, ptr @__exception_flag, align 4
  %.not376 = icmp eq i32 %163, 0
  br i1 %.not376, label %next_stmt191, label %catch_block188

next_stmt162:                                     ; preds = %try_block157
  %164 = call ptr @moksha_box_string(ptr nonnull @98)
  %print_unbox164 = call ptr @moksha_unbox_string(ptr %164)
  %puts364 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox164)
  %165 = load i32, ptr @__exception_flag, align 4
  %.not365 = icmp eq i32 %165, 0
  br i1 %.not365, label %next_stmt165, label %catch_block159

next_stmt165:                                     ; preds = %next_stmt162
  %166 = call ptr @moksha_string_get_char(ptr %153, i32 10)
  %167 = call ptr @moksha_any_to_str(ptr %166)
  %168 = call ptr @moksha_box_string(ptr %167)
  %169 = load i32, ptr @__exception_flag, align 4
  %.not366 = icmp eq i32 %169, 0
  br i1 %.not366, label %next_stmt168, label %catch_block159

next_stmt168:                                     ; preds = %next_stmt165
  %170 = call ptr @moksha_box_string(ptr nonnull @100)
  %unbox_L_f = call double @moksha_unbox_double(ptr %168)
  %unbox_R_f = call double @moksha_unbox_double(ptr %170)
  %fcmptmp = fcmp oeq double %unbox_L_f, %unbox_R_f
  br i1 %fcmptmp, label %then170, label %else171

then170:                                          ; preds = %next_stmt168
  %171 = call ptr @moksha_box_string(ptr nonnull @101)
  %print_unbox173 = call ptr @moksha_unbox_string(ptr %171)
  %puts369 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox173)
  %172 = load i32, ptr @__exception_flag, align 4
  %.not370 = icmp eq i32 %172, 0
  br i1 %.not370, label %ifcont172, label %catch_block159

else171:                                          ; preds = %next_stmt168
  %173 = call ptr @moksha_box_string(ptr nonnull @103)
  %174 = call ptr @moksha_string_concat(ptr %173, ptr %168)
  %print_unbox176 = call ptr @moksha_unbox_string(ptr %174)
  %puts367 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox176)
  %175 = load i32, ptr @__exception_flag, align 4
  %.not368 = icmp eq i32 %175, 0
  br i1 %.not368, label %ifcont172, label %catch_block159

ifcont172:                                        ; preds = %else171, %then170
  %176 = load i32, ptr @__exception_flag, align 4
  %.not371 = icmp eq i32 %176, 0
  br i1 %.not371, label %next_stmt179, label %catch_block159

next_stmt179:                                     ; preds = %ifcont172
  call void @moksha_free(ptr %168)
  call void @moksha_free(ptr %153)
  %177 = load i32, ptr @__exception_flag, align 4
  %.not372 = icmp eq i32 %177, 0
  br i1 %.not372, label %try_cont160, label %catch_block159

catch_block188:                                   ; preds = %next_stmt198, %next_stmt194, %next_stmt191, %try_cont160
  store i32 0, ptr @__exception_flag, align 4
  %178 = load ptr, ptr @__exception_val, align 8
  %179 = call ptr @moksha_box_string(ptr nonnull @115)
  %180 = call ptr @moksha_any_to_str(ptr %178)
  %181 = call ptr @moksha_box_string(ptr %180)
  %182 = call ptr @moksha_string_concat(ptr %179, ptr %181)
  %print_unbox204 = call ptr @moksha_unbox_string(ptr %182)
  %puts383 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox204)
  %183 = load i32, ptr @__exception_flag, align 4
  %.not384 = icmp eq i32 %183, 0
  br i1 %.not384, label %clone_merge, label %common.ret

next_stmt191:                                     ; preds = %try_cont160
  %184 = call ptr @moksha_box_string(ptr nonnull @110)
  %print_unbox193 = call ptr @moksha_unbox_string(ptr %184)
  %puts377 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox193)
  %185 = load i32, ptr @__exception_flag, align 4
  %.not378 = icmp eq i32 %185, 0
  br i1 %.not378, label %next_stmt194, label %catch_block188

next_stmt194:                                     ; preds = %next_stmt191
  %unbox_i = call i32 @moksha_unbox_int(ptr %162)
  %186 = load i32, ptr @__exception_flag, align 4
  %.not379 = icmp eq i32 %186, 0
  br i1 %.not379, label %next_stmt198, label %catch_block188

next_stmt198:                                     ; preds = %next_stmt194
  %187 = call ptr @moksha_box_string(ptr nonnull @112)
  %188 = call ptr @moksha_int_to_str(i32 %unbox_i)
  %189 = call ptr @moksha_box_string(ptr %188)
  %190 = call ptr @moksha_string_concat(ptr %187, ptr %189)
  %191 = call ptr @moksha_box_string(ptr nonnull @113)
  %192 = call ptr @moksha_string_concat(ptr %190, ptr %191)
  %print_unbox200 = call ptr @moksha_unbox_string(ptr %192)
  %puts380 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox200)
  %193 = load i32, ptr @__exception_flag, align 4
  %.not381 = icmp eq i32 %193, 0
  br i1 %.not381, label %clone_merge, label %catch_block188

catch_block209:                                   ; preds = %next_stmt212, %clone_merge, %call_error
  store i32 0, ptr @__exception_flag, align 4
  %194 = load ptr, ptr @__exception_val, align 8
  %195 = call ptr @moksha_box_string(ptr nonnull @122)
  %196 = call ptr @moksha_any_to_str(ptr %194)
  %197 = call ptr @moksha_box_string(ptr %196)
  %198 = call ptr @moksha_string_concat(ptr %195, ptr %197)
  %print_unbox223 = call ptr @moksha_unbox_string(ptr %198)
  %puts388 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox223)
  %199 = load i32, ptr @__exception_flag, align 4
  %.not389 = icmp eq i32 %199, 0
  br i1 %.not389, label %next_stmt224, label %common.ret

clone_merge:                                      ; preds = %catch_block188, %next_stmt198
  %200 = load i32, ptr @__exception_flag, align 4
  %.not385 = icmp eq i32 %200, 0
  br i1 %.not385, label %next_stmt212, label %catch_block209

next_stmt212:                                     ; preds = %clone_merge
  %201 = call ptr @moksha_box_string(ptr nonnull @117)
  %print_unbox214 = call ptr @moksha_unbox_string(ptr %201)
  %puts386 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox214)
  %202 = load i32, ptr @__exception_flag, align 4
  %.not387 = icmp eq i32 %202, 0
  br i1 %.not387, label %call_error, label %catch_block209

call_error:                                       ; preds = %next_stmt212
  %203 = call ptr @moksha_box_string(ptr nonnull @119)
  store ptr %203, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %catch_block209

next_stmt224:                                     ; preds = %catch_block209
  %204 = call ptr @moksha_box_string(ptr nonnull @124)
  %print_unbox226 = call ptr @moksha_unbox_string(ptr %204)
  %puts390 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox226)
  %205 = call ptr @moksha_box_string(ptr nonnull @127)
  %print_unbox231 = call ptr @moksha_unbox_string(ptr %205)
  %puts391 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox231)
  %206 = load i32, ptr @__exception_flag, align 4
  %.not392 = icmp eq i32 %206, 0
  br i1 %.not392, label %next_stmt232, label %catch_block229

catch_block229:                                   ; preds = %next_stmt234, %next_stmt232, %next_stmt224
  store i32 0, ptr @__exception_flag, align 4
  %207 = load ptr, ptr @__exception_val, align 8
  %208 = call ptr @moksha_box_string(ptr nonnull @131)
  %209 = call ptr @moksha_any_to_str(ptr %207)
  %210 = call ptr @moksha_box_string(ptr %209)
  %211 = call ptr @moksha_string_concat(ptr %208, ptr %210)
  %print_unbox240 = call ptr @moksha_unbox_string(ptr %211)
  %puts397 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox240)
  %212 = load i32, ptr @__exception_flag, align 4
  %.not398 = icmp eq i32 %212, 0
  br i1 %.not398, label %try_cont230, label %common.ret

try_cont230:                                      ; preds = %catch_block229, %next_stmt234
  %213 = call ptr @moksha_box_string(ptr nonnull @133)
  %print_unbox243 = call ptr @moksha_unbox_string(ptr %213)
  %puts399 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox243)
  %214 = call ptr @moksha_box_string(ptr nonnull @135)
  call void @moksha_throw_exception(ptr %214)
  store ptr %214, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  %215 = load i32, ptr @__exception_flag, align 4
  %.not400 = icmp eq i32 %215, 0
  br i1 %.not400, label %next_stmt254, label %catch_block247

next_stmt232:                                     ; preds = %next_stmt224
  call void @caller()
  %216 = load i32, ptr @__exception_flag, align 4
  %.not393 = icmp eq i32 %216, 0
  br i1 %.not393, label %next_stmt234, label %catch_block229

next_stmt234:                                     ; preds = %next_stmt232
  %217 = call ptr @moksha_box_string(ptr nonnull @129)
  %print_unbox236 = call ptr @moksha_unbox_string(ptr %217)
  %puts394 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox236)
  %218 = load i32, ptr @__exception_flag, align 4
  %.not395 = icmp eq i32 %218, 0
  br i1 %.not395, label %try_cont230, label %catch_block229

catch_block247:                                   ; preds = %next_stmt254, %try_cont230
  store i32 0, ptr @__exception_flag, align 4
  %219 = call ptr @moksha_box_string(ptr nonnull @138)
  %print_unbox265 = call ptr @moksha_unbox_string(ptr %219)
  %puts405 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox265)
  %220 = load i32, ptr @__exception_flag, align 4
  %.not406 = icmp eq i32 %220, 0
  %221 = load i32, ptr @__exception_flag, align 4
  %.not407 = icmp eq i32 %221, 0
  %or.cond = select i1 %.not406, i1 %.not407, i1 false
  br i1 %or.cond, label %try_cont248, label %common.ret

try_cont248:                                      ; preds = %catch_block247, %next_stmt254
  %222 = call ptr @moksha_box_string(ptr nonnull @142)
  %print_unbox273 = call ptr @moksha_unbox_string(ptr %222)
  %puts408 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox273)
  %223 = call ptr @moksha_box_string(ptr nonnull @144)
  %print_unbox278 = call ptr @moksha_unbox_string(ptr %223)
  %puts409 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox278)
  %224 = load i32, ptr @__exception_flag, align 4
  %.not410 = icmp eq i32 %224, 0
  br i1 %.not410, label %next_stmt279, label %catch_block276

next_stmt254:                                     ; preds = %try_cont230
  %225 = call ptr @moksha_box_string(ptr nonnull @136)
  %print_unbox256 = call ptr @moksha_unbox_string(ptr %225)
  %puts401 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox256)
  %226 = load i32, ptr @__exception_flag, align 4
  %.not402 = icmp eq i32 %226, 0
  br i1 %.not402, label %try_cont248, label %catch_block247

catch_block276:                                   ; preds = %next_stmt282, %next_stmt279, %try_cont248, %next_stmt285
  store i32 0, ptr @__exception_flag, align 4
  %227 = load ptr, ptr @__exception_val, align 8
  %228 = call ptr @moksha_box_string(ptr nonnull @148)
  %229 = call ptr @moksha_any_to_str(ptr %227)
  %230 = call ptr @moksha_box_string(ptr %229)
  %231 = call ptr @moksha_string_concat(ptr %228, ptr %230)
  %print_unbox288 = call ptr @moksha_unbox_string(ptr %231)
  %puts415 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox288)
  %232 = load i32, ptr @__exception_flag, align 4
  %.not416 = icmp eq i32 %232, 0
  br i1 %.not416, label %try_cont277, label %common.ret

try_cont277:                                      ; preds = %catch_block276, %next_stmt285
  %233 = call ptr @moksha_box_string(ptr nonnull @150)
  %print_unbox291 = call ptr @moksha_unbox_string(ptr %233)
  %puts417 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox291)
  br label %common.ret

next_stmt279:                                     ; preds = %try_cont248
  %234 = call ptr @moksha_box_int(i32 0)
  %arr_leaf = call ptr @moksha_alloc_array_fill(i32 -5, ptr %234)
  %235 = load i32, ptr @__exception_flag, align 4
  %.not411 = icmp eq i32 %235, 0
  br i1 %.not411, label %next_stmt282, label %catch_block276

next_stmt282:                                     ; preds = %next_stmt279
  %236 = call ptr @moksha_box_string(ptr nonnull @146)
  %print_unbox284 = call ptr @moksha_unbox_string(ptr %236)
  %puts412 = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox284)
  %237 = load i32, ptr @__exception_flag, align 4
  %.not413 = icmp eq i32 %237, 0
  br i1 %.not413, label %next_stmt285, label %catch_block276

next_stmt285:                                     ; preds = %next_stmt282
  call void @moksha_free(ptr %arr_leaf)
  %238 = load i32, ptr @__exception_flag, align 4
  %.not414 = icmp eq i32 %238, 0
  br i1 %.not414, label %try_cont277, label %catch_block276
}

; Function Attrs: alwaysinline
define void @log(ptr %msg) #1 {
entry:
  %0 = call ptr @moksha_box_string(ptr nonnull @46)
  %1 = call ptr @moksha_any_to_str(ptr %msg)
  %2 = call ptr @moksha_box_string(ptr %1)
  %3 = call ptr @moksha_string_concat(ptr %0, ptr %2)
  %print_unbox = call ptr @moksha_unbox_string(ptr %3)
  %puts = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox)
  ret void
}

; Function Attrs: alwaysinline
define void @performCalc(ptr %x, ptr %y) #1 {
entry:
  %unbox_L_f = call double @moksha_unbox_double(ptr %x)
  %unbox_R_f = call double @moksha_unbox_double(ptr %y)
  %faddtmp = fadd double %unbox_L_f, %unbox_R_f
  %0 = call ptr @moksha_box_double(double %faddtmp)
  %unbox_i = call i32 @moksha_unbox_int(ptr %0)
  %1 = load i32, ptr @__exception_flag, align 4
  %.not = icmp eq i32 %1, 0
  br i1 %.not, label %next_stmt, label %common.ret

next_stmt:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr nonnull @48)
  %3 = call ptr @moksha_int_to_str(i32 %unbox_i)
  %4 = call ptr @moksha_box_string(ptr %3)
  %5 = call ptr @moksha_string_concat(ptr %2, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %puts = call i32 @puts(ptr nonnull dereferenceable(1) %print_unbox)
  br label %common.ret

common.ret:                                       ; preds = %entry, %next_stmt
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
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #2

define void @thrower() {
entry:
  %0 = call ptr @moksha_box_string(ptr nonnull @126)
  call void @moksha_throw_exception(ptr %0)
  store ptr %0, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void
}

define void @caller() {
entry:
  call void @thrower()
  ret void
}

attributes #0 = { nofree nounwind }
attributes #1 = { alwaysinline }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
