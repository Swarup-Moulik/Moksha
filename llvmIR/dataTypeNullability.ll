; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Logger = type { ptr }

@__exception_flag = external global i32
@__exception_val = external global ptr
@0 = private unnamed_addr constant [41 x i8] c"==== DATA TYPES & NULLABILITY SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [14 x i8] c"\0A[Primitives]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@4 = private unnamed_addr constant [14 x i8] c"Swarup Moulik\00", align 1
@5 = private unnamed_addr constant [6 x i8] c"Int: \00", align 1
@6 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@8 = private unnamed_addr constant [9 x i8] c"Double: \00", align 1
@9 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@10 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@11 = private unnamed_addr constant [10 x i8] c"Boolean: \00", align 1
@12 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@13 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@14 = private unnamed_addr constant [9 x i8] c"String: \00", align 1
@15 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@16 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@17 = private unnamed_addr constant [15 x i8] c"\0A[Nullability]\00", align 1
@18 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@19 = private unnamed_addr constant [32 x i8] c"string? (initialized to null): \00", align 1
@20 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [14 x i8] c"I am safe now\00", align 1
@23 = private unnamed_addr constant [27 x i8] c"string? (assigned value): \00", align 1
@24 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [29 x i8] c"int? (initialized to null): \00", align 1
@27 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@29 = private unnamed_addr constant [24 x i8] c"int? (assigned value): \00", align 1
@30 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@32 = private unnamed_addr constant [32 x i8] c"double? (initialized to null): \00", align 1
@33 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@35 = private unnamed_addr constant [27 x i8] c"double? (assigned value): \00", align 1
@36 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@38 = private unnamed_addr constant [33 x i8] c"boolean? (initialized to null): \00", align 1
@39 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@41 = private unnamed_addr constant [27 x i8] c"boolean? (assigned true): \00", align 1
@42 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@43 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@44 = private unnamed_addr constant [12 x i8] c"\0A[Any Type]\00", align 1
@45 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@46 = private unnamed_addr constant [18 x i8] c"Any holding Int: \00", align 1
@47 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@48 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@49 = private unnamed_addr constant [17 x i8] c"Changing type...\00", align 1
@50 = private unnamed_addr constant [21 x i8] c"Any holding String: \00", align 1
@51 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@52 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@53 = private unnamed_addr constant [19 x i8] c"Any holding Null: \00", align 1
@54 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@55 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@56 = private unnamed_addr constant [22 x i8] c"\0A[Table / Dictionary]\00", align 1
@57 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@58 = private unnamed_addr constant [3 x i8] c"id\00", align 1
@59 = private unnamed_addr constant [9 x i8] c"username\00", align 1
@60 = private unnamed_addr constant [10 x i8] c"MokshaDev\00", align 1
@61 = private unnamed_addr constant [8 x i8] c"isAdmin\00", align 1
@62 = private unnamed_addr constant [18 x i8] c"Table Structure: \00", align 1
@63 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@64 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@65 = private unnamed_addr constant [25 x i8] c"\0A[Null Safety Operators]\00", align 1
@66 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@67 = private unnamed_addr constant [24 x i8] c"A. Null Coalescing (??)\00", align 1
@68 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@69 = private unnamed_addr constant [6 x i8] c"Guest\00", align 1
@70 = private unnamed_addr constant [28 x i8] c"User is null, fallback to: \00", align 1
@71 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@72 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@73 = private unnamed_addr constant [6 x i8] c"Admin\00", align 1
@74 = private unnamed_addr constant [6 x i8] c"Guest\00", align 1
@75 = private unnamed_addr constant [20 x i8] c"User is set, uses: \00", align 1
@76 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@77 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@78 = private unnamed_addr constant [29 x i8] c"Score is null, fallback to: \00", align 1
@79 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@80 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@81 = private unnamed_addr constant [27 x i8] c"\0AB. Optional Chaining (?.)\00", align 1
@82 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@83 = private unnamed_addr constant [44 x i8] c"1. Null Object Property (configObj?.port): \00", align 1
@84 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@85 = private unnamed_addr constant [5 x i8] c"port\00", align 1
@86 = private unnamed_addr constant [6 x i8] c"null\0A\00", align 1
@87 = private unnamed_addr constant [13 x i8] c"<Object %p>\0A\00", align 1
@88 = private unnamed_addr constant [46 x i8] c"2. Null Object Index (configObj?['server']): \00", align 1
@89 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@90 = private unnamed_addr constant [7 x i8] c"server\00", align 1
@91 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@92 = private unnamed_addr constant [5 x i8] c"port\00", align 1
@93 = private unnamed_addr constant [7 x i8] c"server\00", align 1
@94 = private unnamed_addr constant [10 x i8] c"localhost\00", align 1
@95 = private unnamed_addr constant [25 x i8] c"3. Valid Object Access: \00", align 1
@96 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@97 = private unnamed_addr constant [5 x i8] c"port\00", align 1
@98 = private unnamed_addr constant [2 x i8] c" \00", align 1
@99 = private unnamed_addr constant [7 x i8] c"server\00", align 1
@100 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@101 = private unnamed_addr constant [33 x i8] c"\0AC. Optional Method Calls (?.())\00", align 1
@102 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Logger = constant [1 x ptr] [ptr @Logger_log]
@103 = private unnamed_addr constant [9 x i8] c"Logger: \00", align 1
@104 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@105 = private unnamed_addr constant [41 x i8] c"1. Call on null object (myLogger?.log): \00", align 1
@106 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@107 = private unnamed_addr constant [22 x i8] c"This should not print\00", align 1
@108 = private unnamed_addr constant [32 x i8] c"(No output above means success)\00", align 1
@109 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@110 = private unnamed_addr constant [26 x i8] c"2. Call on valid object: \00", align 1
@111 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@112 = private unnamed_addr constant [18 x i8] c"This SHOULD print\00", align 1
@113 = private unnamed_addr constant [21 x i8] c"\0A[Template Literals]\00", align 1
@114 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@115 = private unnamed_addr constant [7 x i8] c"Moksha\00", align 1
@116 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@117 = private unnamed_addr constant [11 x i8] c"Language: \00", align 1
@118 = private unnamed_addr constant [3 x i8] c" v\00", align 1
@119 = private unnamed_addr constant [13 x i8] c" running at \00", align 1
@120 = private unnamed_addr constant [8 x i8] c"% speed\00", align 1
@121 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@122 = private unnamed_addr constant [33 x i8] c"\0A[2. Explicit Primitive Casting]\00", align 1
@123 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@124 = private unnamed_addr constant [14 x i8] c"int(99.99) = \00", align 1
@125 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@126 = private unnamed_addr constant [14 x i8] c"double(50) = \00", align 1
@127 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@128 = private unnamed_addr constant [31 x i8] c"\0A[3. Implicit Boxing to 'any']\00", align 1
@129 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@130 = private unnamed_addr constant [12 x i8] c"Boxed Int: \00", align 1
@131 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@132 = private unnamed_addr constant [15 x i8] c"Boxed Double: \00", align 1
@133 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@134 = private unnamed_addr constant [13 x i8] c"Boxed Bool: \00", align 1
@135 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@136 = private unnamed_addr constant [24 x i8] c"\0A[4. Explicit Unboxing]\00", align 1
@137 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@138 = private unnamed_addr constant [18 x i8] c"Unboxed Int + 1: \00", align 1
@139 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@140 = private unnamed_addr constant [24 x i8] c"Unboxed Double + 0.33: \00", align 1
@141 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@142 = private unnamed_addr constant [18 x i8] c"Unboxed Boolean: \00", align 1
@143 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@144 = private unnamed_addr constant [30 x i8] c"==== CONVERSION SHOWCASE ====\00", align 1
@145 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@146 = private unnamed_addr constant [13 x i8] c"ASCII 66 -> \00", align 1
@147 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@148 = private unnamed_addr constant [2 x i8] c"B\00", align 1
@149 = private unnamed_addr constant [13 x i8] c"Letter B -> \00", align 1
@150 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@151 = private unnamed_addr constant [8 x i8] c"3.14159\00", align 1
@152 = private unnamed_addr constant [16 x i8] c"Parsed Double: \00", align 1
@153 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@154 = private unnamed_addr constant [4 x i8] c"100\00", align 1
@155 = private unnamed_addr constant [13 x i8] c"Parsed Int: \00", align 1
@156 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@157 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@158 = private unnamed_addr constant [19 x i8] c"Formatted Number: \00", align 1
@159 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@160 = private unnamed_addr constant [24 x i8] c"\0A=== TEST COMPLETED ===\00", align 1
@161 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %s145 = alloca ptr, align 8
  %s = alloca ptr, align 8
  %x144 = alloca i32, align 4
  %x = alloca i32, align 4
  %numt141 = alloca i32, align 4
  %numt = alloca i32, align 4
  %numStr139 = alloca ptr, align 8
  %numStr = alloca ptr, align 8
  %pit136 = alloca double, align 8
  %pit = alloca double, align 8
  %piStr134 = alloca ptr, align 8
  %piStr = alloca ptr, align 8
  %backToCode132 = alloca i32, align 4
  %backToCode = alloca i32, align 4
  %letter130 = alloca ptr, align 8
  %letter = alloca ptr, align 8
  %charStr128 = alloca ptr, align 8
  %charStr = alloca ptr, align 8
  %code127 = alloca i32, align 4
  %code = alloca i32, align 4
  %unboxB122 = alloca i1, align 1
  %unboxB = alloca i1, align 1
  %unboxD121 = alloca double, align 8
  %unboxD = alloca double, align 8
  %unboxI120 = alloca i32, align 4
  %unboxI = alloca i32, align 4
  %boxBool114 = alloca ptr, align 8
  %boxBool = alloca ptr, align 8
  %boxDouble112 = alloca ptr, align 8
  %boxDouble = alloca ptr, align 8
  %boxInt110 = alloca ptr, align 8
  %boxInt = alloca ptr, align 8
  %floaty106 = alloca double, align 8
  %floaty = alloca double, align 8
  %whole105 = alloca i32, align 4
  %whole = alloca i32, align 4
  %truncated103 = alloca i32, align 4
  %truncated = alloca i32, align 4
  %precise102 = alloca double, align 8
  %precise = alloca double, align 8
  %speed99 = alloca double, align 8
  %speed = alloca double, align 8
  %version98 = alloca i32, align 4
  %version = alloca i32, align 4
  %name97 = alloca ptr, align 8
  %name = alloca ptr, align 8
  %myLogger81 = alloca ptr, align 8
  %myLogger = alloca ptr, align 8
  %configObj64 = alloca ptr, align 8
  %configObj = alloca ptr, align 8
  %finalScore61 = alloca i32, align 4
  %finalScore = alloca i32, align 4
  %maybeScore56 = alloca ptr, align 8
  %maybeScore = alloca ptr, align 8
  %displayName47 = alloca ptr, align 8
  %displayName = alloca ptr, align 8
  %maybeUser46 = alloca ptr, align 8
  %maybeUser = alloca ptr, align 8
  %user41 = alloca ptr, align 8
  %user = alloca ptr, align 8
  %dyn33 = alloca ptr, align 8
  %dyn = alloca ptr, align 8
  %nullableBool26 = alloca ptr, align 8
  %nullableBool = alloca ptr, align 8
  %nullableDouble21 = alloca ptr, align 8
  %nullableDouble = alloca ptr, align 8
  %nullableInt16 = alloca ptr, align 8
  %nullableInt = alloca ptr, align 8
  %nullableStr12 = alloca ptr, align 8
  %nullableStr = alloca ptr, align 8
  %msg5 = alloca ptr, align 8
  %msg = alloca ptr, align 8
  %isCool4 = alloca i1, align 1
  %isCool = alloca i1, align 1
  %pi3 = alloca double, align 8
  %pi = alloca double, align 8
  %num2 = alloca i32, align 4
  %num = alloca i32, align 4
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  store i32 6967, ptr %num, align 4
  store i32 6967, ptr %num2, align 4
  store double 3.141590e+00, ptr %pi, align 8
  store double 3.141590e+00, ptr %pi3, align 8
  store i1 true, ptr %isCool, align 1
  store i1 true, ptr %isCool4, align 1
  %4 = call ptr @moksha_box_string(ptr @4)
  store ptr %4, ptr %msg, align 8
  store ptr %4, ptr %msg5, align 8
  %5 = call ptr @moksha_box_string(ptr @5)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @6, ptr %print_unbox6)
  %7 = load i32, ptr %num2, align 4
  %8 = call i32 (ptr, ...) @printf(ptr @7, i32 %7)
  %9 = call ptr @moksha_box_string(ptr @8)
  %print_unbox7 = call ptr @moksha_unbox_string(ptr %9)
  %10 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox7)
  %11 = load double, ptr %pi3, align 8
  %12 = call i32 (ptr, ...) @printf(ptr @10, double %11)
  %13 = call ptr @moksha_box_string(ptr @11)
  %print_unbox8 = call ptr @moksha_unbox_string(ptr %13)
  %14 = call i32 (ptr, ...) @printf(ptr @12, ptr %print_unbox8)
  %15 = load i1, ptr %isCool4, align 1
  %16 = zext i1 %15 to i32
  %17 = call ptr @moksha_box_bool(i32 %16)
  %18 = call ptr @moksha_any_to_str(ptr %17)
  %19 = call i32 (ptr, ...) @printf(ptr @13, ptr %18)
  %20 = call ptr @moksha_box_string(ptr @14)
  %print_unbox9 = call ptr @moksha_unbox_string(ptr %20)
  %21 = call i32 (ptr, ...) @printf(ptr @15, ptr %print_unbox9)
  %22 = load ptr, ptr %msg5, align 8
  %print_unbox10 = call ptr @moksha_unbox_string(ptr %22)
  %23 = call i32 (ptr, ...) @printf(ptr @16, ptr %print_unbox10)
  %24 = call ptr @moksha_box_string(ptr @17)
  %print_unbox11 = call ptr @moksha_unbox_string(ptr %24)
  %25 = call i32 (ptr, ...) @printf(ptr @18, ptr %print_unbox11)
  %26 = call ptr @moksha_any_to_str(ptr null)
  %27 = call ptr @moksha_box_string(ptr %26)
  store ptr %27, ptr %nullableStr, align 8
  store ptr %27, ptr %nullableStr12, align 8
  %28 = call ptr @moksha_box_string(ptr @19)
  %print_unbox13 = call ptr @moksha_unbox_string(ptr %28)
  %29 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox13)
  %30 = load ptr, ptr %nullableStr12, align 8
  %to_str = call ptr @moksha_any_to_str(ptr %30)
  %31 = call i32 (ptr, ...) @printf(ptr @21, ptr %to_str)
  %32 = call ptr @moksha_box_string(ptr @22)
  store ptr %32, ptr %nullableStr12, align 8
  %33 = call ptr @moksha_box_string(ptr @23)
  %print_unbox14 = call ptr @moksha_unbox_string(ptr %33)
  %34 = call i32 (ptr, ...) @printf(ptr @24, ptr %print_unbox14)
  %35 = load ptr, ptr %nullableStr12, align 8
  %to_str15 = call ptr @moksha_any_to_str(ptr %35)
  %36 = call i32 (ptr, ...) @printf(ptr @25, ptr %to_str15)
  store ptr null, ptr %nullableInt, align 8
  store ptr null, ptr %nullableInt16, align 8
  %37 = call ptr @moksha_box_string(ptr @26)
  %print_unbox17 = call ptr @moksha_unbox_string(ptr %37)
  %38 = call i32 (ptr, ...) @printf(ptr @27, ptr %print_unbox17)
  %39 = load ptr, ptr %nullableInt16, align 8
  %to_str18 = call ptr @moksha_any_to_str(ptr %39)
  %40 = call i32 (ptr, ...) @printf(ptr @28, ptr %to_str18)
  %box_i = call ptr @moksha_box_int(i32 67)
  store ptr %box_i, ptr %nullableInt16, align 8
  %41 = call ptr @moksha_box_string(ptr @29)
  %print_unbox19 = call ptr @moksha_unbox_string(ptr %41)
  %42 = call i32 (ptr, ...) @printf(ptr @30, ptr %print_unbox19)
  %43 = load ptr, ptr %nullableInt16, align 8
  %to_str20 = call ptr @moksha_any_to_str(ptr %43)
  %44 = call i32 (ptr, ...) @printf(ptr @31, ptr %to_str20)
  store ptr null, ptr %nullableDouble, align 8
  store ptr null, ptr %nullableDouble21, align 8
  %45 = call ptr @moksha_box_string(ptr @32)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %45)
  %46 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox22)
  %47 = load ptr, ptr %nullableDouble21, align 8
  %to_str23 = call ptr @moksha_any_to_str(ptr %47)
  %48 = call i32 (ptr, ...) @printf(ptr @34, ptr %to_str23)
  %box_d = call ptr @moksha_box_double(double 0x4058FF5C28F5C28F)
  store ptr %box_d, ptr %nullableDouble21, align 8
  %49 = call ptr @moksha_box_string(ptr @35)
  %print_unbox24 = call ptr @moksha_unbox_string(ptr %49)
  %50 = call i32 (ptr, ...) @printf(ptr @36, ptr %print_unbox24)
  %51 = load ptr, ptr %nullableDouble21, align 8
  %to_str25 = call ptr @moksha_any_to_str(ptr %51)
  %52 = call i32 (ptr, ...) @printf(ptr @37, ptr %to_str25)
  store ptr null, ptr %nullableBool, align 8
  store ptr null, ptr %nullableBool26, align 8
  %53 = call ptr @moksha_box_string(ptr @38)
  %print_unbox27 = call ptr @moksha_unbox_string(ptr %53)
  %54 = call i32 (ptr, ...) @printf(ptr @39, ptr %print_unbox27)
  %55 = load ptr, ptr %nullableBool26, align 8
  %to_str28 = call ptr @moksha_any_to_str(ptr %55)
  %56 = call i32 (ptr, ...) @printf(ptr @40, ptr %to_str28)
  %box_b = call ptr @moksha_box_bool(i32 1)
  store ptr %box_b, ptr %nullableBool26, align 8
  %57 = call ptr @moksha_box_string(ptr @41)
  %print_unbox29 = call ptr @moksha_unbox_string(ptr %57)
  %58 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox29)
  %59 = load ptr, ptr %nullableBool26, align 8
  %to_str30 = call ptr @moksha_any_to_str(ptr %59)
  %60 = call i32 (ptr, ...) @printf(ptr @43, ptr %to_str30)
  %61 = call ptr @moksha_box_string(ptr @44)
  %print_unbox31 = call ptr @moksha_unbox_string(ptr %61)
  %62 = call i32 (ptr, ...) @printf(ptr @45, ptr %print_unbox31)
  %box_i32 = call ptr @moksha_box_int(i32 10)
  store ptr %box_i32, ptr %dyn, align 8
  store ptr %box_i32, ptr %dyn33, align 8
  %63 = call ptr @moksha_box_string(ptr @46)
  %print_unbox34 = call ptr @moksha_unbox_string(ptr %63)
  %64 = call i32 (ptr, ...) @printf(ptr @47, ptr %print_unbox34)
  %65 = load ptr, ptr %dyn33, align 8
  %to_str35 = call ptr @moksha_any_to_str(ptr %65)
  %66 = call i32 (ptr, ...) @printf(ptr @48, ptr %to_str35)
  %67 = call ptr @moksha_box_string(ptr @49)
  store ptr %67, ptr %dyn33, align 8
  %68 = call ptr @moksha_box_string(ptr @50)
  %print_unbox36 = call ptr @moksha_unbox_string(ptr %68)
  %69 = call i32 (ptr, ...) @printf(ptr @51, ptr %print_unbox36)
  %70 = load ptr, ptr %dyn33, align 8
  %to_str37 = call ptr @moksha_any_to_str(ptr %70)
  %71 = call i32 (ptr, ...) @printf(ptr @52, ptr %to_str37)
  store ptr null, ptr %dyn33, align 8
  %72 = call ptr @moksha_box_string(ptr @53)
  %print_unbox38 = call ptr @moksha_unbox_string(ptr %72)
  %73 = call i32 (ptr, ...) @printf(ptr @54, ptr %print_unbox38)
  %74 = load ptr, ptr %dyn33, align 8
  %to_str39 = call ptr @moksha_any_to_str(ptr %74)
  %75 = call i32 (ptr, ...) @printf(ptr @55, ptr %to_str39)
  %76 = call ptr @moksha_box_string(ptr @56)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %76)
  %77 = call i32 (ptr, ...) @printf(ptr @57, ptr %print_unbox40)
  %78 = call ptr @moksha_alloc_table(i32 3)
  %79 = call ptr @moksha_box_string(ptr @58)
  %80 = call ptr @moksha_box_int(i32 101)
  %81 = call ptr @moksha_table_set(ptr %78, ptr %79, ptr %80)
  %82 = call ptr @moksha_box_string(ptr @59)
  %83 = call ptr @moksha_box_string(ptr @60)
  %84 = call ptr @moksha_table_set(ptr %78, ptr %82, ptr %83)
  %85 = call ptr @moksha_box_string(ptr @61)
  %86 = call ptr @moksha_box_bool(i32 1)
  %87 = call ptr @moksha_table_set(ptr %78, ptr %85, ptr %86)
  store ptr %78, ptr %user, align 8
  store ptr %78, ptr %user41, align 8
  %88 = call ptr @moksha_box_string(ptr @62)
  %print_unbox42 = call ptr @moksha_unbox_string(ptr %88)
  %89 = call i32 (ptr, ...) @printf(ptr @63, ptr %print_unbox42)
  %90 = load ptr, ptr %user41, align 8
  %to_str43 = call ptr @moksha_any_to_str(ptr %90)
  %91 = call i32 (ptr, ...) @printf(ptr @64, ptr %to_str43)
  %92 = call ptr @moksha_box_string(ptr @65)
  %print_unbox44 = call ptr @moksha_unbox_string(ptr %92)
  %93 = call i32 (ptr, ...) @printf(ptr @66, ptr %print_unbox44)
  %94 = call ptr @moksha_box_string(ptr @67)
  %print_unbox45 = call ptr @moksha_unbox_string(ptr %94)
  %95 = call i32 (ptr, ...) @printf(ptr @68, ptr %print_unbox45)
  %96 = call ptr @moksha_any_to_str(ptr null)
  %97 = call ptr @moksha_box_string(ptr %96)
  store ptr %97, ptr %maybeUser, align 8
  store ptr %97, ptr %maybeUser46, align 8
  %98 = load ptr, ptr %maybeUser46, align 8
  %99 = call ptr @moksha_box_string(ptr @69)
  %100 = icmp eq ptr %98, null
  br i1 %100, label %coalesce_null, label %coalesce_not_null

coalesce_not_null:                                ; preds = %entry
  br label %coalesce_merge

coalesce_null:                                    ; preds = %entry
  br label %coalesce_merge

coalesce_merge:                                   ; preds = %coalesce_null, %coalesce_not_null
  %coalesce_res = phi ptr [ %98, %coalesce_not_null ], [ %99, %coalesce_null ]
  store ptr %coalesce_res, ptr %displayName, align 8
  store ptr %coalesce_res, ptr %displayName47, align 8
  %101 = call ptr @moksha_box_string(ptr @70)
  %print_unbox48 = call ptr @moksha_unbox_string(ptr %101)
  %102 = call i32 (ptr, ...) @printf(ptr @71, ptr %print_unbox48)
  %103 = load ptr, ptr %displayName47, align 8
  %print_unbox49 = call ptr @moksha_unbox_string(ptr %103)
  %104 = call i32 (ptr, ...) @printf(ptr @72, ptr %print_unbox49)
  %105 = call ptr @moksha_box_string(ptr @73)
  store ptr %105, ptr %maybeUser46, align 8
  %106 = load ptr, ptr %maybeUser46, align 8
  %107 = call ptr @moksha_box_string(ptr @74)
  %108 = icmp eq ptr %106, null
  br i1 %108, label %coalesce_null51, label %coalesce_not_null50

coalesce_not_null50:                              ; preds = %coalesce_merge
  br label %coalesce_merge52

coalesce_null51:                                  ; preds = %coalesce_merge
  br label %coalesce_merge52

coalesce_merge52:                                 ; preds = %coalesce_null51, %coalesce_not_null50
  %coalesce_res53 = phi ptr [ %106, %coalesce_not_null50 ], [ %107, %coalesce_null51 ]
  store ptr %coalesce_res53, ptr %displayName47, align 8
  %109 = call ptr @moksha_box_string(ptr @75)
  %print_unbox54 = call ptr @moksha_unbox_string(ptr %109)
  %110 = call i32 (ptr, ...) @printf(ptr @76, ptr %print_unbox54)
  %111 = load ptr, ptr %displayName47, align 8
  %print_unbox55 = call ptr @moksha_unbox_string(ptr %111)
  %112 = call i32 (ptr, ...) @printf(ptr @77, ptr %print_unbox55)
  store ptr null, ptr %maybeScore, align 8
  store ptr null, ptr %maybeScore56, align 8
  %113 = load ptr, ptr %maybeScore56, align 8
  %114 = icmp eq ptr %113, null
  br i1 %114, label %coalesce_null58, label %coalesce_not_null57

coalesce_not_null57:                              ; preds = %coalesce_merge52
  %115 = call i32 @moksha_unbox_int(ptr %113)
  br label %coalesce_merge59

coalesce_null58:                                  ; preds = %coalesce_merge52
  br label %coalesce_merge59

coalesce_merge59:                                 ; preds = %coalesce_null58, %coalesce_not_null57
  %coalesce_res60 = phi i32 [ %115, %coalesce_not_null57 ], [ 0, %coalesce_null58 ]
  %116 = call ptr @moksha_box_int(i32 %coalesce_res60)
  %unbox_i = call i32 @moksha_unbox_int(ptr %116)
  store i32 %unbox_i, ptr %finalScore, align 4
  store i32 %unbox_i, ptr %finalScore61, align 4
  %117 = call ptr @moksha_box_string(ptr @78)
  %print_unbox62 = call ptr @moksha_unbox_string(ptr %117)
  %118 = call i32 (ptr, ...) @printf(ptr @79, ptr %print_unbox62)
  %119 = load i32, ptr %finalScore61, align 4
  %120 = call i32 (ptr, ...) @printf(ptr @80, i32 %119)
  %121 = call ptr @moksha_box_string(ptr @81)
  %print_unbox63 = call ptr @moksha_unbox_string(ptr %121)
  %122 = call i32 (ptr, ...) @printf(ptr @82, ptr %print_unbox63)
  store ptr null, ptr %configObj, align 8
  store ptr null, ptr %configObj64, align 8
  %123 = call ptr @moksha_box_string(ptr @83)
  %print_unbox65 = call ptr @moksha_unbox_string(ptr %123)
  %124 = call i32 (ptr, ...) @printf(ptr @84, ptr %print_unbox65)
  %loaded_obj_ptr = load ptr, ptr %configObj64, align 8
  %125 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %125, label %opt_mem_null, label %opt_mem_access

opt_mem_access:                                   ; preds = %coalesce_merge59
  %126 = call ptr @moksha_box_string(ptr @85)
  %table_dot_get = call ptr @moksha_table_get(ptr %loaded_obj_ptr, ptr %126)
  br label %opt_mem_cont

opt_mem_null:                                     ; preds = %coalesce_merge59
  br label %opt_mem_cont

opt_mem_cont:                                     ; preds = %opt_mem_null, %opt_mem_access
  %opt_mem_res = phi ptr [ null, %opt_mem_null ], [ %table_dot_get, %opt_mem_access ]
  %is_obj_null = icmp eq ptr %opt_mem_res, null
  br i1 %is_obj_null, label %print_null, label %print_obj

print_null:                                       ; preds = %opt_mem_cont
  %127 = call i32 (ptr, ...) @printf(ptr @86)
  br label %print_merge

print_obj:                                        ; preds = %opt_mem_cont
  %128 = call i32 (ptr, ...) @printf(ptr @87, ptr %opt_mem_res)
  br label %print_merge

print_merge:                                      ; preds = %print_obj, %print_null
  %129 = call ptr @moksha_box_string(ptr @88)
  %print_unbox66 = call ptr @moksha_unbox_string(ptr %129)
  %130 = call i32 (ptr, ...) @printf(ptr @89, ptr %print_unbox66)
  %131 = load ptr, ptr %configObj64, align 8
  %132 = icmp eq ptr %131, null
  br i1 %132, label %opt_idx_null, label %opt_idx_access

opt_idx_access:                                   ; preds = %print_merge
  %133 = call ptr @moksha_box_string(ptr @90)
  %134 = call ptr @moksha_table_get(ptr %131, ptr %133)
  br label %opt_idx_cont

opt_idx_null:                                     ; preds = %print_merge
  br label %opt_idx_cont

opt_idx_cont:                                     ; preds = %opt_idx_null, %opt_idx_access
  %opt_idx_res = phi ptr [ null, %opt_idx_null ], [ %134, %opt_idx_access ]
  %to_str67 = call ptr @moksha_any_to_str(ptr %opt_idx_res)
  %135 = call i32 (ptr, ...) @printf(ptr @91, ptr %to_str67)
  %136 = call ptr @moksha_alloc_table(i32 2)
  %137 = call ptr @moksha_box_string(ptr @92)
  %138 = call ptr @moksha_box_int(i32 8080)
  %139 = call ptr @moksha_table_set(ptr %136, ptr %137, ptr %138)
  %140 = call ptr @moksha_box_string(ptr @93)
  %141 = call ptr @moksha_box_string(ptr @94)
  %142 = call ptr @moksha_table_set(ptr %136, ptr %140, ptr %141)
  store ptr %136, ptr %configObj64, align 8
  %143 = call ptr @moksha_box_string(ptr @95)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %143)
  %144 = call i32 (ptr, ...) @printf(ptr @96, ptr %print_unbox68)
  %loaded_obj_ptr69 = load ptr, ptr %configObj64, align 8
  %145 = icmp eq ptr %loaded_obj_ptr69, null
  br i1 %145, label %opt_mem_null71, label %opt_mem_access70

opt_mem_access70:                                 ; preds = %opt_idx_cont
  %146 = call ptr @moksha_box_string(ptr @97)
  %table_dot_get73 = call ptr @moksha_table_get(ptr %loaded_obj_ptr69, ptr %146)
  br label %opt_mem_cont72

opt_mem_null71:                                   ; preds = %opt_idx_cont
  br label %opt_mem_cont72

opt_mem_cont72:                                   ; preds = %opt_mem_null71, %opt_mem_access70
  %opt_mem_res74 = phi ptr [ null, %opt_mem_null71 ], [ %table_dot_get73, %opt_mem_access70 ]
  %147 = call ptr @moksha_box_string(ptr @98)
  %148 = call ptr @moksha_any_to_str(ptr %opt_mem_res74)
  %149 = call ptr @moksha_box_string(ptr %148)
  %150 = call ptr @moksha_string_concat(ptr %149, ptr %147)
  %151 = load ptr, ptr %configObj64, align 8
  %152 = icmp eq ptr %151, null
  br i1 %152, label %opt_idx_null76, label %opt_idx_access75

opt_idx_access75:                                 ; preds = %opt_mem_cont72
  %153 = call ptr @moksha_box_string(ptr @99)
  %154 = call ptr @moksha_table_get(ptr %151, ptr %153)
  br label %opt_idx_cont77

opt_idx_null76:                                   ; preds = %opt_mem_cont72
  br label %opt_idx_cont77

opt_idx_cont77:                                   ; preds = %opt_idx_null76, %opt_idx_access75
  %opt_idx_res78 = phi ptr [ null, %opt_idx_null76 ], [ %154, %opt_idx_access75 ]
  %155 = call ptr @moksha_any_to_str(ptr %opt_idx_res78)
  %156 = call ptr @moksha_box_string(ptr %155)
  %157 = call ptr @moksha_string_concat(ptr %150, ptr %156)
  %print_unbox79 = call ptr @moksha_unbox_string(ptr %157)
  %158 = call i32 (ptr, ...) @printf(ptr @100, ptr %print_unbox79)
  %159 = call ptr @moksha_box_string(ptr @101)
  %print_unbox80 = call ptr @moksha_unbox_string(ptr %159)
  %160 = call i32 (ptr, ...) @printf(ptr @102, ptr %print_unbox80)
  br i1 true, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %opt_idx_cont77
  %clone_alloc = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 null, i64 8, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %opt_idx_cont77
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %myLogger, align 8
  store ptr %clone_res, ptr %myLogger81, align 8
  %161 = call ptr @moksha_box_string(ptr @105)
  %print_unbox82 = call ptr @moksha_unbox_string(ptr %161)
  %162 = call i32 (ptr, ...) @printf(ptr @106, ptr %print_unbox82)
  %163 = load ptr, ptr %myLogger81, align 8
  %164 = icmp eq ptr %163, null
  br i1 %164, label %opt_call_null, label %opt_call_exec

opt_call_exec:                                    ; preds = %clone_merge
  %165 = call ptr @moksha_box_string(ptr @107)
  %166 = getelementptr inbounds nuw %Logger, ptr %163, i32 0, i32 0
  %vptr = load ptr, ptr %166, align 8
  %167 = getelementptr inbounds ptr, ptr %vptr, i32 0
  %func_ptr = load ptr, ptr %167, align 8
  call void %func_ptr(ptr %163, ptr %165)
  br label %opt_call_cont

opt_call_null:                                    ; preds = %clone_merge
  br label %opt_call_cont

opt_call_cont:                                    ; preds = %opt_call_null, %opt_call_exec
  %opt_call_res = phi ptr [ null, %opt_call_null ], [ null, %opt_call_exec ]
  %168 = call ptr @moksha_box_string(ptr @108)
  %print_unbox83 = call ptr @moksha_unbox_string(ptr %168)
  %169 = call i32 (ptr, ...) @printf(ptr @109, ptr %print_unbox83)
  %alloc_obj = call ptr @calloc(i64 1, i64 8)
  %170 = getelementptr inbounds nuw %Logger, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Logger, ptr %170, align 8
  call void @Logger_constructor(ptr %alloc_obj)
  %171 = icmp eq ptr %alloc_obj, null
  br i1 %171, label %clone_skip85, label %clone_do84

clone_do84:                                       ; preds = %opt_call_cont
  %clone_alloc87 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc87, ptr align 8 %alloc_obj, i64 8, i1 false)
  br label %clone_merge86

clone_skip85:                                     ; preds = %opt_call_cont
  br label %clone_merge86

clone_merge86:                                    ; preds = %clone_skip85, %clone_do84
  %clone_res88 = phi ptr [ %clone_alloc87, %clone_do84 ], [ null, %clone_skip85 ]
  store ptr %clone_res88, ptr %myLogger81, align 8
  %172 = call ptr @moksha_box_string(ptr @110)
  %print_unbox89 = call ptr @moksha_unbox_string(ptr %172)
  %173 = call i32 (ptr, ...) @printf(ptr @111, ptr %print_unbox89)
  %174 = load ptr, ptr %myLogger81, align 8
  %175 = icmp eq ptr %174, null
  br i1 %175, label %opt_call_null91, label %opt_call_exec90

opt_call_exec90:                                  ; preds = %clone_merge86
  %176 = call ptr @moksha_box_string(ptr @112)
  %177 = getelementptr inbounds nuw %Logger, ptr %174, i32 0, i32 0
  %vptr93 = load ptr, ptr %177, align 8
  %178 = getelementptr inbounds ptr, ptr %vptr93, i32 0
  %func_ptr94 = load ptr, ptr %178, align 8
  call void %func_ptr94(ptr %174, ptr %176)
  br label %opt_call_cont92

opt_call_null91:                                  ; preds = %clone_merge86
  br label %opt_call_cont92

opt_call_cont92:                                  ; preds = %opt_call_null91, %opt_call_exec90
  %opt_call_res95 = phi ptr [ null, %opt_call_null91 ], [ null, %opt_call_exec90 ]
  %179 = call ptr @moksha_box_string(ptr @113)
  %print_unbox96 = call ptr @moksha_unbox_string(ptr %179)
  %180 = call i32 (ptr, ...) @printf(ptr @114, ptr %print_unbox96)
  %181 = call ptr @moksha_box_string(ptr @115)
  store ptr %181, ptr %name, align 8
  store ptr %181, ptr %name97, align 8
  store i32 1, ptr %version, align 4
  store i32 1, ptr %version98, align 4
  store double 9.990000e+01, ptr %speed, align 8
  store double 9.990000e+01, ptr %speed99, align 8
  %182 = call ptr @moksha_box_string(ptr @116)
  %183 = call ptr @moksha_box_string(ptr @117)
  %184 = call ptr @moksha_string_concat(ptr %182, ptr %183)
  %185 = load ptr, ptr %name97, align 8
  %186 = call ptr @moksha_string_concat(ptr %184, ptr %185)
  %187 = call ptr @moksha_box_string(ptr @118)
  %188 = call ptr @moksha_string_concat(ptr %186, ptr %187)
  %189 = load i32, ptr %version98, align 4
  %i2s = call ptr @moksha_int_to_str(i32 %189)
  %190 = call ptr @moksha_string_concat(ptr %188, ptr %i2s)
  %191 = call ptr @moksha_box_string(ptr @119)
  %192 = call ptr @moksha_string_concat(ptr %190, ptr %191)
  %193 = load double, ptr %speed99, align 8
  %d2s = call ptr @moksha_double_to_str(double %193)
  %194 = call ptr @moksha_string_concat(ptr %192, ptr %d2s)
  %195 = call ptr @moksha_box_string(ptr @120)
  %196 = call ptr @moksha_string_concat(ptr %194, ptr %195)
  %print_unbox100 = call ptr @moksha_unbox_string(ptr %196)
  %197 = call i32 (ptr, ...) @printf(ptr @121, ptr %print_unbox100)
  %198 = call ptr @moksha_box_string(ptr @122)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %198)
  %199 = call i32 (ptr, ...) @printf(ptr @123, ptr %print_unbox101)
  store double 0x4058FF5C28F5C28F, ptr %precise, align 8
  store double 0x4058FF5C28F5C28F, ptr %precise102, align 8
  %200 = load double, ptr %precise102, align 8
  %cast_d2i = fptosi double %200 to i32
  store i32 %cast_d2i, ptr %truncated, align 4
  store i32 %cast_d2i, ptr %truncated103, align 4
  %201 = call ptr @moksha_box_string(ptr @124)
  %202 = load i32, ptr %truncated103, align 4
  %203 = call ptr @moksha_int_to_str(i32 %202)
  %204 = call ptr @moksha_box_string(ptr %203)
  %205 = call ptr @moksha_string_concat(ptr %201, ptr %204)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %205)
  %206 = call i32 (ptr, ...) @printf(ptr @125, ptr %print_unbox104)
  store i32 50, ptr %whole, align 4
  store i32 50, ptr %whole105, align 4
  %207 = load i32, ptr %whole105, align 4
  %cast_i2d = sitofp i32 %207 to double
  store double %cast_i2d, ptr %floaty, align 8
  store double %cast_i2d, ptr %floaty106, align 8
  %208 = call ptr @moksha_box_string(ptr @126)
  %209 = load double, ptr %floaty106, align 8
  %210 = call ptr @moksha_double_to_str(double %209)
  %211 = call ptr @moksha_box_string(ptr %210)
  %212 = call ptr @moksha_string_concat(ptr %208, ptr %211)
  %print_unbox107 = call ptr @moksha_unbox_string(ptr %212)
  %213 = call i32 (ptr, ...) @printf(ptr @127, ptr %print_unbox107)
  %214 = call ptr @moksha_box_string(ptr @128)
  %print_unbox108 = call ptr @moksha_unbox_string(ptr %214)
  %215 = call i32 (ptr, ...) @printf(ptr @129, ptr %print_unbox108)
  %box_i109 = call ptr @moksha_box_int(i32 123)
  store ptr %box_i109, ptr %boxInt, align 8
  store ptr %box_i109, ptr %boxInt110, align 8
  %box_d111 = call ptr @moksha_box_double(double 4.567000e+01)
  store ptr %box_d111, ptr %boxDouble, align 8
  store ptr %box_d111, ptr %boxDouble112, align 8
  %box_b113 = call ptr @moksha_box_bool(i32 1)
  store ptr %box_b113, ptr %boxBool, align 8
  store ptr %box_b113, ptr %boxBool114, align 8
  %216 = call ptr @moksha_box_string(ptr @130)
  %217 = load ptr, ptr %boxInt110, align 8
  %218 = call ptr @moksha_any_to_str(ptr %217)
  %219 = call ptr @moksha_box_string(ptr %218)
  %220 = call ptr @moksha_string_concat(ptr %216, ptr %219)
  %print_unbox115 = call ptr @moksha_unbox_string(ptr %220)
  %221 = call i32 (ptr, ...) @printf(ptr @131, ptr %print_unbox115)
  %222 = call ptr @moksha_box_string(ptr @132)
  %223 = load ptr, ptr %boxDouble112, align 8
  %224 = call ptr @moksha_any_to_str(ptr %223)
  %225 = call ptr @moksha_box_string(ptr %224)
  %226 = call ptr @moksha_string_concat(ptr %222, ptr %225)
  %print_unbox116 = call ptr @moksha_unbox_string(ptr %226)
  %227 = call i32 (ptr, ...) @printf(ptr @133, ptr %print_unbox116)
  %228 = call ptr @moksha_box_string(ptr @134)
  %229 = load ptr, ptr %boxBool114, align 8
  %230 = call ptr @moksha_any_to_str(ptr %229)
  %231 = call ptr @moksha_box_string(ptr %230)
  %232 = call ptr @moksha_string_concat(ptr %228, ptr %231)
  %print_unbox117 = call ptr @moksha_unbox_string(ptr %232)
  %233 = call i32 (ptr, ...) @printf(ptr @135, ptr %print_unbox117)
  %234 = call ptr @moksha_box_string(ptr @136)
  %print_unbox118 = call ptr @moksha_unbox_string(ptr %234)
  %235 = call i32 (ptr, ...) @printf(ptr @137, ptr %print_unbox118)
  %236 = load ptr, ptr %boxInt110, align 8
  %unbox_i119 = call i32 @moksha_unbox_int(ptr %236)
  store i32 %unbox_i119, ptr %unboxI, align 4
  store i32 %unbox_i119, ptr %unboxI120, align 4
  %237 = load ptr, ptr %boxDouble112, align 8
  %unbox_d = call double @moksha_unbox_double(ptr %237)
  store double %unbox_d, ptr %unboxD, align 8
  store double %unbox_d, ptr %unboxD121, align 8
  %238 = load ptr, ptr %boxBool114, align 8
  %unbox_b = call i32 @moksha_unbox_bool(ptr %238)
  %239 = trunc i32 %unbox_b to i1
  store i1 %239, ptr %unboxB, align 1
  store i1 %239, ptr %unboxB122, align 1
  %240 = call ptr @moksha_box_string(ptr @138)
  %241 = load i32, ptr %unboxI120, align 4
  %addtmp = add i32 %241, 1
  %242 = call ptr @moksha_int_to_str(i32 %addtmp)
  %243 = call ptr @moksha_box_string(ptr %242)
  %244 = call ptr @moksha_string_concat(ptr %240, ptr %243)
  %print_unbox123 = call ptr @moksha_unbox_string(ptr %244)
  %245 = call i32 (ptr, ...) @printf(ptr @139, ptr %print_unbox123)
  %246 = call ptr @moksha_box_string(ptr @140)
  %247 = load double, ptr %unboxD121, align 8
  %faddtmp = fadd double %247, 3.300000e-01
  %248 = call ptr @moksha_double_to_str(double %faddtmp)
  %249 = call ptr @moksha_box_string(ptr %248)
  %250 = call ptr @moksha_string_concat(ptr %246, ptr %249)
  %print_unbox124 = call ptr @moksha_unbox_string(ptr %250)
  %251 = call i32 (ptr, ...) @printf(ptr @141, ptr %print_unbox124)
  %252 = call ptr @moksha_box_string(ptr @142)
  %253 = load i1, ptr %unboxB122, align 1
  %254 = zext i1 %253 to i32
  %255 = call ptr @moksha_box_bool(i32 %254)
  %256 = call ptr @moksha_any_to_str(ptr %255)
  %257 = call ptr @moksha_box_string(ptr %256)
  %258 = call ptr @moksha_string_concat(ptr %252, ptr %257)
  %print_unbox125 = call ptr @moksha_unbox_string(ptr %258)
  %259 = call i32 (ptr, ...) @printf(ptr @143, ptr %print_unbox125)
  %260 = call ptr @moksha_box_string(ptr @144)
  %print_unbox126 = call ptr @moksha_unbox_string(ptr %260)
  %261 = call i32 (ptr, ...) @printf(ptr @145, ptr %print_unbox126)
  store i32 66, ptr %code, align 4
  store i32 66, ptr %code127, align 4
  %262 = load i32, ptr %code127, align 4
  %i2c = call ptr @moksha_int_to_ascii(i32 %262)
  %263 = call ptr @moksha_box_string(ptr %i2c)
  store ptr %263, ptr %charStr, align 8
  store ptr %263, ptr %charStr128, align 8
  %264 = call ptr @moksha_box_string(ptr @146)
  %265 = load ptr, ptr %charStr128, align 8
  %266 = call ptr @moksha_string_concat(ptr %264, ptr %265)
  %print_unbox129 = call ptr @moksha_unbox_string(ptr %266)
  %267 = call i32 (ptr, ...) @printf(ptr @147, ptr %print_unbox129)
  %268 = call ptr @moksha_box_string(ptr @148)
  store ptr %268, ptr %letter, align 8
  store ptr %268, ptr %letter130, align 8
  %269 = load ptr, ptr %letter130, align 8
  %unbox_i131 = call i32 @moksha_unbox_int(ptr %269)
  store i32 %unbox_i131, ptr %backToCode, align 4
  store i32 %unbox_i131, ptr %backToCode132, align 4
  %270 = call ptr @moksha_box_string(ptr @149)
  %271 = load i32, ptr %backToCode132, align 4
  %272 = call ptr @moksha_int_to_str(i32 %271)
  %273 = call ptr @moksha_box_string(ptr %272)
  %274 = call ptr @moksha_string_concat(ptr %270, ptr %273)
  %print_unbox133 = call ptr @moksha_unbox_string(ptr %274)
  %275 = call i32 (ptr, ...) @printf(ptr @150, ptr %print_unbox133)
  %276 = call ptr @moksha_box_string(ptr @151)
  store ptr %276, ptr %piStr, align 8
  store ptr %276, ptr %piStr134, align 8
  %277 = load ptr, ptr %piStr134, align 8
  %unbox_d135 = call double @moksha_unbox_double(ptr %277)
  store double %unbox_d135, ptr %pit, align 8
  store double %unbox_d135, ptr %pit136, align 8
  %278 = call ptr @moksha_box_string(ptr @152)
  %279 = load double, ptr %pit136, align 8
  %faddtmp137 = fadd double %279, 1.000000e-05
  %280 = call ptr @moksha_double_to_str(double %faddtmp137)
  %281 = call ptr @moksha_box_string(ptr %280)
  %282 = call ptr @moksha_string_concat(ptr %278, ptr %281)
  %print_unbox138 = call ptr @moksha_unbox_string(ptr %282)
  %283 = call i32 (ptr, ...) @printf(ptr @153, ptr %print_unbox138)
  %284 = call ptr @moksha_box_string(ptr @154)
  store ptr %284, ptr %numStr, align 8
  store ptr %284, ptr %numStr139, align 8
  %285 = load ptr, ptr %numStr139, align 8
  %unbox_i140 = call i32 @moksha_unbox_int(ptr %285)
  store i32 %unbox_i140, ptr %numt, align 4
  store i32 %unbox_i140, ptr %numt141, align 4
  %286 = call ptr @moksha_box_string(ptr @155)
  %287 = load i32, ptr %numt141, align 4
  %addtmp142 = add i32 %287, 1
  %288 = call ptr @moksha_int_to_str(i32 %addtmp142)
  %289 = call ptr @moksha_box_string(ptr %288)
  %290 = call ptr @moksha_string_concat(ptr %286, ptr %289)
  %print_unbox143 = call ptr @moksha_unbox_string(ptr %290)
  %291 = call i32 (ptr, ...) @printf(ptr @156, ptr %print_unbox143)
  store i32 500, ptr %x, align 4
  store i32 500, ptr %x144, align 4
  %292 = call ptr @moksha_box_string(ptr @157)
  %293 = load i32, ptr %x144, align 4
  %294 = call ptr @moksha_int_to_str(i32 %293)
  %295 = call ptr @moksha_box_string(ptr %294)
  %296 = call ptr @moksha_string_concat(ptr %292, ptr %295)
  store ptr %296, ptr %s, align 8
  store ptr %296, ptr %s145, align 8
  %297 = call ptr @moksha_box_string(ptr @158)
  %298 = load ptr, ptr %s145, align 8
  %299 = call ptr @moksha_string_concat(ptr %297, ptr %298)
  %print_unbox146 = call ptr @moksha_unbox_string(ptr %299)
  %300 = call i32 (ptr, ...) @printf(ptr @159, ptr %print_unbox146)
  %301 = call ptr @moksha_box_string(ptr @160)
  %print_unbox147 = call ptr @moksha_unbox_string(ptr %301)
  %302 = call i32 (ptr, ...) @printf(ptr @161, ptr %print_unbox147)
  ret i32 0
}

define void @Logger_log(ptr %this, ptr %msg) {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %0 = call ptr @moksha_box_string(ptr @103)
  %1 = load ptr, ptr %msg1, align 8
  %2 = call ptr @moksha_string_concat(ptr %0, ptr %1)
  %print_unbox = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @104, ptr %print_unbox)
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Logger_constructor(ptr %this) {
entry:
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
