; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Env_12 = type { ptr, ptr }

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [46 x i8] c"==== FUNCTIONS, LAMBDAS & CLOSURES SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [25 x i8] c"\0A[1. Standard Functions]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [15 x i8] c"Good evening, \00", align 1
@5 = private unnamed_addr constant [4 x i8] c"Hi \00", align 1
@6 = private unnamed_addr constant [21 x i8] c"System Status Code: \00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@8 = private unnamed_addr constant [16 x i8] c"add(10.5, 20): \00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@10 = private unnamed_addr constant [18 x i8] c"circleArea(2.0): \00", align 1
@11 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@12 = private unnamed_addr constant [7 x i8] c"Swarup\00", align 1
@13 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@14 = private unnamed_addr constant [7 x i8] c"Moksha\00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@16 = private unnamed_addr constant [14 x i8] c"\0A[2. Lambdas]\00", align 1
@17 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@18 = private unnamed_addr constant [22 x i8] c"Square of 5 (Typed): \00", align 1
@19 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@20 = private unnamed_addr constant [27 x i8] c"Multiply 6 * 7 (Untyped): \00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [18 x i8] c"Max of (10, 42): \00", align 1
@23 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@24 = private unnamed_addr constant [10 x i8] c"Hello 1, \00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [7 x i8] c"Swarup\00", align 1
@27 = private unnamed_addr constant [10 x i8] c"Hello 2, \00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@29 = private unnamed_addr constant [7 x i8] c"Swarup\00", align 1
@30 = private unnamed_addr constant [10 x i8] c"Hello 3, \00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@32 = private unnamed_addr constant [7 x i8] c"Swarup\00", align 1
@33 = private unnamed_addr constant [5 x i8] c" add\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"new\00", align 1
@35 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@36 = private unnamed_addr constant [22 x i8] c"Higher Order Result: \00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@38 = private unnamed_addr constant [11 x i8] c"Log Level \00", align 1
@39 = private unnamed_addr constant [3 x i8] c": \00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@41 = private unnamed_addr constant [15 x i8] c"Error occurred\00", align 1
@42 = private unnamed_addr constant [15 x i8] c"System failure\00", align 1
@43 = private unnamed_addr constant [15 x i8] c"Fibonacci(6): \00", align 1
@44 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@45 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@46 = private unnamed_addr constant [6 x i8] c"X :- \00", align 1
@47 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@48 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@49 = private unnamed_addr constant [6 x i8] c"X :- \00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@51 = private unnamed_addr constant [17 x i8] c"Array Lambda 1: \00", align 1
@52 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@53 = private unnamed_addr constant [17 x i8] c"Array Lambda 2: \00", align 1
@54 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@55 = private unnamed_addr constant [10 x i8] c"Counter: \00", align 1
@56 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@57 = private unnamed_addr constant [10 x i8] c"Counter: \00", align 1
@58 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %myCounter84 = alloca ptr, align 8
  %myCounter = alloca ptr, align 8
  %makeCounter80 = alloca ptr, align 8
  %makeCounter = alloca ptr, align 8
  %ops69 = alloca ptr, align 8
  %ops = alloca ptr, align 8
  %f61 = alloca ptr, align 8
  %f = alloca ptr, align 8
  %x57 = alloca i32, align 4
  %x = alloca i32, align 4
  %res53 = alloca ptr, align 8
  %res = alloca ptr, align 8
  %hell46 = alloca ptr, align 8
  %hell = alloca ptr, align 8
  %greet340 = alloca ptr, align 8
  %greet3 = alloca ptr, align 8
  %greet234 = alloca ptr, align 8
  %greet2 = alloca ptr, align 8
  %greet128 = alloca ptr, align 8
  %greet1 = alloca ptr, align 8
  %max21 = alloca ptr, align 8
  %max = alloca ptr, align 8
  %multiply14 = alloca ptr, align 8
  %multiply = alloca ptr, align 8
  %square10 = alloca ptr, align 8
  %square = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  %4 = call ptr @moksha_box_string(ptr @8)
  %5 = call ptr @moksha_box_double(double 1.050000e+01)
  %call = call double @add(ptr %5, i32 20)
  %6 = call ptr @moksha_double_to_str(double %call)
  %7 = call ptr @moksha_box_string(ptr %6)
  %8 = call ptr @moksha_string_concat(ptr %4, ptr %7)
  %print_unbox2 = call ptr @moksha_unbox_string(ptr %8)
  %9 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox2)
  %10 = call ptr @moksha_box_string(ptr @10)
  %call3 = call double @circleArea(double 2.000000e+00)
  %11 = call ptr @moksha_double_to_str(double %call3)
  %12 = call ptr @moksha_box_string(ptr %11)
  %13 = call ptr @moksha_string_concat(ptr %10, ptr %12)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %13)
  %14 = call i32 (ptr, ...) @printf(ptr @11, ptr %print_unbox4)
  %15 = call ptr @moksha_box_string(ptr @12)
  %call5 = call ptr @greet(ptr %15, i1 true)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %call5)
  %16 = call i32 (ptr, ...) @printf(ptr @13, ptr %print_unbox6)
  %17 = call ptr @moksha_box_string(ptr @14)
  %call7 = call ptr @greet(ptr %17, i1 false)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %call7)
  %18 = call i32 (ptr, ...) @printf(ptr @15, ptr %print_unbox8)
  %19 = call ptr @moksha_box_int(i32 200)
  call void @printStatus(ptr %19)
  %20 = call ptr @moksha_box_string(ptr @16)
  %print_unbox9 = call ptr @moksha_unbox_string(ptr %20)
  %21 = call i32 (ptr, ...) @printf(ptr @17, ptr %print_unbox9)
  %env_alloc = call ptr @malloc(i64 8)
  %closure = call ptr @moksha_create_closure(ptr @__lambda_0, i32 8, ptr %env_alloc)
  store ptr %closure, ptr %square, align 8
  store ptr %closure, ptr %square10, align 8
  %22 = call ptr @moksha_box_string(ptr @18)
  %23 = load ptr, ptr %square10, align 8
  %24 = call ptr @moksha_box_int(i32 5)
  %raw_func_ptr = call ptr @moksha_get_closure_func(ptr %23)
  %env_ptr = call ptr @moksha_get_closure_env(ptr %23)
  %closure_call = call ptr %raw_func_ptr(ptr %env_ptr, ptr %24)
  %25 = call ptr @moksha_any_to_str(ptr %closure_call)
  %26 = call ptr @moksha_box_string(ptr %25)
  %27 = call ptr @moksha_string_concat(ptr %22, ptr %26)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %27)
  %28 = call i32 (ptr, ...) @printf(ptr @19, ptr %print_unbox11)
  %env_alloc12 = call ptr @malloc(i64 8)
  %closure13 = call ptr @moksha_create_closure(ptr @__lambda_1, i32 8, ptr %env_alloc12)
  store ptr %closure13, ptr %multiply, align 8
  store ptr %closure13, ptr %multiply14, align 8
  %29 = call ptr @moksha_box_string(ptr @20)
  %30 = load ptr, ptr %multiply14, align 8
  %31 = call ptr @moksha_box_int(i32 6)
  %32 = call ptr @moksha_box_int(i32 7)
  %raw_func_ptr15 = call ptr @moksha_get_closure_func(ptr %30)
  %env_ptr16 = call ptr @moksha_get_closure_env(ptr %30)
  %closure_call17 = call ptr %raw_func_ptr15(ptr %env_ptr16, ptr %31, ptr %32)
  %33 = call ptr @moksha_any_to_str(ptr %closure_call17)
  %34 = call ptr @moksha_box_string(ptr %33)
  %35 = call ptr @moksha_string_concat(ptr %29, ptr %34)
  %print_unbox18 = call ptr @moksha_unbox_string(ptr %35)
  %36 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox18)
  %env_alloc19 = call ptr @malloc(i64 8)
  %closure20 = call ptr @moksha_create_closure(ptr @__lambda_2, i32 8, ptr %env_alloc19)
  store ptr %closure20, ptr %max, align 8
  store ptr %closure20, ptr %max21, align 8
  %37 = call ptr @moksha_box_string(ptr @22)
  %38 = load ptr, ptr %max21, align 8
  %39 = call ptr @moksha_box_int(i32 10)
  %40 = call ptr @moksha_box_int(i32 42)
  %raw_func_ptr22 = call ptr @moksha_get_closure_func(ptr %38)
  %env_ptr23 = call ptr @moksha_get_closure_env(ptr %38)
  %closure_call24 = call ptr %raw_func_ptr22(ptr %env_ptr23, ptr %39, ptr %40)
  %41 = call ptr @moksha_any_to_str(ptr %closure_call24)
  %42 = call ptr @moksha_box_string(ptr %41)
  %43 = call ptr @moksha_string_concat(ptr %37, ptr %42)
  %print_unbox25 = call ptr @moksha_unbox_string(ptr %43)
  %44 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox25)
  %env_alloc26 = call ptr @malloc(i64 8)
  %closure27 = call ptr @moksha_create_closure(ptr @__lambda_3, i32 8, ptr %env_alloc26)
  store ptr %closure27, ptr %greet1, align 8
  store ptr %closure27, ptr %greet128, align 8
  %45 = load ptr, ptr %greet128, align 8
  %46 = call ptr @moksha_box_string(ptr @26)
  %raw_func_ptr29 = call ptr @moksha_get_closure_func(ptr %45)
  %env_ptr30 = call ptr @moksha_get_closure_env(ptr %45)
  %closure_call31 = call ptr %raw_func_ptr29(ptr %env_ptr30, ptr %46)
  %env_alloc32 = call ptr @malloc(i64 8)
  %closure33 = call ptr @moksha_create_closure(ptr @__lambda_4, i32 8, ptr %env_alloc32)
  store ptr %closure33, ptr %greet2, align 8
  store ptr %closure33, ptr %greet234, align 8
  %47 = load ptr, ptr %greet234, align 8
  %48 = call ptr @moksha_box_string(ptr @29)
  %raw_func_ptr35 = call ptr @moksha_get_closure_func(ptr %47)
  %env_ptr36 = call ptr @moksha_get_closure_env(ptr %47)
  %closure_call37 = call ptr %raw_func_ptr35(ptr %env_ptr36, ptr %48)
  %env_alloc38 = call ptr @malloc(i64 8)
  %closure39 = call ptr @moksha_create_closure(ptr @__lambda_5, i32 8, ptr %env_alloc38)
  store ptr %closure39, ptr %greet3, align 8
  store ptr %closure39, ptr %greet340, align 8
  %49 = load ptr, ptr %greet340, align 8
  %50 = call ptr @moksha_box_string(ptr @32)
  %raw_func_ptr41 = call ptr @moksha_get_closure_func(ptr %49)
  %env_ptr42 = call ptr @moksha_get_closure_env(ptr %49)
  %closure_call43 = call ptr %raw_func_ptr41(ptr %env_ptr42, ptr %50)
  %env_alloc44 = call ptr @malloc(i64 8)
  %closure45 = call ptr @moksha_create_closure(ptr @__lambda_6, i32 8, ptr %env_alloc44)
  store ptr %closure45, ptr %hell, align 8
  store ptr %closure45, ptr %hell46, align 8
  %51 = load ptr, ptr %hell46, align 8
  %52 = call ptr @moksha_box_string(ptr @34)
  %raw_func_ptr47 = call ptr @moksha_get_closure_func(ptr %51)
  %env_ptr48 = call ptr @moksha_get_closure_env(ptr %51)
  %closure_call49 = call ptr %raw_func_ptr47(ptr %env_ptr48, ptr %52)
  %to_str = call ptr @moksha_any_to_str(ptr %closure_call49)
  %53 = call i32 (ptr, ...) @printf(ptr @35, ptr %to_str)
  %env_alloc50 = call ptr @malloc(i64 8)
  %closure51 = call ptr @moksha_create_closure(ptr @__lambda_7, i32 8, ptr %env_alloc50)
  %call52 = call i32 @applyOp(i32 11, i32 5, ptr %closure51)
  %box_i = call ptr @moksha_box_int(i32 %call52)
  store ptr %box_i, ptr %res, align 8
  store ptr %box_i, ptr %res53, align 8
  %54 = call ptr @moksha_box_string(ptr @36)
  %55 = load ptr, ptr %res53, align 8
  %56 = call ptr @moksha_any_to_str(ptr %55)
  %57 = call ptr @moksha_box_string(ptr %56)
  %58 = call ptr @moksha_string_concat(ptr %54, ptr %57)
  %print_unbox54 = call ptr @moksha_unbox_string(ptr %58)
  %59 = call i32 (ptr, ...) @printf(ptr @37, ptr %print_unbox54)
  %60 = call ptr @moksha_box_string(ptr @41)
  call void @log(ptr %60, i32 1)
  %61 = call ptr @moksha_box_string(ptr @42)
  call void @log(ptr %61, i32 3)
  %62 = call ptr @moksha_box_string(ptr @43)
  %63 = call ptr @moksha_box_int(i32 6)
  %call55 = call i32 @fib(ptr %63)
  %64 = call ptr @moksha_int_to_str(i32 %call55)
  %65 = call ptr @moksha_box_string(ptr %64)
  %66 = call ptr @moksha_string_concat(ptr %62, ptr %65)
  %print_unbox56 = call ptr @moksha_unbox_string(ptr %66)
  %67 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox56)
  store i32 100, ptr %x, align 4
  store i32 100, ptr %x57, align 4
  %68 = call ptr @moksha_box_string(ptr @45)
  %69 = call ptr @moksha_box_string(ptr @46)
  %70 = call ptr @moksha_string_concat(ptr %68, ptr %69)
  %71 = load i32, ptr %x57, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %71)
  %72 = call ptr @moksha_box_string(ptr %i2s)
  %73 = call ptr @moksha_string_concat(ptr %70, ptr %72)
  %print_unbox58 = call ptr @moksha_unbox_string(ptr %73)
  %74 = call i32 (ptr, ...) @printf(ptr @47, ptr %print_unbox58)
  %env_alloc59 = call ptr @malloc(i64 8)
  %closure60 = call ptr @moksha_create_closure(ptr @__lambda_8, i32 8, ptr %env_alloc59)
  store ptr %closure60, ptr %f, align 8
  store ptr %closure60, ptr %f61, align 8
  %75 = load ptr, ptr %f61, align 8
  %76 = call ptr @moksha_box_int(i32 20)
  %raw_func_ptr62 = call ptr @moksha_get_closure_func(ptr %75)
  %env_ptr63 = call ptr @moksha_get_closure_env(ptr %75)
  %closure_call64 = call ptr %raw_func_ptr62(ptr %env_ptr63, ptr %76)
  %77 = call ptr @moksha_alloc_array(i32 2)
  %env_alloc65 = call ptr @malloc(i64 8)
  %closure66 = call ptr @moksha_create_closure(ptr @__lambda_9, i32 8, ptr %env_alloc65)
  %78 = call ptr @moksha_array_push_val(ptr %77, ptr %closure66)
  %env_alloc67 = call ptr @malloc(i64 8)
  %closure68 = call ptr @moksha_create_closure(ptr @__lambda_10, i32 8, ptr %env_alloc67)
  %79 = call ptr @moksha_array_push_val(ptr %77, ptr %closure68)
  store ptr %77, ptr %ops, align 8
  store ptr %77, ptr %ops69, align 8
  %80 = call ptr @moksha_box_string(ptr @51)
  %81 = load ptr, ptr %ops69, align 8
  %82 = call ptr @moksha_array_get(ptr %81, i32 0)
  %83 = call ptr @moksha_box_int(i32 5)
  %raw_func_ptr70 = call ptr @moksha_get_closure_func(ptr %82)
  %env_ptr71 = call ptr @moksha_get_closure_env(ptr %82)
  %closure_call72 = call ptr %raw_func_ptr70(ptr %env_ptr71, ptr %83)
  %84 = call ptr @moksha_any_to_str(ptr %closure_call72)
  %85 = call ptr @moksha_box_string(ptr %84)
  %86 = call ptr @moksha_string_concat(ptr %80, ptr %85)
  %print_unbox73 = call ptr @moksha_unbox_string(ptr %86)
  %87 = call i32 (ptr, ...) @printf(ptr @52, ptr %print_unbox73)
  %88 = call ptr @moksha_box_string(ptr @53)
  %89 = load ptr, ptr %ops69, align 8
  %90 = call ptr @moksha_array_get(ptr %89, i32 1)
  %91 = call ptr @moksha_box_int(i32 5)
  %raw_func_ptr74 = call ptr @moksha_get_closure_func(ptr %90)
  %env_ptr75 = call ptr @moksha_get_closure_env(ptr %90)
  %closure_call76 = call ptr %raw_func_ptr74(ptr %env_ptr75, ptr %91)
  %92 = call ptr @moksha_any_to_str(ptr %closure_call76)
  %93 = call ptr @moksha_box_string(ptr %92)
  %94 = call ptr @moksha_string_concat(ptr %88, ptr %93)
  %print_unbox77 = call ptr @moksha_unbox_string(ptr %94)
  %95 = call i32 (ptr, ...) @printf(ptr @54, ptr %print_unbox77)
  %env_alloc78 = call ptr @malloc(i64 8)
  %closure79 = call ptr @moksha_create_closure(ptr @__lambda_11, i32 8, ptr %env_alloc78)
  store ptr %closure79, ptr %makeCounter, align 8
  store ptr %closure79, ptr %makeCounter80, align 8
  %96 = load ptr, ptr %makeCounter80, align 8
  %raw_func_ptr81 = call ptr @moksha_get_closure_func(ptr %96)
  %env_ptr82 = call ptr @moksha_get_closure_env(ptr %96)
  %closure_call83 = call ptr %raw_func_ptr81(ptr %env_ptr82)
  store ptr %closure_call83, ptr %myCounter, align 8
  store ptr %closure_call83, ptr %myCounter84, align 8
  %97 = call ptr @moksha_box_string(ptr @55)
  %98 = load ptr, ptr %myCounter84, align 8
  %raw_func_ptr85 = call ptr @moksha_get_closure_func(ptr %98)
  %env_ptr86 = call ptr @moksha_get_closure_env(ptr %98)
  %closure_call87 = call ptr %raw_func_ptr85(ptr %env_ptr86)
  %99 = call ptr @moksha_any_to_str(ptr %closure_call87)
  %100 = call ptr @moksha_box_string(ptr %99)
  %101 = call ptr @moksha_string_concat(ptr %97, ptr %100)
  %print_unbox88 = call ptr @moksha_unbox_string(ptr %101)
  %102 = call i32 (ptr, ...) @printf(ptr @56, ptr %print_unbox88)
  %103 = call ptr @moksha_box_string(ptr @57)
  %104 = load ptr, ptr %myCounter84, align 8
  %raw_func_ptr89 = call ptr @moksha_get_closure_func(ptr %104)
  %env_ptr90 = call ptr @moksha_get_closure_env(ptr %104)
  %closure_call91 = call ptr %raw_func_ptr89(ptr %env_ptr90)
  %105 = call ptr @moksha_any_to_str(ptr %closure_call91)
  %106 = call ptr @moksha_box_string(ptr %105)
  %107 = call ptr @moksha_string_concat(ptr %103, ptr %106)
  %print_unbox92 = call ptr @moksha_unbox_string(ptr %107)
  %108 = call i32 (ptr, ...) @printf(ptr @58, ptr %print_unbox92)
  ret i32 0
}

define double @add(ptr %a, i32 %b) {
entry:
  %b2 = alloca i32, align 4
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store i32 %b, ptr %b2, align 4
  %0 = load ptr, ptr %a1, align 8
  %1 = load i32, ptr %b2, align 4
  %unbox_L_f = call double @moksha_unbox_double(ptr %0)
  %2 = sitofp i32 %1 to double
  %faddtmp = fadd double %unbox_L_f, %2
  ret double %faddtmp
}

define double @circleArea(double %r) {
entry:
  %r1 = alloca double, align 8
  store double %r, ptr %r1, align 8
  %0 = load double, ptr %r1, align 8
  %fmultmp = fmul double 3.141590e+00, %0
  %1 = load double, ptr %r1, align 8
  %fmultmp2 = fmul double %fmultmp, %1
  ret double %fmultmp2
}

define ptr @greet(ptr %name, i1 %isFormal) {
entry:
  %isFormal2 = alloca i1, align 1
  %name1 = alloca ptr, align 8
  store ptr %name, ptr %name1, align 8
  store i1 %isFormal, ptr %isFormal2, align 1
  %0 = load i1, ptr %isFormal2, align 1
  br i1 %0, label %then, label %else

then:                                             ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @4)
  %2 = load ptr, ptr %name1, align 8
  %3 = call ptr @moksha_string_concat(ptr %1, ptr %2)
  ret ptr %3

else:                                             ; preds = %entry
  %4 = call ptr @moksha_box_string(ptr @5)
  %5 = load ptr, ptr %name1, align 8
  %6 = call ptr @moksha_string_concat(ptr %4, ptr %5)
  ret ptr %6

ifcont:                                           ; No predecessors!
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %ifcont
  ret ptr null

unwind:                                           ; preds = %ifcont
  ret ptr null
}

define void @printStatus(ptr %code) {
entry:
  %code1 = alloca ptr, align 8
  store ptr %code, ptr %code1, align 8
  %0 = call ptr @moksha_box_string(ptr @6)
  %1 = load ptr, ptr %code1, align 8
  %2 = call ptr @moksha_any_to_str(ptr %1)
  %3 = call ptr @moksha_box_string(ptr %2)
  %4 = call ptr @moksha_string_concat(ptr %0, ptr %3)
  %print_unbox = call ptr @moksha_unbox_string(ptr %4)
  %5 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox)
  %6 = load i32, ptr @__exception_flag, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define internal ptr @__lambda_0(ptr %0, ptr %x) {
entry:
  %x1 = alloca i32, align 4
  %1 = call i32 @moksha_unbox_int(ptr %x)
  store i32 %1, ptr %x1, align 4
  %2 = load i32, ptr %x1, align 4
  %3 = load i32, ptr %x1, align 4
  %multmp = mul i32 %2, %3
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %6 = call ptr @moksha_box_int(i32 %multmp)
  ret ptr %6

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_1(ptr %0, ptr %a, ptr %b) {
entry:
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  %1 = load ptr, ptr %a1, align 8
  %2 = load ptr, ptr %b2, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %1)
  %unbox_R_f = call double @moksha_unbox_double(ptr %2)
  %fmultmp = fmul double %unbox_L_f, %unbox_R_f
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %5 = call ptr @moksha_box_double(double %fmultmp)
  ret ptr %5

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_2(ptr %0, ptr %a, ptr %b) {
entry:
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  %1 = load ptr, ptr %a1, align 8
  %2 = load ptr, ptr %b2, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %1)
  %unbox_R_f = call double @moksha_unbox_double(ptr %2)
  %fcmptmp = fcmp ogt double %unbox_L_f, %unbox_R_f
  br i1 %fcmptmp, label %then, label %else

then:                                             ; preds = %entry
  %3 = load ptr, ptr %a1, align 8
  ret ptr %3

else:                                             ; preds = %entry
  %4 = load ptr, ptr %b2, align 8
  ret ptr %4

ifcont:                                           ; No predecessors!
  %5 = load i32, ptr @__exception_flag, align 4
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %ifcont
  ret ptr %4

unwind:                                           ; preds = %ifcont
  ret ptr null
}

define internal ptr @__lambda_3(ptr %0, ptr %name) {
entry:
  %name1 = alloca ptr, align 8
  store ptr %name, ptr %name1, align 8
  %1 = call ptr @moksha_box_string(ptr @24)
  %2 = load ptr, ptr %name1, align 8
  %3 = call ptr @moksha_any_to_str(ptr %2)
  %4 = call ptr @moksha_box_string(ptr %3)
  %5 = call ptr @moksha_string_concat(ptr %1, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @25, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret ptr %5

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_4(ptr %0, ptr %name) {
entry:
  %name1 = alloca ptr, align 8
  store ptr %name, ptr %name1, align 8
  %1 = call ptr @moksha_box_string(ptr @27)
  %2 = load ptr, ptr %name1, align 8
  %3 = call ptr @moksha_string_concat(ptr %1, ptr %2)
  %print_unbox = call ptr @moksha_unbox_string(ptr %3)
  %4 = call i32 (ptr, ...) @printf(ptr @28, ptr %print_unbox)
  %5 = load i32, ptr @__exception_flag, align 4
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret ptr %3

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_5(ptr %0, ptr %name) {
entry:
  %name1 = alloca ptr, align 8
  store ptr %name, ptr %name1, align 8
  %1 = call ptr @moksha_box_string(ptr @30)
  %2 = load ptr, ptr %name1, align 8
  %3 = call ptr @moksha_any_to_str(ptr %2)
  %4 = call ptr @moksha_box_string(ptr %3)
  %5 = call ptr @moksha_string_concat(ptr %1, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @31, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret ptr %5

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_6(ptr %0, ptr %word) {
entry:
  %word1 = alloca ptr, align 8
  store ptr %word, ptr %word1, align 8
  %1 = load ptr, ptr %word1, align 8
  %2 = call ptr @moksha_box_string(ptr @33)
  %3 = call ptr @moksha_any_to_str(ptr %1)
  %4 = call ptr @moksha_box_string(ptr %3)
  %5 = call ptr @moksha_string_concat(ptr %4, ptr %2)
  ret ptr %5
}

define i32 @applyOp(i32 %a, i32 %b, ptr %operation) {
entry:
  %operation3 = alloca ptr, align 8
  %b2 = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  store i32 %b, ptr %b2, align 4
  store ptr %operation, ptr %operation3, align 8
  %0 = load ptr, ptr %operation3, align 8
  %1 = load i32, ptr %a1, align 4
  %2 = call ptr @moksha_box_int(i32 %1)
  %3 = load i32, ptr %b2, align 4
  %4 = call ptr @moksha_box_int(i32 %3)
  %raw_func_ptr = call ptr @moksha_get_closure_func(ptr %0)
  %env_ptr = call ptr @moksha_get_closure_env(ptr %0)
  %closure_call = call ptr %raw_func_ptr(ptr %env_ptr, ptr %2, ptr %4)
  %5 = call i32 @moksha_unbox_int(ptr %closure_call)
  ret i32 %5
}

define internal ptr @__lambda_7(ptr %0, ptr %x, ptr %y) {
entry:
  %y2 = alloca ptr, align 8
  %x1 = alloca ptr, align 8
  store ptr %x, ptr %x1, align 8
  store ptr %y, ptr %y2, align 8
  %1 = load ptr, ptr %x1, align 8
  %2 = load ptr, ptr %y2, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %1)
  %unbox_R_f = call double @moksha_unbox_double(ptr %2)
  %fsubtmp = fsub double %unbox_L_f, %unbox_R_f
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %5 = call ptr @moksha_box_double(double %fsubtmp)
  ret ptr %5

unwind:                                           ; preds = %entry
  ret ptr null
}

define void @log(ptr %msg, i32 %level) {
entry:
  %level2 = alloca i32, align 4
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  store i32 %level, ptr %level2, align 4
  %0 = call ptr @moksha_box_string(ptr @38)
  %1 = load i32, ptr %level2, align 4
  %2 = call ptr @moksha_int_to_str(i32 %1)
  %3 = call ptr @moksha_box_string(ptr %2)
  %4 = call ptr @moksha_string_concat(ptr %0, ptr %3)
  %5 = call ptr @moksha_box_string(ptr @39)
  %6 = call ptr @moksha_string_concat(ptr %4, ptr %5)
  %7 = load ptr, ptr %msg1, align 8
  %8 = call ptr @moksha_string_concat(ptr %6, ptr %7)
  %print_unbox = call ptr @moksha_unbox_string(ptr %8)
  %9 = call i32 (ptr, ...) @printf(ptr @40, ptr %print_unbox)
  %10 = load i32, ptr @__exception_flag, align 4
  %11 = icmp ne i32 %10, 0
  br i1 %11, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define i32 @fib(ptr %n) {
entry:
  %n1 = alloca ptr, align 8
  store ptr %n, ptr %n1, align 8
  %0 = load ptr, ptr %n1, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %0)
  %fcmptmp = fcmp ole double %unbox_L_f, 1.000000e+00
  br i1 %fcmptmp, label %then, label %else

then:                                             ; preds = %entry
  %1 = load ptr, ptr %n1, align 8
  %2 = call i32 @moksha_unbox_int(ptr %1)
  ret i32 %2

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %ifcont
  %5 = load ptr, ptr %n1, align 8
  %unbox_L_f2 = call double @moksha_unbox_double(ptr %5)
  %fsubtmp = fsub double %unbox_L_f2, 1.000000e+00
  %6 = call ptr @moksha_box_double(double %fsubtmp)
  %call = call i32 @fib(ptr %6)
  %7 = load ptr, ptr %n1, align 8
  %unbox_L_f3 = call double @moksha_unbox_double(ptr %7)
  %fsubtmp4 = fsub double %unbox_L_f3, 2.000000e+00
  %8 = call ptr @moksha_box_double(double %fsubtmp4)
  %call5 = call i32 @fib(ptr %8)
  %addtmp = add i32 %call, %call5
  ret i32 %addtmp

unwind:                                           ; preds = %ifcont
  ret i32 0
}

define internal ptr @__lambda_8(ptr %0, ptr %x) {
entry:
  %x1 = alloca i32, align 4
  %1 = call i32 @moksha_unbox_int(ptr %x)
  store i32 %1, ptr %x1, align 4
  %2 = call ptr @moksha_box_string(ptr @48)
  %3 = call ptr @moksha_box_string(ptr @49)
  %4 = call ptr @moksha_string_concat(ptr %2, ptr %3)
  %5 = load i32, ptr %x1, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %5)
  %6 = call ptr @moksha_box_string(ptr %i2s)
  %7 = call ptr @moksha_string_concat(ptr %4, ptr %6)
  %print_unbox = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox)
  %9 = load i32, ptr @__exception_flag, align 4
  %10 = icmp ne i32 %9, 0
  br i1 %10, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret ptr %7

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_9(ptr %0, ptr %x) {
entry:
  %x1 = alloca ptr, align 8
  store ptr %x, ptr %x1, align 8
  %1 = load ptr, ptr %x1, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %1)
  %fmultmp = fmul double %unbox_L_f, 2.000000e+00
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %4 = call ptr @moksha_box_double(double %fmultmp)
  ret ptr %4

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_10(ptr %0, ptr %x) {
entry:
  %x1 = alloca ptr, align 8
  store ptr %x, ptr %x1, align 8
  %1 = load ptr, ptr %x1, align 8
  %2 = load ptr, ptr %x1, align 8
  %unbox_L_f = call double @moksha_unbox_double(ptr %1)
  %unbox_R_f = call double @moksha_unbox_double(ptr %2)
  %fmultmp = fmul double %unbox_L_f, %unbox_R_f
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %5 = call ptr @moksha_box_double(double %fmultmp)
  ret ptr %5

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_11(ptr %0) {
entry:
  %count1 = alloca ptr, align 8
  %count = alloca i32, align 4
  store i32 0, ptr %count, align 4
  %heap_var = call ptr @malloc(i64 4)
  store ptr %heap_var, ptr %count1, align 8
  store i32 0, ptr %heap_var, align 4
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %env_alloc = call ptr @malloc(i64 16)
  %3 = getelementptr inbounds nuw %Env_12, ptr %env_alloc, i32 0, i32 1
  %4 = load ptr, ptr %count1, align 8
  store ptr %4, ptr %3, align 8
  %closure = call ptr @moksha_create_closure(ptr @__lambda_12, i32 16, ptr %env_alloc)
  ret ptr %closure

unwind:                                           ; preds = %entry
  ret ptr null
}

define internal ptr @__lambda_12(ptr %0) {
entry:
  %count = alloca ptr, align 8
  %1 = getelementptr inbounds nuw %Env_12, ptr %0, i32 0, i32 1
  %cap_ptr = load ptr, ptr %1, align 8
  store ptr %cap_ptr, ptr %count, align 8
  %heap_addr = load ptr, ptr %count, align 8
  %2 = load i32, ptr %heap_addr, align 4
  %addtmp = add i32 %2, 1
  %heap_addr1 = load ptr, ptr %count, align 8
  store i32 %addtmp, ptr %heap_addr1, align 4
  %3 = load i32, ptr @__exception_flag, align 4
  %4 = icmp ne i32 %3, 0
  br i1 %4, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %heap_addr2 = load ptr, ptr %count, align 8
  %5 = load i32, ptr %heap_addr2, align 4
  %6 = call ptr @moksha_box_int(i32 %5)
  ret ptr %6

unwind:                                           ; preds = %entry
  ret ptr null
}
