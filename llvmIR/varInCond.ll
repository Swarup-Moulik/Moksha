; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [42 x i8] c"==== CONDITIONALS & TRUTHINESS SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [22 x i8] c"\0A[1. IF / ELSE Logic]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [15 x i8] c"if(true): PASS\00", align 1
@5 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@6 = private unnamed_addr constant [15 x i8] c"if(true): FAIL\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@8 = private unnamed_addr constant [16 x i8] c"if(false): FAIL\00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@10 = private unnamed_addr constant [16 x i8] c"if(false): PASS\00", align 1
@11 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@12 = private unnamed_addr constant [12 x i8] c"if(1): PASS\00", align 1
@13 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@14 = private unnamed_addr constant [12 x i8] c"if(1): FAIL\00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@16 = private unnamed_addr constant [12 x i8] c"if(0): FAIL\00", align 1
@17 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@18 = private unnamed_addr constant [12 x i8] c"if(0): PASS\00", align 1
@19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@20 = private unnamed_addr constant [14 x i8] c"if(-42): PASS\00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [14 x i8] c"if(-42): FAIL\00", align 1
@23 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@24 = private unnamed_addr constant [19 x i8] c"if(bool var): PASS\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [18 x i8] c"if(int var): PASS\00", align 1
@27 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@28 = private unnamed_addr constant [19 x i8] c"if(zero var): FAIL\00", align 1
@29 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@30 = private unnamed_addr constant [19 x i8] c"if(zero var): PASS\00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@32 = private unnamed_addr constant [17 x i8] c"\0A[2. WHILE LOOP]\00", align 1
@33 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@34 = private unnamed_addr constant [39 x i8] c"Counting down 3 to 1 using while(int):\00", align 1
@35 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@36 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@37 = private unnamed_addr constant [11 x i8] c"  count = \00", align 1
@38 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@39 = private unnamed_addr constant [22 x i8] c"Testing while(false):\00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@41 = private unnamed_addr constant [25 x i8] c"FAIL: Should not execute\00", align 1
@42 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@43 = private unnamed_addr constant [19 x i8] c"PASS: Loop skipped\00", align 1
@44 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@45 = private unnamed_addr constant [20 x i8] c"\0A[3. DO-WHILE LOOP]\00", align 1
@46 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@47 = private unnamed_addr constant [28 x i8] c"Do-While (false condition):\00", align 1
@48 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@49 = private unnamed_addr constant [27 x i8] c"  PASS: Body executed once\00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@51 = private unnamed_addr constant [39 x i8] c"Do-While using integer logic (2 to 1):\00", align 1
@52 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@53 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@54 = private unnamed_addr constant [7 x i8] c"  d = \00", align 1
@55 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@56 = private unnamed_addr constant [15 x i8] c"\0A[4. FOR LOOP]\00", align 1
@57 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@58 = private unnamed_addr constant [23 x i8] c"Standard loop (i < 3):\00", align 1
@59 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@60 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@61 = private unnamed_addr constant [7 x i8] c"  i = \00", align 1
@62 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@63 = private unnamed_addr constant [47 x i8] c"Loop using integer as condition (j-- until 0):\00", align 1
@64 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@65 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@66 = private unnamed_addr constant [7 x i8] c"  j = \00", align 1
@67 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@68 = private unnamed_addr constant [18 x i8] c"\0A[5. IN OPERATOR]\00", align 1
@69 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@70 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@71 = private unnamed_addr constant [7 x i8] c"Found \00", align 1
@72 = private unnamed_addr constant [16 x i8] c" in array: PASS\00", align 1
@73 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@74 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@75 = private unnamed_addr constant [14 x i8] c"Did not find \00", align 1
@76 = private unnamed_addr constant [16 x i8] c" in array: FAIL\00", align 1
@77 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@78 = private unnamed_addr constant [24 x i8] c"Found 10 in array: PASS\00", align 1
@79 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@80 = private unnamed_addr constant [31 x i8] c"Did not find 10 in array: FAIL\00", align 1
@81 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@82 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@83 = private unnamed_addr constant [7 x i8] c"Found \00", align 1
@84 = private unnamed_addr constant [16 x i8] c" in array: FAIL\00", align 1
@85 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@86 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@87 = private unnamed_addr constant [14 x i8] c"Did not find \00", align 1
@88 = private unnamed_addr constant [16 x i8] c" in array: PASS\00", align 1
@89 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@90 = private unnamed_addr constant [3 x i8] c"id\00", align 1
@91 = private unnamed_addr constant [5 x i8] c"name\00", align 1
@92 = private unnamed_addr constant [7 x i8] c"Moksha\00", align 1
@93 = private unnamed_addr constant [5 x i8] c"name\00", align 1
@94 = private unnamed_addr constant [32 x i8] c"Key 'name' found in table: PASS\00", align 1
@95 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@96 = private unnamed_addr constant [27 x i8] c"Key 'name' not found: FAIL\00", align 1
@97 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@98 = private unnamed_addr constant [24 x i8] c"\0A[6. STRING TRUTHINESS]\00", align 1
@99 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@100 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@101 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@102 = private unnamed_addr constant [18 x i8] c"if('Hello'): PASS\00", align 1
@103 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@104 = private unnamed_addr constant [18 x i8] c"if('Hello'): FAIL\00", align 1
@105 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@106 = private unnamed_addr constant [31 x i8] c"if(''): FAIL (Should be false)\00", align 1
@107 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@108 = private unnamed_addr constant [13 x i8] c"if(''): PASS\00", align 1
@109 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@110 = private unnamed_addr constant [6 x i8] c"Done.\00", align 1
@111 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %sEmpty183 = alloca ptr, align 8
  %sEmpty = alloca ptr, align 8
  %sFull182 = alloca ptr, align 8
  %sFull = alloca ptr, align 8
  %user171 = alloca ptr, align 8
  %user = alloca ptr, align 8
  %in_found158 = alloca i1, align 1
  %in_idx157 = alloca i32, align 4
  %in_found142 = alloca i1, align 1
  %in_idx141 = alloca i32, align 4
  %in_found = alloca i1, align 1
  %in_idx = alloca i32, align 4
  %missing125 = alloca i32, align 4
  %missing = alloca i32, align 4
  %val124 = alloca i32, align 4
  %val = alloca i32, align 4
  %nums123 = alloca ptr, align 8
  %nums = alloca ptr, align 8
  %j113 = alloca i32, align 4
  %j = alloca i32, align 4
  %i102 = alloca i32, align 4
  %i = alloca i32, align 4
  %d87 = alloca i32, align 4
  %d = alloca i32, align 4
  %count67 = alloca i32, align 4
  %count = alloca i32, align 4
  %zVar44 = alloca i32, align 4
  %zVar = alloca i32, align 4
  %iVar43 = alloca i32, align 4
  %iVar = alloca i32, align 4
  %bVar42 = alloca i1, align 1
  %bVar = alloca i1, align 1
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  br i1 true, label %then, label %else

then:                                             ; preds = %entry
  %4 = call ptr @moksha_box_string(ptr @4)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %4)
  %5 = call i32 (ptr, ...) @printf(ptr @5, ptr %print_unbox2)
  %6 = load i32, ptr @__exception_flag, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %unwind, label %next_stmt

else:                                             ; preds = %entry
  %8 = call ptr @moksha_box_string(ptr @6)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %8)
  %9 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox3)
  %10 = load i32, ptr @__exception_flag, align 4
  %11 = icmp ne i32 %10, 0
  br i1 %11, label %unwind5, label %next_stmt4

ifcont:                                           ; preds = %next_stmt4, %next_stmt
  br i1 false, label %then6, label %else7

next_stmt:                                        ; preds = %then
  br label %ifcont

unwind:                                           ; preds = %then
  ret i32 0

next_stmt4:                                       ; preds = %else
  br label %ifcont

unwind5:                                          ; preds = %else
  ret i32 0

then6:                                            ; preds = %ifcont
  %12 = call ptr @moksha_box_string(ptr @8)
  %print_unbox9 = call ptr @moksha_unbox_string(ptr %12)
  %13 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox9)
  %14 = load i32, ptr @__exception_flag, align 4
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %unwind11, label %next_stmt10

else7:                                            ; preds = %ifcont
  %16 = call ptr @moksha_box_string(ptr @10)
  %print_unbox12 = call ptr @moksha_unbox_string(ptr %16)
  %17 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox12)
  %18 = load i32, ptr @__exception_flag, align 4
  %19 = icmp ne i32 %18, 0
  br i1 %19, label %unwind14, label %next_stmt13

ifcont8:                                          ; preds = %next_stmt13, %next_stmt10
  br i1 true, label %then15, label %else16

next_stmt10:                                      ; preds = %then6
  br label %ifcont8

unwind11:                                         ; preds = %then6
  ret i32 0

next_stmt13:                                      ; preds = %else7
  br label %ifcont8

unwind14:                                         ; preds = %else7
  ret i32 0

then15:                                           ; preds = %ifcont8
  %20 = call ptr @moksha_box_string(ptr @12)
  %print_unbox18 = call ptr @moksha_unbox_string(ptr %20)
  %21 = call i32 (ptr, ...) @printf(ptr @13, ptr %print_unbox18)
  %22 = load i32, ptr @__exception_flag, align 4
  %23 = icmp ne i32 %22, 0
  br i1 %23, label %unwind20, label %next_stmt19

else16:                                           ; preds = %ifcont8
  %24 = call ptr @moksha_box_string(ptr @14)
  %print_unbox21 = call ptr @moksha_unbox_string(ptr %24)
  %25 = call i32 (ptr, ...) @printf(ptr @15, ptr %print_unbox21)
  %26 = load i32, ptr @__exception_flag, align 4
  %27 = icmp ne i32 %26, 0
  br i1 %27, label %unwind23, label %next_stmt22

ifcont17:                                         ; preds = %next_stmt22, %next_stmt19
  br i1 false, label %then24, label %else25

next_stmt19:                                      ; preds = %then15
  br label %ifcont17

unwind20:                                         ; preds = %then15
  ret i32 0

next_stmt22:                                      ; preds = %else16
  br label %ifcont17

unwind23:                                         ; preds = %else16
  ret i32 0

then24:                                           ; preds = %ifcont17
  %28 = call ptr @moksha_box_string(ptr @16)
  %print_unbox27 = call ptr @moksha_unbox_string(ptr %28)
  %29 = call i32 (ptr, ...) @printf(ptr @17, ptr %print_unbox27)
  %30 = load i32, ptr @__exception_flag, align 4
  %31 = icmp ne i32 %30, 0
  br i1 %31, label %unwind29, label %next_stmt28

else25:                                           ; preds = %ifcont17
  %32 = call ptr @moksha_box_string(ptr @18)
  %print_unbox30 = call ptr @moksha_unbox_string(ptr %32)
  %33 = call i32 (ptr, ...) @printf(ptr @19, ptr %print_unbox30)
  %34 = load i32, ptr @__exception_flag, align 4
  %35 = icmp ne i32 %34, 0
  br i1 %35, label %unwind32, label %next_stmt31

ifcont26:                                         ; preds = %next_stmt31, %next_stmt28
  br i1 true, label %then33, label %else34

next_stmt28:                                      ; preds = %then24
  br label %ifcont26

unwind29:                                         ; preds = %then24
  ret i32 0

next_stmt31:                                      ; preds = %else25
  br label %ifcont26

unwind32:                                         ; preds = %else25
  ret i32 0

then33:                                           ; preds = %ifcont26
  %36 = call ptr @moksha_box_string(ptr @20)
  %print_unbox36 = call ptr @moksha_unbox_string(ptr %36)
  %37 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox36)
  %38 = load i32, ptr @__exception_flag, align 4
  %39 = icmp ne i32 %38, 0
  br i1 %39, label %unwind38, label %next_stmt37

else34:                                           ; preds = %ifcont26
  %40 = call ptr @moksha_box_string(ptr @22)
  %print_unbox39 = call ptr @moksha_unbox_string(ptr %40)
  %41 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox39)
  %42 = load i32, ptr @__exception_flag, align 4
  %43 = icmp ne i32 %42, 0
  br i1 %43, label %unwind41, label %next_stmt40

ifcont35:                                         ; preds = %next_stmt40, %next_stmt37
  store i1 true, ptr %bVar, align 1
  store i1 true, ptr %bVar42, align 1
  store i32 100, ptr %iVar, align 4
  store i32 100, ptr %iVar43, align 4
  store i32 0, ptr %zVar, align 4
  store i32 0, ptr %zVar44, align 4
  %44 = load i1, ptr %bVar42, align 1
  br i1 %44, label %then45, label %else46

next_stmt37:                                      ; preds = %then33
  br label %ifcont35

unwind38:                                         ; preds = %then33
  ret i32 0

next_stmt40:                                      ; preds = %else34
  br label %ifcont35

unwind41:                                         ; preds = %else34
  ret i32 0

then45:                                           ; preds = %ifcont35
  %45 = call ptr @moksha_box_string(ptr @24)
  %print_unbox48 = call ptr @moksha_unbox_string(ptr %45)
  %46 = call i32 (ptr, ...) @printf(ptr @25, ptr %print_unbox48)
  %47 = load i32, ptr @__exception_flag, align 4
  %48 = icmp ne i32 %47, 0
  br i1 %48, label %unwind50, label %next_stmt49

else46:                                           ; preds = %ifcont35
  br label %ifcont47

ifcont47:                                         ; preds = %else46, %next_stmt49
  %49 = load i32, ptr %iVar43, align 4
  %50 = icmp ne i32 %49, 0
  br i1 %50, label %then51, label %else52

next_stmt49:                                      ; preds = %then45
  br label %ifcont47

unwind50:                                         ; preds = %then45
  ret i32 0

then51:                                           ; preds = %ifcont47
  %51 = call ptr @moksha_box_string(ptr @26)
  %print_unbox54 = call ptr @moksha_unbox_string(ptr %51)
  %52 = call i32 (ptr, ...) @printf(ptr @27, ptr %print_unbox54)
  %53 = load i32, ptr @__exception_flag, align 4
  %54 = icmp ne i32 %53, 0
  br i1 %54, label %unwind56, label %next_stmt55

else52:                                           ; preds = %ifcont47
  br label %ifcont53

ifcont53:                                         ; preds = %else52, %next_stmt55
  %55 = load i32, ptr %zVar44, align 4
  %56 = icmp ne i32 %55, 0
  br i1 %56, label %then57, label %else58

next_stmt55:                                      ; preds = %then51
  br label %ifcont53

unwind56:                                         ; preds = %then51
  ret i32 0

then57:                                           ; preds = %ifcont53
  %57 = call ptr @moksha_box_string(ptr @28)
  %print_unbox60 = call ptr @moksha_unbox_string(ptr %57)
  %58 = call i32 (ptr, ...) @printf(ptr @29, ptr %print_unbox60)
  %59 = load i32, ptr @__exception_flag, align 4
  %60 = icmp ne i32 %59, 0
  br i1 %60, label %unwind62, label %next_stmt61

else58:                                           ; preds = %ifcont53
  %61 = call ptr @moksha_box_string(ptr @30)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %61)
  %62 = call i32 (ptr, ...) @printf(ptr @31, ptr %print_unbox63)
  %63 = load i32, ptr @__exception_flag, align 4
  %64 = icmp ne i32 %63, 0
  br i1 %64, label %unwind65, label %next_stmt64

ifcont59:                                         ; preds = %next_stmt64, %next_stmt61
  %65 = call ptr @moksha_box_string(ptr @32)
  %print_unbox66 = call ptr @moksha_unbox_string(ptr %65)
  %66 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox66)
  store i32 3, ptr %count, align 4
  store i32 3, ptr %count67, align 4
  %67 = call ptr @moksha_box_string(ptr @34)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %67)
  %68 = call i32 (ptr, ...) @printf(ptr @35, ptr %print_unbox68)
  br label %whilecond

next_stmt61:                                      ; preds = %then57
  br label %ifcont59

unwind62:                                         ; preds = %then57
  ret i32 0

next_stmt64:                                      ; preds = %else58
  br label %ifcont59

unwind65:                                         ; preds = %else58
  ret i32 0

whilecond:                                        ; preds = %next_stmt72, %ifcont59
  %69 = load i32, ptr %count67, align 4
  %70 = icmp ne i32 %69, 0
  br i1 %70, label %whileloop, label %whileafter

whileloop:                                        ; preds = %whilecond
  %71 = call ptr @moksha_box_string(ptr @36)
  %72 = call ptr @moksha_box_string(ptr @37)
  %73 = call ptr @moksha_string_concat(ptr %71, ptr %72)
  %74 = load i32, ptr %count67, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %74)
  %75 = call ptr @moksha_box_string(ptr %i2s)
  %76 = call ptr @moksha_string_concat(ptr %73, ptr %75)
  %print_unbox69 = call ptr @moksha_unbox_string(ptr %76)
  %77 = call i32 (ptr, ...) @printf(ptr @38, ptr %print_unbox69)
  %78 = load i32, ptr @__exception_flag, align 4
  %79 = icmp ne i32 %78, 0
  br i1 %79, label %unwind71, label %next_stmt70

whileafter:                                       ; preds = %whilecond
  %80 = call ptr @moksha_box_string(ptr @39)
  %print_unbox74 = call ptr @moksha_unbox_string(ptr %80)
  %81 = call i32 (ptr, ...) @printf(ptr @40, ptr %print_unbox74)
  br label %whilecond75

next_stmt70:                                      ; preds = %whileloop
  %oldval = load i32, ptr %count67, align 4
  %dec = sub i32 %oldval, 1
  store i32 %dec, ptr %count67, align 4
  %82 = load i32, ptr @__exception_flag, align 4
  %83 = icmp ne i32 %82, 0
  br i1 %83, label %unwind73, label %next_stmt72

unwind71:                                         ; preds = %whileloop
  ret i32 0

next_stmt72:                                      ; preds = %next_stmt70
  br label %whilecond

unwind73:                                         ; preds = %next_stmt70
  ret i32 0

whilecond75:                                      ; preds = %next_stmt79, %whileafter
  br i1 false, label %whileloop76, label %whileafter77

whileloop76:                                      ; preds = %whilecond75
  %84 = call ptr @moksha_box_string(ptr @41)
  %print_unbox78 = call ptr @moksha_unbox_string(ptr %84)
  %85 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox78)
  %86 = load i32, ptr @__exception_flag, align 4
  %87 = icmp ne i32 %86, 0
  br i1 %87, label %unwind80, label %next_stmt79

whileafter77:                                     ; preds = %whilecond75
  %88 = call ptr @moksha_box_string(ptr @43)
  %print_unbox81 = call ptr @moksha_unbox_string(ptr %88)
  %89 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox81)
  %90 = call ptr @moksha_box_string(ptr @45)
  %print_unbox82 = call ptr @moksha_unbox_string(ptr %90)
  %91 = call i32 (ptr, ...) @printf(ptr @46, ptr %print_unbox82)
  %92 = call ptr @moksha_box_string(ptr @47)
  %print_unbox83 = call ptr @moksha_unbox_string(ptr %92)
  %93 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox83)
  br label %dowhileloop

next_stmt79:                                      ; preds = %whileloop76
  br label %whilecond75

unwind80:                                         ; preds = %whileloop76
  ret i32 0

dowhileloop:                                      ; preds = %dowhilecond, %whileafter77
  %94 = call ptr @moksha_box_string(ptr @49)
  %print_unbox84 = call ptr @moksha_unbox_string(ptr %94)
  %95 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox84)
  %96 = load i32, ptr @__exception_flag, align 4
  %97 = icmp ne i32 %96, 0
  br i1 %97, label %unwind86, label %next_stmt85

dowhilecond:                                      ; preds = %next_stmt85
  br i1 false, label %dowhileloop, label %dowhileafter

dowhileafter:                                     ; preds = %dowhilecond
  store i32 2, ptr %d, align 4
  store i32 2, ptr %d87, align 4
  %98 = call ptr @moksha_box_string(ptr @51)
  %print_unbox88 = call ptr @moksha_unbox_string(ptr %98)
  %99 = call i32 (ptr, ...) @printf(ptr @52, ptr %print_unbox88)
  br label %dowhileloop89

next_stmt85:                                      ; preds = %dowhileloop
  br label %dowhilecond

unwind86:                                         ; preds = %dowhileloop
  ret i32 0

dowhileloop89:                                    ; preds = %dowhilecond90, %dowhileafter
  %100 = call ptr @moksha_box_string(ptr @53)
  %101 = call ptr @moksha_box_string(ptr @54)
  %102 = call ptr @moksha_string_concat(ptr %100, ptr %101)
  %103 = load i32, ptr %d87, align 4
  %i2s92 = call ptr @moksha_int_to_str(i32 %103)
  %104 = call ptr @moksha_box_string(ptr %i2s92)
  %105 = call ptr @moksha_string_concat(ptr %102, ptr %104)
  %print_unbox93 = call ptr @moksha_unbox_string(ptr %105)
  %106 = call i32 (ptr, ...) @printf(ptr @55, ptr %print_unbox93)
  %107 = load i32, ptr @__exception_flag, align 4
  %108 = icmp ne i32 %107, 0
  br i1 %108, label %unwind95, label %next_stmt94

dowhilecond90:                                    ; preds = %next_stmt98
  %109 = load i32, ptr %d87, align 4
  %110 = icmp ne i32 %109, 0
  br i1 %110, label %dowhileloop89, label %dowhileafter91

dowhileafter91:                                   ; preds = %dowhilecond90
  %111 = call ptr @moksha_box_string(ptr @56)
  %print_unbox100 = call ptr @moksha_unbox_string(ptr %111)
  %112 = call i32 (ptr, ...) @printf(ptr @57, ptr %print_unbox100)
  %113 = call ptr @moksha_box_string(ptr @58)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %113)
  %114 = call i32 (ptr, ...) @printf(ptr @59, ptr %print_unbox101)
  store i32 0, ptr %i, align 4
  store i32 0, ptr %i102, align 4
  br label %forcond

next_stmt94:                                      ; preds = %dowhileloop89
  %oldval96 = load i32, ptr %d87, align 4
  %dec97 = sub i32 %oldval96, 1
  store i32 %dec97, ptr %d87, align 4
  %115 = load i32, ptr @__exception_flag, align 4
  %116 = icmp ne i32 %115, 0
  br i1 %116, label %unwind99, label %next_stmt98

unwind95:                                         ; preds = %dowhileloop89
  ret i32 0

next_stmt98:                                      ; preds = %next_stmt94
  br label %dowhilecond90

unwind99:                                         ; preds = %next_stmt94
  ret i32 0

forcond:                                          ; preds = %forinc, %dowhileafter91
  %117 = load i32, ptr %i102, align 4
  %icmptmp = icmp slt i32 %117, 3
  br i1 %icmptmp, label %forloop, label %forafter

forloop:                                          ; preds = %forcond
  %118 = call ptr @moksha_box_string(ptr @60)
  %119 = call ptr @moksha_box_string(ptr @61)
  %120 = call ptr @moksha_string_concat(ptr %118, ptr %119)
  %121 = load i32, ptr %i102, align 4
  %i2s103 = call ptr @moksha_int_to_str(i32 %121)
  %122 = call ptr @moksha_box_string(ptr %i2s103)
  %123 = call ptr @moksha_string_concat(ptr %120, ptr %122)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %123)
  %124 = call i32 (ptr, ...) @printf(ptr @62, ptr %print_unbox104)
  %125 = load i32, ptr @__exception_flag, align 4
  %126 = icmp ne i32 %125, 0
  br i1 %126, label %unwind106, label %next_stmt105

forinc:                                           ; preds = %next_stmt105
  %oldval107 = load i32, ptr %i102, align 4
  %inc = add i32 %oldval107, 1
  store i32 %inc, ptr %i102, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  %127 = call ptr @moksha_box_string(ptr @63)
  %print_unbox108 = call ptr @moksha_unbox_string(ptr %127)
  %128 = call i32 (ptr, ...) @printf(ptr @64, ptr %print_unbox108)
  store i32 2, ptr %j, align 4
  store i32 2, ptr %j113, align 4
  br label %forcond109

next_stmt105:                                     ; preds = %forloop
  br label %forinc

unwind106:                                        ; preds = %forloop
  ret i32 0

forcond109:                                       ; preds = %forinc111, %forafter
  %129 = load i32, ptr %j113, align 4
  %130 = icmp ne i32 %129, 0
  br i1 %130, label %forloop110, label %forafter112

forloop110:                                       ; preds = %forcond109
  %131 = call ptr @moksha_box_string(ptr @65)
  %132 = call ptr @moksha_box_string(ptr @66)
  %133 = call ptr @moksha_string_concat(ptr %131, ptr %132)
  %134 = load i32, ptr %j113, align 4
  %i2s114 = call ptr @moksha_int_to_str(i32 %134)
  %135 = call ptr @moksha_box_string(ptr %i2s114)
  %136 = call ptr @moksha_string_concat(ptr %133, ptr %135)
  %print_unbox115 = call ptr @moksha_unbox_string(ptr %136)
  %137 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox115)
  %138 = load i32, ptr @__exception_flag, align 4
  %139 = icmp ne i32 %138, 0
  br i1 %139, label %unwind117, label %next_stmt116

forinc111:                                        ; preds = %next_stmt116
  %oldval118 = load i32, ptr %j113, align 4
  %dec119 = sub i32 %oldval118, 1
  store i32 %dec119, ptr %j113, align 4
  br label %forcond109

forafter112:                                      ; preds = %forcond109
  %140 = call ptr @moksha_box_string(ptr @68)
  %print_unbox120 = call ptr @moksha_unbox_string(ptr %140)
  %141 = call i32 (ptr, ...) @printf(ptr @69, ptr %print_unbox120)
  %142 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 10)
  %143 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i)
  %box_i121 = call ptr @moksha_box_int(i32 20)
  %144 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i121)
  %box_i122 = call ptr @moksha_box_int(i32 30)
  %145 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i122)
  store ptr %142, ptr %nums, align 8
  store ptr %142, ptr %nums123, align 8
  store i32 20, ptr %val, align 4
  store i32 20, ptr %val124, align 4
  store i32 99, ptr %missing, align 4
  store i32 99, ptr %missing125, align 4
  %146 = load i32, ptr %val124, align 4
  %147 = load ptr, ptr %nums123, align 8
  %148 = call i32 @moksha_get_length(ptr %147)
  store i32 0, ptr %in_idx, align 4
  store i1 false, ptr %in_found, align 1
  br label %in_cond

next_stmt116:                                     ; preds = %forloop110
  br label %forinc111

unwind117:                                        ; preds = %forloop110
  ret i32 0

in_cond:                                          ; preds = %in_inc, %forafter112
  %149 = load i32, ptr %in_idx, align 4
  %150 = icmp slt i32 %149, %148
  br i1 %150, label %in_body, label %in_end

in_body:                                          ; preds = %in_cond
  %151 = call ptr @moksha_array_get(ptr %147, i32 %149)
  %152 = call i32 @moksha_unbox_int(ptr %151)
  %153 = icmp eq i32 %146, %152
  br i1 %153, label %in_match, label %in_inc

in_inc:                                           ; preds = %in_body
  %154 = add i32 %149, 1
  store i32 %154, ptr %in_idx, align 4
  br label %in_cond

in_end:                                           ; preds = %in_match, %in_cond
  %155 = load i1, ptr %in_found, align 1
  br i1 %155, label %then126, label %else127

in_match:                                         ; preds = %in_body
  store i1 true, ptr %in_found, align 1
  br label %in_end

then126:                                          ; preds = %in_end
  %156 = call ptr @moksha_box_string(ptr @70)
  %157 = call ptr @moksha_box_string(ptr @71)
  %158 = call ptr @moksha_string_concat(ptr %156, ptr %157)
  %159 = load i32, ptr %val124, align 4
  %i2s129 = call ptr @moksha_int_to_str(i32 %159)
  %160 = call ptr @moksha_box_string(ptr %i2s129)
  %161 = call ptr @moksha_string_concat(ptr %158, ptr %160)
  %162 = call ptr @moksha_box_string(ptr @72)
  %163 = call ptr @moksha_string_concat(ptr %161, ptr %162)
  %print_unbox130 = call ptr @moksha_unbox_string(ptr %163)
  %164 = call i32 (ptr, ...) @printf(ptr @73, ptr %print_unbox130)
  %165 = load i32, ptr @__exception_flag, align 4
  %166 = icmp ne i32 %165, 0
  br i1 %166, label %unwind132, label %next_stmt131

else127:                                          ; preds = %in_end
  %167 = call ptr @moksha_box_string(ptr @74)
  %168 = call ptr @moksha_box_string(ptr @75)
  %169 = call ptr @moksha_string_concat(ptr %167, ptr %168)
  %170 = load i32, ptr %val124, align 4
  %i2s133 = call ptr @moksha_int_to_str(i32 %170)
  %171 = call ptr @moksha_box_string(ptr %i2s133)
  %172 = call ptr @moksha_string_concat(ptr %169, ptr %171)
  %173 = call ptr @moksha_box_string(ptr @76)
  %174 = call ptr @moksha_string_concat(ptr %172, ptr %173)
  %print_unbox134 = call ptr @moksha_unbox_string(ptr %174)
  %175 = call i32 (ptr, ...) @printf(ptr @77, ptr %print_unbox134)
  %176 = load i32, ptr @__exception_flag, align 4
  %177 = icmp ne i32 %176, 0
  br i1 %177, label %unwind136, label %next_stmt135

ifcont128:                                        ; preds = %next_stmt135, %next_stmt131
  %178 = load ptr, ptr %nums123, align 8
  %179 = call i32 @moksha_get_length(ptr %178)
  store i32 0, ptr %in_idx141, align 4
  store i1 false, ptr %in_found142, align 1
  br label %in_cond137

next_stmt131:                                     ; preds = %then126
  br label %ifcont128

unwind132:                                        ; preds = %then126
  ret i32 0

next_stmt135:                                     ; preds = %else127
  br label %ifcont128

unwind136:                                        ; preds = %else127
  ret i32 0

in_cond137:                                       ; preds = %in_inc139, %ifcont128
  %180 = load i32, ptr %in_idx141, align 4
  %181 = icmp slt i32 %180, %179
  br i1 %181, label %in_body138, label %in_end140

in_body138:                                       ; preds = %in_cond137
  %182 = call ptr @moksha_array_get(ptr %178, i32 %180)
  %183 = call i32 @moksha_unbox_int(ptr %182)
  %184 = icmp eq i32 10, %183
  br i1 %184, label %in_match143, label %in_inc139

in_inc139:                                        ; preds = %in_body138
  %185 = add i32 %180, 1
  store i32 %185, ptr %in_idx141, align 4
  br label %in_cond137

in_end140:                                        ; preds = %in_match143, %in_cond137
  %186 = load i1, ptr %in_found142, align 1
  br i1 %186, label %then144, label %else145

in_match143:                                      ; preds = %in_body138
  store i1 true, ptr %in_found142, align 1
  br label %in_end140

then144:                                          ; preds = %in_end140
  %187 = call ptr @moksha_box_string(ptr @78)
  %print_unbox147 = call ptr @moksha_unbox_string(ptr %187)
  %188 = call i32 (ptr, ...) @printf(ptr @79, ptr %print_unbox147)
  %189 = load i32, ptr @__exception_flag, align 4
  %190 = icmp ne i32 %189, 0
  br i1 %190, label %unwind149, label %next_stmt148

else145:                                          ; preds = %in_end140
  %191 = call ptr @moksha_box_string(ptr @80)
  %print_unbox150 = call ptr @moksha_unbox_string(ptr %191)
  %192 = call i32 (ptr, ...) @printf(ptr @81, ptr %print_unbox150)
  %193 = load i32, ptr @__exception_flag, align 4
  %194 = icmp ne i32 %193, 0
  br i1 %194, label %unwind152, label %next_stmt151

ifcont146:                                        ; preds = %next_stmt151, %next_stmt148
  %195 = load i32, ptr %missing125, align 4
  %196 = load ptr, ptr %nums123, align 8
  %197 = call i32 @moksha_get_length(ptr %196)
  store i32 0, ptr %in_idx157, align 4
  store i1 false, ptr %in_found158, align 1
  br label %in_cond153

next_stmt148:                                     ; preds = %then144
  br label %ifcont146

unwind149:                                        ; preds = %then144
  ret i32 0

next_stmt151:                                     ; preds = %else145
  br label %ifcont146

unwind152:                                        ; preds = %else145
  ret i32 0

in_cond153:                                       ; preds = %in_inc155, %ifcont146
  %198 = load i32, ptr %in_idx157, align 4
  %199 = icmp slt i32 %198, %197
  br i1 %199, label %in_body154, label %in_end156

in_body154:                                       ; preds = %in_cond153
  %200 = call ptr @moksha_array_get(ptr %196, i32 %198)
  %201 = call i32 @moksha_unbox_int(ptr %200)
  %202 = icmp eq i32 %195, %201
  br i1 %202, label %in_match159, label %in_inc155

in_inc155:                                        ; preds = %in_body154
  %203 = add i32 %198, 1
  store i32 %203, ptr %in_idx157, align 4
  br label %in_cond153

in_end156:                                        ; preds = %in_match159, %in_cond153
  %204 = load i1, ptr %in_found158, align 1
  br i1 %204, label %then160, label %else161

in_match159:                                      ; preds = %in_body154
  store i1 true, ptr %in_found158, align 1
  br label %in_end156

then160:                                          ; preds = %in_end156
  %205 = call ptr @moksha_box_string(ptr @82)
  %206 = call ptr @moksha_box_string(ptr @83)
  %207 = call ptr @moksha_string_concat(ptr %205, ptr %206)
  %208 = load i32, ptr %missing125, align 4
  %i2s163 = call ptr @moksha_int_to_str(i32 %208)
  %209 = call ptr @moksha_box_string(ptr %i2s163)
  %210 = call ptr @moksha_string_concat(ptr %207, ptr %209)
  %211 = call ptr @moksha_box_string(ptr @84)
  %212 = call ptr @moksha_string_concat(ptr %210, ptr %211)
  %print_unbox164 = call ptr @moksha_unbox_string(ptr %212)
  %213 = call i32 (ptr, ...) @printf(ptr @85, ptr %print_unbox164)
  %214 = load i32, ptr @__exception_flag, align 4
  %215 = icmp ne i32 %214, 0
  br i1 %215, label %unwind166, label %next_stmt165

else161:                                          ; preds = %in_end156
  %216 = call ptr @moksha_box_string(ptr @86)
  %217 = call ptr @moksha_box_string(ptr @87)
  %218 = call ptr @moksha_string_concat(ptr %216, ptr %217)
  %219 = load i32, ptr %missing125, align 4
  %i2s167 = call ptr @moksha_int_to_str(i32 %219)
  %220 = call ptr @moksha_box_string(ptr %i2s167)
  %221 = call ptr @moksha_string_concat(ptr %218, ptr %220)
  %222 = call ptr @moksha_box_string(ptr @88)
  %223 = call ptr @moksha_string_concat(ptr %221, ptr %222)
  %print_unbox168 = call ptr @moksha_unbox_string(ptr %223)
  %224 = call i32 (ptr, ...) @printf(ptr @89, ptr %print_unbox168)
  %225 = load i32, ptr @__exception_flag, align 4
  %226 = icmp ne i32 %225, 0
  br i1 %226, label %unwind170, label %next_stmt169

ifcont162:                                        ; preds = %next_stmt169, %next_stmt165
  %227 = call ptr @moksha_alloc_table(i32 2)
  %228 = call ptr @moksha_box_string(ptr @90)
  %229 = call ptr @moksha_box_int(i32 1)
  %230 = call ptr @moksha_table_set(ptr %227, ptr %228, ptr %229)
  %231 = call ptr @moksha_box_string(ptr @91)
  %232 = call ptr @moksha_box_string(ptr @92)
  %233 = call ptr @moksha_table_set(ptr %227, ptr %231, ptr %232)
  store ptr %227, ptr %user, align 8
  store ptr %227, ptr %user171, align 8
  %234 = call ptr @moksha_box_string(ptr @93)
  %235 = load ptr, ptr %user171, align 8
  %236 = call ptr @moksha_table_get(ptr %235, ptr %234)
  %237 = icmp ne ptr %236, null
  br i1 %237, label %then172, label %else173

next_stmt165:                                     ; preds = %then160
  br label %ifcont162

unwind166:                                        ; preds = %then160
  ret i32 0

next_stmt169:                                     ; preds = %else161
  br label %ifcont162

unwind170:                                        ; preds = %else161
  ret i32 0

then172:                                          ; preds = %ifcont162
  %238 = call ptr @moksha_box_string(ptr @94)
  %print_unbox175 = call ptr @moksha_unbox_string(ptr %238)
  %239 = call i32 (ptr, ...) @printf(ptr @95, ptr %print_unbox175)
  %240 = load i32, ptr @__exception_flag, align 4
  %241 = icmp ne i32 %240, 0
  br i1 %241, label %unwind177, label %next_stmt176

else173:                                          ; preds = %ifcont162
  %242 = call ptr @moksha_box_string(ptr @96)
  %print_unbox178 = call ptr @moksha_unbox_string(ptr %242)
  %243 = call i32 (ptr, ...) @printf(ptr @97, ptr %print_unbox178)
  %244 = load i32, ptr @__exception_flag, align 4
  %245 = icmp ne i32 %244, 0
  br i1 %245, label %unwind180, label %next_stmt179

ifcont174:                                        ; preds = %next_stmt179, %next_stmt176
  %246 = call ptr @moksha_box_string(ptr @98)
  %print_unbox181 = call ptr @moksha_unbox_string(ptr %246)
  %247 = call i32 (ptr, ...) @printf(ptr @99, ptr %print_unbox181)
  %248 = call ptr @moksha_box_string(ptr @100)
  store ptr %248, ptr %sFull, align 8
  store ptr %248, ptr %sFull182, align 8
  %249 = call ptr @moksha_box_string(ptr @101)
  store ptr %249, ptr %sEmpty, align 8
  store ptr %249, ptr %sEmpty183, align 8
  %250 = load ptr, ptr %sFull182, align 8
  %251 = call i32 @moksha_strlen(ptr %250)
  %str_truth = icmp ne i32 %251, 0
  br i1 %str_truth, label %then184, label %else185

next_stmt176:                                     ; preds = %then172
  br label %ifcont174

unwind177:                                        ; preds = %then172
  ret i32 0

next_stmt179:                                     ; preds = %else173
  br label %ifcont174

unwind180:                                        ; preds = %else173
  ret i32 0

then184:                                          ; preds = %ifcont174
  %252 = call ptr @moksha_box_string(ptr @102)
  %print_unbox187 = call ptr @moksha_unbox_string(ptr %252)
  %253 = call i32 (ptr, ...) @printf(ptr @103, ptr %print_unbox187)
  %254 = load i32, ptr @__exception_flag, align 4
  %255 = icmp ne i32 %254, 0
  br i1 %255, label %unwind189, label %next_stmt188

else185:                                          ; preds = %ifcont174
  %256 = call ptr @moksha_box_string(ptr @104)
  %print_unbox190 = call ptr @moksha_unbox_string(ptr %256)
  %257 = call i32 (ptr, ...) @printf(ptr @105, ptr %print_unbox190)
  %258 = load i32, ptr @__exception_flag, align 4
  %259 = icmp ne i32 %258, 0
  br i1 %259, label %unwind192, label %next_stmt191

ifcont186:                                        ; preds = %next_stmt191, %next_stmt188
  %260 = load ptr, ptr %sEmpty183, align 8
  %261 = call i32 @moksha_strlen(ptr %260)
  %str_truth193 = icmp ne i32 %261, 0
  br i1 %str_truth193, label %then194, label %else195

next_stmt188:                                     ; preds = %then184
  br label %ifcont186

unwind189:                                        ; preds = %then184
  ret i32 0

next_stmt191:                                     ; preds = %else185
  br label %ifcont186

unwind192:                                        ; preds = %else185
  ret i32 0

then194:                                          ; preds = %ifcont186
  %262 = call ptr @moksha_box_string(ptr @106)
  %print_unbox197 = call ptr @moksha_unbox_string(ptr %262)
  %263 = call i32 (ptr, ...) @printf(ptr @107, ptr %print_unbox197)
  %264 = load i32, ptr @__exception_flag, align 4
  %265 = icmp ne i32 %264, 0
  br i1 %265, label %unwind199, label %next_stmt198

else195:                                          ; preds = %ifcont186
  %266 = call ptr @moksha_box_string(ptr @108)
  %print_unbox200 = call ptr @moksha_unbox_string(ptr %266)
  %267 = call i32 (ptr, ...) @printf(ptr @109, ptr %print_unbox200)
  %268 = load i32, ptr @__exception_flag, align 4
  %269 = icmp ne i32 %268, 0
  br i1 %269, label %unwind202, label %next_stmt201

ifcont196:                                        ; preds = %next_stmt201, %next_stmt198
  %270 = call ptr @moksha_box_string(ptr @110)
  %print_unbox203 = call ptr @moksha_unbox_string(ptr %270)
  %271 = call i32 (ptr, ...) @printf(ptr @111, ptr %print_unbox203)
  ret i32 0

next_stmt198:                                     ; preds = %then194
  br label %ifcont196

unwind199:                                        ; preds = %then194
  ret i32 0

next_stmt201:                                     ; preds = %else195
  br label %ifcont196

unwind202:                                        ; preds = %else195
  ret i32 0
}
