; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [18 x i8] c"\0AEnter a string: \00", align 1
@1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@2 = private unnamed_addr constant [19 x i8] c"Enter array size: \00", align 1
@3 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@4 = private unnamed_addr constant [7 x i8] c"Enter \00", align 1
@5 = private unnamed_addr constant [22 x i8] c" words for the array:\00", align 1
@6 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"ar[\00", align 1
@8 = private unnamed_addr constant [4 x i8] c"]: \00", align 1
@9 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@10 = private unnamed_addr constant [21 x i8] c"\0AEnter a table key: \00", align 1
@11 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@12 = private unnamed_addr constant [17 x i8] c"Enter value for \00", align 1
@13 = private unnamed_addr constant [3 x i8] c": \00", align 1
@14 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@15 = private unnamed_addr constant [13 x i8] c"unchangeable\00", align 1
@16 = private unnamed_addr constant [6 x i8] c"fixed\00", align 1
@17 = private unnamed_addr constant [22 x i8] c"\0A--- Loop Display ---\00", align 1
@18 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@19 = private unnamed_addr constant [23 x i8] c"Pretty Print Array :- \00", align 1
@20 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@21 = private unnamed_addr constant [28 x i8] c"\0AArray (For-In Value Only):\00", align 1
@22 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@23 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@24 = private unnamed_addr constant [31 x i8] c"\0AArray (For-In Index & Value):\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [2 x i8] c"[\00", align 1
@27 = private unnamed_addr constant [5 x i8] c"] = \00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@29 = private unnamed_addr constant [23 x i8] c"\0AArray (For-In Value):\00", align 1
@30 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@31 = private unnamed_addr constant [11 x i8] c"Element = \00", align 1
@32 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@33 = private unnamed_addr constant [21 x i8] c"\0ATable (Key, Value):\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@35 = private unnamed_addr constant [6 x i8] c"Key: \00", align 1
@36 = private unnamed_addr constant [8 x i8] c", Val: \00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@38 = private unnamed_addr constant [14 x i8] c"\0ATable (Key):\00", align 1
@39 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@40 = private unnamed_addr constant [6 x i8] c"Key: \00", align 1
@41 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@42 = private unnamed_addr constant [18 x i8] c"\0AString (For-In):\00", align 1
@43 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@44 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@45 = private unnamed_addr constant [28 x i8] c"\0A--- Complex Assignment ---\00", align 1
@46 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@47 = private unnamed_addr constant [23 x i8] c"4th letter of string: \00", align 1
@48 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@49 = private unnamed_addr constant [18 x i8] c"Assigned ar[3] = \00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@51 = private unnamed_addr constant [35 x i8] c"String too short for index 3 test.\00", align 1
@52 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@53 = private unnamed_addr constant [10 x i8] c"fromArray\00", align 1
@54 = private unnamed_addr constant [29 x i8] c"Assigned tab['fromArray'] = \00", align 1
@55 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@56 = private unnamed_addr constant [9 x i8] c"Table : \00", align 1
@57 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@58 = private unnamed_addr constant [19 x i8] c"Length of array : \00", align 1
@59 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@60 = private unnamed_addr constant [18 x i8] c"\0A--- Deletion ---\00", align 1
@61 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@62 = private unnamed_addr constant [17 x i8] c"Deleting ar[3] (\00", align 1
@63 = private unnamed_addr constant [5 x i8] c")...\00", align 1
@64 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@65 = private unnamed_addr constant [21 x i8] c"Array after delete: \00", align 1
@66 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@67 = private unnamed_addr constant [19 x i8] c"Length of array : \00", align 1
@68 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@69 = private unnamed_addr constant [14 x i8] c"Deleting tab[\00", align 1
@70 = private unnamed_addr constant [5 x i8] c"]...\00", align 1
@71 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@72 = private unnamed_addr constant [21 x i8] c"Table after delete: \00", align 1
@73 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@74 = private unnamed_addr constant [24 x i8] c"\0A=== TEST COMPLETED ===\00", align 1
@75 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %valAr111 = alloca ptr, align 8
  %valAr = alloca ptr, align 8
  %ch = alloca ptr, align 8
  %forin_idx74 = alloca i32, align 4
  %key68 = alloca ptr, align 8
  %forin_idx61 = alloca i32, align 4
  %val54 = alloca ptr, align 8
  %key = alloca ptr, align 8
  %forin_idx49 = alloca i32, align 4
  %val43 = alloca ptr, align 8
  %forin_idx37 = alloca i32, align 4
  %val31 = alloca ptr, align 8
  %idx = alloca i32, align 4
  %forin_idx25 = alloca i32, align 4
  %val = alloca ptr, align 8
  %forin_idx = alloca i32, align 4
  %k14 = alloca ptr, align 8
  %k = alloca ptr, align 8
  %tab11 = alloca ptr, align 8
  %tab = alloca ptr, align 8
  %i6 = alloca i32, align 4
  %i = alloca i32, align 4
  %ar4 = alloca ptr, align 8
  %ar = alloca ptr, align 8
  %size3 = alloca i32, align 4
  %size = alloca i32, align 4
  %w1 = alloca ptr, align 8
  %w = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %read_s = call ptr @moksha_read_string()
  store ptr %read_s, ptr %w, align 8
  store ptr %read_s, ptr %w1, align 8
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox2)
  %read_i = call i32 @moksha_read_int()
  store i32 %read_i, ptr %size, align 4
  store i32 %read_i, ptr %size3, align 4
  %4 = load i32, ptr %size3, align 4
  %arr_leaf = call ptr @moksha_alloc_array_fill(i32 %4, ptr null)
  store ptr %arr_leaf, ptr %ar, align 8
  store ptr %arr_leaf, ptr %ar4, align 8
  %5 = call ptr @moksha_box_string(ptr @4)
  %6 = load i32, ptr %size3, align 4
  %7 = call ptr @moksha_int_to_str(i32 %6)
  %8 = call ptr @moksha_box_string(ptr %7)
  %9 = call ptr @moksha_string_concat(ptr %5, ptr %8)
  %10 = call ptr @moksha_box_string(ptr @5)
  %11 = call ptr @moksha_string_concat(ptr %9, ptr %10)
  %print_unbox5 = call ptr @moksha_unbox_string(ptr %11)
  %12 = call i32 (ptr, ...) @printf(ptr @6, ptr %print_unbox5)
  store i32 0, ptr %i, align 4
  store i32 0, ptr %i6, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %13 = load i32, ptr %i6, align 4
  %14 = load i32, ptr %size3, align 4
  %icmptmp = icmp slt i32 %13, %14
  br i1 %icmptmp, label %forloop, label %forafter

forloop:                                          ; preds = %forcond
  %15 = call ptr @moksha_box_string(ptr @7)
  %16 = load i32, ptr %i6, align 4
  %17 = call ptr @moksha_int_to_str(i32 %16)
  %18 = call ptr @moksha_box_string(ptr %17)
  %19 = call ptr @moksha_string_concat(ptr %15, ptr %18)
  %20 = call ptr @moksha_box_string(ptr @8)
  %21 = call ptr @moksha_string_concat(ptr %19, ptr %20)
  %print_unbox7 = call ptr @moksha_unbox_string(ptr %21)
  %22 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox7)
  %23 = load i32, ptr @__exception_flag, align 4
  %24 = icmp ne i32 %23, 0
  br i1 %24, label %unwind, label %next_stmt

forinc:                                           ; preds = %next_stmt9
  %oldval = load i32, ptr %i6, align 4
  %inc = add i32 %oldval, 1
  store i32 %inc, ptr %i6, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  %25 = call ptr @moksha_alloc_table(i32 0)
  store ptr %25, ptr %tab, align 8
  store ptr %25, ptr %tab11, align 8
  %26 = call ptr @moksha_box_string(ptr @10)
  %print_unbox12 = call ptr @moksha_unbox_string(ptr %26)
  %27 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox12)
  %read_s13 = call ptr @moksha_read_string()
  store ptr %read_s13, ptr %k, align 8
  store ptr %read_s13, ptr %k14, align 8
  %28 = call ptr @moksha_box_string(ptr @12)
  %29 = load ptr, ptr %k14, align 8
  %30 = call ptr @moksha_string_concat(ptr %28, ptr %29)
  %31 = call ptr @moksha_box_string(ptr @13)
  %32 = call ptr @moksha_string_concat(ptr %30, ptr %31)
  %print_unbox15 = call ptr @moksha_unbox_string(ptr %32)
  %33 = call i32 (ptr, ...) @printf(ptr @14, ptr %print_unbox15)
  %read_s16 = call ptr @moksha_read_string()
  %34 = load ptr, ptr %tab11, align 8
  %35 = load ptr, ptr %k14, align 8
  %36 = call ptr @moksha_table_set(ptr %34, ptr %35, ptr %read_s16)
  %37 = call ptr @moksha_box_string(ptr @15)
  %38 = load ptr, ptr %tab11, align 8
  %39 = call ptr @moksha_box_string(ptr @16)
  %40 = call ptr @moksha_table_set(ptr %38, ptr %39, ptr %37)
  %41 = call ptr @moksha_box_string(ptr @17)
  %print_unbox17 = call ptr @moksha_unbox_string(ptr %41)
  %42 = call i32 (ptr, ...) @printf(ptr @18, ptr %print_unbox17)
  %43 = call ptr @moksha_box_string(ptr @19)
  %44 = load ptr, ptr %ar4, align 8
  %45 = call ptr @moksha_any_to_str(ptr %44)
  %46 = call ptr @moksha_box_string(ptr %45)
  %47 = call ptr @moksha_string_concat(ptr %43, ptr %46)
  %print_unbox18 = call ptr @moksha_unbox_string(ptr %47)
  %48 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox18)
  %49 = call ptr @moksha_box_string(ptr @21)
  %print_unbox19 = call ptr @moksha_unbox_string(ptr %49)
  %50 = call i32 (ptr, ...) @printf(ptr @22, ptr %print_unbox19)
  %51 = load ptr, ptr %ar4, align 8
  %len = call i32 @moksha_get_length(ptr %51)
  store i32 0, ptr %forin_idx, align 4
  br label %forin_cond

next_stmt:                                        ; preds = %forloop
  %read_s8 = call ptr @moksha_read_string()
  %52 = load ptr, ptr %ar4, align 8
  %53 = load i32, ptr %i6, align 4
  call void @moksha_array_set(ptr %52, i32 %53, ptr %read_s8)
  %54 = load i32, ptr @__exception_flag, align 4
  %55 = icmp ne i32 %54, 0
  br i1 %55, label %unwind10, label %next_stmt9

unwind:                                           ; preds = %forloop
  ret i32 0

next_stmt9:                                       ; preds = %next_stmt
  br label %forinc

unwind10:                                         ; preds = %next_stmt
  ret i32 0

forin_cond:                                       ; preds = %forin_inc, %forafter
  %56 = load i32, ptr %forin_idx, align 4
  %57 = icmp slt i32 %56, %len
  br i1 %57, label %forin_body, label %forin_after

forin_body:                                       ; preds = %forin_cond
  %arr_val = call ptr @moksha_array_get(ptr %51, i32 %56)
  store ptr %arr_val, ptr %val, align 8
  %58 = load ptr, ptr %val, align 8
  %print_unbox20 = call ptr @moksha_unbox_string(ptr %58)
  %59 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox20)
  %60 = load i32, ptr @__exception_flag, align 4
  %61 = icmp ne i32 %60, 0
  br i1 %61, label %unwind22, label %next_stmt21

forin_inc:                                        ; preds = %next_stmt21
  %62 = add i32 %56, 1
  store i32 %62, ptr %forin_idx, align 4
  br label %forin_cond

forin_after:                                      ; preds = %forin_cond
  %63 = call ptr @moksha_box_string(ptr @24)
  %print_unbox23 = call ptr @moksha_unbox_string(ptr %63)
  %64 = call i32 (ptr, ...) @printf(ptr @25, ptr %print_unbox23)
  %65 = load ptr, ptr %ar4, align 8
  %len24 = call i32 @moksha_get_length(ptr %65)
  store i32 0, ptr %forin_idx25, align 4
  br label %forin_cond26

next_stmt21:                                      ; preds = %forin_body
  br label %forin_inc

unwind22:                                         ; preds = %forin_body
  ret i32 0

forin_cond26:                                     ; preds = %forin_inc28, %forin_after
  %66 = load i32, ptr %forin_idx25, align 4
  %67 = icmp slt i32 %66, %len24
  br i1 %67, label %forin_body27, label %forin_after29

forin_body27:                                     ; preds = %forin_cond26
  %arr_val30 = call ptr @moksha_array_get(ptr %65, i32 %66)
  store i32 %66, ptr %idx, align 4
  store ptr %arr_val30, ptr %val31, align 8
  %68 = call ptr @moksha_box_string(ptr @26)
  %69 = load i32, ptr %idx, align 4
  %70 = call ptr @moksha_int_to_str(i32 %69)
  %71 = call ptr @moksha_box_string(ptr %70)
  %72 = call ptr @moksha_string_concat(ptr %68, ptr %71)
  %73 = call ptr @moksha_box_string(ptr @27)
  %74 = call ptr @moksha_string_concat(ptr %72, ptr %73)
  %75 = load ptr, ptr %val31, align 8
  %76 = call ptr @moksha_string_concat(ptr %74, ptr %75)
  %print_unbox32 = call ptr @moksha_unbox_string(ptr %76)
  %77 = call i32 (ptr, ...) @printf(ptr @28, ptr %print_unbox32)
  %78 = load i32, ptr @__exception_flag, align 4
  %79 = icmp ne i32 %78, 0
  br i1 %79, label %unwind34, label %next_stmt33

forin_inc28:                                      ; preds = %next_stmt33
  %80 = add i32 %66, 1
  store i32 %80, ptr %forin_idx25, align 4
  br label %forin_cond26

forin_after29:                                    ; preds = %forin_cond26
  %81 = call ptr @moksha_box_string(ptr @29)
  %print_unbox35 = call ptr @moksha_unbox_string(ptr %81)
  %82 = call i32 (ptr, ...) @printf(ptr @30, ptr %print_unbox35)
  %83 = load ptr, ptr %ar4, align 8
  %len36 = call i32 @moksha_get_length(ptr %83)
  store i32 0, ptr %forin_idx37, align 4
  br label %forin_cond38

next_stmt33:                                      ; preds = %forin_body27
  br label %forin_inc28

unwind34:                                         ; preds = %forin_body27
  ret i32 0

forin_cond38:                                     ; preds = %forin_inc40, %forin_after29
  %84 = load i32, ptr %forin_idx37, align 4
  %85 = icmp slt i32 %84, %len36
  br i1 %85, label %forin_body39, label %forin_after41

forin_body39:                                     ; preds = %forin_cond38
  %arr_val42 = call ptr @moksha_array_get(ptr %83, i32 %84)
  store ptr %arr_val42, ptr %val43, align 8
  %86 = call ptr @moksha_box_string(ptr @31)
  %87 = load ptr, ptr %val43, align 8
  %88 = call ptr @moksha_string_concat(ptr %86, ptr %87)
  %print_unbox44 = call ptr @moksha_unbox_string(ptr %88)
  %89 = call i32 (ptr, ...) @printf(ptr @32, ptr %print_unbox44)
  %90 = load i32, ptr @__exception_flag, align 4
  %91 = icmp ne i32 %90, 0
  br i1 %91, label %unwind46, label %next_stmt45

forin_inc40:                                      ; preds = %next_stmt45
  %92 = add i32 %84, 1
  store i32 %92, ptr %forin_idx37, align 4
  br label %forin_cond38

forin_after41:                                    ; preds = %forin_cond38
  %93 = call ptr @moksha_box_string(ptr @33)
  %print_unbox47 = call ptr @moksha_unbox_string(ptr %93)
  %94 = call i32 (ptr, ...) @printf(ptr @34, ptr %print_unbox47)
  %95 = load ptr, ptr %tab11, align 8
  %table_keys = call ptr @moksha_table_keys(ptr %95)
  %len48 = call i32 @moksha_get_length(ptr %table_keys)
  store i32 0, ptr %forin_idx49, align 4
  br label %forin_cond50

next_stmt45:                                      ; preds = %forin_body39
  br label %forin_inc40

unwind46:                                         ; preds = %forin_body39
  ret i32 0

forin_cond50:                                     ; preds = %forin_inc52, %forin_after41
  %96 = load i32, ptr %forin_idx49, align 4
  %97 = icmp slt i32 %96, %len48
  br i1 %97, label %forin_body51, label %forin_after53

forin_body51:                                     ; preds = %forin_cond50
  %key_val = call ptr @moksha_array_get(ptr %table_keys, i32 %96)
  %real_val = call ptr @moksha_table_get(ptr %95, ptr %key_val)
  store ptr %key_val, ptr %key, align 8
  store ptr %real_val, ptr %val54, align 8
  %98 = call ptr @moksha_box_string(ptr @35)
  %99 = load ptr, ptr %key, align 8
  %100 = call ptr @moksha_any_to_str(ptr %99)
  %101 = call ptr @moksha_box_string(ptr %100)
  %102 = call ptr @moksha_string_concat(ptr %98, ptr %101)
  %103 = call ptr @moksha_box_string(ptr @36)
  %104 = call ptr @moksha_string_concat(ptr %102, ptr %103)
  %105 = load ptr, ptr %val54, align 8
  %106 = call ptr @moksha_any_to_str(ptr %105)
  %107 = call ptr @moksha_box_string(ptr %106)
  %108 = call ptr @moksha_string_concat(ptr %104, ptr %107)
  %print_unbox55 = call ptr @moksha_unbox_string(ptr %108)
  %109 = call i32 (ptr, ...) @printf(ptr @37, ptr %print_unbox55)
  %110 = load i32, ptr @__exception_flag, align 4
  %111 = icmp ne i32 %110, 0
  br i1 %111, label %unwind57, label %next_stmt56

forin_inc52:                                      ; preds = %next_stmt56
  %112 = add i32 %96, 1
  store i32 %112, ptr %forin_idx49, align 4
  br label %forin_cond50

forin_after53:                                    ; preds = %forin_cond50
  %113 = call ptr @moksha_box_string(ptr @38)
  %print_unbox58 = call ptr @moksha_unbox_string(ptr %113)
  %114 = call i32 (ptr, ...) @printf(ptr @39, ptr %print_unbox58)
  %115 = load ptr, ptr %tab11, align 8
  %table_keys59 = call ptr @moksha_table_keys(ptr %115)
  %len60 = call i32 @moksha_get_length(ptr %table_keys59)
  store i32 0, ptr %forin_idx61, align 4
  br label %forin_cond62

next_stmt56:                                      ; preds = %forin_body51
  br label %forin_inc52

unwind57:                                         ; preds = %forin_body51
  ret i32 0

forin_cond62:                                     ; preds = %forin_inc64, %forin_after53
  %116 = load i32, ptr %forin_idx61, align 4
  %117 = icmp slt i32 %116, %len60
  br i1 %117, label %forin_body63, label %forin_after65

forin_body63:                                     ; preds = %forin_cond62
  %key_val66 = call ptr @moksha_array_get(ptr %table_keys59, i32 %116)
  %real_val67 = call ptr @moksha_table_get(ptr %115, ptr %key_val66)
  store ptr %key_val66, ptr %key68, align 8
  %118 = call ptr @moksha_box_string(ptr @40)
  %119 = load ptr, ptr %key68, align 8
  %120 = call ptr @moksha_any_to_str(ptr %119)
  %121 = call ptr @moksha_box_string(ptr %120)
  %122 = call ptr @moksha_string_concat(ptr %118, ptr %121)
  %print_unbox69 = call ptr @moksha_unbox_string(ptr %122)
  %123 = call i32 (ptr, ...) @printf(ptr @41, ptr %print_unbox69)
  %124 = load i32, ptr @__exception_flag, align 4
  %125 = icmp ne i32 %124, 0
  br i1 %125, label %unwind71, label %next_stmt70

forin_inc64:                                      ; preds = %next_stmt70
  %126 = add i32 %116, 1
  store i32 %126, ptr %forin_idx61, align 4
  br label %forin_cond62

forin_after65:                                    ; preds = %forin_cond62
  %127 = call ptr @moksha_box_string(ptr @42)
  %print_unbox72 = call ptr @moksha_unbox_string(ptr %127)
  %128 = call i32 (ptr, ...) @printf(ptr @43, ptr %print_unbox72)
  %129 = load ptr, ptr %w1, align 8
  %130 = call i32 @moksha_strlen(ptr %129)
  %icmptmp73 = icmp sgt i32 %130, 0
  br i1 %icmptmp73, label %then, label %else

next_stmt70:                                      ; preds = %forin_body63
  br label %forin_inc64

unwind71:                                         ; preds = %forin_body63
  ret i32 0

then:                                             ; preds = %forin_after65
  %131 = load ptr, ptr %w1, align 8
  %str_len = call i32 @moksha_strlen(ptr %131)
  store i32 0, ptr %forin_idx74, align 4
  br label %forin_cond75

else:                                             ; preds = %forin_after65
  br label %ifcont

ifcont:                                           ; preds = %else, %next_stmt82
  %132 = call ptr @moksha_box_string(ptr @45)
  %print_unbox84 = call ptr @moksha_unbox_string(ptr %132)
  %133 = call i32 (ptr, ...) @printf(ptr @46, ptr %print_unbox84)
  %134 = load ptr, ptr %w1, align 8
  %135 = call i32 @moksha_strlen(ptr %134)
  %icmptmp85 = icmp sgt i32 %135, 3
  br i1 %icmptmp85, label %then86, label %else87

forin_cond75:                                     ; preds = %forin_inc77, %then
  %136 = load i32, ptr %forin_idx74, align 4
  %137 = icmp slt i32 %136, %str_len
  br i1 %137, label %forin_body76, label %forin_after78

forin_body76:                                     ; preds = %forin_cond75
  %char_val = call ptr @moksha_string_get_char(ptr %131, i32 %136)
  store ptr %char_val, ptr %ch, align 8
  %138 = load ptr, ptr %ch, align 8
  %print_unbox79 = call ptr @moksha_unbox_string(ptr %138)
  %139 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox79)
  %140 = load i32, ptr @__exception_flag, align 4
  %141 = icmp ne i32 %140, 0
  br i1 %141, label %unwind81, label %next_stmt80

forin_inc77:                                      ; preds = %next_stmt80
  %142 = add i32 %136, 1
  store i32 %142, ptr %forin_idx74, align 4
  br label %forin_cond75

forin_after78:                                    ; preds = %forin_cond75
  %143 = load i32, ptr @__exception_flag, align 4
  %144 = icmp ne i32 %143, 0
  br i1 %144, label %unwind83, label %next_stmt82

next_stmt80:                                      ; preds = %forin_body76
  br label %forin_inc77

unwind81:                                         ; preds = %forin_body76
  ret i32 0

next_stmt82:                                      ; preds = %forin_after78
  br label %ifcont

unwind83:                                         ; preds = %forin_after78
  ret i32 0

then86:                                           ; preds = %ifcont
  %145 = call ptr @moksha_box_string(ptr @47)
  %146 = load ptr, ptr %w1, align 8
  %147 = call ptr @moksha_string_get_char(ptr %146, i32 3)
  %148 = call ptr @moksha_any_to_str(ptr %147)
  %149 = call ptr @moksha_box_string(ptr %148)
  %150 = call ptr @moksha_string_concat(ptr %145, ptr %149)
  %print_unbox89 = call ptr @moksha_unbox_string(ptr %150)
  %151 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox89)
  %152 = load i32, ptr @__exception_flag, align 4
  %153 = icmp ne i32 %152, 0
  br i1 %153, label %unwind91, label %next_stmt90

else87:                                           ; preds = %ifcont
  %154 = call ptr @moksha_box_string(ptr @51)
  %print_unbox103 = call ptr @moksha_unbox_string(ptr %154)
  %155 = call i32 (ptr, ...) @printf(ptr @52, ptr %print_unbox103)
  %156 = load i32, ptr @__exception_flag, align 4
  %157 = icmp ne i32 %156, 0
  br i1 %157, label %unwind105, label %next_stmt104

ifcont88:                                         ; preds = %next_stmt104, %next_stmt101
  %158 = load ptr, ptr %ar4, align 8
  %len_tmp106 = call i32 @moksha_get_length(ptr %158)
  %icmptmp107 = icmp sgt i32 %len_tmp106, 3
  br i1 %icmptmp107, label %then108, label %else109

next_stmt90:                                      ; preds = %then86
  %159 = load ptr, ptr %ar4, align 8
  %len_tmp = call i32 @moksha_get_length(ptr %159)
  %icmptmp92 = icmp sgt i32 %len_tmp, 3
  br i1 %icmptmp92, label %then93, label %else94

unwind91:                                         ; preds = %then86
  ret i32 0

then93:                                           ; preds = %next_stmt90
  %160 = load ptr, ptr %w1, align 8
  %161 = call ptr @moksha_string_get_char(ptr %160, i32 3)
  %162 = load ptr, ptr %ar4, align 8
  call void @moksha_array_set(ptr %162, i32 3, ptr %161)
  %163 = load i32, ptr @__exception_flag, align 4
  %164 = icmp ne i32 %163, 0
  br i1 %164, label %unwind97, label %next_stmt96

else94:                                           ; preds = %next_stmt90
  br label %ifcont95

ifcont95:                                         ; preds = %else94, %next_stmt99
  %165 = load i32, ptr @__exception_flag, align 4
  %166 = icmp ne i32 %165, 0
  br i1 %166, label %unwind102, label %next_stmt101

next_stmt96:                                      ; preds = %then93
  %167 = call ptr @moksha_box_string(ptr @49)
  %168 = load ptr, ptr %w1, align 8
  %169 = call ptr @moksha_string_get_char(ptr %168, i32 3)
  %170 = call ptr @moksha_any_to_str(ptr %169)
  %171 = call ptr @moksha_box_string(ptr %170)
  %172 = call ptr @moksha_string_concat(ptr %167, ptr %171)
  %print_unbox98 = call ptr @moksha_unbox_string(ptr %172)
  %173 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox98)
  %174 = load i32, ptr @__exception_flag, align 4
  %175 = icmp ne i32 %174, 0
  br i1 %175, label %unwind100, label %next_stmt99

unwind97:                                         ; preds = %then93
  ret i32 0

next_stmt99:                                      ; preds = %next_stmt96
  br label %ifcont95

unwind100:                                        ; preds = %next_stmt96
  ret i32 0

next_stmt101:                                     ; preds = %ifcont95
  br label %ifcont88

unwind102:                                        ; preds = %ifcont95
  ret i32 0

next_stmt104:                                     ; preds = %else87
  br label %ifcont88

unwind105:                                        ; preds = %else87
  ret i32 0

then108:                                          ; preds = %ifcont88
  %176 = load ptr, ptr %ar4, align 8
  %177 = call ptr @moksha_array_get(ptr %176, i32 3)
  store ptr %177, ptr %valAr, align 8
  store ptr %177, ptr %valAr111, align 8
  %178 = load i32, ptr @__exception_flag, align 4
  %179 = icmp ne i32 %178, 0
  br i1 %179, label %unwind113, label %next_stmt112

else109:                                          ; preds = %ifcont88
  br label %ifcont110

ifcont110:                                        ; preds = %else109, %next_stmt117
  %180 = call ptr @moksha_box_string(ptr @56)
  %181 = load ptr, ptr %tab11, align 8
  %182 = call ptr @moksha_any_to_str(ptr %181)
  %183 = call ptr @moksha_box_string(ptr %182)
  %184 = call ptr @moksha_string_concat(ptr %180, ptr %183)
  %print_unbox119 = call ptr @moksha_unbox_string(ptr %184)
  %185 = call i32 (ptr, ...) @printf(ptr @57, ptr %print_unbox119)
  %186 = call ptr @moksha_box_string(ptr @58)
  %187 = load ptr, ptr %ar4, align 8
  %len_tmp120 = call i32 @moksha_get_length(ptr %187)
  %188 = call ptr @moksha_int_to_str(i32 %len_tmp120)
  %189 = call ptr @moksha_box_string(ptr %188)
  %190 = call ptr @moksha_string_concat(ptr %186, ptr %189)
  %print_unbox121 = call ptr @moksha_unbox_string(ptr %190)
  %191 = call i32 (ptr, ...) @printf(ptr @59, ptr %print_unbox121)
  %192 = call ptr @moksha_box_string(ptr @60)
  %print_unbox122 = call ptr @moksha_unbox_string(ptr %192)
  %193 = call i32 (ptr, ...) @printf(ptr @61, ptr %print_unbox122)
  %194 = load ptr, ptr %ar4, align 8
  %len_tmp123 = call i32 @moksha_get_length(ptr %194)
  %icmptmp124 = icmp sgt i32 %len_tmp123, 3
  br i1 %icmptmp124, label %then125, label %else126

next_stmt112:                                     ; preds = %then108
  %195 = load ptr, ptr %valAr111, align 8
  %196 = load ptr, ptr %tab11, align 8
  %197 = call ptr @moksha_box_string(ptr @53)
  %198 = call ptr @moksha_table_set(ptr %196, ptr %197, ptr %195)
  %199 = load i32, ptr @__exception_flag, align 4
  %200 = icmp ne i32 %199, 0
  br i1 %200, label %unwind115, label %next_stmt114

unwind113:                                        ; preds = %then108
  ret i32 0

next_stmt114:                                     ; preds = %next_stmt112
  %201 = call ptr @moksha_box_string(ptr @54)
  %202 = load ptr, ptr %valAr111, align 8
  %203 = call ptr @moksha_string_concat(ptr %201, ptr %202)
  %print_unbox116 = call ptr @moksha_unbox_string(ptr %203)
  %204 = call i32 (ptr, ...) @printf(ptr @55, ptr %print_unbox116)
  %205 = load i32, ptr @__exception_flag, align 4
  %206 = icmp ne i32 %205, 0
  br i1 %206, label %unwind118, label %next_stmt117

unwind115:                                        ; preds = %next_stmt112
  ret i32 0

next_stmt117:                                     ; preds = %next_stmt114
  br label %ifcont110

unwind118:                                        ; preds = %next_stmt114
  ret i32 0

then125:                                          ; preds = %ifcont110
  %207 = call ptr @moksha_box_string(ptr @62)
  %208 = load ptr, ptr %ar4, align 8
  %209 = call ptr @moksha_array_get(ptr %208, i32 3)
  %210 = call ptr @moksha_string_concat(ptr %207, ptr %209)
  %211 = call ptr @moksha_box_string(ptr @63)
  %212 = call ptr @moksha_string_concat(ptr %210, ptr %211)
  %print_unbox128 = call ptr @moksha_unbox_string(ptr %212)
  %213 = call i32 (ptr, ...) @printf(ptr @64, ptr %print_unbox128)
  %214 = load i32, ptr @__exception_flag, align 4
  %215 = icmp ne i32 %214, 0
  br i1 %215, label %unwind130, label %next_stmt129

else126:                                          ; preds = %ifcont110
  br label %ifcont127

ifcont127:                                        ; preds = %else126, %next_stmt134
  %216 = call ptr @moksha_box_string(ptr @67)
  %217 = load ptr, ptr %ar4, align 8
  %len_tmp136 = call i32 @moksha_get_length(ptr %217)
  %218 = call ptr @moksha_int_to_str(i32 %len_tmp136)
  %219 = call ptr @moksha_box_string(ptr %218)
  %220 = call ptr @moksha_string_concat(ptr %216, ptr %219)
  %print_unbox137 = call ptr @moksha_unbox_string(ptr %220)
  %221 = call i32 (ptr, ...) @printf(ptr @68, ptr %print_unbox137)
  %222 = call ptr @moksha_box_string(ptr @69)
  %223 = load ptr, ptr %k14, align 8
  %224 = call ptr @moksha_string_concat(ptr %222, ptr %223)
  %225 = call ptr @moksha_box_string(ptr @70)
  %226 = call ptr @moksha_string_concat(ptr %224, ptr %225)
  %print_unbox138 = call ptr @moksha_unbox_string(ptr %226)
  %227 = call i32 (ptr, ...) @printf(ptr @71, ptr %print_unbox138)
  %228 = load ptr, ptr %tab11, align 8
  %229 = load ptr, ptr %k14, align 8
  call void @moksha_table_delete(ptr %228, ptr %229)
  %230 = call ptr @moksha_box_string(ptr @72)
  %231 = load ptr, ptr %tab11, align 8
  %232 = call ptr @moksha_any_to_str(ptr %231)
  %233 = call ptr @moksha_box_string(ptr %232)
  %234 = call ptr @moksha_string_concat(ptr %230, ptr %233)
  %print_unbox139 = call ptr @moksha_unbox_string(ptr %234)
  %235 = call i32 (ptr, ...) @printf(ptr @73, ptr %print_unbox139)
  %236 = call ptr @moksha_box_string(ptr @74)
  %print_unbox140 = call ptr @moksha_unbox_string(ptr %236)
  %237 = call i32 (ptr, ...) @printf(ptr @75, ptr %print_unbox140)
  ret i32 0

next_stmt129:                                     ; preds = %then125
  %238 = load ptr, ptr %ar4, align 8
  call void @moksha_array_delete(ptr %238, i32 3)
  %239 = load i32, ptr @__exception_flag, align 4
  %240 = icmp ne i32 %239, 0
  br i1 %240, label %unwind132, label %next_stmt131

unwind130:                                        ; preds = %then125
  ret i32 0

next_stmt131:                                     ; preds = %next_stmt129
  %241 = call ptr @moksha_box_string(ptr @65)
  %242 = load ptr, ptr %ar4, align 8
  %243 = call ptr @moksha_any_to_str(ptr %242)
  %244 = call ptr @moksha_box_string(ptr %243)
  %245 = call ptr @moksha_string_concat(ptr %241, ptr %244)
  %print_unbox133 = call ptr @moksha_unbox_string(ptr %245)
  %246 = call i32 (ptr, ...) @printf(ptr @66, ptr %print_unbox133)
  %247 = load i32, ptr @__exception_flag, align 4
  %248 = icmp ne i32 %247, 0
  br i1 %248, label %unwind135, label %next_stmt134

unwind132:                                        ; preds = %next_stmt129
  ret i32 0

next_stmt134:                                     ; preds = %next_stmt131
  br label %ifcont127

unwind135:                                        ; preds = %next_stmt131
  ret i32 0
}
