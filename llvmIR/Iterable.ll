; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = external global i32
@__exception_val = external global ptr
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
@17 = private unnamed_addr constant [10 x i8] c"fromArray\00", align 1
@18 = private unnamed_addr constant [24 x i8] c"\0A=== TEST COMPLETED ===\00", align 1
@19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@20 = private unnamed_addr constant [22 x i8] c"Iterations executed: \00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %len123 = alloca i32, align 4
  %len121 = alloca i32, align 4
  %ch = alloca ptr, align 8
  %forin_idx89 = alloca i32, align 4
  %key85 = alloca ptr, align 8
  %forin_idx78 = alloca i32, align 4
  %val73 = alloca ptr, align 8
  %key = alloca ptr, align 8
  %forin_idx68 = alloca i32, align 4
  %val64 = alloca ptr, align 8
  %forin_idx58 = alloca i32, align 4
  %val54 = alloca ptr, align 8
  %idx = alloca i32, align 4
  %forin_idx48 = alloca i32, align 4
  %val = alloca ptr, align 8
  %forin_idx = alloca i32, align 4
  %tab38 = alloca ptr, align 8
  %tab = alloca ptr, align 8
  %i30 = alloca i32, align 4
  %i29 = alloca i32, align 4
  %ar22 = alloca ptr, align 8
  %ar = alloca ptr, align 8
  %iter19 = alloca i32, align 4
  %iter = alloca i32, align 4
  %kValue14 = alloca ptr, align 8
  %kValue = alloca ptr, align 8
  %k12 = alloca ptr, align 8
  %k = alloca ptr, align 8
  %i7 = alloca i32, align 4
  %i = alloca i32, align 4
  %arInput5 = alloca ptr, align 8
  %arInput = alloca ptr, align 8
  %size4 = alloca i32, align 4
  %size = alloca i32, align 4
  %w2 = alloca ptr, align 8
  %w = alloca ptr, align 8
  %ITERATIONS1 = alloca i32, align 4
  %ITERATIONS = alloca i32, align 4
  store i32 100000, ptr %ITERATIONS, align 4
  store i32 100000, ptr %ITERATIONS1, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_read_string()
  store ptr %2, ptr %w, align 8
  store ptr %2, ptr %w2, align 8
  %3 = call ptr @moksha_box_string(ptr @2)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %3)
  %4 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox3)
  %5 = call i32 @moksha_read_int()
  store i32 %5, ptr %size, align 4
  store i32 %5, ptr %size4, align 4
  %6 = load i32, ptr %size4, align 4
  %arr_leaf = call ptr @moksha_alloc_array_fill(i32 %6, ptr null)
  store ptr %arr_leaf, ptr %arInput, align 8
  store ptr %arr_leaf, ptr %arInput5, align 8
  %7 = call ptr @moksha_box_string(ptr @4)
  %8 = load i32, ptr %size4, align 4
  %9 = call ptr @moksha_int_to_str(i32 %8)
  %10 = call ptr @moksha_box_string(ptr %9)
  %11 = call ptr @moksha_string_concat(ptr %7, ptr %10)
  %12 = call ptr @moksha_box_string(ptr @5)
  %13 = call ptr @moksha_string_concat(ptr %11, ptr %12)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %13)
  %14 = call i32 (ptr, ...) @printf(ptr @6, ptr %print_unbox6)
  store i32 0, ptr %i, align 4
  store i32 0, ptr %i7, align 4
  br label %forcond

forcond:                                          ; preds = %forinc, %entry
  %15 = load i32, ptr %i7, align 4
  %16 = load i32, ptr %size4, align 4
  %icmptmp = icmp slt i32 %15, %16
  br i1 %icmptmp, label %forloop, label %forafter

forloop:                                          ; preds = %forcond
  %17 = call ptr @moksha_box_string(ptr @7)
  %18 = load i32, ptr %i7, align 4
  %19 = call ptr @moksha_int_to_str(i32 %18)
  %20 = call ptr @moksha_box_string(ptr %19)
  %21 = call ptr @moksha_string_concat(ptr %17, ptr %20)
  %22 = call ptr @moksha_box_string(ptr @8)
  %23 = call ptr @moksha_string_concat(ptr %21, ptr %22)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %23)
  %24 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox8)
  %25 = load i32, ptr @__exception_flag, align 4
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %unwind, label %next_stmt

forinc:                                           ; preds = %next_stmt9
  %oldval = load i32, ptr %i7, align 4
  %inc = add i32 %oldval, 1
  store i32 %inc, ptr %i7, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  %27 = call ptr @moksha_box_string(ptr @10)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %27)
  %28 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox11)
  %29 = call ptr @moksha_read_string()
  store ptr %29, ptr %k, align 8
  store ptr %29, ptr %k12, align 8
  %30 = call ptr @moksha_box_string(ptr @12)
  %31 = load ptr, ptr %k12, align 8
  %32 = call ptr @moksha_string_concat(ptr %30, ptr %31)
  %33 = call ptr @moksha_box_string(ptr @13)
  %34 = call ptr @moksha_string_concat(ptr %32, ptr %33)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %34)
  %35 = call i32 (ptr, ...) @printf(ptr @14, ptr %print_unbox13)
  %36 = call ptr @moksha_read_string()
  store ptr %36, ptr %kValue, align 8
  store ptr %36, ptr %kValue14, align 8
  store i32 0, ptr %iter, align 4
  store i32 0, ptr %iter19, align 4
  br label %forcond15

next_stmt:                                        ; preds = %forloop
  %37 = call ptr @moksha_read_string()
  %38 = load ptr, ptr %arInput5, align 8
  %39 = load i32, ptr %i7, align 4
  call void @moksha_array_set(ptr %38, i32 %39, ptr %37)
  %40 = load i32, ptr @__exception_flag, align 4
  %41 = icmp ne i32 %40, 0
  br i1 %41, label %unwind10, label %next_stmt9

unwind:                                           ; preds = %forloop
  ret i32 0

next_stmt9:                                       ; preds = %next_stmt
  br label %forinc

unwind10:                                         ; preds = %next_stmt
  ret i32 0

forcond15:                                        ; preds = %forinc17, %forafter
  %42 = load i32, ptr %iter19, align 4
  %43 = load i32, ptr %ITERATIONS1, align 4
  %icmptmp20 = icmp slt i32 %42, %43
  br i1 %icmptmp20, label %forloop16, label %forafter18

forloop16:                                        ; preds = %forcond15
  %44 = load i32, ptr %size4, align 4
  %arr_leaf21 = call ptr @moksha_alloc_array_fill(i32 %44, ptr null)
  store ptr %arr_leaf21, ptr %ar, align 8
  store ptr %arr_leaf21, ptr %ar22, align 8
  %45 = load i32, ptr @__exception_flag, align 4
  %46 = icmp ne i32 %45, 0
  br i1 %46, label %unwind24, label %next_stmt23

forinc17:                                         ; preds = %next_stmt124
  %oldval126 = load i32, ptr %iter19, align 4
  %inc127 = add i32 %oldval126, 1
  store i32 %inc127, ptr %iter19, align 4
  br label %forcond15

forafter18:                                       ; preds = %forcond15
  %47 = call ptr @moksha_box_string(ptr @18)
  %print_unbox128 = call ptr @moksha_unbox_string(ptr %47)
  %48 = call i32 (ptr, ...) @printf(ptr @19, ptr %print_unbox128)
  %49 = call ptr @moksha_box_string(ptr @20)
  %50 = load i32, ptr %ITERATIONS1, align 4
  %51 = call ptr @moksha_int_to_str(i32 %50)
  %52 = call ptr @moksha_box_string(ptr %51)
  %53 = call ptr @moksha_string_concat(ptr %49, ptr %52)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %53)
  %54 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox129)
  ret i32 0

next_stmt23:                                      ; preds = %forloop16
  store i32 0, ptr %i29, align 4
  store i32 0, ptr %i30, align 4
  br label %forcond25

unwind24:                                         ; preds = %forloop16
  ret i32 0

forcond25:                                        ; preds = %forinc27, %next_stmt23
  %55 = load i32, ptr %i30, align 4
  %56 = load i32, ptr %size4, align 4
  %icmptmp31 = icmp slt i32 %55, %56
  br i1 %icmptmp31, label %forloop26, label %forafter28

forloop26:                                        ; preds = %forcond25
  %57 = load ptr, ptr %arInput5, align 8
  %58 = load i32, ptr %i30, align 4
  %59 = call ptr @moksha_array_get(ptr %57, i32 %58)
  %60 = load ptr, ptr %ar22, align 8
  %61 = load i32, ptr %i30, align 4
  call void @moksha_array_set(ptr %60, i32 %61, ptr %59)
  %62 = load i32, ptr @__exception_flag, align 4
  %63 = icmp ne i32 %62, 0
  br i1 %63, label %unwind33, label %next_stmt32

forinc27:                                         ; preds = %next_stmt32
  %oldval34 = load i32, ptr %i30, align 4
  %inc35 = add i32 %oldval34, 1
  store i32 %inc35, ptr %i30, align 4
  br label %forcond25

forafter28:                                       ; preds = %forcond25
  %64 = load i32, ptr @__exception_flag, align 4
  %65 = icmp ne i32 %64, 0
  br i1 %65, label %unwind37, label %next_stmt36

next_stmt32:                                      ; preds = %forloop26
  br label %forinc27

unwind33:                                         ; preds = %forloop26
  ret i32 0

next_stmt36:                                      ; preds = %forafter28
  %66 = call ptr @moksha_alloc_table(i32 0)
  store ptr %66, ptr %tab, align 8
  store ptr %66, ptr %tab38, align 8
  %67 = load i32, ptr @__exception_flag, align 4
  %68 = icmp ne i32 %67, 0
  br i1 %68, label %unwind40, label %next_stmt39

unwind37:                                         ; preds = %forafter28
  ret i32 0

next_stmt39:                                      ; preds = %next_stmt36
  %69 = load ptr, ptr %kValue14, align 8
  %70 = load ptr, ptr %tab38, align 8
  %71 = load ptr, ptr %k12, align 8
  %72 = call ptr @moksha_table_set(ptr %70, ptr %71, ptr %69)
  %73 = load i32, ptr @__exception_flag, align 4
  %74 = icmp ne i32 %73, 0
  br i1 %74, label %unwind42, label %next_stmt41

unwind40:                                         ; preds = %next_stmt36
  ret i32 0

next_stmt41:                                      ; preds = %next_stmt39
  %75 = call ptr @moksha_box_string(ptr @15)
  %76 = load ptr, ptr %tab38, align 8
  %77 = call ptr @moksha_box_string(ptr @16)
  %78 = call ptr @moksha_table_set(ptr %76, ptr %77, ptr %75)
  %79 = load i32, ptr @__exception_flag, align 4
  %80 = icmp ne i32 %79, 0
  br i1 %80, label %unwind44, label %next_stmt43

unwind42:                                         ; preds = %next_stmt39
  ret i32 0

next_stmt43:                                      ; preds = %next_stmt41
  %81 = load ptr, ptr %ar22, align 8
  %len = call i32 @moksha_get_length(ptr %81)
  store i32 0, ptr %forin_idx, align 4
  br label %forin_cond

unwind44:                                         ; preds = %next_stmt41
  ret i32 0

forin_cond:                                       ; preds = %forin_inc, %next_stmt43
  %82 = load i32, ptr %forin_idx, align 4
  %83 = icmp slt i32 %82, %len
  br i1 %83, label %forin_body, label %forin_after

forin_body:                                       ; preds = %forin_cond
  %arr_val = call ptr @moksha_array_get_unsafe(ptr %81, i32 %82)
  store ptr %arr_val, ptr %val, align 8
  br label %forin_inc

forin_inc:                                        ; preds = %forin_body
  %84 = add i32 %82, 1
  store i32 %84, ptr %forin_idx, align 4
  br label %forin_cond

forin_after:                                      ; preds = %forin_cond
  %85 = load i32, ptr @__exception_flag, align 4
  %86 = icmp ne i32 %85, 0
  br i1 %86, label %unwind46, label %next_stmt45

next_stmt45:                                      ; preds = %forin_after
  %87 = load ptr, ptr %ar22, align 8
  %len47 = call i32 @moksha_get_length(ptr %87)
  store i32 0, ptr %forin_idx48, align 4
  br label %forin_cond49

unwind46:                                         ; preds = %forin_after
  ret i32 0

forin_cond49:                                     ; preds = %forin_inc51, %next_stmt45
  %88 = load i32, ptr %forin_idx48, align 4
  %89 = icmp slt i32 %88, %len47
  br i1 %89, label %forin_body50, label %forin_after52

forin_body50:                                     ; preds = %forin_cond49
  %arr_val53 = call ptr @moksha_array_get_unsafe(ptr %87, i32 %88)
  store i32 %88, ptr %idx, align 4
  store ptr %arr_val53, ptr %val54, align 8
  br label %forin_inc51

forin_inc51:                                      ; preds = %forin_body50
  %90 = add i32 %88, 1
  store i32 %90, ptr %forin_idx48, align 4
  br label %forin_cond49

forin_after52:                                    ; preds = %forin_cond49
  %91 = load i32, ptr @__exception_flag, align 4
  %92 = icmp ne i32 %91, 0
  br i1 %92, label %unwind56, label %next_stmt55

next_stmt55:                                      ; preds = %forin_after52
  %93 = load ptr, ptr %ar22, align 8
  %len57 = call i32 @moksha_get_length(ptr %93)
  store i32 0, ptr %forin_idx58, align 4
  br label %forin_cond59

unwind56:                                         ; preds = %forin_after52
  ret i32 0

forin_cond59:                                     ; preds = %forin_inc61, %next_stmt55
  %94 = load i32, ptr %forin_idx58, align 4
  %95 = icmp slt i32 %94, %len57
  br i1 %95, label %forin_body60, label %forin_after62

forin_body60:                                     ; preds = %forin_cond59
  %arr_val63 = call ptr @moksha_array_get_unsafe(ptr %93, i32 %94)
  store ptr %arr_val63, ptr %val64, align 8
  br label %forin_inc61

forin_inc61:                                      ; preds = %forin_body60
  %96 = add i32 %94, 1
  store i32 %96, ptr %forin_idx58, align 4
  br label %forin_cond59

forin_after62:                                    ; preds = %forin_cond59
  %97 = load i32, ptr @__exception_flag, align 4
  %98 = icmp ne i32 %97, 0
  br i1 %98, label %unwind66, label %next_stmt65

next_stmt65:                                      ; preds = %forin_after62
  %99 = load ptr, ptr %tab38, align 8
  %table_keys = call ptr @moksha_table_keys(ptr %99)
  %len67 = call i32 @moksha_get_length(ptr %table_keys)
  store i32 0, ptr %forin_idx68, align 4
  br label %forin_cond69

unwind66:                                         ; preds = %forin_after62
  ret i32 0

forin_cond69:                                     ; preds = %forin_inc71, %next_stmt65
  %100 = load i32, ptr %forin_idx68, align 4
  %101 = icmp slt i32 %100, %len67
  br i1 %101, label %forin_body70, label %forin_after72

forin_body70:                                     ; preds = %forin_cond69
  %key_val = call ptr @moksha_array_get(ptr %table_keys, i32 %100)
  %real_val = call ptr @moksha_table_get(ptr %99, ptr %key_val)
  store ptr %key_val, ptr %key, align 8
  store ptr %real_val, ptr %val73, align 8
  br label %forin_inc71

forin_inc71:                                      ; preds = %forin_body70
  %102 = add i32 %100, 1
  store i32 %102, ptr %forin_idx68, align 4
  br label %forin_cond69

forin_after72:                                    ; preds = %forin_cond69
  %103 = load i32, ptr @__exception_flag, align 4
  %104 = icmp ne i32 %103, 0
  br i1 %104, label %unwind75, label %next_stmt74

next_stmt74:                                      ; preds = %forin_after72
  %105 = load ptr, ptr %tab38, align 8
  %table_keys76 = call ptr @moksha_table_keys(ptr %105)
  %len77 = call i32 @moksha_get_length(ptr %table_keys76)
  store i32 0, ptr %forin_idx78, align 4
  br label %forin_cond79

unwind75:                                         ; preds = %forin_after72
  ret i32 0

forin_cond79:                                     ; preds = %forin_inc81, %next_stmt74
  %106 = load i32, ptr %forin_idx78, align 4
  %107 = icmp slt i32 %106, %len77
  br i1 %107, label %forin_body80, label %forin_after82

forin_body80:                                     ; preds = %forin_cond79
  %key_val83 = call ptr @moksha_array_get(ptr %table_keys76, i32 %106)
  %real_val84 = call ptr @moksha_table_get(ptr %105, ptr %key_val83)
  store ptr %key_val83, ptr %key85, align 8
  br label %forin_inc81

forin_inc81:                                      ; preds = %forin_body80
  %108 = add i32 %106, 1
  store i32 %108, ptr %forin_idx78, align 4
  br label %forin_cond79

forin_after82:                                    ; preds = %forin_cond79
  %109 = load i32, ptr @__exception_flag, align 4
  %110 = icmp ne i32 %109, 0
  br i1 %110, label %unwind87, label %next_stmt86

next_stmt86:                                      ; preds = %forin_after82
  %111 = load ptr, ptr %w2, align 8
  %112 = call i32 @moksha_strlen(ptr %111)
  %icmptmp88 = icmp sgt i32 %112, 0
  br i1 %icmptmp88, label %then, label %else

unwind87:                                         ; preds = %forin_after82
  ret i32 0

then:                                             ; preds = %next_stmt86
  %113 = load ptr, ptr %w2, align 8
  %str_len = call i32 @moksha_strlen(ptr %113)
  store i32 0, ptr %forin_idx89, align 4
  br label %forin_cond90

else:                                             ; preds = %next_stmt86
  br label %ifcont

ifcont:                                           ; preds = %else, %next_stmt94
  %114 = load i32, ptr @__exception_flag, align 4
  %115 = icmp ne i32 %114, 0
  br i1 %115, label %unwind97, label %next_stmt96

forin_cond90:                                     ; preds = %forin_inc92, %then
  %116 = load i32, ptr %forin_idx89, align 4
  %117 = icmp slt i32 %116, %str_len
  br i1 %117, label %forin_body91, label %forin_after93

forin_body91:                                     ; preds = %forin_cond90
  %char_val = call ptr @moksha_string_get_char(ptr %113, i32 %116)
  store ptr %char_val, ptr %ch, align 8
  br label %forin_inc92

forin_inc92:                                      ; preds = %forin_body91
  %118 = add i32 %116, 1
  store i32 %118, ptr %forin_idx89, align 4
  br label %forin_cond90

forin_after93:                                    ; preds = %forin_cond90
  %119 = load i32, ptr @__exception_flag, align 4
  %120 = icmp ne i32 %119, 0
  br i1 %120, label %unwind95, label %next_stmt94

next_stmt94:                                      ; preds = %forin_after93
  br label %ifcont

unwind95:                                         ; preds = %forin_after93
  ret i32 0

next_stmt96:                                      ; preds = %ifcont
  %121 = load ptr, ptr %w2, align 8
  %122 = call i32 @moksha_strlen(ptr %121)
  %icmptmp98 = icmp sgt i32 %122, 3
  br i1 %icmptmp98, label %then99, label %else100

unwind97:                                         ; preds = %ifcont
  ret i32 0

then99:                                           ; preds = %next_stmt96
  %123 = load ptr, ptr %ar22, align 8
  %len_tmp = call i32 @moksha_get_length(ptr %123)
  %icmptmp102 = icmp sgt i32 %len_tmp, 3
  br i1 %icmptmp102, label %then103, label %else104

else100:                                          ; preds = %next_stmt96
  br label %ifcont101

ifcont101:                                        ; preds = %else100, %next_stmt108
  %124 = load i32, ptr @__exception_flag, align 4
  %125 = icmp ne i32 %124, 0
  br i1 %125, label %unwind111, label %next_stmt110

then103:                                          ; preds = %then99
  %126 = load ptr, ptr %w2, align 8
  %127 = call ptr @moksha_string_get_char(ptr %126, i32 3)
  %128 = load ptr, ptr %ar22, align 8
  call void @moksha_array_set(ptr %128, i32 3, ptr %127)
  %129 = load i32, ptr @__exception_flag, align 4
  %130 = icmp ne i32 %129, 0
  br i1 %130, label %unwind107, label %next_stmt106

else104:                                          ; preds = %then99
  br label %ifcont105

ifcont105:                                        ; preds = %else104, %next_stmt106
  %131 = load i32, ptr @__exception_flag, align 4
  %132 = icmp ne i32 %131, 0
  br i1 %132, label %unwind109, label %next_stmt108

next_stmt106:                                     ; preds = %then103
  br label %ifcont105

unwind107:                                        ; preds = %then103
  ret i32 0

next_stmt108:                                     ; preds = %ifcont105
  br label %ifcont101

unwind109:                                        ; preds = %ifcont105
  ret i32 0

next_stmt110:                                     ; preds = %ifcont101
  %133 = load ptr, ptr %ar22, align 8
  %len_tmp112 = call i32 @moksha_get_length(ptr %133)
  %icmptmp113 = icmp sgt i32 %len_tmp112, 3
  br i1 %icmptmp113, label %then114, label %else115

unwind111:                                        ; preds = %ifcont101
  ret i32 0

then114:                                          ; preds = %next_stmt110
  %134 = load ptr, ptr %ar22, align 8
  %135 = call ptr @moksha_array_get(ptr %134, i32 3)
  %136 = load ptr, ptr %tab38, align 8
  %137 = call ptr @moksha_box_string(ptr @17)
  %138 = call ptr @moksha_table_set(ptr %136, ptr %137, ptr %135)
  %139 = load i32, ptr @__exception_flag, align 4
  %140 = icmp ne i32 %139, 0
  br i1 %140, label %unwind118, label %next_stmt117

else115:                                          ; preds = %next_stmt110
  br label %ifcont116

ifcont116:                                        ; preds = %else115, %next_stmt117
  %141 = load i32, ptr @__exception_flag, align 4
  %142 = icmp ne i32 %141, 0
  br i1 %142, label %unwind120, label %next_stmt119

next_stmt117:                                     ; preds = %then114
  br label %ifcont116

unwind118:                                        ; preds = %then114
  ret i32 0

next_stmt119:                                     ; preds = %ifcont116
  %143 = load ptr, ptr %ar22, align 8
  %len_tmp122 = call i32 @moksha_get_length(ptr %143)
  store i32 %len_tmp122, ptr %len121, align 4
  store i32 %len_tmp122, ptr %len123, align 4
  %144 = load i32, ptr @__exception_flag, align 4
  %145 = icmp ne i32 %144, 0
  br i1 %145, label %unwind125, label %next_stmt124

unwind120:                                        ; preds = %ifcont116
  ret i32 0

next_stmt124:                                     ; preds = %next_stmt119
  %146 = load ptr, ptr %tab38, align 8
  call void @moksha_free(ptr %146)
  %147 = load ptr, ptr %ar22, align 8
  call void @moksha_free(ptr %147)
  br label %forinc17

unwind125:                                        ; preds = %next_stmt119
  ret i32 0
}
