; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = external global i32
@__exception_val = external global ptr
@0 = private unnamed_addr constant [18 x i8] c"\0AEnter a string: \00", align 1
@__str_lit_cache_0 = internal global ptr null
@1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@2 = private unnamed_addr constant [19 x i8] c"Enter array size: \00", align 1
@__str_lit_cache_1 = internal global ptr null
@3 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@4 = private unnamed_addr constant [7 x i8] c"Enter \00", align 1
@__str_lit_cache_2 = internal global ptr null
@5 = private unnamed_addr constant [22 x i8] c" words for the array:\00", align 1
@__str_lit_cache_3 = internal global ptr null
@6 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"ar[\00", align 1
@__str_lit_cache_4 = internal global ptr null
@8 = private unnamed_addr constant [4 x i8] c"]: \00", align 1
@__str_lit_cache_5 = internal global ptr null
@9 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@10 = private unnamed_addr constant [21 x i8] c"\0AEnter a table key: \00", align 1
@__str_lit_cache_6 = internal global ptr null
@11 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@12 = private unnamed_addr constant [17 x i8] c"Enter value for \00", align 1
@__str_lit_cache_7 = internal global ptr null
@13 = private unnamed_addr constant [3 x i8] c": \00", align 1
@__str_lit_cache_8 = internal global ptr null
@14 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@15 = private unnamed_addr constant [17 x i8] c"IndexOutOfBounds\00", align 1
@16 = private unnamed_addr constant [13 x i8] c"unchangeable\00", align 1
@__str_lit_cache_9 = internal global ptr null
@17 = private unnamed_addr constant [6 x i8] c"fixed\00", align 1
@__str_lit_cache_10 = internal global ptr null
@18 = private unnamed_addr constant [17 x i8] c"IndexOutOfBounds\00", align 1
@19 = private unnamed_addr constant [10 x i8] c"fromArray\00", align 1
@__str_lit_cache_11 = internal global ptr null
@20 = private unnamed_addr constant [24 x i8] c"\0A=== TEST COMPLETED ===\00", align 1
@__str_lit_cache_12 = internal global ptr null
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [22 x i8] c"Iterations executed: \00", align 1
@__str_lit_cache_13 = internal global ptr null
@23 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare ptr @calloc(i64, i64)

declare void @moksha_print_val(ptr, i32)

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
  %len139 = alloca i32, align 4
  %len133 = alloca i32, align 4
  %ch = alloca ptr, align 8
  %forin_idx83 = alloca i32, align 4
  %key79 = alloca ptr, align 8
  %tbl_idx73 = alloca i32, align 4
  %val69 = alloca ptr, align 8
  %key = alloca ptr, align 8
  %tbl_idx = alloca i32, align 4
  %val66 = alloca ptr, align 8
  %forin_idx59 = alloca i32, align 4
  %val55 = alloca ptr, align 8
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
  %0 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_0, ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_read_string()
  store ptr %2, ptr %w, align 8
  store ptr %2, ptr %w2, align 8
  %3 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_1, ptr @2)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %3)
  %4 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox3)
  %5 = call i32 @moksha_read_int()
  store i32 %5, ptr %size, align 4
  store i32 %5, ptr %size4, align 4
  %6 = load i32, ptr %size4, align 4
  %arr_leaf = call ptr @moksha_alloc_array_fill(i32 %6, ptr null)
  store ptr %arr_leaf, ptr %arInput, align 8
  store ptr %arr_leaf, ptr %arInput5, align 8
  %7 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_2, ptr @4)
  %8 = load i32, ptr %size4, align 4
  %9 = call ptr @moksha_int_to_str(i32 %8)
  %10 = call ptr @moksha_box_string(ptr %9)
  %11 = call ptr @moksha_string_concat(ptr %7, ptr %10)
  %12 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_3, ptr @5)
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
  %17 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_4, ptr @7)
  %18 = load i32, ptr %i7, align 4
  %19 = call ptr @moksha_int_to_str(i32 %18)
  %20 = call ptr @moksha_box_string(ptr %19)
  %21 = call ptr @moksha_string_concat(ptr %17, ptr %20)
  %22 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_5, ptr @8)
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
  %27 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_6, ptr @10)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %27)
  %28 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox11)
  %29 = call ptr @moksha_read_string()
  store ptr %29, ptr %k, align 8
  store ptr %29, ptr %k12, align 8
  %30 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_7, ptr @12)
  %31 = load ptr, ptr %k12, align 8
  %32 = call ptr @moksha_string_concat(ptr %30, ptr %31)
  %33 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_8, ptr @13)
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

forinc17:                                         ; preds = %next_stmt140
  %oldval142 = load i32, ptr %iter19, align 4
  %inc143 = add i32 %oldval142, 1
  store i32 %inc143, ptr %iter19, align 4
  br label %forcond15

forafter18:                                       ; preds = %forcond15
  %47 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_12, ptr @20)
  %print_unbox144 = call ptr @moksha_unbox_string(ptr %47)
  %48 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox144)
  %49 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_13, ptr @22)
  %50 = load i32, ptr %ITERATIONS1, align 4
  %51 = call ptr @moksha_int_to_str(i32 %50)
  %52 = call ptr @moksha_box_string(ptr %51)
  %53 = call ptr @moksha_string_concat(ptr %49, ptr %52)
  %print_unbox145 = call ptr @moksha_unbox_string(ptr %53)
  %54 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox145)
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
  %59 = getelementptr inbounds i8, ptr %57, i32 12
  %arr_len = load i32, ptr %59, align 4
  %60 = icmp uge i32 %58, %arr_len
  br i1 %60, label %bounds_fail, label %bounds_ok

forinc27:                                         ; preds = %next_stmt32
  %oldval34 = load i32, ptr %i30, align 4
  %inc35 = add i32 %oldval34, 1
  store i32 %inc35, ptr %i30, align 4
  br label %forcond25

forafter28:                                       ; preds = %forcond25
  %61 = load i32, ptr @__exception_flag, align 4
  %62 = icmp ne i32 %61, 0
  br i1 %62, label %unwind37, label %next_stmt36

bounds_ok:                                        ; preds = %forloop26
  %63 = getelementptr inbounds i8, ptr %57, i32 16
  %64 = getelementptr ptr, ptr %63, i32 %58
  %fast_elem = load ptr, ptr %64, align 8
  %65 = load ptr, ptr %ar22, align 8
  %66 = load i32, ptr %i30, align 4
  call void @moksha_array_set(ptr %65, i32 %66, ptr %fast_elem)
  %67 = load i32, ptr @__exception_flag, align 4
  %68 = icmp ne i32 %67, 0
  br i1 %68, label %unwind33, label %next_stmt32

bounds_fail:                                      ; preds = %forloop26
  %69 = call ptr @moksha_box_string(ptr @15)
  call void @moksha_throw_exception(ptr %69)
  store ptr %69, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

next_stmt32:                                      ; preds = %bounds_ok
  br label %forinc27

unwind33:                                         ; preds = %bounds_ok
  ret i32 0

next_stmt36:                                      ; preds = %forafter28
  %70 = call ptr @moksha_alloc_table(i32 0)
  store ptr %70, ptr %tab, align 8
  store ptr %70, ptr %tab38, align 8
  %71 = load i32, ptr @__exception_flag, align 4
  %72 = icmp ne i32 %71, 0
  br i1 %72, label %unwind40, label %next_stmt39

unwind37:                                         ; preds = %forafter28
  ret i32 0

next_stmt39:                                      ; preds = %next_stmt36
  %73 = load ptr, ptr %kValue14, align 8
  %74 = load ptr, ptr %tab38, align 8
  %75 = load ptr, ptr %k12, align 8
  %76 = call ptr @moksha_table_set(ptr %74, ptr %75, ptr %73)
  %77 = load i32, ptr @__exception_flag, align 4
  %78 = icmp ne i32 %77, 0
  br i1 %78, label %unwind42, label %next_stmt41

unwind40:                                         ; preds = %next_stmt36
  ret i32 0

next_stmt41:                                      ; preds = %next_stmt39
  %79 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_9, ptr @16)
  %80 = load ptr, ptr %tab38, align 8
  %81 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_10, ptr @17)
  %82 = call ptr @moksha_table_set(ptr %80, ptr %81, ptr %79)
  %83 = load i32, ptr @__exception_flag, align 4
  %84 = icmp ne i32 %83, 0
  br i1 %84, label %unwind44, label %next_stmt43

unwind42:                                         ; preds = %next_stmt39
  ret i32 0

next_stmt43:                                      ; preds = %next_stmt41
  %85 = load ptr, ptr %ar22, align 8
  %len = call i32 @moksha_get_length(ptr %85)
  store i32 0, ptr %forin_idx, align 4
  br label %forin_cond

unwind44:                                         ; preds = %next_stmt41
  ret i32 0

forin_cond:                                       ; preds = %forin_inc, %next_stmt43
  %86 = load i32, ptr %forin_idx, align 4
  %87 = icmp slt i32 %86, %len
  br i1 %87, label %forin_body, label %forin_after

forin_body:                                       ; preds = %forin_cond
  %88 = getelementptr inbounds i8, ptr %85, i32 16
  %fast_elem_ptr = getelementptr ptr, ptr %88, i32 %86
  %fast_val = load ptr, ptr %fast_elem_ptr, align 8
  store ptr %fast_val, ptr %val, align 8
  br label %forin_inc

forin_inc:                                        ; preds = %forin_body
  %89 = add i32 %86, 1
  store i32 %89, ptr %forin_idx, align 4
  br label %forin_cond

forin_after:                                      ; preds = %forin_cond
  %90 = load i32, ptr @__exception_flag, align 4
  %91 = icmp ne i32 %90, 0
  br i1 %91, label %unwind46, label %next_stmt45

next_stmt45:                                      ; preds = %forin_after
  %92 = load ptr, ptr %ar22, align 8
  %len47 = call i32 @moksha_get_length(ptr %92)
  store i32 0, ptr %forin_idx48, align 4
  br label %forin_cond49

unwind46:                                         ; preds = %forin_after
  ret i32 0

forin_cond49:                                     ; preds = %forin_inc51, %next_stmt45
  %93 = load i32, ptr %forin_idx48, align 4
  %94 = icmp slt i32 %93, %len47
  br i1 %94, label %forin_body50, label %forin_after52

forin_body50:                                     ; preds = %forin_cond49
  %95 = getelementptr inbounds i8, ptr %92, i32 16
  %fast_elem_ptr53 = getelementptr ptr, ptr %95, i32 %93
  %fast_val54 = load ptr, ptr %fast_elem_ptr53, align 8
  store i32 %93, ptr %idx, align 4
  store ptr %fast_val54, ptr %val55, align 8
  br label %forin_inc51

forin_inc51:                                      ; preds = %forin_body50
  %96 = add i32 %93, 1
  store i32 %96, ptr %forin_idx48, align 4
  br label %forin_cond49

forin_after52:                                    ; preds = %forin_cond49
  %97 = load i32, ptr @__exception_flag, align 4
  %98 = icmp ne i32 %97, 0
  br i1 %98, label %unwind57, label %next_stmt56

next_stmt56:                                      ; preds = %forin_after52
  %99 = load ptr, ptr %ar22, align 8
  %len58 = call i32 @moksha_get_length(ptr %99)
  store i32 0, ptr %forin_idx59, align 4
  br label %forin_cond60

unwind57:                                         ; preds = %forin_after52
  ret i32 0

forin_cond60:                                     ; preds = %forin_inc62, %next_stmt56
  %100 = load i32, ptr %forin_idx59, align 4
  %101 = icmp slt i32 %100, %len58
  br i1 %101, label %forin_body61, label %forin_after63

forin_body61:                                     ; preds = %forin_cond60
  %102 = getelementptr inbounds i8, ptr %99, i32 16
  %fast_elem_ptr64 = getelementptr ptr, ptr %102, i32 %100
  %fast_val65 = load ptr, ptr %fast_elem_ptr64, align 8
  store ptr %fast_val65, ptr %val66, align 8
  br label %forin_inc62

forin_inc62:                                      ; preds = %forin_body61
  %103 = add i32 %100, 1
  store i32 %103, ptr %forin_idx59, align 4
  br label %forin_cond60

forin_after63:                                    ; preds = %forin_cond60
  %104 = load i32, ptr @__exception_flag, align 4
  %105 = icmp ne i32 %104, 0
  br i1 %105, label %unwind68, label %next_stmt67

next_stmt67:                                      ; preds = %forin_after63
  %106 = load ptr, ptr %tab38, align 8
  %tbl_cap = call i32 @moksha_table_capacity(ptr %106)
  store i32 0, ptr %tbl_idx, align 4
  br label %tbl_cond

unwind68:                                         ; preds = %forin_after63
  ret i32 0

tbl_cond:                                         ; preds = %tbl_inc, %next_stmt67
  %107 = load i32, ptr %tbl_idx, align 4
  %108 = icmp slt i32 %107, %tbl_cap
  br i1 %108, label %tbl_check, label %tbl_after

tbl_check:                                        ; preds = %tbl_cond
  %109 = call ptr @moksha_table_get_entry_key(ptr %106, i32 %107)
  %110 = icmp ne ptr %109, null
  br i1 %110, label %tbl_body, label %tbl_inc

tbl_body:                                         ; preds = %tbl_check
  %111 = call ptr @moksha_table_get_entry_val(ptr %106, i32 %107)
  store ptr %109, ptr %key, align 8
  store ptr %111, ptr %val69, align 8
  br label %tbl_inc

tbl_inc:                                          ; preds = %tbl_body, %tbl_check
  %112 = add i32 %107, 1
  store i32 %112, ptr %tbl_idx, align 4
  br label %tbl_cond

tbl_after:                                        ; preds = %tbl_cond
  %113 = load i32, ptr @__exception_flag, align 4
  %114 = icmp ne i32 %113, 0
  br i1 %114, label %unwind71, label %next_stmt70

next_stmt70:                                      ; preds = %tbl_after
  %115 = load ptr, ptr %tab38, align 8
  %tbl_cap72 = call i32 @moksha_table_capacity(ptr %115)
  store i32 0, ptr %tbl_idx73, align 4
  br label %tbl_cond74

unwind71:                                         ; preds = %tbl_after
  ret i32 0

tbl_cond74:                                       ; preds = %tbl_inc77, %next_stmt70
  %116 = load i32, ptr %tbl_idx73, align 4
  %117 = icmp slt i32 %116, %tbl_cap72
  br i1 %117, label %tbl_check75, label %tbl_after78

tbl_check75:                                      ; preds = %tbl_cond74
  %118 = call ptr @moksha_table_get_entry_key(ptr %115, i32 %116)
  %119 = icmp ne ptr %118, null
  br i1 %119, label %tbl_body76, label %tbl_inc77

tbl_body76:                                       ; preds = %tbl_check75
  %120 = call ptr @moksha_table_get_entry_val(ptr %115, i32 %116)
  store ptr %118, ptr %key79, align 8
  br label %tbl_inc77

tbl_inc77:                                        ; preds = %tbl_body76, %tbl_check75
  %121 = add i32 %116, 1
  store i32 %121, ptr %tbl_idx73, align 4
  br label %tbl_cond74

tbl_after78:                                      ; preds = %tbl_cond74
  %122 = load i32, ptr @__exception_flag, align 4
  %123 = icmp ne i32 %122, 0
  br i1 %123, label %unwind81, label %next_stmt80

next_stmt80:                                      ; preds = %tbl_after78
  %124 = load ptr, ptr %w2, align 8
  %125 = icmp eq ptr %124, null
  br i1 %125, label %len_null, label %len_safe

unwind81:                                         ; preds = %tbl_after78
  ret i32 0

len_safe:                                         ; preds = %next_stmt80
  %126 = getelementptr inbounds i8, ptr %124, i32 16
  %inline_len = load i32, ptr %126, align 4
  br label %len_merge

len_null:                                         ; preds = %next_stmt80
  br label %len_merge

len_merge:                                        ; preds = %len_safe, %len_null
  %len_res = phi i32 [ 0, %len_null ], [ %inline_len, %len_safe ]
  %icmptmp82 = icmp sgt i32 %len_res, 0
  br i1 %icmptmp82, label %then, label %else

then:                                             ; preds = %len_merge
  %127 = load ptr, ptr %w2, align 8
  %str_len = call i32 @moksha_strlen(ptr %127)
  store i32 0, ptr %forin_idx83, align 4
  br label %forin_cond84

else:                                             ; preds = %len_merge
  br label %ifcont

ifcont:                                           ; preds = %else, %next_stmt88
  %128 = load i32, ptr @__exception_flag, align 4
  %129 = icmp ne i32 %128, 0
  br i1 %129, label %unwind91, label %next_stmt90

forin_cond84:                                     ; preds = %forin_inc86, %then
  %130 = load i32, ptr %forin_idx83, align 4
  %131 = icmp slt i32 %130, %str_len
  br i1 %131, label %forin_body85, label %forin_after87

forin_body85:                                     ; preds = %forin_cond84
  %char_val = call ptr @moksha_string_get_char(ptr %127, i32 %130)
  store ptr %char_val, ptr %ch, align 8
  br label %forin_inc86

forin_inc86:                                      ; preds = %forin_body85
  %132 = add i32 %130, 1
  store i32 %132, ptr %forin_idx83, align 4
  br label %forin_cond84

forin_after87:                                    ; preds = %forin_cond84
  %133 = load i32, ptr @__exception_flag, align 4
  %134 = icmp ne i32 %133, 0
  br i1 %134, label %unwind89, label %next_stmt88

next_stmt88:                                      ; preds = %forin_after87
  br label %ifcont

unwind89:                                         ; preds = %forin_after87
  ret i32 0

next_stmt90:                                      ; preds = %ifcont
  %135 = load ptr, ptr %w2, align 8
  %136 = icmp eq ptr %135, null
  br i1 %136, label %len_null93, label %len_safe92

unwind91:                                         ; preds = %ifcont
  ret i32 0

len_safe92:                                       ; preds = %next_stmt90
  %137 = getelementptr inbounds i8, ptr %135, i32 16
  %inline_len95 = load i32, ptr %137, align 4
  br label %len_merge94

len_null93:                                       ; preds = %next_stmt90
  br label %len_merge94

len_merge94:                                      ; preds = %len_safe92, %len_null93
  %len_res96 = phi i32 [ 0, %len_null93 ], [ %inline_len95, %len_safe92 ]
  %icmptmp97 = icmp sgt i32 %len_res96, 3
  br i1 %icmptmp97, label %then98, label %else99

then98:                                           ; preds = %len_merge94
  %138 = load ptr, ptr %ar22, align 8
  %139 = icmp eq ptr %138, null
  br i1 %139, label %len_null102, label %len_safe101

else99:                                           ; preds = %len_merge94
  br label %ifcont100

ifcont100:                                        ; preds = %else99, %next_stmt112
  %140 = load i32, ptr @__exception_flag, align 4
  %141 = icmp ne i32 %140, 0
  br i1 %141, label %unwind115, label %next_stmt114

len_safe101:                                      ; preds = %then98
  %142 = getelementptr inbounds i8, ptr %138, i32 12
  %inline_len104 = load i32, ptr %142, align 4
  br label %len_merge103

len_null102:                                      ; preds = %then98
  br label %len_merge103

len_merge103:                                     ; preds = %len_safe101, %len_null102
  %len_res105 = phi i32 [ 0, %len_null102 ], [ %inline_len104, %len_safe101 ]
  %icmptmp106 = icmp sgt i32 %len_res105, 3
  br i1 %icmptmp106, label %then107, label %else108

then107:                                          ; preds = %len_merge103
  %143 = load ptr, ptr %w2, align 8
  %144 = call ptr @moksha_string_get_char(ptr %143, i32 3)
  %145 = load ptr, ptr %ar22, align 8
  call void @moksha_array_set(ptr %145, i32 3, ptr %144)
  %146 = load i32, ptr @__exception_flag, align 4
  %147 = icmp ne i32 %146, 0
  br i1 %147, label %unwind111, label %next_stmt110

else108:                                          ; preds = %len_merge103
  br label %ifcont109

ifcont109:                                        ; preds = %else108, %next_stmt110
  %148 = load i32, ptr @__exception_flag, align 4
  %149 = icmp ne i32 %148, 0
  br i1 %149, label %unwind113, label %next_stmt112

next_stmt110:                                     ; preds = %then107
  br label %ifcont109

unwind111:                                        ; preds = %then107
  ret i32 0

next_stmt112:                                     ; preds = %ifcont109
  br label %ifcont100

unwind113:                                        ; preds = %ifcont109
  ret i32 0

next_stmt114:                                     ; preds = %ifcont100
  %150 = load ptr, ptr %ar22, align 8
  %151 = icmp eq ptr %150, null
  br i1 %151, label %len_null117, label %len_safe116

unwind115:                                        ; preds = %ifcont100
  ret i32 0

len_safe116:                                      ; preds = %next_stmt114
  %152 = getelementptr inbounds i8, ptr %150, i32 12
  %inline_len119 = load i32, ptr %152, align 4
  br label %len_merge118

len_null117:                                      ; preds = %next_stmt114
  br label %len_merge118

len_merge118:                                     ; preds = %len_safe116, %len_null117
  %len_res120 = phi i32 [ 0, %len_null117 ], [ %inline_len119, %len_safe116 ]
  %icmptmp121 = icmp sgt i32 %len_res120, 3
  br i1 %icmptmp121, label %then122, label %else123

then122:                                          ; preds = %len_merge118
  %153 = load ptr, ptr %ar22, align 8
  %154 = getelementptr inbounds i8, ptr %153, i32 12
  %arr_len125 = load i32, ptr %154, align 4
  %155 = icmp uge i32 3, %arr_len125
  br i1 %155, label %bounds_fail127, label %bounds_ok126

else123:                                          ; preds = %len_merge118
  br label %ifcont124

ifcont124:                                        ; preds = %else123, %next_stmt129
  %156 = load i32, ptr @__exception_flag, align 4
  %157 = icmp ne i32 %156, 0
  br i1 %157, label %unwind132, label %next_stmt131

bounds_ok126:                                     ; preds = %then122
  %158 = getelementptr inbounds i8, ptr %153, i32 16
  %159 = getelementptr ptr, ptr %158, i32 3
  %fast_elem128 = load ptr, ptr %159, align 8
  %160 = load ptr, ptr %tab38, align 8
  %161 = call ptr @moksha_get_static_string(ptr @__str_lit_cache_11, ptr @19)
  %162 = call ptr @moksha_table_set(ptr %160, ptr %161, ptr %fast_elem128)
  %163 = load i32, ptr @__exception_flag, align 4
  %164 = icmp ne i32 %163, 0
  br i1 %164, label %unwind130, label %next_stmt129

bounds_fail127:                                   ; preds = %then122
  %165 = call ptr @moksha_box_string(ptr @18)
  call void @moksha_throw_exception(ptr %165)
  store ptr %165, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

next_stmt129:                                     ; preds = %bounds_ok126
  br label %ifcont124

unwind130:                                        ; preds = %bounds_ok126
  ret i32 0

next_stmt131:                                     ; preds = %ifcont124
  %166 = load ptr, ptr %ar22, align 8
  %167 = icmp eq ptr %166, null
  br i1 %167, label %len_null135, label %len_safe134

unwind132:                                        ; preds = %ifcont124
  ret i32 0

len_safe134:                                      ; preds = %next_stmt131
  %168 = getelementptr inbounds i8, ptr %166, i32 12
  %inline_len137 = load i32, ptr %168, align 4
  br label %len_merge136

len_null135:                                      ; preds = %next_stmt131
  br label %len_merge136

len_merge136:                                     ; preds = %len_safe134, %len_null135
  %len_res138 = phi i32 [ 0, %len_null135 ], [ %inline_len137, %len_safe134 ]
  store i32 %len_res138, ptr %len133, align 4
  store i32 %len_res138, ptr %len139, align 4
  %169 = load i32, ptr @__exception_flag, align 4
  %170 = icmp ne i32 %169, 0
  br i1 %170, label %unwind141, label %next_stmt140

next_stmt140:                                     ; preds = %len_merge136
  %171 = load ptr, ptr %tab38, align 8
  call void @moksha_free(ptr %171)
  %172 = load ptr, ptr %ar22, align 8
  call void @moksha_free(ptr %172)
  br label %forinc17

unwind141:                                        ; preds = %len_merge136
  ret i32 0
}

declare ptr @moksha_get_static_string(ptr, ptr)

declare i32 @moksha_table_capacity(ptr)

declare ptr @moksha_table_get_entry_key(ptr, i32)

declare ptr @moksha_table_get_entry_val(ptr, i32)
