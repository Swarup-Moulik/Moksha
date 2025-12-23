; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Counter = type { ptr, i32 }

@__exception_flag = global i32 0
@__exception_val = global ptr null
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
@63 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@64 = private unnamed_addr constant [27 x i8] c"(10 + 2) * 3 + (20 / 4) = \00", align 1
@65 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@66 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@67 = private unnamed_addr constant [41 x i8] c"==== ADVANCED MATH & OPERATOR SUITE ====\00", align 1
@68 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@69 = private unnamed_addr constant [18 x i8] c"\0A[Division Types]\00", align 1
@70 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@71 = private unnamed_addr constant [19 x i8] c"5 / 2 (Standard): \00", align 1
@72 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@73 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@74 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@75 = private unnamed_addr constant [24 x i8] c"10.0 / 4.0 (Standard): \00", align 1
@76 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@77 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@78 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@79 = private unnamed_addr constant [23 x i8] c"int(5 / 2) (Integer): \00", align 1
@80 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@81 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@82 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@83 = private unnamed_addr constant [28 x i8] c"int(10.0 / 4.0) (Integer): \00", align 1
@84 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@85 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@86 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@87 = private unnamed_addr constant [18 x i8] c"\0A[Power Operator]\00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@89 = private unnamed_addr constant [10 x i8] c"2 ** 3 = \00", align 1
@90 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@91 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@92 = private unnamed_addr constant [25 x i8] c"\0A[Shift & Not Operators]\00", align 1
@93 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@94 = private unnamed_addr constant [23 x i8] c"8 << 1 (Left Shift) = \00", align 1
@95 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@96 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@97 = private unnamed_addr constant [24 x i8] c"8 >> 2 (Right Shift) = \00", align 1
@98 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@99 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@100 = private unnamed_addr constant [20 x i8] c"~0 (Bitwise Not) = \00", align 1
@101 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@102 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@103 = private unnamed_addr constant [20 x i8] c"\0A[Implicit Casting]\00", align 1
@104 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@105 = private unnamed_addr constant [12 x i8] c"10 + 2.5 = \00", align 1
@106 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@107 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@108 = private unnamed_addr constant [17 x i8] c"int(10 / 2.5) = \00", align 1
@109 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@110 = private unnamed_addr constant [38 x i8] c"ArithmeticException: Division by zero\00", align 1
@111 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@112 = private unnamed_addr constant [42 x i8] c"==== UNARY INCREMENT/DECREMENT SUITE ====\00", align 1
@113 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@114 = private unnamed_addr constant [26 x i8] c"\0A[Integer Prefix/Postfix]\00", align 1
@115 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@116 = private unnamed_addr constant [13 x i8] c"Initial a2: \00", align 1
@117 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@118 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@119 = private unnamed_addr constant [17 x i8] c"Postfix (a2++): \00", align 1
@120 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@121 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@122 = private unnamed_addr constant [16 x i8] c"After Postfix: \00", align 1
@123 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@124 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@125 = private unnamed_addr constant [16 x i8] c"Prefix (++a2): \00", align 1
@126 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@127 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@128 = private unnamed_addr constant [17 x i8] c"Postfix (b2--): \00", align 1
@129 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@130 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@131 = private unnamed_addr constant [16 x i8] c"After Postfix: \00", align 1
@132 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@133 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@134 = private unnamed_addr constant [16 x i8] c"Prefix (--b2): \00", align 1
@135 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@136 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@137 = private unnamed_addr constant [25 x i8] c"\0A[Double Prefix/Postfix]\00", align 1
@138 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@139 = private unnamed_addr constant [13 x i8] c"Initial d3: \00", align 1
@140 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@141 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@142 = private unnamed_addr constant [13 x i8] c"After d3++: \00", align 1
@143 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@144 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@145 = private unnamed_addr constant [13 x i8] c"After --d3: \00", align 1
@146 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@147 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@148 = private unnamed_addr constant [15 x i8] c"\0A[Expressions]\00", align 1
@149 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@150 = private unnamed_addr constant [9 x i8] c"Result: \00", align 1
@151 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@152 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@153 = private unnamed_addr constant [11 x i8] c"Final x2: \00", align 1
@154 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@155 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@156 = private unnamed_addr constant [11 x i8] c"Final y2: \00", align 1
@157 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@158 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@159 = private unnamed_addr constant [26 x i8] c"\0A[Object Field Increment]\00", align 1
@160 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@161 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@162 = private unnamed_addr constant [19 x i8] c"c.count after ++: \00", align 1
@163 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@164 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@165 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@166 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@167 = private unnamed_addr constant [26 x i8] c"c.count after prefix ++: \00", align 1
@168 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@169 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@170 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

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
  %c133 = alloca ptr, align 8
  %c = alloca ptr, align 8
  %res128 = alloca i32, align 4
  %res = alloca i32, align 4
  %y2121 = alloca i32, align 4
  %y2 = alloca i32, align 4
  %x2120 = alloca i32, align 4
  %x2 = alloca i32, align 4
  %d3113 = alloca double, align 8
  %d3 = alloca double, align 8
  %b2105 = alloca i32, align 4
  %b2 = alloca i32, align 4
  %a298 = alloca i32, align 4
  %a297 = alloca i32, align 4
  %d285 = alloca double, align 8
  %d2 = alloca double, align 8
  %i284 = alloca i32, align 4
  %i2 = alloca i32, align 4
  %bitNot81 = alloca i32, align 4
  %bitNot = alloca i32, align 4
  %val78 = alloca i32, align 4
  %val = alloca i32, align 4
  %exp75 = alloca i32, align 4
  %exp = alloca i32, align 4
  %base74 = alloca i32, align 4
  %base = alloca i32, align 4
  %complex49 = alloca i32, align 4
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
  br i1 false, label %math_error44, label %math_safe45

math_error44:                                     ; preds = %math_merge22
  %101 = call ptr @moksha_box_string(ptr @63)
  call void @moksha_throw_exception(ptr %101)
  store ptr %101, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge46

math_safe45:                                      ; preds = %math_merge22
  br label %math_merge46

math_merge46:                                     ; preds = %math_safe45, %math_error44
  %math_res47 = phi i32 [ 0, %math_error44 ], [ 5, %math_safe45 ]
  %addtmp48 = add i32 %multmp43, %math_res47
  store i32 %addtmp48, ptr %complex, align 4
  store i32 %addtmp48, ptr %complex49, align 4
  %102 = call ptr @moksha_box_string(ptr @64)
  %print_unbox50 = call ptr @moksha_unbox_string(ptr %102)
  %103 = call i32 (ptr, ...) @printf(ptr @65, ptr %print_unbox50)
  %104 = load i32, ptr %complex49, align 4
  %105 = call i32 (ptr, ...) @printf(ptr @66, i32 %104)
  %106 = call ptr @moksha_box_string(ptr @67)
  %print_unbox51 = call ptr @moksha_unbox_string(ptr %106)
  %107 = call i32 (ptr, ...) @printf(ptr @68, ptr %print_unbox51)
  %108 = call ptr @moksha_box_string(ptr @69)
  %print_unbox52 = call ptr @moksha_unbox_string(ptr %108)
  %109 = call i32 (ptr, ...) @printf(ptr @70, ptr %print_unbox52)
  %110 = call ptr @moksha_box_string(ptr @71)
  %print_unbox53 = call ptr @moksha_unbox_string(ptr %110)
  %111 = call i32 (ptr, ...) @printf(ptr @72, ptr %print_unbox53)
  br i1 false, label %math_error54, label %math_safe55

math_error54:                                     ; preds = %math_merge46
  %112 = call ptr @moksha_box_string(ptr @73)
  call void @moksha_throw_exception(ptr %112)
  store ptr %112, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge56

math_safe55:                                      ; preds = %math_merge46
  br label %math_merge56

math_merge56:                                     ; preds = %math_safe55, %math_error54
  %math_res57 = phi i32 [ 0, %math_error54 ], [ 2, %math_safe55 ]
  %113 = call i32 (ptr, ...) @printf(ptr @74, i32 %math_res57)
  %114 = call ptr @moksha_box_string(ptr @75)
  %print_unbox58 = call ptr @moksha_unbox_string(ptr %114)
  %115 = call i32 (ptr, ...) @printf(ptr @76, ptr %print_unbox58)
  br i1 false, label %math_error59, label %math_safe60

math_error59:                                     ; preds = %math_merge56
  %116 = call ptr @moksha_box_string(ptr @77)
  call void @moksha_throw_exception(ptr %116)
  store ptr %116, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge61

math_safe60:                                      ; preds = %math_merge56
  br label %math_merge61

math_merge61:                                     ; preds = %math_safe60, %math_error59
  %math_res62 = phi double [ 0.000000e+00, %math_error59 ], [ 2.500000e+00, %math_safe60 ]
  %117 = call i32 (ptr, ...) @printf(ptr @78, double %math_res62)
  %118 = call ptr @moksha_box_string(ptr @79)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %118)
  %119 = call i32 (ptr, ...) @printf(ptr @80, ptr %print_unbox63)
  br i1 false, label %math_error64, label %math_safe65

math_error64:                                     ; preds = %math_merge61
  %120 = call ptr @moksha_box_string(ptr @81)
  call void @moksha_throw_exception(ptr %120)
  store ptr %120, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge66

math_safe65:                                      ; preds = %math_merge61
  br label %math_merge66

math_merge66:                                     ; preds = %math_safe65, %math_error64
  %math_res67 = phi i32 [ 0, %math_error64 ], [ 2, %math_safe65 ]
  %121 = call i32 (ptr, ...) @printf(ptr @82, i32 %math_res67)
  %122 = call ptr @moksha_box_string(ptr @83)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %122)
  %123 = call i32 (ptr, ...) @printf(ptr @84, ptr %print_unbox68)
  br i1 false, label %math_error69, label %math_safe70

math_error69:                                     ; preds = %math_merge66
  %124 = call ptr @moksha_box_string(ptr @85)
  call void @moksha_throw_exception(ptr %124)
  store ptr %124, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge71

math_safe70:                                      ; preds = %math_merge66
  br label %math_merge71

math_merge71:                                     ; preds = %math_safe70, %math_error69
  %math_res72 = phi double [ 0.000000e+00, %math_error69 ], [ 2.500000e+00, %math_safe70 ]
  %cast_d2i = fptosi double %math_res72 to i32
  %125 = call i32 (ptr, ...) @printf(ptr @86, i32 %cast_d2i)
  %126 = call ptr @moksha_box_string(ptr @87)
  %print_unbox73 = call ptr @moksha_unbox_string(ptr %126)
  %127 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox73)
  store i32 2, ptr %base, align 4
  store i32 2, ptr %base74, align 4
  store i32 3, ptr %exp, align 4
  store i32 3, ptr %exp75, align 4
  %128 = call ptr @moksha_box_string(ptr @89)
  %print_unbox76 = call ptr @moksha_unbox_string(ptr %128)
  %129 = call i32 (ptr, ...) @printf(ptr @90, ptr %print_unbox76)
  %130 = load i32, ptr %base74, align 4
  %131 = load i32, ptr %exp75, align 4
  %132 = sitofp i32 %130 to double
  %133 = sitofp i32 %131 to double
  %powtmp = call double @llvm.pow.f64(double %132, double %133)
  %pow_int_cast = fptosi double %powtmp to i32
  %134 = call i32 (ptr, ...) @printf(ptr @91, i32 %pow_int_cast)
  %135 = call ptr @moksha_box_string(ptr @92)
  %print_unbox77 = call ptr @moksha_unbox_string(ptr %135)
  %136 = call i32 (ptr, ...) @printf(ptr @93, ptr %print_unbox77)
  store i32 8, ptr %val, align 4
  store i32 8, ptr %val78, align 4
  %137 = call ptr @moksha_box_string(ptr @94)
  %print_unbox79 = call ptr @moksha_unbox_string(ptr %137)
  %138 = call i32 (ptr, ...) @printf(ptr @95, ptr %print_unbox79)
  %139 = load i32, ptr %val78, align 4
  %shltmp = shl i32 %139, 1
  %140 = call i32 (ptr, ...) @printf(ptr @96, i32 %shltmp)
  %141 = call ptr @moksha_box_string(ptr @97)
  %print_unbox80 = call ptr @moksha_unbox_string(ptr %141)
  %142 = call i32 (ptr, ...) @printf(ptr @98, ptr %print_unbox80)
  %143 = load i32, ptr %val78, align 4
  %ashrtmp = ashr i32 %143, 2
  %144 = call i32 (ptr, ...) @printf(ptr @99, i32 %ashrtmp)
  store i32 0, ptr %bitNot, align 4
  store i32 0, ptr %bitNot81, align 4
  %145 = call ptr @moksha_box_string(ptr @100)
  %print_unbox82 = call ptr @moksha_unbox_string(ptr %145)
  %146 = call i32 (ptr, ...) @printf(ptr @101, ptr %print_unbox82)
  %147 = load i32, ptr %bitNot81, align 4
  %bitnot_tmp = xor i32 %147, -1
  %148 = call i32 (ptr, ...) @printf(ptr @102, i32 %bitnot_tmp)
  %149 = call ptr @moksha_box_string(ptr @103)
  %print_unbox83 = call ptr @moksha_unbox_string(ptr %149)
  %150 = call i32 (ptr, ...) @printf(ptr @104, ptr %print_unbox83)
  store i32 10, ptr %i2, align 4
  store i32 10, ptr %i284, align 4
  store double 2.500000e+00, ptr %d2, align 8
  store double 2.500000e+00, ptr %d285, align 8
  %151 = call ptr @moksha_box_string(ptr @105)
  %print_unbox86 = call ptr @moksha_unbox_string(ptr %151)
  %152 = call i32 (ptr, ...) @printf(ptr @106, ptr %print_unbox86)
  %153 = load i32, ptr %i284, align 4
  %154 = load double, ptr %d285, align 8
  %155 = sitofp i32 %153 to double
  %faddtmp87 = fadd double %155, %154
  %156 = call i32 (ptr, ...) @printf(ptr @107, double %faddtmp87)
  %157 = call ptr @moksha_box_string(ptr @108)
  %print_unbox88 = call ptr @moksha_unbox_string(ptr %157)
  %158 = call i32 (ptr, ...) @printf(ptr @109, ptr %print_unbox88)
  %159 = load i32, ptr %i284, align 4
  %160 = load double, ptr %d285, align 8
  %161 = fcmp oeq double %160, 0.000000e+00
  br i1 %161, label %math_error89, label %math_safe90

math_error89:                                     ; preds = %math_merge71
  %162 = call ptr @moksha_box_string(ptr @110)
  call void @moksha_throw_exception(ptr %162)
  store ptr %162, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  br label %math_merge91

math_safe90:                                      ; preds = %math_merge71
  %163 = sitofp i32 %159 to double
  %fdivtmp92 = fdiv double %163, %160
  br label %math_merge91

math_merge91:                                     ; preds = %math_safe90, %math_error89
  %math_res93 = phi double [ 0.000000e+00, %math_error89 ], [ %fdivtmp92, %math_safe90 ]
  %cast_d2i94 = fptosi double %math_res93 to i32
  %164 = call i32 (ptr, ...) @printf(ptr @111, i32 %cast_d2i94)
  %165 = call ptr @moksha_box_string(ptr @112)
  %print_unbox95 = call ptr @moksha_unbox_string(ptr %165)
  %166 = call i32 (ptr, ...) @printf(ptr @113, ptr %print_unbox95)
  %167 = call ptr @moksha_box_string(ptr @114)
  %print_unbox96 = call ptr @moksha_unbox_string(ptr %167)
  %168 = call i32 (ptr, ...) @printf(ptr @115, ptr %print_unbox96)
  store i32 5, ptr %a297, align 4
  store i32 5, ptr %a298, align 4
  %169 = call ptr @moksha_box_string(ptr @116)
  %print_unbox99 = call ptr @moksha_unbox_string(ptr %169)
  %170 = call i32 (ptr, ...) @printf(ptr @117, ptr %print_unbox99)
  %171 = load i32, ptr %a298, align 4
  %172 = call i32 (ptr, ...) @printf(ptr @118, i32 %171)
  %173 = call ptr @moksha_box_string(ptr @119)
  %print_unbox100 = call ptr @moksha_unbox_string(ptr %173)
  %174 = call i32 (ptr, ...) @printf(ptr @120, ptr %print_unbox100)
  %oldval = load i32, ptr %a298, align 4
  %inc = add i32 %oldval, 1
  store i32 %inc, ptr %a298, align 4
  %175 = call i32 (ptr, ...) @printf(ptr @121, i32 %oldval)
  %176 = call ptr @moksha_box_string(ptr @122)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %176)
  %177 = call i32 (ptr, ...) @printf(ptr @123, ptr %print_unbox101)
  %178 = load i32, ptr %a298, align 4
  %179 = call i32 (ptr, ...) @printf(ptr @124, i32 %178)
  %180 = call ptr @moksha_box_string(ptr @125)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %180)
  %181 = call i32 (ptr, ...) @printf(ptr @126, ptr %print_unbox102)
  %oldval103 = load i32, ptr %a298, align 4
  %inc104 = add i32 %oldval103, 1
  store i32 %inc104, ptr %a298, align 4
  %182 = call i32 (ptr, ...) @printf(ptr @127, i32 %inc104)
  store i32 5, ptr %b2, align 4
  store i32 5, ptr %b2105, align 4
  %183 = call ptr @moksha_box_string(ptr @128)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %183)
  %184 = call i32 (ptr, ...) @printf(ptr @129, ptr %print_unbox106)
  %oldval107 = load i32, ptr %b2105, align 4
  %dec = sub i32 %oldval107, 1
  store i32 %dec, ptr %b2105, align 4
  %185 = call i32 (ptr, ...) @printf(ptr @130, i32 %oldval107)
  %186 = call ptr @moksha_box_string(ptr @131)
  %print_unbox108 = call ptr @moksha_unbox_string(ptr %186)
  %187 = call i32 (ptr, ...) @printf(ptr @132, ptr %print_unbox108)
  %188 = load i32, ptr %b2105, align 4
  %189 = call i32 (ptr, ...) @printf(ptr @133, i32 %188)
  %190 = call ptr @moksha_box_string(ptr @134)
  %print_unbox109 = call ptr @moksha_unbox_string(ptr %190)
  %191 = call i32 (ptr, ...) @printf(ptr @135, ptr %print_unbox109)
  %oldval110 = load i32, ptr %b2105, align 4
  %dec111 = sub i32 %oldval110, 1
  store i32 %dec111, ptr %b2105, align 4
  %192 = call i32 (ptr, ...) @printf(ptr @136, i32 %dec111)
  %193 = call ptr @moksha_box_string(ptr @137)
  %print_unbox112 = call ptr @moksha_unbox_string(ptr %193)
  %194 = call i32 (ptr, ...) @printf(ptr @138, ptr %print_unbox112)
  store double 2.500000e+00, ptr %d3, align 8
  store double 2.500000e+00, ptr %d3113, align 8
  %195 = call ptr @moksha_box_string(ptr @139)
  %print_unbox114 = call ptr @moksha_unbox_string(ptr %195)
  %196 = call i32 (ptr, ...) @printf(ptr @140, ptr %print_unbox114)
  %197 = load double, ptr %d3113, align 8
  %198 = call i32 (ptr, ...) @printf(ptr @141, double %197)
  %oldval115 = load double, ptr %d3113, align 8
  %finc = fadd double %oldval115, 1.000000e+00
  store double %finc, ptr %d3113, align 8
  %199 = call ptr @moksha_box_string(ptr @142)
  %print_unbox116 = call ptr @moksha_unbox_string(ptr %199)
  %200 = call i32 (ptr, ...) @printf(ptr @143, ptr %print_unbox116)
  %201 = load double, ptr %d3113, align 8
  %202 = call i32 (ptr, ...) @printf(ptr @144, double %201)
  %oldval117 = load double, ptr %d3113, align 8
  %fdec = fsub double %oldval117, 1.000000e+00
  store double %fdec, ptr %d3113, align 8
  %203 = call ptr @moksha_box_string(ptr @145)
  %print_unbox118 = call ptr @moksha_unbox_string(ptr %203)
  %204 = call i32 (ptr, ...) @printf(ptr @146, ptr %print_unbox118)
  %205 = load double, ptr %d3113, align 8
  %206 = call i32 (ptr, ...) @printf(ptr @147, double %205)
  %207 = call ptr @moksha_box_string(ptr @148)
  %print_unbox119 = call ptr @moksha_unbox_string(ptr %207)
  %208 = call i32 (ptr, ...) @printf(ptr @149, ptr %print_unbox119)
  store i32 10, ptr %x2, align 4
  store i32 10, ptr %x2120, align 4
  store i32 5, ptr %y2, align 4
  store i32 5, ptr %y2121, align 4
  %209 = load i32, ptr %x2120, align 4
  %oldval122 = load i32, ptr %y2121, align 4
  %inc123 = add i32 %oldval122, 1
  store i32 %inc123, ptr %y2121, align 4
  %multmp124 = mul i32 %209, %oldval122
  %oldval125 = load i32, ptr %x2120, align 4
  %inc126 = add i32 %oldval125, 1
  store i32 %inc126, ptr %x2120, align 4
  %addtmp127 = add i32 %multmp124, %inc126
  store i32 %addtmp127, ptr %res, align 4
  store i32 %addtmp127, ptr %res128, align 4
  %210 = call ptr @moksha_box_string(ptr @150)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %210)
  %211 = call i32 (ptr, ...) @printf(ptr @151, ptr %print_unbox129)
  %212 = load i32, ptr %res128, align 4
  %213 = call i32 (ptr, ...) @printf(ptr @152, i32 %212)
  %214 = call ptr @moksha_box_string(ptr @153)
  %print_unbox130 = call ptr @moksha_unbox_string(ptr %214)
  %215 = call i32 (ptr, ...) @printf(ptr @154, ptr %print_unbox130)
  %216 = load i32, ptr %x2120, align 4
  %217 = call i32 (ptr, ...) @printf(ptr @155, i32 %216)
  %218 = call ptr @moksha_box_string(ptr @156)
  %print_unbox131 = call ptr @moksha_unbox_string(ptr %218)
  %219 = call i32 (ptr, ...) @printf(ptr @157, ptr %print_unbox131)
  %220 = load i32, ptr %y2121, align 4
  %221 = call i32 (ptr, ...) @printf(ptr @158, i32 %220)
  %222 = call ptr @moksha_box_string(ptr @159)
  %print_unbox132 = call ptr @moksha_unbox_string(ptr %222)
  %223 = call i32 (ptr, ...) @printf(ptr @160, ptr %print_unbox132)
  %alloc_obj = call ptr @calloc(i64 1, i64 16)
  call void @Counter_constructor(ptr %alloc_obj)
  %224 = icmp eq ptr %alloc_obj, null
  br i1 %224, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %math_merge91
  %clone_alloc = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 %alloc_obj, i64 16, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %math_merge91
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %c, align 8
  store ptr %clone_res, ptr %c133, align 8
  %loaded_obj_ptr = load ptr, ptr %c133, align 8
  %225 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %225, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %clone_merge
  %226 = call ptr @moksha_box_string(ptr @161)
  store ptr %226, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe:                                         ; preds = %clone_merge
  %227 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr, i32 0, i32 1
  %oldval134 = load i32, ptr %227, align 4
  %inc135 = add i32 %oldval134, 1
  store i32 %inc135, ptr %227, align 4
  %228 = call ptr @moksha_box_string(ptr @162)
  %print_unbox136 = call ptr @moksha_unbox_string(ptr %228)
  %229 = call i32 (ptr, ...) @printf(ptr @163, ptr %print_unbox136)
  %loaded_obj_ptr137 = load ptr, ptr %c133, align 8
  %230 = icmp eq ptr %loaded_obj_ptr137, null
  br i1 %230, label %mem_error138, label %mem_safe139

mem_error138:                                     ; preds = %mem_safe
  %231 = call ptr @moksha_box_string(ptr @164)
  store ptr %231, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe139:                                      ; preds = %mem_safe
  %232 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr137, i32 0, i32 1
  %233 = load i32, ptr %232, align 4
  %234 = call i32 (ptr, ...) @printf(ptr @165, i32 %233)
  %loaded_obj_ptr140 = load ptr, ptr %c133, align 8
  %235 = icmp eq ptr %loaded_obj_ptr140, null
  br i1 %235, label %mem_error141, label %mem_safe142

mem_error141:                                     ; preds = %mem_safe139
  %236 = call ptr @moksha_box_string(ptr @166)
  store ptr %236, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe142:                                      ; preds = %mem_safe139
  %237 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr140, i32 0, i32 1
  %oldval143 = load i32, ptr %237, align 4
  %inc144 = add i32 %oldval143, 1
  store i32 %inc144, ptr %237, align 4
  %238 = call ptr @moksha_box_string(ptr @167)
  %print_unbox145 = call ptr @moksha_unbox_string(ptr %238)
  %239 = call i32 (ptr, ...) @printf(ptr @168, ptr %print_unbox145)
  %loaded_obj_ptr146 = load ptr, ptr %c133, align 8
  %240 = icmp eq ptr %loaded_obj_ptr146, null
  br i1 %240, label %mem_error147, label %mem_safe148

mem_error147:                                     ; preds = %mem_safe142
  %241 = call ptr @moksha_box_string(ptr @169)
  store ptr %241, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe148:                                      ; preds = %mem_safe142
  %242 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr146, i32 0, i32 1
  %243 = load i32, ptr %242, align 4
  %244 = call i32 (ptr, ...) @printf(ptr @170, i32 %243)
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
