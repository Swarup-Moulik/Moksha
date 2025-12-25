; ModuleID = 'moksha_module'
source_filename = "moksha_module"

@__exception_flag = external global i32
@__exception_val = external global ptr
@0 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@1 = private unnamed_addr constant [11 x i8] c"Result :- \00", align 1
@2 = private unnamed_addr constant [8 x i8] c" , done\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@5 = private unnamed_addr constant [11 x i8] c"Result :- \00", align 1
@6 = private unnamed_addr constant [8 x i8] c" , done\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %c3 = alloca i32, align 4
  %c = alloca i32, align 4
  %b2 = alloca i32, align 4
  %b = alloca i32, align 4
  %a1 = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 60, ptr %a, align 4
  store i32 60, ptr %a1, align 4
  store i32 9, ptr %b, align 4
  store i32 9, ptr %b2, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %1 = call ptr @moksha_box_string(ptr @1)
  %2 = call ptr @moksha_string_concat(ptr %0, ptr %1)
  %3 = load i32, ptr %a1, align 4
  %4 = load i32, ptr %b2, align 4
  %addtmp = add i32 %3, %4
  %i2s = call ptr @moksha_int_to_str(i32 %addtmp)
  %5 = call ptr @moksha_string_concat(ptr %2, ptr %i2s)
  %6 = call ptr @moksha_box_string(ptr @2)
  %7 = call ptr @moksha_string_concat(ptr %5, ptr %6)
  %print_unbox = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox)
  store i32 7, ptr %c, align 4
  store i32 7, ptr %c3, align 4
  %9 = call ptr @moksha_box_string(ptr @4)
  %10 = call ptr @moksha_box_string(ptr @5)
  %11 = call ptr @moksha_string_concat(ptr %9, ptr %10)
  %12 = load i32, ptr %a1, align 4
  %13 = load i32, ptr %c3, align 4
  %addtmp4 = add i32 %12, %13
  %i2s5 = call ptr @moksha_int_to_str(i32 %addtmp4)
  %14 = call ptr @moksha_string_concat(ptr %11, ptr %i2s5)
  %15 = call ptr @moksha_box_string(ptr @6)
  %16 = call ptr @moksha_string_concat(ptr %14, ptr %15)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %16)
  %17 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox6)
  ret i32 0
}
