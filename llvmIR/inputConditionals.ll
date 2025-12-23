; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = global i32 0
@__exception_val = global ptr null
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
  %val = alloca i32, align 4
  %idx = alloca i32, align 4
  %forin_idx = alloca i32, align 4
  %numbers131 = alloca ptr, align 8
  %numbers = alloca ptr, align 8
  %i122 = alloca i32, align 4
  %i = alloca i32, align 4
  %d112 = alloca i32, align 4
  %d = alloca i32, align 4
  %w103 = alloca i32, align 4
  %w = alloca i32, align 4
  %rank83 = alloca i32, align 4
  %rank = alloca i32, align 4
  %ch66 = alloca ptr, align 8
  %ch = alloca ptr, align 8
  %x46 = alloca i32, align 4
  %x = alloca i32, align 4
  %temp36 = alloca double, align 8
  %temp = alloca double, align 8
  %score23 = alloca i32, align 4
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
  %read_i = call i32 @moksha_read_int()
  store i32 %read_i, ptr %iVal, align 4
  store i32 %read_i, ptr %iVal3, align 4
  %6 = call ptr @moksha_box_string(ptr @6)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %6)
  %7 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox4)
  %read_d = call double @moksha_read_double()
  store double %read_d, ptr %dVal, align 8
  store double %read_d, ptr %dVal5, align 8
  %8 = call ptr @moksha_box_string(ptr @8)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %8)
  %9 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox6)
  %read_b = call i32 @moksha_read_bool()
  store i32 %read_b, ptr %bVal, align 4
  store i32 %read_b, ptr %bVal7, align 4
  %10 = call ptr @moksha_box_string(ptr @10)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %10)
  %11 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox8)
  %read_s = call ptr @moksha_read_string()
  store ptr %read_s, ptr %sVal, align 8
  store ptr %read_s, ptr %sVal9, align 8
  %12 = call ptr @moksha_box_string(ptr @12)
  %13 = call ptr @moksha_box_string(ptr @13)
  %14 = call ptr @moksha_string_concat(ptr %12, ptr %13)
  %15 = load i32, ptr %iVal3, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %15)
  %16 = call ptr @moksha_box_string(ptr %i2s)
  %17 = call ptr @moksha_string_concat(ptr %14, ptr %16)
  %18 = call ptr @moksha_box_string(ptr @14)
  %19 = call ptr @moksha_string_concat(ptr %17, ptr %18)
  %20 = load double, ptr %dVal5, align 8
  %d2s = call ptr @moksha_double_to_str(double %20)
  %21 = call ptr @moksha_box_string(ptr %d2s)
  %22 = call ptr @moksha_string_concat(ptr %19, ptr %21)
  %23 = call ptr @moksha_box_string(ptr @15)
  %24 = call ptr @moksha_string_concat(ptr %22, ptr %23)
  %25 = load i1, ptr %bVal7, align 1
  %bool_ext = zext i1 %25 to i32
  %box_b = call ptr @moksha_box_bool(i32 %bool_ext)
  %b2s = call ptr @moksha_any_to_str(ptr %box_b)
  %26 = call ptr @moksha_box_string(ptr %b2s)
  %27 = call ptr @moksha_string_concat(ptr %24, ptr %26)
  %28 = call ptr @moksha_box_string(ptr @16)
  %29 = call ptr @moksha_string_concat(ptr %27, ptr %28)
  %30 = load ptr, ptr %sVal9, align 8
  %31 = call ptr @moksha_string_concat(ptr %29, ptr %30)
  %32 = call ptr @moksha_box_string(ptr @17)
  %33 = call ptr @moksha_string_concat(ptr %31, ptr %32)
  %print_unbox10 = call ptr @moksha_unbox_string(ptr %33)
  %34 = call i32 (ptr, ...) @printf(ptr @18, ptr %print_unbox10)
  %35 = load i32, ptr %iVal3, align 4
  %icmptmp = icmp sgt i32 %35, 100
  br i1 %icmptmp, label %then, label %else

then:                                             ; preds = %entry
  %36 = call ptr @moksha_box_string(ptr @19)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %36)
  %37 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox11)
  %38 = load i32, ptr @__exception_flag, align 4
  %39 = icmp ne i32 %38, 0
  br i1 %39, label %unwind, label %next_stmt

else:                                             ; preds = %entry
  %40 = load i32, ptr %iVal3, align 4
  %icmptmp12 = icmp sgt i32 %40, 0
  br i1 %icmptmp12, label %then13, label %else14

ifcont:                                           ; preds = %ifcont15, %next_stmt
  %41 = call ptr @moksha_box_string(ptr @25)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %41)
  %42 = call i32 (ptr, ...) @printf(ptr @26, ptr %print_unbox22)
  store i32 95, ptr %score, align 4
  store i32 95, ptr %score23, align 4
  %43 = call ptr @moksha_box_string(ptr @27)
  %44 = call ptr @moksha_box_string(ptr @28)
  %45 = call ptr @moksha_string_concat(ptr %43, ptr %44)
  %46 = load i32, ptr %score23, align 4
  %i2s24 = call ptr @moksha_int_to_str(i32 %46)
  %47 = call ptr @moksha_box_string(ptr %i2s24)
  %48 = call ptr @moksha_string_concat(ptr %45, ptr %47)
  %49 = call ptr @moksha_box_string(ptr @29)
  %50 = call ptr @moksha_string_concat(ptr %48, ptr %49)
  %print_unbox25 = call ptr @moksha_unbox_string(ptr %50)
  %51 = call i32 (ptr, ...) @printf(ptr @30, ptr %print_unbox25)
  %52 = load i32, ptr %score23, align 4
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
    i32 80, label %case26
    i32 81, label %case26
    i32 82, label %case26
    i32 83, label %case26
    i32 84, label %case26
    i32 85, label %case26
    i32 86, label %case26
    i32 87, label %case26
    i32 88, label %case26
    i32 89, label %case26
  ]

next_stmt:                                        ; preds = %then
  br label %ifcont

unwind:                                           ; preds = %then
  ret i32 0

then13:                                           ; preds = %else
  %53 = call ptr @moksha_box_string(ptr @21)
  %print_unbox16 = call ptr @moksha_unbox_string(ptr %53)
  %54 = call i32 (ptr, ...) @printf(ptr @22, ptr %print_unbox16)
  %55 = load i32, ptr @__exception_flag, align 4
  %56 = icmp ne i32 %55, 0
  br i1 %56, label %unwind18, label %next_stmt17

else14:                                           ; preds = %else
  %57 = call ptr @moksha_box_string(ptr @23)
  %print_unbox19 = call ptr @moksha_unbox_string(ptr %57)
  %58 = call i32 (ptr, ...) @printf(ptr @24, ptr %print_unbox19)
  %59 = load i32, ptr @__exception_flag, align 4
  %60 = icmp ne i32 %59, 0
  br i1 %60, label %unwind21, label %next_stmt20

ifcont15:                                         ; preds = %next_stmt20, %next_stmt17
  br label %ifcont

next_stmt17:                                      ; preds = %then13
  br label %ifcont15

unwind18:                                         ; preds = %then13
  ret i32 0

next_stmt20:                                      ; preds = %else14
  br label %ifcont15

unwind21:                                         ; preds = %else14
  ret i32 0

switchend:                                        ; preds = %next_stmt34, %next_stmt31, %next_stmt28
  store double 3.000000e+01, ptr %temp, align 8
  store double 3.000000e+01, ptr %temp36, align 8
  %61 = call ptr @moksha_box_string(ptr @37)
  %62 = call ptr @moksha_box_string(ptr @38)
  %63 = call ptr @moksha_string_concat(ptr %61, ptr %62)
  %64 = load double, ptr %temp36, align 8
  %d2s37 = call ptr @moksha_double_to_str(double %64)
  %65 = call ptr @moksha_box_string(ptr %d2s37)
  %66 = call ptr @moksha_string_concat(ptr %63, ptr %65)
  %67 = call ptr @moksha_box_string(ptr @39)
  %68 = call ptr @moksha_string_concat(ptr %66, ptr %67)
  %print_unbox38 = call ptr @moksha_unbox_string(ptr %68)
  %69 = call i32 (ptr, ...) @printf(ptr @40, ptr %print_unbox38)
  %70 = load double, ptr %temp36, align 8
  br label %check_case_0

default:                                          ; preds = %ifcont
  %71 = call ptr @moksha_box_string(ptr @35)
  %print_unbox33 = call ptr @moksha_unbox_string(ptr %71)
  %72 = call i32 (ptr, ...) @printf(ptr @36, ptr %print_unbox33)
  %73 = load i32, ptr @__exception_flag, align 4
  %74 = icmp ne i32 %73, 0
  br i1 %74, label %unwind35, label %next_stmt34

case:                                             ; preds = %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont
  %75 = call ptr @moksha_box_string(ptr @31)
  %print_unbox27 = call ptr @moksha_unbox_string(ptr %75)
  %76 = call i32 (ptr, ...) @printf(ptr @32, ptr %print_unbox27)
  %77 = load i32, ptr @__exception_flag, align 4
  %78 = icmp ne i32 %77, 0
  br i1 %78, label %unwind29, label %next_stmt28

case26:                                           ; preds = %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont, %ifcont
  %79 = call ptr @moksha_box_string(ptr @33)
  %print_unbox30 = call ptr @moksha_unbox_string(ptr %79)
  %80 = call i32 (ptr, ...) @printf(ptr @34, ptr %print_unbox30)
  %81 = load i32, ptr @__exception_flag, align 4
  %82 = icmp ne i32 %81, 0
  br i1 %82, label %unwind32, label %next_stmt31

next_stmt28:                                      ; preds = %case
  br label %switchend

unwind29:                                         ; preds = %case
  ret i32 0

next_stmt31:                                      ; preds = %case26
  br label %switchend

unwind32:                                         ; preds = %case26
  ret i32 0

next_stmt34:                                      ; preds = %default
  br label %switchend

unwind35:                                         ; preds = %default
  ret i32 0

switchend39:                                      ; preds = %next_stmt44, %next_stmt41
  store i32 50, ptr %x, align 4
  store i32 50, ptr %x46, align 4
  %83 = call ptr @moksha_box_string(ptr @45)
  %84 = call ptr @moksha_box_string(ptr @46)
  %85 = call ptr @moksha_string_concat(ptr %83, ptr %84)
  %86 = load i32, ptr %x46, align 4
  %i2s47 = call ptr @moksha_int_to_str(i32 %86)
  %87 = call ptr @moksha_box_string(ptr %i2s47)
  %88 = call ptr @moksha_string_concat(ptr %85, ptr %87)
  %89 = call ptr @moksha_box_string(ptr @47)
  %90 = call ptr @moksha_string_concat(ptr %88, ptr %89)
  %print_unbox48 = call ptr @moksha_unbox_string(ptr %90)
  %91 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox48)
  br label %check_case_050

check_case_0:                                     ; preds = %switchend
  %92 = fcmp oge double %70, 2.550000e+01
  %93 = fcmp ole double %70, 3.550000e+01
  %94 = and i1 %92, %93
  br i1 %94, label %exec_case, label %check_case_1

exec_case:                                        ; preds = %check_case_0
  %95 = call ptr @moksha_box_string(ptr @41)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %95)
  %96 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox40)
  %97 = load i32, ptr @__exception_flag, align 4
  %98 = icmp ne i32 %97, 0
  br i1 %98, label %unwind42, label %next_stmt41

check_case_1:                                     ; preds = %check_case_0
  %99 = call ptr @moksha_box_string(ptr @43)
  %print_unbox43 = call ptr @moksha_unbox_string(ptr %99)
  %100 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox43)
  %101 = load i32, ptr @__exception_flag, align 4
  %102 = icmp ne i32 %101, 0
  br i1 %102, label %unwind45, label %next_stmt44

next_stmt41:                                      ; preds = %exec_case
  br label %switchend39

unwind42:                                         ; preds = %exec_case
  ret i32 0

next_stmt44:                                      ; preds = %check_case_1
  br label %switchend39

unwind45:                                         ; preds = %check_case_1
  ret i32 0

switchend49:                                      ; preds = %next_stmt64, %next_stmt61, %next_stmt56
  %103 = call ptr @moksha_box_string(ptr @55)
  store ptr %103, ptr %ch, align 8
  store ptr %103, ptr %ch66, align 8
  %104 = call ptr @moksha_box_string(ptr @56)
  %105 = call ptr @moksha_box_string(ptr @57)
  %106 = call ptr @moksha_string_concat(ptr %104, ptr %105)
  %107 = load ptr, ptr %ch66, align 8
  %108 = call ptr @moksha_string_concat(ptr %106, ptr %107)
  %109 = call ptr @moksha_box_string(ptr @58)
  %110 = call ptr @moksha_string_concat(ptr %108, ptr %109)
  %print_unbox67 = call ptr @moksha_unbox_string(ptr %110)
  %111 = call i32 (ptr, ...) @printf(ptr @59, ptr %print_unbox67)
  %112 = load ptr, ptr %ch66, align 8
  br label %check_case_069

check_case_050:                                   ; preds = %switchend39
  %113 = load i32, ptr %x46, align 4
  %icmptmp53 = icmp slt i32 20, %113
  %114 = load i32, ptr %x46, align 4
  %icmptmp54 = icmp slt i32 %114, 100
  %115 = icmp eq i1 true, %icmptmp54
  br i1 %115, label %exec_case51, label %check_case_152

exec_case51:                                      ; preds = %check_case_050
  %116 = call ptr @moksha_box_string(ptr @49)
  %print_unbox55 = call ptr @moksha_unbox_string(ptr %116)
  %117 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox55)
  %118 = load i32, ptr @__exception_flag, align 4
  %119 = icmp ne i32 %118, 0
  br i1 %119, label %unwind57, label %next_stmt56

check_case_152:                                   ; preds = %check_case_050
  %120 = load i32, ptr %x46, align 4
  %icmptmp59 = icmp sge i32 %120, 100
  %121 = icmp eq i1 true, %icmptmp59
  br i1 %121, label %exec_case58, label %check_case_2

next_stmt56:                                      ; preds = %exec_case51
  br label %switchend49

unwind57:                                         ; preds = %exec_case51
  ret i32 0

exec_case58:                                      ; preds = %check_case_152
  %122 = call ptr @moksha_box_string(ptr @51)
  %print_unbox60 = call ptr @moksha_unbox_string(ptr %122)
  %123 = call i32 (ptr, ...) @printf(ptr @52, ptr %print_unbox60)
  %124 = load i32, ptr @__exception_flag, align 4
  %125 = icmp ne i32 %124, 0
  br i1 %125, label %unwind62, label %next_stmt61

check_case_2:                                     ; preds = %check_case_152
  %126 = call ptr @moksha_box_string(ptr @53)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %126)
  %127 = call i32 (ptr, ...) @printf(ptr @54, ptr %print_unbox63)
  %128 = load i32, ptr @__exception_flag, align 4
  %129 = icmp ne i32 %128, 0
  br i1 %129, label %unwind65, label %next_stmt64

next_stmt61:                                      ; preds = %exec_case58
  br label %switchend49

unwind62:                                         ; preds = %exec_case58
  ret i32 0

next_stmt64:                                      ; preds = %check_case_2
  br label %switchend49

unwind65:                                         ; preds = %check_case_2
  ret i32 0

switchend68:                                      ; preds = %next_stmt81, %next_stmt78, %next_stmt73
  store i32 1, ptr %rank, align 4
  store i32 1, ptr %rank83, align 4
  %130 = call ptr @moksha_box_string(ptr @70)
  %131 = call ptr @moksha_box_string(ptr @71)
  %132 = call ptr @moksha_string_concat(ptr %130, ptr %131)
  %133 = load i32, ptr %rank83, align 4
  %i2s84 = call ptr @moksha_int_to_str(i32 %133)
  %134 = call ptr @moksha_box_string(ptr %i2s84)
  %135 = call ptr @moksha_string_concat(ptr %132, ptr %134)
  %136 = call ptr @moksha_box_string(ptr @72)
  %137 = call ptr @moksha_string_concat(ptr %135, ptr %136)
  %print_unbox85 = call ptr @moksha_unbox_string(ptr %137)
  %138 = call i32 (ptr, ...) @printf(ptr @73, ptr %print_unbox85)
  %139 = load i32, ptr %rank83, align 4
  switch i32 %139, label %default87 [
    i32 1, label %case88
    i32 2, label %case89
    i32 3, label %case90
    i32 4, label %case91
  ]

check_case_069:                                   ; preds = %switchend49
  %140 = call ptr @moksha_box_string(ptr @60)
  %141 = call ptr @moksha_box_string(ptr @61)
  %142 = call i32 @strcmp(ptr %112, ptr %140)
  %143 = icmp sge i32 %142, 0
  %144 = call i32 @strcmp(ptr %112, ptr %141)
  %145 = icmp sle i32 %144, 0
  %146 = and i1 %143, %145
  br i1 %146, label %exec_case70, label %check_case_171

exec_case70:                                      ; preds = %check_case_069
  %147 = call ptr @moksha_box_string(ptr @62)
  %print_unbox72 = call ptr @moksha_unbox_string(ptr %147)
  %148 = call i32 (ptr, ...) @printf(ptr @63, ptr %print_unbox72)
  %149 = load i32, ptr @__exception_flag, align 4
  %150 = icmp ne i32 %149, 0
  br i1 %150, label %unwind74, label %next_stmt73

check_case_171:                                   ; preds = %check_case_069
  %151 = call ptr @moksha_box_string(ptr @64)
  %152 = call ptr @moksha_box_string(ptr @65)
  %153 = call i32 @strcmp(ptr %112, ptr %151)
  %154 = icmp sge i32 %153, 0
  %155 = call i32 @strcmp(ptr %112, ptr %152)
  %156 = icmp sle i32 %155, 0
  %157 = and i1 %154, %156
  br i1 %157, label %exec_case75, label %check_case_276

next_stmt73:                                      ; preds = %exec_case70
  br label %switchend68

unwind74:                                         ; preds = %exec_case70
  ret i32 0

exec_case75:                                      ; preds = %check_case_171
  %158 = call ptr @moksha_box_string(ptr @66)
  %print_unbox77 = call ptr @moksha_unbox_string(ptr %158)
  %159 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox77)
  %160 = load i32, ptr @__exception_flag, align 4
  %161 = icmp ne i32 %160, 0
  br i1 %161, label %unwind79, label %next_stmt78

check_case_276:                                   ; preds = %check_case_171
  %162 = call ptr @moksha_box_string(ptr @68)
  %print_unbox80 = call ptr @moksha_unbox_string(ptr %162)
  %163 = call i32 (ptr, ...) @printf(ptr @69, ptr %print_unbox80)
  %164 = load i32, ptr @__exception_flag, align 4
  %165 = icmp ne i32 %164, 0
  br i1 %165, label %unwind82, label %next_stmt81

next_stmt78:                                      ; preds = %exec_case75
  br label %switchend68

unwind79:                                         ; preds = %exec_case75
  ret i32 0

next_stmt81:                                      ; preds = %check_case_276
  br label %switchend68

unwind82:                                         ; preds = %check_case_276
  ret i32 0

switchend86:                                      ; preds = %next_stmt99, %next_stmt96, %next_stmt93
  %166 = call ptr @moksha_box_string(ptr @80)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %166)
  %167 = call i32 (ptr, ...) @printf(ptr @81, ptr %print_unbox101)
  %168 = call ptr @moksha_box_string(ptr @82)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %168)
  %169 = call i32 (ptr, ...) @printf(ptr @83, ptr %print_unbox102)
  store i32 3, ptr %w, align 4
  store i32 3, ptr %w103, align 4
  br label %whilecond

default87:                                        ; preds = %switchend68
  %170 = call ptr @moksha_box_string(ptr @78)
  %print_unbox98 = call ptr @moksha_unbox_string(ptr %170)
  %171 = call i32 (ptr, ...) @printf(ptr @79, ptr %print_unbox98)
  %172 = load i32, ptr @__exception_flag, align 4
  %173 = icmp ne i32 %172, 0
  br i1 %173, label %unwind100, label %next_stmt99

case88:                                           ; preds = %switchend68
  br label %case89

case89:                                           ; preds = %switchend68, %case88
  br label %case90

case90:                                           ; preds = %switchend68, %case89
  %174 = call ptr @moksha_box_string(ptr @74)
  %print_unbox92 = call ptr @moksha_unbox_string(ptr %174)
  %175 = call i32 (ptr, ...) @printf(ptr @75, ptr %print_unbox92)
  %176 = load i32, ptr @__exception_flag, align 4
  %177 = icmp ne i32 %176, 0
  br i1 %177, label %unwind94, label %next_stmt93

case91:                                           ; preds = %switchend68
  %178 = call ptr @moksha_box_string(ptr @76)
  %print_unbox95 = call ptr @moksha_unbox_string(ptr %178)
  %179 = call i32 (ptr, ...) @printf(ptr @77, ptr %print_unbox95)
  %180 = load i32, ptr @__exception_flag, align 4
  %181 = icmp ne i32 %180, 0
  br i1 %181, label %unwind97, label %next_stmt96

next_stmt93:                                      ; preds = %case90
  br label %switchend86

unwind94:                                         ; preds = %case90
  ret i32 0

next_stmt96:                                      ; preds = %case91
  br label %switchend86

unwind97:                                         ; preds = %case91
  ret i32 0

next_stmt99:                                      ; preds = %default87
  br label %switchend86

unwind100:                                        ; preds = %default87
  ret i32 0

whilecond:                                        ; preds = %next_stmt109, %switchend86
  %182 = load i32, ptr %w103, align 4
  %icmptmp104 = icmp sgt i32 %182, 0
  br i1 %icmptmp104, label %whileloop, label %whileafter

whileloop:                                        ; preds = %whilecond
  %183 = call ptr @moksha_box_string(ptr @84)
  %184 = call ptr @moksha_box_string(ptr @85)
  %185 = call ptr @moksha_string_concat(ptr %183, ptr %184)
  %186 = load i32, ptr %w103, align 4
  %i2s105 = call ptr @moksha_int_to_str(i32 %186)
  %187 = call ptr @moksha_box_string(ptr %i2s105)
  %188 = call ptr @moksha_string_concat(ptr %185, ptr %187)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %188)
  %189 = call i32 (ptr, ...) @printf(ptr @86, ptr %print_unbox106)
  %190 = load i32, ptr @__exception_flag, align 4
  %191 = icmp ne i32 %190, 0
  br i1 %191, label %unwind108, label %next_stmt107

whileafter:                                       ; preds = %whilecond
  %192 = call ptr @moksha_box_string(ptr @87)
  %print_unbox111 = call ptr @moksha_unbox_string(ptr %192)
  %193 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox111)
  store i32 0, ptr %d, align 4
  store i32 0, ptr %d112, align 4
  br label %dowhileloop

next_stmt107:                                     ; preds = %whileloop
  %oldval = load i32, ptr %w103, align 4
  %dec = sub i32 %oldval, 1
  store i32 %dec, ptr %w103, align 4
  %194 = load i32, ptr @__exception_flag, align 4
  %195 = icmp ne i32 %194, 0
  br i1 %195, label %unwind110, label %next_stmt109

unwind108:                                        ; preds = %whileloop
  ret i32 0

next_stmt109:                                     ; preds = %next_stmt107
  br label %whilecond

unwind110:                                        ; preds = %next_stmt107
  ret i32 0

dowhileloop:                                      ; preds = %dowhilecond, %whileafter
  %196 = call ptr @moksha_box_string(ptr @89)
  %197 = call ptr @moksha_box_string(ptr @90)
  %198 = call ptr @moksha_string_concat(ptr %196, ptr %197)
  %199 = load i32, ptr %d112, align 4
  %i2s113 = call ptr @moksha_int_to_str(i32 %199)
  %200 = call ptr @moksha_box_string(ptr %i2s113)
  %201 = call ptr @moksha_string_concat(ptr %198, ptr %200)
  %202 = call ptr @moksha_box_string(ptr @91)
  %203 = call ptr @moksha_string_concat(ptr %201, ptr %202)
  %print_unbox114 = call ptr @moksha_unbox_string(ptr %203)
  %204 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox114)
  %205 = load i32, ptr @__exception_flag, align 4
  %206 = icmp ne i32 %205, 0
  br i1 %206, label %unwind116, label %next_stmt115

dowhilecond:                                      ; preds = %next_stmt118
  %207 = load i32, ptr %d112, align 4
  %icmptmp120 = icmp slt i32 %207, 0
  br i1 %icmptmp120, label %dowhileloop, label %dowhileafter

dowhileafter:                                     ; preds = %dowhilecond
  %208 = call ptr @moksha_box_string(ptr @93)
  %print_unbox121 = call ptr @moksha_unbox_string(ptr %208)
  %209 = call i32 (ptr, ...) @printf(ptr @94, ptr %print_unbox121)
  store i32 0, ptr %i, align 4
  store i32 0, ptr %i122, align 4
  br label %forcond

next_stmt115:                                     ; preds = %dowhileloop
  %oldval117 = load i32, ptr %d112, align 4
  %inc = add i32 %oldval117, 1
  store i32 %inc, ptr %d112, align 4
  %210 = load i32, ptr @__exception_flag, align 4
  %211 = icmp ne i32 %210, 0
  br i1 %211, label %unwind119, label %next_stmt118

unwind116:                                        ; preds = %dowhileloop
  ret i32 0

next_stmt118:                                     ; preds = %next_stmt115
  br label %dowhilecond

unwind119:                                        ; preds = %next_stmt115
  ret i32 0

forcond:                                          ; preds = %forinc, %dowhileafter
  %212 = load i32, ptr %i122, align 4
  %icmptmp123 = icmp slt i32 %212, 5
  br i1 %icmptmp123, label %forloop, label %forafter

forloop:                                          ; preds = %forcond
  %213 = call ptr @moksha_box_string(ptr @95)
  %214 = call ptr @moksha_box_string(ptr @96)
  %215 = call ptr @moksha_string_concat(ptr %213, ptr %214)
  %216 = load i32, ptr %i122, align 4
  %i2s124 = call ptr @moksha_int_to_str(i32 %216)
  %217 = call ptr @moksha_box_string(ptr %i2s124)
  %218 = call ptr @moksha_string_concat(ptr %215, ptr %217)
  %print_unbox125 = call ptr @moksha_unbox_string(ptr %218)
  %219 = call i32 (ptr, ...) @printf(ptr @97, ptr %print_unbox125)
  %220 = load i32, ptr @__exception_flag, align 4
  %221 = icmp ne i32 %220, 0
  br i1 %221, label %unwind127, label %next_stmt126

forinc:                                           ; preds = %next_stmt126
  %222 = load i32, ptr %i122, align 4
  %addtmp = add i32 %222, 2
  store i32 %addtmp, ptr %i122, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  %223 = call ptr @moksha_box_string(ptr @98)
  %print_unbox128 = call ptr @moksha_unbox_string(ptr %223)
  %224 = call i32 (ptr, ...) @printf(ptr @99, ptr %print_unbox128)
  %225 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 10)
  %226 = call ptr @moksha_array_push_val(ptr %225, ptr %box_i)
  %box_i129 = call ptr @moksha_box_int(i32 20)
  %227 = call ptr @moksha_array_push_val(ptr %225, ptr %box_i129)
  %box_i130 = call ptr @moksha_box_int(i32 30)
  %228 = call ptr @moksha_array_push_val(ptr %225, ptr %box_i130)
  store ptr %225, ptr %numbers, align 8
  store ptr %225, ptr %numbers131, align 8
  %229 = load ptr, ptr %numbers131, align 8
  %len = call i32 @moksha_get_length(ptr %229)
  store i32 0, ptr %forin_idx, align 4
  br label %forin_cond

next_stmt126:                                     ; preds = %forloop
  br label %forinc

unwind127:                                        ; preds = %forloop
  ret i32 0

forin_cond:                                       ; preds = %forin_inc, %forafter
  %230 = load i32, ptr %forin_idx, align 4
  %231 = icmp slt i32 %230, %len
  br i1 %231, label %forin_body, label %forin_after

forin_body:                                       ; preds = %forin_cond
  %arr_val = call ptr @moksha_array_get(ptr %229, i32 %230)
  %unbox_i = call i32 @moksha_unbox_int(ptr %arr_val)
  store i32 %230, ptr %idx, align 4
  store i32 %unbox_i, ptr %val, align 4
  %232 = call ptr @moksha_box_string(ptr @100)
  %233 = call ptr @moksha_box_string(ptr @101)
  %234 = call ptr @moksha_string_concat(ptr %232, ptr %233)
  %235 = load i32, ptr %idx, align 4
  %i2s132 = call ptr @moksha_int_to_str(i32 %235)
  %236 = call ptr @moksha_box_string(ptr %i2s132)
  %237 = call ptr @moksha_string_concat(ptr %234, ptr %236)
  %238 = call ptr @moksha_box_string(ptr @102)
  %239 = call ptr @moksha_string_concat(ptr %237, ptr %238)
  %240 = load i32, ptr %val, align 4
  %i2s133 = call ptr @moksha_int_to_str(i32 %240)
  %241 = call ptr @moksha_box_string(ptr %i2s133)
  %242 = call ptr @moksha_string_concat(ptr %239, ptr %241)
  %print_unbox134 = call ptr @moksha_unbox_string(ptr %242)
  %243 = call i32 (ptr, ...) @printf(ptr @103, ptr %print_unbox134)
  %244 = load i32, ptr @__exception_flag, align 4
  %245 = icmp ne i32 %244, 0
  br i1 %245, label %unwind136, label %next_stmt135

forin_inc:                                        ; preds = %next_stmt135
  %246 = add i32 %230, 1
  store i32 %246, ptr %forin_idx, align 4
  br label %forin_cond

forin_after:                                      ; preds = %forin_cond
  %247 = call ptr @moksha_box_string(ptr @104)
  %print_unbox137 = call ptr @moksha_unbox_string(ptr %247)
  %248 = call i32 (ptr, ...) @printf(ptr @105, ptr %print_unbox137)
  ret i32 0

next_stmt135:                                     ; preds = %forin_body
  br label %forin_inc

unwind136:                                        ; preds = %forin_body
  ret i32 0
}
