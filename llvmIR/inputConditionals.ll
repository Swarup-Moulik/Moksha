; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = external global i32
@__exception_val = external global ptr
@0 = private unnamed_addr constant [37 x i8] c"==== CONTROL FLOW & LOOPS SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [19 x i8] c"\0A[Input & If/Else]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [19 x i8] c"Enter an integer: \00", align 1
@5 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@6 = private unnamed_addr constant [17 x i8] c"Enter a double: \00", align 1
@7 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@8 = private unnamed_addr constant [18 x i8] c"Enter a boolean: \00", align 1
@9 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@10 = private unnamed_addr constant [17 x i8] c"Enter a string: \00", align 1
@11 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@12 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@13 = private unnamed_addr constant [15 x i8] c"Inputs: Int = \00", align 1
@14 = private unnamed_addr constant [12 x i8] c", Double = \00", align 1
@15 = private unnamed_addr constant [10 x i8] c", Bool = \00", align 1
@16 = private unnamed_addr constant [13 x i8] c", String = '\00", align 1
@17 = private unnamed_addr constant [2 x i8] c"'\00", align 1
@18 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@19 = private unnamed_addr constant [21 x i8] c"Input is huge (>100)\00", align 1
@20 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@21 = private unnamed_addr constant [18 x i8] c"Input is positive\00", align 1
@22 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@23 = private unnamed_addr constant [26 x i8] c"Input is zero or negative\00", align 1
@24 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@25 = private unnamed_addr constant [16 x i8] c"\0A[Switch Cases]\00", align 1
@26 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@27 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@28 = private unnamed_addr constant [7 x i8] c"Score \00", align 1
@29 = private unnamed_addr constant [3 x i8] c": \00", align 1
@30 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@31 = private unnamed_addr constant [23 x i8] c"Grade A (Range 90-100)\00", align 1
@32 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@33 = private unnamed_addr constant [22 x i8] c"Grade B (Range 80-89)\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@35 = private unnamed_addr constant [12 x i8] c"Other Grade\00", align 1
@36 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@37 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@38 = private unnamed_addr constant [6 x i8] c"Temp \00", align 1
@39 = private unnamed_addr constant [3 x i8] c": \00", align 1
@40 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@41 = private unnamed_addr constant [19 x i8] c"Warm (25.5 - 35.5)\00", align 1
@42 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@43 = private unnamed_addr constant [13 x i8] c"Out of range\00", align 1
@44 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@45 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@46 = private unnamed_addr constant [11 x i8] c"Complex x=\00", align 1
@47 = private unnamed_addr constant [3 x i8] c": \00", align 1
@48 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@49 = private unnamed_addr constant [19 x i8] c"Between 20 and 100\00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@51 = private unnamed_addr constant [13 x i8] c"Large (>100)\00", align 1
@52 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@53 = private unnamed_addr constant [12 x i8] c"Small (<20)\00", align 1
@54 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@55 = private unnamed_addr constant [2 x i8] c"b\00", align 1
@56 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@57 = private unnamed_addr constant [7 x i8] c"Given \00", align 1
@58 = private unnamed_addr constant [12 x i8] c" between : \00", align 1
@59 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@60 = private unnamed_addr constant [2 x i8] c"a\00", align 1
@61 = private unnamed_addr constant [2 x i8] c"d\00", align 1
@62 = private unnamed_addr constant [17 x i8] c"Between a and d \00", align 1
@63 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@64 = private unnamed_addr constant [2 x i8] c"A\00", align 1
@65 = private unnamed_addr constant [2 x i8] c"D\00", align 1
@66 = private unnamed_addr constant [17 x i8] c"Between A and D \00", align 1
@67 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@68 = private unnamed_addr constant [11 x i8] c"Wrong Case\00", align 1
@69 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@70 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@71 = private unnamed_addr constant [14 x i8] c"Testing Rank \00", align 1
@72 = private unnamed_addr constant [3 x i8] c": \00", align 1
@73 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@74 = private unnamed_addr constant [22 x i8] c"Top Tier (1, 2, or 3)\00", align 1
@75 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@76 = private unnamed_addr constant [16 x i8] c"Middle Tier (4)\00", align 1
@77 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@78 = private unnamed_addr constant [11 x i8] c"Lower Tier\00", align 1
@79 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@80 = private unnamed_addr constant [9 x i8] c"\0A[Loops]\00", align 1
@81 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@82 = private unnamed_addr constant [34 x i8] c"--- While (Count down 3 to 1) ---\00", align 1
@83 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@84 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@85 = private unnamed_addr constant [5 x i8] c"w = \00", align 1
@86 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@87 = private unnamed_addr constant [28 x i8] c"--- Do-While (Run once) ---\00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@89 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@90 = private unnamed_addr constant [5 x i8] c"d = \00", align 1
@91 = private unnamed_addr constant [15 x i8] c" (Inside loop)\00", align 1
@92 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@93 = private unnamed_addr constant [34 x i8] c"--- For Loop (0 to 4, Step 2) ---\00", align 1
@94 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@95 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@96 = private unnamed_addr constant [5 x i8] c"i = \00", align 1
@97 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@98 = private unnamed_addr constant [28 x i8] c"--- For-In Loop (Array) ---\00", align 1
@99 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@100 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@101 = private unnamed_addr constant [7 x i8] c"Index \00", align 1
@102 = private unnamed_addr constant [10 x i8] c" = Value \00", align 1
@103 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@104 = private unnamed_addr constant [6 x i8] c"Done.\00", align 1
@105 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %val = alloca i32, align 4
  %idx = alloca i32, align 4
  %forin_idx = alloca i32, align 4
  %numbers140 = alloca ptr, align 8
  %numbers = alloca ptr, align 8
  %i130 = alloca i32, align 4
  %i = alloca i32, align 4
  %d119 = alloca i32, align 4
  %d = alloca i32, align 4
  %w109 = alloca i32, align 4
  %w = alloca i32, align 4
  %rank88 = alloca i32, align 4
  %rank = alloca i32, align 4
  %ch71 = alloca ptr, align 8
  %ch = alloca ptr, align 8
  %x50 = alloca i32, align 4
  %x = alloca i32, align 4
  %temp39 = alloca double, align 8
  %temp = alloca double, align 8
  %score25 = alloca i32, align 4
  %score = alloca i32, align 4
  %sVal9 = alloca ptr, align 8
  %sVal = alloca ptr, align 8
  %bVal7 = alloca i1, align 1
  %bVal = alloca i1, align 1
  %dVal5 = alloca double, align 8
  %dVal = alloca double, align 8
  %iVal3 = alloca i32, align 4
  %iVal = alloca i32, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  %4 = call ptr @moksha_box_string(ptr @4)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %4)
  %5 = call i32 (ptr, ...) @printf(ptr @5, ptr %print_unbox2)
  %6 = call i32 @moksha_read_int()
  store i32 %6, ptr %iVal, align 4
  store i32 %6, ptr %iVal3, align 4
  %7 = call ptr @moksha_box_string(ptr @6)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox4)
  %9 = call double @moksha_read_double()
  store double %9, ptr %dVal, align 8
  store double %9, ptr %dVal5, align 8
  %10 = call ptr @moksha_box_string(ptr @8)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %10)
  %11 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox6)
  %12 = call i32 @moksha_read_bool()
  %bool_trunc = trunc i32 %12 to i1
  store i1 %bool_trunc, ptr %bVal, align 1
  store i1 %bool_trunc, ptr %bVal7, align 1
  %13 = call ptr @moksha_box_string(ptr @10)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %13)
  %14 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox8)
  %15 = call ptr @moksha_read_string()
  store ptr %15, ptr %sVal, align 8
  store ptr %15, ptr %sVal9, align 8
  %16 = call ptr @moksha_box_string(ptr @12)
  %17 = call ptr @moksha_box_string(ptr @13)
  %18 = call ptr @moksha_string_concat(ptr %16, ptr %17)
  %19 = load i32, ptr %iVal3, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %19)
  %box_s = call ptr @moksha_box_string(ptr %i2s)
  %20 = call ptr @moksha_string_concat(ptr %18, ptr %box_s)
  %21 = call ptr @moksha_box_string(ptr @14)
  %22 = call ptr @moksha_string_concat(ptr %20, ptr %21)
  %23 = load double, ptr %dVal5, align 8
  %d2s = call ptr @moksha_double_to_str(double %23)
  %box_s10 = call ptr @moksha_box_string(ptr %d2s)
  %24 = call ptr @moksha_string_concat(ptr %22, ptr %box_s10)
  %25 = call ptr @moksha_box_string(ptr @15)
  %26 = call ptr @moksha_string_concat(ptr %24, ptr %25)
  %27 = load i1, ptr %bVal7, align 1
  %bool_ext = zext i1 %27 to i32
  %box_b = call ptr @moksha_box_bool(i32 %bool_ext)
  %b2s = call ptr @moksha_any_to_str(ptr %box_b)
  %box_s11 = call ptr @moksha_box_string(ptr %b2s)
  %28 = call ptr @moksha_string_concat(ptr %26, ptr %box_s11)
  %29 = call ptr @moksha_box_string(ptr @16)
  %30 = call ptr @moksha_string_concat(ptr %28, ptr %29)
  %31 = load ptr, ptr %sVal9, align 8
  %32 = call ptr @moksha_string_concat(ptr %30, ptr %31)
  %33 = call ptr @moksha_box_string(ptr @17)
  %34 = call ptr @moksha_string_concat(ptr %32, ptr %33)
  %print_unbox12 = call ptr @moksha_unbox_string(ptr %34)
  %35 = call i32 (ptr, ...) @printf(ptr @18, ptr %print_unbox12)
  %36 = load i32, ptr %iVal3, align 4
  %icmptmp = icmp sgt i32 %36, 100
  br i1 %icmptmp, label %then, label %else

then:                                             ; preds = %entry
  %37 = call ptr @moksha_box_string(ptr @19)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %37)
  %38 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox13)
  %39 = load i32, ptr @__exception_flag, align 4
  %40 = icmp ne i32 %39, 0
  br i1 %40, label %unwind, label %next_stmt

else:                                             ; preds = %entry
  %41 = load i32, ptr %iVal3, align 4
  %icmptmp14 = icmp sgt i32 %41, 0
  br i1 %icmptmp14, label %then15, label %else16

ifcont:                                           ; preds = %ifcont17, %next_stmt
  %42 = call ptr @moksha_box_string(ptr @25)
  %print_unbox24 = call ptr @moksha_unbox_string(ptr %42)
  %43 = call i32 (ptr, ...) @printf(ptr @26, ptr %print_unbox24)
  store i32 95, ptr %score, align 4
  store i32 95, ptr %score25, align 4
  %44 = call ptr @moksha_box_string(ptr @27)
  %45 = call ptr @moksha_box_string(ptr @28)
  %46 = call ptr @moksha_string_concat(ptr %44, ptr %45)
  %47 = load i32, ptr %score25, align 4
  %i2s26 = call ptr @moksha_int_to_str(i32 %47)
  %box_s27 = call ptr @moksha_box_string(ptr %i2s26)
  %48 = call ptr @moksha_string_concat(ptr %46, ptr %box_s27)
  %49 = call ptr @moksha_box_string(ptr @29)
  %50 = call ptr @moksha_string_concat(ptr %48, ptr %49)
  %print_unbox28 = call ptr @moksha_unbox_string(ptr %50)
  %51 = call i32 (ptr, ...) @printf(ptr @30, ptr %print_unbox28)
  %52 = load i32, ptr %score25, align 4
  switch i32 %52, label %default [
    i32 90, label %case
    i32 91, label %case
    i32 92, label %case
    i32 93, label %case
    i32 94, label %case
    i32 95, label %case
    i32 96, label %case
    i32 97, label %case
    i32 98, label %case
    i32 99, label %case
    i32 100, label %case
    i32 80, label %case29
    i32 81, label %case29
    i32 82, label %case29
    i32 83, label %case29
    i32 84, label %case29
    i32 85, label %case29
    i32 86, label %case29
    i32 87, label %case29
    i32 88, label %case29
    i32 89, label %case29
  ]

next_stmt:                                        ; preds = %then
  br label %ifcont

unwind:                                           ; preds = %then
  ret i32 0

then15:                                           ; preds = %else
  %53 = call ptr @moksha_box_string(ptr @21)
  %print_unbox18 = call ptr @moksha_unbox_string(ptr %53)
  %54 = call i32 (ptr, ...) @printf(ptr @22, ptr %print_unbox18)
  %55 = load i32, ptr @__exception_flag, align 4
  %56 = icmp ne i32 %55, 0
  br i1 %56, label %unwind20, label %next_stmt19

else16:                                           ; preds = %else
  %57 = call ptr @moksha_box_string(ptr @23)
  %print_unbox21 = call ptr @moksha_unbox_string(ptr %57)
  %58 = call i32 (ptr, ...) @printf(ptr @24, ptr %print_unbox21)
  %59 = load i32, ptr @__exception_flag, align 4
  %60 = icmp ne i32 %59, 0
  br i1 %60, label %unwind23, label %next_stmt22

ifcont17:                                         ; preds = %next_stmt22, %next_stmt19
  br label %ifcont

next_stmt19:                                      ; preds = %then15
  br label %ifcont17

unwind20:                                         ; preds = %then15
  ret i32 0

next_stmt22:                                      ; preds = %else16
  br label %ifcont17

unwind23:                                         ; preds = %else16
  ret i32 0

switchend:                                        ; preds = %next_stmt37, %next_stmt34, %next_stmt31
  store double 3.000000e+01, ptr %temp, align 8
  store double 3.000000e+01, ptr %temp39, align 8
  %61 = call ptr @moksha_box_string(ptr @37)
  %62 = call ptr @moksha_box_string(ptr @38)
  %63 = call ptr @moksha_string_concat(ptr %61, ptr %62)
  %64 = load double, ptr %temp39, align 8
  %d2s40 = call ptr @moksha_double_to_str(double %64)
  %box_s41 = call ptr @moksha_box_string(ptr %d2s40)
  %65 = call ptr @moksha_string_concat(ptr %63, ptr %box_s41)
  %66 = call ptr @moksha_box_string(ptr @39)
  %67 = call ptr @moksha_string_concat(ptr %65, ptr %66)
  %print_unbox42 = call ptr @moksha_unbox_string(ptr %67)
  %68 = call i32 (ptr, ...) @printf(ptr @40, ptr %print_unbox42)
  %69 = load double, ptr %temp39, align 8
  br label %check_case_0

default:                                          ; preds = %ifcont
  %70 = call ptr @moksha_box_string(ptr @35)
  %print_unbox36 = call ptr @moksha_unbox_string(ptr %70)
  %71 = call i32 (ptr, ...) @printf(ptr @36, ptr %print_unbox36)
  %72 = load i32, ptr @__exception_flag, align 4
  %73 = icmp ne i32 %72, 0
  br i1 %73, label %unwind38, label %next_stmt37

case:                                             ; preds = %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont
  %74 = call ptr @moksha_box_string(ptr @31)
  %print_unbox30 = call ptr @moksha_unbox_string(ptr %74)
  %75 = call i32 (ptr, ...) @printf(ptr @32, ptr %print_unbox30)
  %76 = load i32, ptr @__exception_flag, align 4
  %77 = icmp ne i32 %76, 0
  br i1 %77, label %unwind32, label %next_stmt31

case29:                                           ; preds = %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont
  %78 = call ptr @moksha_box_string(ptr @33)
  %print_unbox33 = call ptr @moksha_unbox_string(ptr %78)
  %79 = call i32 (ptr, ...) @printf(ptr @34, ptr %print_unbox33)
  %80 = load i32, ptr @__exception_flag, align 4
  %81 = icmp ne i32 %80, 0
  br i1 %81, label %unwind35, label %next_stmt34

next_stmt31:                                      ; preds = %case
  br label %switchend

unwind32:                                         ; preds = %case
  ret i32 0

next_stmt34:                                      ; preds = %case29
  br label %switchend

unwind35:                                         ; preds = %case29
  ret i32 0

next_stmt37:                                      ; preds = %default
  br label %switchend

unwind38:                                         ; preds = %default
  ret i32 0

switchend43:                                      ; preds = %next_stmt48, %next_stmt45
  store i32 50, ptr %x, align 4
  store i32 50, ptr %x50, align 4
  %82 = call ptr @moksha_box_string(ptr @45)
  %83 = call ptr @moksha_box_string(ptr @46)
  %84 = call ptr @moksha_string_concat(ptr %82, ptr %83)
  %85 = load i32, ptr %x50, align 4
  %i2s51 = call ptr @moksha_int_to_str(i32 %85)
  %box_s52 = call ptr @moksha_box_string(ptr %i2s51)
  %86 = call ptr @moksha_string_concat(ptr %84, ptr %box_s52)
  %87 = call ptr @moksha_box_string(ptr @47)
  %88 = call ptr @moksha_string_concat(ptr %86, ptr %87)
  %print_unbox53 = call ptr @moksha_unbox_string(ptr %88)
  %89 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox53)
  br label %check_case_055

check_case_0:                                     ; preds = %switchend
  %90 = fcmp oge double %69, 2.550000e+01
  %91 = fcmp ole double %69, 3.550000e+01
  %92 = and i1 %90, %91
  br i1 %92, label %exec_case, label %check_case_1

exec_case:                                        ; preds = %check_case_0
  %93 = call ptr @moksha_box_string(ptr @41)
  %print_unbox44 = call ptr @moksha_unbox_string(ptr %93)
  %94 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox44)
  %95 = load i32, ptr @__exception_flag, align 4
  %96 = icmp ne i32 %95, 0
  br i1 %96, label %unwind46, label %next_stmt45

check_case_1:                                     ; preds = %check_case_0
  %97 = call ptr @moksha_box_string(ptr @43)
  %print_unbox47 = call ptr @moksha_unbox_string(ptr %97)
  %98 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox47)
  %99 = load i32, ptr @__exception_flag, align 4
  %100 = icmp ne i32 %99, 0
  br i1 %100, label %unwind49, label %next_stmt48

next_stmt45:                                      ; preds = %exec_case
  br label %switchend43

unwind46:                                         ; preds = %exec_case
  ret i32 0

next_stmt48:                                      ; preds = %check_case_1
  br label %switchend43

unwind49:                                         ; preds = %check_case_1
  ret i32 0

switchend54:                                      ; preds = %next_stmt69, %next_stmt66, %next_stmt61
  %101 = call ptr @moksha_box_string(ptr @55)
  store ptr %101, ptr %ch, align 8
  store ptr %101, ptr %ch71, align 8
  %102 = call ptr @moksha_box_string(ptr @56)
  %103 = call ptr @moksha_box_string(ptr @57)
  %104 = call ptr @moksha_string_concat(ptr %102, ptr %103)
  %105 = load ptr, ptr %ch71, align 8
  %106 = call ptr @moksha_string_concat(ptr %104, ptr %105)
  %107 = call ptr @moksha_box_string(ptr @58)
  %108 = call ptr @moksha_string_concat(ptr %106, ptr %107)
  %print_unbox72 = call ptr @moksha_unbox_string(ptr %108)
  %109 = call i32 (ptr, ...) @printf(ptr @59, ptr %print_unbox72)
  %110 = load ptr, ptr %ch71, align 8
  br label %check_case_074

check_case_055:                                   ; preds = %switchend43
  %111 = load i32, ptr %x50, align 4
  %icmptmp58 = icmp slt i32 20, %111
  %112 = load i32, ptr %x50, align 4
  %icmptmp59 = icmp slt i32 %112, 100
  %113 = icmp eq i1 true, %icmptmp59
  br i1 %113, label %exec_case56, label %check_case_157

exec_case56:                                      ; preds = %check_case_055
  %114 = call ptr @moksha_box_string(ptr @49)
  %print_unbox60 = call ptr @moksha_unbox_string(ptr %114)
  %115 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox60)
  %116 = load i32, ptr @__exception_flag, align 4
  %117 = icmp ne i32 %116, 0
  br i1 %117, label %unwind62, label %next_stmt61

check_case_157:                                   ; preds = %check_case_055
  %118 = load i32, ptr %x50, align 4
  %icmptmp64 = icmp sge i32 %118, 100
  %119 = icmp eq i1 true, %icmptmp64
  br i1 %119, label %exec_case63, label %check_case_2

next_stmt61:                                      ; preds = %exec_case56
  br label %switchend54

unwind62:                                         ; preds = %exec_case56
  ret i32 0

exec_case63:                                      ; preds = %check_case_157
  %120 = call ptr @moksha_box_string(ptr @51)
  %print_unbox65 = call ptr @moksha_unbox_string(ptr %120)
  %121 = call i32 (ptr, ...) @printf(ptr @52, ptr %print_unbox65)
  %122 = load i32, ptr @__exception_flag, align 4
  %123 = icmp ne i32 %122, 0
  br i1 %123, label %unwind67, label %next_stmt66

check_case_2:                                     ; preds = %check_case_157
  %124 = call ptr @moksha_box_string(ptr @53)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %124)
  %125 = call i32 (ptr, ...) @printf(ptr @54, ptr %print_unbox68)
  %126 = load i32, ptr @__exception_flag, align 4
  %127 = icmp ne i32 %126, 0
  br i1 %127, label %unwind70, label %next_stmt69

next_stmt66:                                      ; preds = %exec_case63
  br label %switchend54

unwind67:                                         ; preds = %exec_case63
  ret i32 0

next_stmt69:                                      ; preds = %check_case_2
  br label %switchend54

unwind70:                                         ; preds = %check_case_2
  ret i32 0

switchend73:                                      ; preds = %next_stmt86, %next_stmt83, %next_stmt78
  store i32 1, ptr %rank, align 4
  store i32 1, ptr %rank88, align 4
  %128 = call ptr @moksha_box_string(ptr @70)
  %129 = call ptr @moksha_box_string(ptr @71)
  %130 = call ptr @moksha_string_concat(ptr %128, ptr %129)
  %131 = load i32, ptr %rank88, align 4
  %i2s89 = call ptr @moksha_int_to_str(i32 %131)
  %box_s90 = call ptr @moksha_box_string(ptr %i2s89)
  %132 = call ptr @moksha_string_concat(ptr %130, ptr %box_s90)
  %133 = call ptr @moksha_box_string(ptr @72)
  %134 = call ptr @moksha_string_concat(ptr %132, ptr %133)
  %print_unbox91 = call ptr @moksha_unbox_string(ptr %134)
  %135 = call i32 (ptr, ...) @printf(ptr @73, ptr %print_unbox91)
  %136 = load i32, ptr %rank88, align 4
  switch i32 %136, label %default93 [
    i32 1, label %case94
    i32 2, label %case95
    i32 3, label %case96
    i32 4, label %case97
  ]

check_case_074:                                   ; preds = %switchend54
  %137 = call ptr @moksha_box_string(ptr @60)
  %138 = call ptr @moksha_box_string(ptr @61)
  %139 = call i32 @strcmp(ptr %110, ptr %137)
  %140 = icmp sge i32 %139, 0
  %141 = call i32 @strcmp(ptr %110, ptr %138)
  %142 = icmp sle i32 %141, 0
  %143 = and i1 %140, %142
  br i1 %143, label %exec_case75, label %check_case_176

exec_case75:                                      ; preds = %check_case_074
  %144 = call ptr @moksha_box_string(ptr @62)
  %print_unbox77 = call ptr @moksha_unbox_string(ptr %144)
  %145 = call i32 (ptr, ...) @printf(ptr @63, ptr %print_unbox77)
  %146 = load i32, ptr @__exception_flag, align 4
  %147 = icmp ne i32 %146, 0
  br i1 %147, label %unwind79, label %next_stmt78

check_case_176:                                   ; preds = %check_case_074
  %148 = call ptr @moksha_box_string(ptr @64)
  %149 = call ptr @moksha_box_string(ptr @65)
  %150 = call i32 @strcmp(ptr %110, ptr %148)
  %151 = icmp sge i32 %150, 0
  %152 = call i32 @strcmp(ptr %110, ptr %149)
  %153 = icmp sle i32 %152, 0
  %154 = and i1 %151, %153
  br i1 %154, label %exec_case80, label %check_case_281

next_stmt78:                                      ; preds = %exec_case75
  br label %switchend73

unwind79:                                         ; preds = %exec_case75
  ret i32 0

exec_case80:                                      ; preds = %check_case_176
  %155 = call ptr @moksha_box_string(ptr @66)
  %print_unbox82 = call ptr @moksha_unbox_string(ptr %155)
  %156 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox82)
  %157 = load i32, ptr @__exception_flag, align 4
  %158 = icmp ne i32 %157, 0
  br i1 %158, label %unwind84, label %next_stmt83

check_case_281:                                   ; preds = %check_case_176
  %159 = call ptr @moksha_box_string(ptr @68)
  %print_unbox85 = call ptr @moksha_unbox_string(ptr %159)
  %160 = call i32 (ptr, ...) @printf(ptr @69, ptr %print_unbox85)
  %161 = load i32, ptr @__exception_flag, align 4
  %162 = icmp ne i32 %161, 0
  br i1 %162, label %unwind87, label %next_stmt86

next_stmt83:                                      ; preds = %exec_case80
  br label %switchend73

unwind84:                                         ; preds = %exec_case80
  ret i32 0

next_stmt86:                                      ; preds = %check_case_281
  br label %switchend73

unwind87:                                         ; preds = %check_case_281
  ret i32 0

switchend92:                                      ; preds = %next_stmt105, %next_stmt102, %next_stmt99
  %163 = call ptr @moksha_box_string(ptr @80)
  %print_unbox107 = call ptr @moksha_unbox_string(ptr %163)
  %164 = call i32 (ptr, ...) @printf(ptr @81, ptr %print_unbox107)
  %165 = call ptr @moksha_box_string(ptr @82)
  %print_unbox108 = call ptr @moksha_unbox_string(ptr %165)
  %166 = call i32 (ptr, ...) @printf(ptr @83, ptr %print_unbox108)
  store i32 3, ptr %w, align 4
  store i32 3, ptr %w109, align 4
  br label %whilecond

default93:                                        ; preds = %switchend73
  %167 = call ptr @moksha_box_string(ptr @78)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %167)
  %168 = call i32 (ptr, ...) @printf(ptr @79, ptr %print_unbox104)
  %169 = load i32, ptr @__exception_flag, align 4
  %170 = icmp ne i32 %169, 0
  br i1 %170, label %unwind106, label %next_stmt105

case94:                                           ; preds = %switchend73
  br label %case95

case95:                                           ; preds = %switchend73, %case94
  br label %case96

case96:                                           ; preds = %switchend73, %case95
  %171 = call ptr @moksha_box_string(ptr @74)
  %print_unbox98 = call ptr @moksha_unbox_string(ptr %171)
  %172 = call i32 (ptr, ...) @printf(ptr @75, ptr %print_unbox98)
  %173 = load i32, ptr @__exception_flag, align 4
  %174 = icmp ne i32 %173, 0
  br i1 %174, label %unwind100, label %next_stmt99

case97:                                           ; preds = %switchend73
  %175 = call ptr @moksha_box_string(ptr @76)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %175)
  %176 = call i32 (ptr, ...) @printf(ptr @77, ptr %print_unbox101)
  %177 = load i32, ptr @__exception_flag, align 4
  %178 = icmp ne i32 %177, 0
  br i1 %178, label %unwind103, label %next_stmt102

next_stmt99:                                      ; preds = %case96
  br label %switchend92

unwind100:                                        ; preds = %case96
  ret i32 0

next_stmt102:                                     ; preds = %case97
  br label %switchend92

unwind103:                                        ; preds = %case97
  ret i32 0

next_stmt105:                                     ; preds = %default93
  br label %switchend92

unwind106:                                        ; preds = %default93
  ret i32 0

whilecond:                                        ; preds = %next_stmt116, %switchend92
  %179 = load i32, ptr %w109, align 4
  %icmptmp110 = icmp sgt i32 %179, 0
  br i1 %icmptmp110, label %whileloop, label %whileafter

whileloop:                                        ; preds = %whilecond
  %180 = call ptr @moksha_box_string(ptr @84)
  %181 = call ptr @moksha_box_string(ptr @85)
  %182 = call ptr @moksha_string_concat(ptr %180, ptr %181)
  %183 = load i32, ptr %w109, align 4
  %i2s111 = call ptr @moksha_int_to_str(i32 %183)
  %box_s112 = call ptr @moksha_box_string(ptr %i2s111)
  %184 = call ptr @moksha_string_concat(ptr %182, ptr %box_s112)
  %print_unbox113 = call ptr @moksha_unbox_string(ptr %184)
  %185 = call i32 (ptr, ...) @printf(ptr @86, ptr %print_unbox113)
  %186 = load i32, ptr @__exception_flag, align 4
  %187 = icmp ne i32 %186, 0
  br i1 %187, label %unwind115, label %next_stmt114

whileafter:                                       ; preds = %whilecond
  %188 = call ptr @moksha_box_string(ptr @87)
  %print_unbox118 = call ptr @moksha_unbox_string(ptr %188)
  %189 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox118)
  store i32 0, ptr %d, align 4
  store i32 0, ptr %d119, align 4
  br label %dowhileloop

next_stmt114:                                     ; preds = %whileloop
  %oldval = load i32, ptr %w109, align 4
  %dec = sub i32 %oldval, 1
  store i32 %dec, ptr %w109, align 4
  %190 = load i32, ptr @__exception_flag, align 4
  %191 = icmp ne i32 %190, 0
  br i1 %191, label %unwind117, label %next_stmt116

unwind115:                                        ; preds = %whileloop
  ret i32 0

next_stmt116:                                     ; preds = %next_stmt114
  br label %whilecond

unwind117:                                        ; preds = %next_stmt114
  ret i32 0

dowhileloop:                                      ; preds = %dowhilecond, %whileafter
  %192 = call ptr @moksha_box_string(ptr @89)
  %193 = call ptr @moksha_box_string(ptr @90)
  %194 = call ptr @moksha_string_concat(ptr %192, ptr %193)
  %195 = load i32, ptr %d119, align 4
  %i2s120 = call ptr @moksha_int_to_str(i32 %195)
  %box_s121 = call ptr @moksha_box_string(ptr %i2s120)
  %196 = call ptr @moksha_string_concat(ptr %194, ptr %box_s121)
  %197 = call ptr @moksha_box_string(ptr @91)
  %198 = call ptr @moksha_string_concat(ptr %196, ptr %197)
  %print_unbox122 = call ptr @moksha_unbox_string(ptr %198)
  %199 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox122)
  %200 = load i32, ptr @__exception_flag, align 4
  %201 = icmp ne i32 %200, 0
  br i1 %201, label %unwind124, label %next_stmt123

dowhilecond:                                      ; preds = %next_stmt126
  %202 = load i32, ptr %d119, align 4
  %icmptmp128 = icmp slt i32 %202, 0
  br i1 %icmptmp128, label %dowhileloop, label %dowhileafter

dowhileafter:                                     ; preds = %dowhilecond
  %203 = call ptr @moksha_box_string(ptr @93)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %203)
  %204 = call i32 (ptr, ...) @printf(ptr @94, ptr %print_unbox129)
  store i32 0, ptr %i, align 4
  store i32 0, ptr %i130, align 4
  br label %forcond

next_stmt123:                                     ; preds = %dowhileloop
  %oldval125 = load i32, ptr %d119, align 4
  %inc = add i32 %oldval125, 1
  store i32 %inc, ptr %d119, align 4
  %205 = load i32, ptr @__exception_flag, align 4
  %206 = icmp ne i32 %205, 0
  br i1 %206, label %unwind127, label %next_stmt126

unwind124:                                        ; preds = %dowhileloop
  ret i32 0

next_stmt126:                                     ; preds = %next_stmt123
  br label %dowhilecond

unwind127:                                        ; preds = %next_stmt123
  ret i32 0

forcond:                                          ; preds = %forinc, %dowhileafter
  %207 = load i32, ptr %i130, align 4
  %icmptmp131 = icmp slt i32 %207, 5
  br i1 %icmptmp131, label %forloop, label %forafter

forloop:                                          ; preds = %forcond
  %208 = call ptr @moksha_box_string(ptr @95)
  %209 = call ptr @moksha_box_string(ptr @96)
  %210 = call ptr @moksha_string_concat(ptr %208, ptr %209)
  %211 = load i32, ptr %i130, align 4
  %i2s132 = call ptr @moksha_int_to_str(i32 %211)
  %box_s133 = call ptr @moksha_box_string(ptr %i2s132)
  %212 = call ptr @moksha_string_concat(ptr %210, ptr %box_s133)
  %print_unbox134 = call ptr @moksha_unbox_string(ptr %212)
  %213 = call i32 (ptr, ...) @printf(ptr @97, ptr %print_unbox134)
  %214 = load i32, ptr @__exception_flag, align 4
  %215 = icmp ne i32 %214, 0
  br i1 %215, label %unwind136, label %next_stmt135

forinc:                                           ; preds = %next_stmt135
  %216 = load i32, ptr %i130, align 4
  %addtmp = add i32 %216, 2
  store i32 %addtmp, ptr %i130, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  %217 = call ptr @moksha_box_string(ptr @98)
  %print_unbox137 = call ptr @moksha_unbox_string(ptr %217)
  %218 = call i32 (ptr, ...) @printf(ptr @99, ptr %print_unbox137)
  %219 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 10)
  %220 = call ptr @moksha_array_push_val(ptr %219, ptr %box_i)
  %box_i138 = call ptr @moksha_box_int(i32 20)
  %221 = call ptr @moksha_array_push_val(ptr %219, ptr %box_i138)
  %box_i139 = call ptr @moksha_box_int(i32 30)
  %222 = call ptr @moksha_array_push_val(ptr %219, ptr %box_i139)
  store ptr %219, ptr %numbers, align 8
  store ptr %219, ptr %numbers140, align 8
  %223 = load ptr, ptr %numbers140, align 8
  %len = call i32 @moksha_get_length(ptr %223)
  store i32 0, ptr %forin_idx, align 4
  br label %forin_cond

next_stmt135:                                     ; preds = %forloop
  br label %forinc

unwind136:                                        ; preds = %forloop
  ret i32 0

forin_cond:                                       ; preds = %forin_inc, %forafter
  %224 = load i32, ptr %forin_idx, align 4
  %225 = icmp slt i32 %224, %len
  br i1 %225, label %forin_body, label %forin_after

forin_body:                                       ; preds = %forin_cond
  %arr_val = call ptr @moksha_array_get_unsafe(ptr %223, i32 %224)
  %unbox_i = call i32 @moksha_unbox_int(ptr %arr_val)
  store i32 %224, ptr %idx, align 4
  store i32 %unbox_i, ptr %val, align 4
  %226 = call ptr @moksha_box_string(ptr @100)
  %227 = call ptr @moksha_box_string(ptr @101)
  %228 = call ptr @moksha_string_concat(ptr %226, ptr %227)
  %229 = load i32, ptr %idx, align 4
  %i2s141 = call ptr @moksha_int_to_str(i32 %229)
  %box_s142 = call ptr @moksha_box_string(ptr %i2s141)
  %230 = call ptr @moksha_string_concat(ptr %228, ptr %box_s142)
  %231 = call ptr @moksha_box_string(ptr @102)
  %232 = call ptr @moksha_string_concat(ptr %230, ptr %231)
  %233 = load i32, ptr %val, align 4
  %i2s143 = call ptr @moksha_int_to_str(i32 %233)
  %box_s144 = call ptr @moksha_box_string(ptr %i2s143)
  %234 = call ptr @moksha_string_concat(ptr %232, ptr %box_s144)
  %print_unbox145 = call ptr @moksha_unbox_string(ptr %234)
  %235 = call i32 (ptr, ...) @printf(ptr @103, ptr %print_unbox145)
  %236 = load i32, ptr @__exception_flag, align 4
  %237 = icmp ne i32 %236, 0
  br i1 %237, label %unwind147, label %next_stmt146

forin_inc:                                        ; preds = %next_stmt146
  %238 = add i32 %224, 1
  store i32 %238, ptr %forin_idx, align 4
  br label %forin_cond

forin_after:                                      ; preds = %forin_cond
  %239 = call ptr @moksha_box_string(ptr @104)
  %print_unbox148 = call ptr @moksha_unbox_string(ptr %239)
  %240 = call i32 (ptr, ...) @printf(ptr @105, ptr %print_unbox148)
  ret i32 0

next_stmt146:                                     ; preds = %forin_body
  br label %forin_inc

unwind147:                                        ; preds = %forin_body
  ret i32 0
}
