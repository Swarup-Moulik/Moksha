; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Counter = type { ptr, i32 }

@__exception_flag = external global i32
@__exception_val = external global ptr
@0 = private unnamed_addr constant [32 x i8] c"==== ARITHMETIC TEST SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [22 x i8] c"\0A[Integer Operations]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [10 x i8] c"10 + 3 = \00", align 1
@5 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@6 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@7 = private unnamed_addr constant [10 x i8] c"10 - 3 = \00", align 1
@8 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@10 = private unnamed_addr constant [10 x i8] c"10 * 3 = \00", align 1
@11 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@12 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@13 = private unnamed_addr constant [10 x i8] c"10 / 3 = \00", align 1
@14 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@15 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@16 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@17 = private unnamed_addr constant [10 x i8] c"10 % 3 = \00", align 1
@18 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@19 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@20 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@21 = private unnamed_addr constant [21 x i8] c"\0A[Double Operations]\00", align 1
@22 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@23 = private unnamed_addr constant [14 x i8] c"10.5 + 2.5 = \00", align 1
@24 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@26 = private unnamed_addr constant [14 x i8] c"10.5 - 2.5 = \00", align 1
@27 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@29 = private unnamed_addr constant [14 x i8] c"10.5 * 2.5 = \00", align 1
@30 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@32 = private unnamed_addr constant [14 x i8] c"10.5 / 2.5 = \00", align 1
@33 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@34 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@35 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@36 = private unnamed_addr constant [25 x i8] c"\0A[Implicit Type Casting]\00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@38 = private unnamed_addr constant [26 x i8] c"5 (int) + 2.5 (double) = \00", align 1
@39 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@41 = private unnamed_addr constant [26 x i8] c"2.5 (double) - 5 (int) = \00", align 1
@42 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@43 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@44 = private unnamed_addr constant [26 x i8] c"5 (int) * 2.5 (double) = \00", align 1
@45 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@46 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@47 = private unnamed_addr constant [34 x i8] c"Assigned 10 (int) to double var: \00", align 1
@48 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@49 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@50 = private unnamed_addr constant [22 x i8] c"\0A[Bitwise Operations]\00", align 1
@51 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@52 = private unnamed_addr constant [23 x i8] c"5 & 3 (0101 & 0011) = \00", align 1
@53 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@54 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@55 = private unnamed_addr constant [23 x i8] c"5 | 3 (0101 | 0011) = \00", align 1
@56 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@57 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@58 = private unnamed_addr constant [23 x i8] c"5 ^ 3 (0101 ^ 0011) = \00", align 1
@59 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@60 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@61 = private unnamed_addr constant [33 x i8] c"\0A[Complex Expression Precedence]\00", align 1
@62 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@63 = private unnamed_addr constant [27 x i8] c"(10 + 2) * 3 + (20 / 4) = \00", align 1
@64 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@65 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@66 = private unnamed_addr constant [41 x i8] c"==== ADVANCED MATH & OPERATOR SUITE ====\00", align 1
@67 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@68 = private unnamed_addr constant [18 x i8] c"\0A[Division Types]\00", align 1
@69 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@70 = private unnamed_addr constant [19 x i8] c"5 / 2 (Standard): \00", align 1
@71 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@72 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@73 = private unnamed_addr constant [24 x i8] c"10.0 / 4.0 (Standard): \00", align 1
@74 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@75 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@76 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@77 = private unnamed_addr constant [23 x i8] c"int(5 / 2) (Integer): \00", align 1
@78 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@79 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@80 = private unnamed_addr constant [28 x i8] c"int(10.0 / 4.0) (Integer): \00", align 1
@81 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@82 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@83 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@84 = private unnamed_addr constant [18 x i8] c"\0A[Power Operator]\00", align 1
@85 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@86 = private unnamed_addr constant [10 x i8] c"2 ** 3 = \00", align 1
@87 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@89 = private unnamed_addr constant [25 x i8] c"\0A[Shift & Not Operators]\00", align 1
@90 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@91 = private unnamed_addr constant [23 x i8] c"8 << 1 (Left Shift) = \00", align 1
@92 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@93 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@94 = private unnamed_addr constant [24 x i8] c"8 >> 2 (Right Shift) = \00", align 1
@95 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@96 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@97 = private unnamed_addr constant [20 x i8] c"~0 (Bitwise Not) = \00", align 1
@98 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@99 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@100 = private unnamed_addr constant [20 x i8] c"\0A[Implicit Casting]\00", align 1
@101 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@102 = private unnamed_addr constant [12 x i8] c"10 + 2.5 = \00", align 1
@103 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@104 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@105 = private unnamed_addr constant [17 x i8] c"int(10 / 2.5) = \00", align 1
@106 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@107 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@108 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@109 = private unnamed_addr constant [42 x i8] c"==== UNARY INCREMENT/DECREMENT SUITE ====\00", align 1
@110 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@111 = private unnamed_addr constant [26 x i8] c"\0A[Integer Prefix/Postfix]\00", align 1
@112 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@113 = private unnamed_addr constant [13 x i8] c"Initial a2: \00", align 1
@114 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@115 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@116 = private unnamed_addr constant [17 x i8] c"Postfix (a2++): \00", align 1
@117 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@118 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@119 = private unnamed_addr constant [16 x i8] c"After Postfix: \00", align 1
@120 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@121 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@122 = private unnamed_addr constant [16 x i8] c"Prefix (++a2): \00", align 1
@123 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@124 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@125 = private unnamed_addr constant [17 x i8] c"Postfix (b2--): \00", align 1
@126 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@127 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@128 = private unnamed_addr constant [16 x i8] c"After Postfix: \00", align 1
@129 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@130 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@131 = private unnamed_addr constant [16 x i8] c"Prefix (--b2): \00", align 1
@132 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@133 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@134 = private unnamed_addr constant [25 x i8] c"\0A[Double Prefix/Postfix]\00", align 1
@135 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@136 = private unnamed_addr constant [13 x i8] c"Initial d3: \00", align 1
@137 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@138 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@139 = private unnamed_addr constant [13 x i8] c"After d3++: \00", align 1
@140 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@141 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@142 = private unnamed_addr constant [13 x i8] c"After --d3: \00", align 1
@143 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@144 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@145 = private unnamed_addr constant [15 x i8] c"\0A[Expressions]\00", align 1
@146 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@147 = private unnamed_addr constant [9 x i8] c"Result: \00", align 1
@148 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@149 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@150 = private unnamed_addr constant [11 x i8] c"Final x2: \00", align 1
@151 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@152 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@153 = private unnamed_addr constant [11 x i8] c"Final y2: \00", align 1
@154 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@155 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@156 = private unnamed_addr constant [26 x i8] c"\0A[Object Field Increment]\00", align 1
@157 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@158 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@159 = private unnamed_addr constant [19 x i8] c"c.count after ++: \00", align 1
@160 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@161 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@162 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@163 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@164 = private unnamed_addr constant [26 x i8] c"c.count after prefix ++: \00", align 1
@165 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@166 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@167 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare ptr @calloc(i64, i64)

declare void @moksha_free(ptr)

declare ptr @strcpy(ptr, ptr)

declare ptr @memcpy(ptr, ptr, i64)

declare ptr @memset(ptr, i32, i64)

declare i32 @strlen(ptr)

declare i32 @puts(ptr)

declare i32 @sprintf(ptr, ptr, ...)

declare i32 @strcmp(ptr, ptr)

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

declare ptr @moksha_ptr_to_str(ptr)

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

declare i32 @moksha_strlen(ptr)

declare i32 @moksha_read_int()

declare double @moksha_read_double()

declare i32 @moksha_unbox_bool(ptr)

declare i32 @moksha_read_bool()

declare ptr @moksha_read_string()

declare void @moksha_throw_exception(ptr)

declare ptr @moksha_create_closure(ptr, i32, ptr)

declare ptr @moksha_get_closure_env(ptr)

declare ptr @moksha_get_closure_func(ptr)

define i32 @main() {
entry:
  %c121 = alloca ptr, align 8
  %c = alloca ptr, align 8
  %res116 = alloca i32, align 4
  %res = alloca i32, align 4
  %y2109 = alloca i32, align 4
  %y2 = alloca i32, align 4
  %x2108 = alloca i32, align 4
  %x2 = alloca i32, align 4
  %d3101 = alloca double, align 8
  %d3 = alloca double, align 8
  %b293 = alloca i32, align 4
  %b2 = alloca i32, align 4
  %a286 = alloca i32, align 4
  %a285 = alloca i32, align 4
  %d273 = alloca double, align 8
  %d2 = alloca double, align 8
  %i272 = alloca i32, align 4
  %i2 = alloca i32, align 4
  %bitNot69 = alloca i32, align 4
  %bitNot = alloca i32, align 4
  %val66 = alloca i32, align 4
  %val = alloca i32, align 4
  %exp63 = alloca i32, align 4
  %exp = alloca i32, align 4
  %base62 = alloca i32, align 4
  %base = alloca i32, align 4
  %complex45 = alloca i32, align 4
  %complex = alloca i32, align 4
  %bitB37 = alloca i32, align 4
  %bitB = alloca i32, align 4
  %bitA36 = alloca i32, align 4
  %bitA = alloca i32, align 4
  %promoted33 = alloca double, align 8
  %promoted = alloca double, align 8
  %d26 = alloca double, align 8
  %d = alloca double, align 8
  %i25 = alloca i32, align 4
  %i = alloca i32, align 4
  %y15 = alloca double, align 8
  %y = alloca double, align 8
  %x14 = alloca double, align 8
  %x = alloca double, align 8
  %b3 = alloca i32, align 4
  %b = alloca i32, align 4
  %a2 = alloca i32, align 4
  %a = alloca i32, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  store i32 10, ptr %a, align 4
  store i32 10, ptr %a2, align 4
  store i32 3, ptr %b, align 4
  store i32 3, ptr %b3, align 4
  %4 = call ptr @moksha_box_string(ptr @4)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %4)
  %5 = call i32 (ptr, ...) @printf(ptr @5, ptr %print_unbox4)
  %6 = load i32, ptr %a2, align 4
  %7 = load i32, ptr %b3, align 4
  %addtmp = add i32 %6, %7
  %8 = call i32 (ptr, ...) @printf(ptr @6, i32 %addtmp)
  %9 = call ptr @moksha_box_string(ptr @7)
  %print_unbox5 = call ptr @moksha_unbox_string(ptr %9)
  %10 = call i32 (ptr, ...) @printf(ptr @8, ptr %print_unbox5)
  %11 = load i32, ptr %a2, align 4
  %12 = load i32, ptr %b3, align 4
  %subtmp = sub i32 %11, %12
  %13 = call i32 (ptr, ...) @printf(ptr @9, i32 %subtmp)
  %14 = call ptr @moksha_box_string(ptr @10)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %14)
  %15 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox6)
  %16 = load i32, ptr %a2, align 4
  %17 = load i32, ptr %b3, align 4
  %multmp = mul i32 %16, %17
  %18 = call i32 (ptr, ...) @printf(ptr @12, i32 %multmp)
  %19 = call ptr @moksha_box_string(ptr @13)
  %print_unbox7 = call ptr @moksha_unbox_string(ptr %19)
  %20 = call i32 (ptr, ...) @printf(ptr @14, ptr %print_unbox7)
  %21 = load i32, ptr %a2, align 4
  %22 = load i32, ptr %b3, align 4
  %23 = icmp eq i32 %22, 0
  br i1 %23, label %math_error, label %math_safe

math_error:                                       ; preds = %entry
  %24 = call ptr @moksha_box_string(ptr @15)
  call void @moksha_throw_exception(ptr %24)
  store ptr %24, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge

math_safe:                                        ; preds = %entry
  %sdivtmp = sdiv i32 %21, %22
  br label %math_merge

math_merge:                                       ; preds = %math_safe, %math_error
  %math_res = phi i32 [ 0, %math_error ], [ %sdivtmp, %math_safe ]
  %25 = call i32 (ptr, ...) @printf(ptr @16, i32 %math_res)
  %26 = call ptr @moksha_box_string(ptr @17)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %26)
  %27 = call i32 (ptr, ...) @printf(ptr @18, ptr %print_unbox8)
  %28 = load i32, ptr %a2, align 4
  %29 = load i32, ptr %b3, align 4
  %30 = icmp eq i32 %29, 0
  br i1 %30, label %math_error9, label %math_safe10

math_error9:                                      ; preds = %math_merge
  %31 = call ptr @moksha_box_string(ptr @19)
  call void @moksha_throw_exception(ptr %31)
  store ptr %31, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge11

math_safe10:                                      ; preds = %math_merge
  %sremtmp = srem i32 %28, %29
  br label %math_merge11

math_merge11:                                     ; preds = %math_safe10, %math_error9
  %math_res12 = phi i32 [ 0, %math_error9 ], [ %sremtmp, %math_safe10 ]
  %32 = call i32 (ptr, ...) @printf(ptr @20, i32 %math_res12)
  %33 = call ptr @moksha_box_string(ptr @21)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %33)
  %34 = call i32 (ptr, ...) @printf(ptr @22, ptr %print_unbox13)
  store double 1.050000e+01, ptr %x, align 8
  store double 1.050000e+01, ptr %x14, align 8
  store double 2.500000e+00, ptr %y, align 8
  store double 2.500000e+00, ptr %y15, align 8
  %35 = call ptr @moksha_box_string(ptr @23)
  %print_unbox16 = call ptr @moksha_unbox_string(ptr %35)
  %36 = call i32 (ptr, ...) @printf(ptr @24, ptr %print_unbox16)
  %37 = load double, ptr %x14, align 8
  %38 = load double, ptr %y15, align 8
  %faddtmp = fadd double %37, %38
  %39 = call i32 (ptr, ...) @printf(ptr @25, double %faddtmp)
  %40 = call ptr @moksha_box_string(ptr @26)
  %print_unbox17 = call ptr @moksha_unbox_string(ptr %40)
  %41 = call i32 (ptr, ...) @printf(ptr @27, ptr %print_unbox17)
  %42 = load double, ptr %x14, align 8
  %43 = load double, ptr %y15, align 8
  %fsubtmp = fsub double %42, %43
  %44 = call i32 (ptr, ...) @printf(ptr @28, double %fsubtmp)
  %45 = call ptr @moksha_box_string(ptr @29)
  %print_unbox18 = call ptr @moksha_unbox_string(ptr %45)
  %46 = call i32 (ptr, ...) @printf(ptr @30, ptr %print_unbox18)
  %47 = load double, ptr %x14, align 8
  %fmultmp = fmul double %47, 2.000000e+00
  %48 = call i32 (ptr, ...) @printf(ptr @31, double %fmultmp)
  %49 = call ptr @moksha_box_string(ptr @32)
  %print_unbox19 = call ptr @moksha_unbox_string(ptr %49)
  %50 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox19)
  %51 = load double, ptr %x14, align 8
  %52 = load double, ptr %y15, align 8
  %53 = fcmp oeq double %52, 0.000000e+00
  br i1 %53, label %math_error20, label %math_safe21

math_error20:                                     ; preds = %math_merge11
  %54 = call ptr @moksha_box_string(ptr @34)
  call void @moksha_throw_exception(ptr %54)
  store ptr %54, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge22

math_safe21:                                      ; preds = %math_merge11
  %fdivtmp = fdiv double %51, %52
  br label %math_merge22

math_merge22:                                     ; preds = %math_safe21, %math_error20
  %math_res23 = phi double [ 0.000000e+00, %math_error20 ], [ %fdivtmp, %math_safe21 ]
  %55 = call i32 (ptr, ...) @printf(ptr @35, double %math_res23)
  %56 = call ptr @moksha_box_string(ptr @36)
  %print_unbox24 = call ptr @moksha_unbox_string(ptr %56)
  %57 = call i32 (ptr, ...) @printf(ptr @37, ptr %print_unbox24)
  store i32 5, ptr %i, align 4
  store i32 5, ptr %i25, align 4
  store double 2.500000e+00, ptr %d, align 8
  store double 2.500000e+00, ptr %d26, align 8
  %58 = call ptr @moksha_box_string(ptr @38)
  %print_unbox27 = call ptr @moksha_unbox_string(ptr %58)
  %59 = call i32 (ptr, ...) @printf(ptr @39, ptr %print_unbox27)
  %60 = load i32, ptr %i25, align 4
  %61 = load double, ptr %d26, align 8
  %62 = sitofp i32 %60 to double
  %faddtmp28 = fadd double %62, %61
  %63 = call i32 (ptr, ...) @printf(ptr @40, double %faddtmp28)
  %64 = call ptr @moksha_box_string(ptr @41)
  %print_unbox29 = call ptr @moksha_unbox_string(ptr %64)
  %65 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox29)
  %66 = load double, ptr %d26, align 8
  %67 = load i32, ptr %i25, align 4
  %68 = sitofp i32 %67 to double
  %fsubtmp30 = fsub double %66, %68
  %69 = call i32 (ptr, ...) @printf(ptr @43, double %fsubtmp30)
  %70 = call ptr @moksha_box_string(ptr @44)
  %print_unbox31 = call ptr @moksha_unbox_string(ptr %70)
  %71 = call i32 (ptr, ...) @printf(ptr @45, ptr %print_unbox31)
  %72 = load i32, ptr %i25, align 4
  %73 = load double, ptr %d26, align 8
  %74 = sitofp i32 %72 to double
  %fmultmp32 = fmul double %74, %73
  %75 = call i32 (ptr, ...) @printf(ptr @46, double %fmultmp32)
  store double 1.000000e+01, ptr %promoted, align 8
  store double 1.000000e+01, ptr %promoted33, align 8
  %76 = call ptr @moksha_box_string(ptr @47)
  %print_unbox34 = call ptr @moksha_unbox_string(ptr %76)
  %77 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox34)
  %78 = load double, ptr %promoted33, align 8
  %79 = call i32 (ptr, ...) @printf(ptr @49, double %78)
  %80 = call ptr @moksha_box_string(ptr @50)
  %print_unbox35 = call ptr @moksha_unbox_string(ptr %80)
  %81 = call i32 (ptr, ...) @printf(ptr @51, ptr %print_unbox35)
  store i32 5, ptr %bitA, align 4
  store i32 5, ptr %bitA36, align 4
  store i32 3, ptr %bitB, align 4
  store i32 3, ptr %bitB37, align 4
  %82 = call ptr @moksha_box_string(ptr @52)
  %print_unbox38 = call ptr @moksha_unbox_string(ptr %82)
  %83 = call i32 (ptr, ...) @printf(ptr @53, ptr %print_unbox38)
  %84 = load i32, ptr %bitA36, align 4
  %85 = load i32, ptr %bitB37, align 4
  %andtmp = and i32 %84, %85
  %86 = call i32 (ptr, ...) @printf(ptr @54, i32 %andtmp)
  %87 = call ptr @moksha_box_string(ptr @55)
  %print_unbox39 = call ptr @moksha_unbox_string(ptr %87)
  %88 = call i32 (ptr, ...) @printf(ptr @56, ptr %print_unbox39)
  %89 = load i32, ptr %bitA36, align 4
  %90 = load i32, ptr %bitB37, align 4
  %ortmp = or i32 %89, %90
  %91 = call i32 (ptr, ...) @printf(ptr @57, i32 %ortmp)
  %92 = call ptr @moksha_box_string(ptr @58)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %92)
  %93 = call i32 (ptr, ...) @printf(ptr @59, ptr %print_unbox40)
  %94 = load i32, ptr %bitA36, align 4
  %95 = load i32, ptr %bitB37, align 4
  %xortmp = xor i32 %94, %95
  %96 = call i32 (ptr, ...) @printf(ptr @60, i32 %xortmp)
  %97 = call ptr @moksha_box_string(ptr @61)
  %print_unbox41 = call ptr @moksha_unbox_string(ptr %97)
  %98 = call i32 (ptr, ...) @printf(ptr @62, ptr %print_unbox41)
  %99 = load i32, ptr %a2, align 4
  %addtmp42 = add i32 %99, 2
  %100 = load i32, ptr %b3, align 4
  %multmp43 = mul i32 %addtmp42, %100
  %addtmp44 = add i32 %multmp43, 5
  store i32 %addtmp44, ptr %complex, align 4
  store i32 %addtmp44, ptr %complex45, align 4
  %101 = call ptr @moksha_box_string(ptr @63)
  %print_unbox46 = call ptr @moksha_unbox_string(ptr %101)
  %102 = call i32 (ptr, ...) @printf(ptr @64, ptr %print_unbox46)
  %103 = load i32, ptr %complex45, align 4
  %104 = call i32 (ptr, ...) @printf(ptr @65, i32 %103)
  %105 = call ptr @moksha_box_string(ptr @66)
  %print_unbox47 = call ptr @moksha_unbox_string(ptr %105)
  %106 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox47)
  %107 = call ptr @moksha_box_string(ptr @68)
  %print_unbox48 = call ptr @moksha_unbox_string(ptr %107)
  %108 = call i32 (ptr, ...) @printf(ptr @69, ptr %print_unbox48)
  %109 = call ptr @moksha_box_string(ptr @70)
  %print_unbox49 = call ptr @moksha_unbox_string(ptr %109)
  %110 = call i32 (ptr, ...) @printf(ptr @71, ptr %print_unbox49)
  %111 = call i32 (ptr, ...) @printf(ptr @72, i32 2)
  %112 = call ptr @moksha_box_string(ptr @73)
  %print_unbox50 = call ptr @moksha_unbox_string(ptr %112)
  %113 = call i32 (ptr, ...) @printf(ptr @74, ptr %print_unbox50)
  br i1 false, label %math_error51, label %math_safe52

math_error51:                                     ; preds = %math_merge22
  %114 = call ptr @moksha_box_string(ptr @75)
  call void @moksha_throw_exception(ptr %114)
  store ptr %114, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge53

math_safe52:                                      ; preds = %math_merge22
  br label %math_merge53

math_merge53:                                     ; preds = %math_safe52, %math_error51
  %math_res54 = phi double [ 0.000000e+00, %math_error51 ], [ 2.500000e+00, %math_safe52 ]
  %115 = call i32 (ptr, ...) @printf(ptr @76, double %math_res54)
  %116 = call ptr @moksha_box_string(ptr @77)
  %print_unbox55 = call ptr @moksha_unbox_string(ptr %116)
  %117 = call i32 (ptr, ...) @printf(ptr @78, ptr %print_unbox55)
  %118 = call i32 (ptr, ...) @printf(ptr @79, i32 2)
  %119 = call ptr @moksha_box_string(ptr @80)
  %print_unbox56 = call ptr @moksha_unbox_string(ptr %119)
  %120 = call i32 (ptr, ...) @printf(ptr @81, ptr %print_unbox56)
  br i1 false, label %math_error57, label %math_safe58

math_error57:                                     ; preds = %math_merge53
  %121 = call ptr @moksha_box_string(ptr @82)
  call void @moksha_throw_exception(ptr %121)
  store ptr %121, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge59

math_safe58:                                      ; preds = %math_merge53
  br label %math_merge59

math_merge59:                                     ; preds = %math_safe58, %math_error57
  %math_res60 = phi double [ 0.000000e+00, %math_error57 ], [ 2.500000e+00, %math_safe58 ]
  %cast_d2i = fptosi double %math_res60 to i32
  %122 = call i32 (ptr, ...) @printf(ptr @83, i32 %cast_d2i)
  %123 = call ptr @moksha_box_string(ptr @84)
  %print_unbox61 = call ptr @moksha_unbox_string(ptr %123)
  %124 = call i32 (ptr, ...) @printf(ptr @85, ptr %print_unbox61)
  store i32 2, ptr %base, align 4
  store i32 2, ptr %base62, align 4
  store i32 3, ptr %exp, align 4
  store i32 3, ptr %exp63, align 4
  %125 = call ptr @moksha_box_string(ptr @86)
  %print_unbox64 = call ptr @moksha_unbox_string(ptr %125)
  %126 = call i32 (ptr, ...) @printf(ptr @87, ptr %print_unbox64)
  %127 = load i32, ptr %base62, align 4
  %128 = load i32, ptr %exp63, align 4
  %129 = sitofp i32 %127 to double
  %130 = sitofp i32 %128 to double
  %powtmp = call double @llvm.pow.f64(double %129, double %130)
  %pow_int_cast = fptosi double %powtmp to i32
  %131 = call i32 (ptr, ...) @printf(ptr @88, i32 %pow_int_cast)
  %132 = call ptr @moksha_box_string(ptr @89)
  %print_unbox65 = call ptr @moksha_unbox_string(ptr %132)
  %133 = call i32 (ptr, ...) @printf(ptr @90, ptr %print_unbox65)
  store i32 8, ptr %val, align 4
  store i32 8, ptr %val66, align 4
  %134 = call ptr @moksha_box_string(ptr @91)
  %print_unbox67 = call ptr @moksha_unbox_string(ptr %134)
  %135 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox67)
  %136 = load i32, ptr %val66, align 4
  %shltmp = shl i32 %136, 1
  %137 = call i32 (ptr, ...) @printf(ptr @93, i32 %shltmp)
  %138 = call ptr @moksha_box_string(ptr @94)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %138)
  %139 = call i32 (ptr, ...) @printf(ptr @95, ptr %print_unbox68)
  %140 = load i32, ptr %val66, align 4
  %ashrtmp = ashr i32 %140, 2
  %141 = call i32 (ptr, ...) @printf(ptr @96, i32 %ashrtmp)
  store i32 0, ptr %bitNot, align 4
  store i32 0, ptr %bitNot69, align 4
  %142 = call ptr @moksha_box_string(ptr @97)
  %print_unbox70 = call ptr @moksha_unbox_string(ptr %142)
  %143 = call i32 (ptr, ...) @printf(ptr @98, ptr %print_unbox70)
  %144 = load i32, ptr %bitNot69, align 4
  %bitnot_tmp = xor i32 %144, -1
  %145 = call i32 (ptr, ...) @printf(ptr @99, i32 %bitnot_tmp)
  %146 = call ptr @moksha_box_string(ptr @100)
  %print_unbox71 = call ptr @moksha_unbox_string(ptr %146)
  %147 = call i32 (ptr, ...) @printf(ptr @101, ptr %print_unbox71)
  store i32 10, ptr %i2, align 4
  store i32 10, ptr %i272, align 4
  store double 2.500000e+00, ptr %d2, align 8
  store double 2.500000e+00, ptr %d273, align 8
  %148 = call ptr @moksha_box_string(ptr @102)
  %print_unbox74 = call ptr @moksha_unbox_string(ptr %148)
  %149 = call i32 (ptr, ...) @printf(ptr @103, ptr %print_unbox74)
  %150 = load i32, ptr %i272, align 4
  %151 = load double, ptr %d273, align 8
  %152 = sitofp i32 %150 to double
  %faddtmp75 = fadd double %152, %151
  %153 = call i32 (ptr, ...) @printf(ptr @104, double %faddtmp75)
  %154 = call ptr @moksha_box_string(ptr @105)
  %print_unbox76 = call ptr @moksha_unbox_string(ptr %154)
  %155 = call i32 (ptr, ...) @printf(ptr @106, ptr %print_unbox76)
  %156 = load i32, ptr %i272, align 4
  %157 = load double, ptr %d273, align 8
  %158 = fcmp oeq double %157, 0.000000e+00
  br i1 %158, label %math_error77, label %math_safe78

math_error77:                                     ; preds = %math_merge59
  %159 = call ptr @moksha_box_string(ptr @107)
  call void @moksha_throw_exception(ptr %159)
  store ptr %159, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge79

math_safe78:                                      ; preds = %math_merge59
  %160 = sitofp i32 %156 to double
  %fdivtmp80 = fdiv double %160, %157
  br label %math_merge79

math_merge79:                                     ; preds = %math_safe78, %math_error77
  %math_res81 = phi double [ 0.000000e+00, %math_error77 ], [ %fdivtmp80, %math_safe78 ]
  %cast_d2i82 = fptosi double %math_res81 to i32
  %161 = call i32 (ptr, ...) @printf(ptr @108, i32 %cast_d2i82)
  %162 = call ptr @moksha_box_string(ptr @109)
  %print_unbox83 = call ptr @moksha_unbox_string(ptr %162)
  %163 = call i32 (ptr, ...) @printf(ptr @110, ptr %print_unbox83)
  %164 = call ptr @moksha_box_string(ptr @111)
  %print_unbox84 = call ptr @moksha_unbox_string(ptr %164)
  %165 = call i32 (ptr, ...) @printf(ptr @112, ptr %print_unbox84)
  store i32 5, ptr %a285, align 4
  store i32 5, ptr %a286, align 4
  %166 = call ptr @moksha_box_string(ptr @113)
  %print_unbox87 = call ptr @moksha_unbox_string(ptr %166)
  %167 = call i32 (ptr, ...) @printf(ptr @114, ptr %print_unbox87)
  %168 = load i32, ptr %a286, align 4
  %169 = call i32 (ptr, ...) @printf(ptr @115, i32 %168)
  %170 = call ptr @moksha_box_string(ptr @116)
  %print_unbox88 = call ptr @moksha_unbox_string(ptr %170)
  %171 = call i32 (ptr, ...) @printf(ptr @117, ptr %print_unbox88)
  %oldval = load i32, ptr %a286, align 4
  %inc = add i32 %oldval, 1
  store i32 %inc, ptr %a286, align 4
  %172 = call i32 (ptr, ...) @printf(ptr @118, i32 %oldval)
  %173 = call ptr @moksha_box_string(ptr @119)
  %print_unbox89 = call ptr @moksha_unbox_string(ptr %173)
  %174 = call i32 (ptr, ...) @printf(ptr @120, ptr %print_unbox89)
  %175 = load i32, ptr %a286, align 4
  %176 = call i32 (ptr, ...) @printf(ptr @121, i32 %175)
  %177 = call ptr @moksha_box_string(ptr @122)
  %print_unbox90 = call ptr @moksha_unbox_string(ptr %177)
  %178 = call i32 (ptr, ...) @printf(ptr @123, ptr %print_unbox90)
  %oldval91 = load i32, ptr %a286, align 4
  %inc92 = add i32 %oldval91, 1
  store i32 %inc92, ptr %a286, align 4
  %179 = call i32 (ptr, ...) @printf(ptr @124, i32 %inc92)
  store i32 5, ptr %b2, align 4
  store i32 5, ptr %b293, align 4
  %180 = call ptr @moksha_box_string(ptr @125)
  %print_unbox94 = call ptr @moksha_unbox_string(ptr %180)
  %181 = call i32 (ptr, ...) @printf(ptr @126, ptr %print_unbox94)
  %oldval95 = load i32, ptr %b293, align 4
  %dec = sub i32 %oldval95, 1
  store i32 %dec, ptr %b293, align 4
  %182 = call i32 (ptr, ...) @printf(ptr @127, i32 %oldval95)
  %183 = call ptr @moksha_box_string(ptr @128)
  %print_unbox96 = call ptr @moksha_unbox_string(ptr %183)
  %184 = call i32 (ptr, ...) @printf(ptr @129, ptr %print_unbox96)
  %185 = load i32, ptr %b293, align 4
  %186 = call i32 (ptr, ...) @printf(ptr @130, i32 %185)
  %187 = call ptr @moksha_box_string(ptr @131)
  %print_unbox97 = call ptr @moksha_unbox_string(ptr %187)
  %188 = call i32 (ptr, ...) @printf(ptr @132, ptr %print_unbox97)
  %oldval98 = load i32, ptr %b293, align 4
  %dec99 = sub i32 %oldval98, 1
  store i32 %dec99, ptr %b293, align 4
  %189 = call i32 (ptr, ...) @printf(ptr @133, i32 %dec99)
  %190 = call ptr @moksha_box_string(ptr @134)
  %print_unbox100 = call ptr @moksha_unbox_string(ptr %190)
  %191 = call i32 (ptr, ...) @printf(ptr @135, ptr %print_unbox100)
  store double 2.500000e+00, ptr %d3, align 8
  store double 2.500000e+00, ptr %d3101, align 8
  %192 = call ptr @moksha_box_string(ptr @136)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %192)
  %193 = call i32 (ptr, ...) @printf(ptr @137, ptr %print_unbox102)
  %194 = load double, ptr %d3101, align 8
  %195 = call i32 (ptr, ...) @printf(ptr @138, double %194)
  %oldval103 = load double, ptr %d3101, align 8
  %finc = fadd double %oldval103, 1.000000e+00
  store double %finc, ptr %d3101, align 8
  %196 = call ptr @moksha_box_string(ptr @139)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %196)
  %197 = call i32 (ptr, ...) @printf(ptr @140, ptr %print_unbox104)
  %198 = load double, ptr %d3101, align 8
  %199 = call i32 (ptr, ...) @printf(ptr @141, double %198)
  %oldval105 = load double, ptr %d3101, align 8
  %fdec = fsub double %oldval105, 1.000000e+00
  store double %fdec, ptr %d3101, align 8
  %200 = call ptr @moksha_box_string(ptr @142)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %200)
  %201 = call i32 (ptr, ...) @printf(ptr @143, ptr %print_unbox106)
  %202 = load double, ptr %d3101, align 8
  %203 = call i32 (ptr, ...) @printf(ptr @144, double %202)
  %204 = call ptr @moksha_box_string(ptr @145)
  %print_unbox107 = call ptr @moksha_unbox_string(ptr %204)
  %205 = call i32 (ptr, ...) @printf(ptr @146, ptr %print_unbox107)
  store i32 10, ptr %x2, align 4
  store i32 10, ptr %x2108, align 4
  store i32 5, ptr %y2, align 4
  store i32 5, ptr %y2109, align 4
  %206 = load i32, ptr %x2108, align 4
  %oldval110 = load i32, ptr %y2109, align 4
  %inc111 = add i32 %oldval110, 1
  store i32 %inc111, ptr %y2109, align 4
  %multmp112 = mul i32 %206, %oldval110
  %oldval113 = load i32, ptr %x2108, align 4
  %inc114 = add i32 %oldval113, 1
  store i32 %inc114, ptr %x2108, align 4
  %addtmp115 = add i32 %multmp112, %inc114
  store i32 %addtmp115, ptr %res, align 4
  store i32 %addtmp115, ptr %res116, align 4
  %207 = call ptr @moksha_box_string(ptr @147)
  %print_unbox117 = call ptr @moksha_unbox_string(ptr %207)
  %208 = call i32 (ptr, ...) @printf(ptr @148, ptr %print_unbox117)
  %209 = load i32, ptr %res116, align 4
  %210 = call i32 (ptr, ...) @printf(ptr @149, i32 %209)
  %211 = call ptr @moksha_box_string(ptr @150)
  %print_unbox118 = call ptr @moksha_unbox_string(ptr %211)
  %212 = call i32 (ptr, ...) @printf(ptr @151, ptr %print_unbox118)
  %213 = load i32, ptr %x2108, align 4
  %214 = call i32 (ptr, ...) @printf(ptr @152, i32 %213)
  %215 = call ptr @moksha_box_string(ptr @153)
  %print_unbox119 = call ptr @moksha_unbox_string(ptr %215)
  %216 = call i32 (ptr, ...) @printf(ptr @154, ptr %print_unbox119)
  %217 = load i32, ptr %y2109, align 4
  %218 = call i32 (ptr, ...) @printf(ptr @155, i32 %217)
  %219 = call ptr @moksha_box_string(ptr @156)
  %print_unbox120 = call ptr @moksha_unbox_string(ptr %219)
  %220 = call i32 (ptr, ...) @printf(ptr @157, ptr %print_unbox120)
  %alloc_obj = call ptr @calloc(i64 1, i64 16)
  call void @Counter_constructor(ptr %alloc_obj)
  %221 = icmp eq ptr %alloc_obj, null
  br i1 %221, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %math_merge79
  %clone_alloc = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 %alloc_obj, i64 16, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %math_merge79
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %c, align 8
  store ptr %clone_res, ptr %c121, align 8
  %loaded_obj_ptr = load ptr, ptr %c121, align 8
  %222 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %222, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %clone_merge
  %223 = call ptr @moksha_box_string(ptr @158)
  store ptr %223, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe:                                         ; preds = %clone_merge
  %224 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr, i32 0, i32 1
  %oldval122 = load i32, ptr %224, align 4
  %inc123 = add i32 %oldval122, 1
  store i32 %inc123, ptr %224, align 4
  %225 = call ptr @moksha_box_string(ptr @159)
  %print_unbox124 = call ptr @moksha_unbox_string(ptr %225)
  %226 = call i32 (ptr, ...) @printf(ptr @160, ptr %print_unbox124)
  %loaded_obj_ptr125 = load ptr, ptr %c121, align 8
  %227 = icmp eq ptr %loaded_obj_ptr125, null
  br i1 %227, label %mem_error126, label %mem_safe127

mem_error126:                                     ; preds = %mem_safe
  %228 = call ptr @moksha_box_string(ptr @161)
  store ptr %228, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe127:                                      ; preds = %mem_safe
  %229 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr125, i32 0, i32 1
  %230 = load i32, ptr %229, align 4
  %231 = call i32 (ptr, ...) @printf(ptr @162, i32 %230)
  %loaded_obj_ptr128 = load ptr, ptr %c121, align 8
  %232 = icmp eq ptr %loaded_obj_ptr128, null
  br i1 %232, label %mem_error129, label %mem_safe130

mem_error129:                                     ; preds = %mem_safe127
  %233 = call ptr @moksha_box_string(ptr @163)
  store ptr %233, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe130:                                      ; preds = %mem_safe127
  %234 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr128, i32 0, i32 1
  %oldval131 = load i32, ptr %234, align 4
  %inc132 = add i32 %oldval131, 1
  store i32 %inc132, ptr %234, align 4
  %235 = call ptr @moksha_box_string(ptr @164)
  %print_unbox133 = call ptr @moksha_unbox_string(ptr %235)
  %236 = call i32 (ptr, ...) @printf(ptr @165, ptr %print_unbox133)
  %loaded_obj_ptr134 = load ptr, ptr %c121, align 8
  %237 = icmp eq ptr %loaded_obj_ptr134, null
  br i1 %237, label %mem_error135, label %mem_safe136

mem_error135:                                     ; preds = %mem_safe130
  %238 = call ptr @moksha_box_string(ptr @166)
  store ptr %238, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe136:                                      ; preds = %mem_safe130
  %239 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr134, i32 0, i32 1
  %240 = load i32, ptr %239, align 4
  %241 = call i32 (ptr, ...) @printf(ptr @167, i32 %240)
  ret i32 0
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.pow.f64(double, double) #0

define void @Counter_constructor(ptr %this) {
entry:
  %0 = getelementptr inbounds nuw %Counter, ptr %this, i32 0, i32 1
  store i32 0, ptr %0, align 4
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #1

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
