; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Dog = type { ptr, ptr }
%Cat = type { ptr, ptr }
%Animal = type { ptr, ptr }
%Shape = type { ptr }
%Circle = type { ptr }
%FilledCircle = type { ptr }
%Computer = type { ptr }
%Device = type { ptr }
%BaseLogger = type { ptr }
%FileLogger = type { ptr }
%Counter = type { ptr, i32, double }

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [41 x i8] c"==== POLYMORPHISM & REFERENCE SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [23 x i8] c"\0A[1. Basic Overriding]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Animal = constant [1 x ptr] [ptr @Animal_speak]
@4 = private unnamed_addr constant [7 x i8] c"Animal\00", align 1
@5 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@6 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@7 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@8 = private unnamed_addr constant [24 x i8] c" makes a generic sound.\00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Dog = constant [1 x ptr] [ptr @Dog_speak]
@10 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@11 = private unnamed_addr constant [14 x i8] c" barks: Woof!\00", align 1
@12 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Cat = constant [1 x ptr] [ptr @Cat_speak]
@13 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@14 = private unnamed_addr constant [14 x i8] c" meows: Meow!\00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@16 = private unnamed_addr constant [6 x i8] c"Buddy\00", align 1
@17 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@18 = private unnamed_addr constant [9 x i8] c"Whiskers\00", align 1
@19 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@20 = private unnamed_addr constant [30 x i8] c"\0A[2. Base Reference Dispatch]\00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@22 = private unnamed_addr constant [26 x i8] c"-> Requesting speak() on \00", align 1
@23 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@24 = private unnamed_addr constant [4 x i8] c"...\00", align 1
@25 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@26 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@27 = private unnamed_addr constant [8 x i8] c"Generic\00", align 1
@28 = private unnamed_addr constant [30 x i8] c"\0A[3. Multi-Level Inheritance]\00", align 1
@29 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Shape = constant [1 x ptr] [ptr @Shape_draw]
@30 = private unnamed_addr constant [14 x i8] c"Drawing Shape\00", align 1
@31 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Circle = constant [1 x ptr] [ptr @Circle_draw]
@32 = private unnamed_addr constant [15 x i8] c"Drawing Circle\00", align 1
@33 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_FilledCircle = constant [1 x ptr] [ptr @FilledCircle_draw]
@34 = private unnamed_addr constant [27 x i8] c"Drawing FilledCircle (Red)\00", align 1
@35 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@36 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@37 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@38 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@39 = private unnamed_addr constant [25 x i8] c"\0A[4. Partial Overriding]\00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Device = constant [2 x ptr] [ptr @Device_start, ptr @Device_stop]
@41 = private unnamed_addr constant [19 x i8] c"Device starting...\00", align 1
@42 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@43 = private unnamed_addr constant [19 x i8] c"Device stopping...\00", align 1
@44 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Computer = constant [2 x ptr] [ptr @Computer_start, ptr @Device_stop]
@45 = private unnamed_addr constant [23 x i8] c"Computer booting OS...\00", align 1
@46 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@47 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@48 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@49 = private unnamed_addr constant [39 x i8] c"\0A[5. Polymorphism inside Constructors]\00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_BaseLogger = constant [1 x ptr] [ptr @BaseLogger_log]
@51 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@52 = private unnamed_addr constant [25 x i8] c"Base Constructor Running\00", align 1
@53 = private unnamed_addr constant [13 x i8] c"[BASE LOG]: \00", align 1
@54 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_FileLogger = constant [1 x ptr] [ptr @FileLogger_log]
@55 = private unnamed_addr constant [13 x i8] c"[FILE LOG]: \00", align 1
@56 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@57 = private unnamed_addr constant [23 x i8] c"Creating BaseLogger...\00", align 1
@58 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@59 = private unnamed_addr constant [23 x i8] c"Creating FileLogger...\00", align 1
@60 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@61 = private unnamed_addr constant [30 x i8] c"\0A[6. Pass By Reference (ref)]\00", align 1
@62 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@63 = private unnamed_addr constant [25 x i8] c"\0A--- A. Integer Swap ---\00", align 1
@64 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@65 = private unnamed_addr constant [11 x i8] c"Before: x=\00", align 1
@66 = private unnamed_addr constant [5 x i8] c", y=\00", align 1
@67 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@68 = private unnamed_addr constant [11 x i8] c"After:  x=\00", align 1
@69 = private unnamed_addr constant [5 x i8] c", y=\00", align 1
@70 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@71 = private unnamed_addr constant [28 x i8] c"\0A--- B. Double Mutation ---\00", align 1
@72 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@73 = private unnamed_addr constant [13 x i8] c"Doubled PI: \00", align 1
@74 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@75 = private unnamed_addr constant [27 x i8] c"\0A--- C. Boolean Toggle ---\00", align 1
@76 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@77 = private unnamed_addr constant [16 x i8] c"Active before: \00", align 1
@78 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@79 = private unnamed_addr constant [16 x i8] c"Active after:  \00", align 1
@80 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@81 = private unnamed_addr constant [32 x i8] c"\0A--- D. String Reassignment ---\00", align 1
@82 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@83 = private unnamed_addr constant [14 x i8] c"Moksha Master\00", align 1
@84 = private unnamed_addr constant [7 x i8] c"Novice\00", align 1
@85 = private unnamed_addr constant [14 x i8] c"User before: \00", align 1
@86 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@87 = private unnamed_addr constant [14 x i8] c"User after:  \00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@89 = private unnamed_addr constant [30 x i8] c"\0A--- E. Array Replacement ---\00", align 1
@90 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@91 = private unnamed_addr constant [18 x i8] c"Array[0] before: \00", align 1
@92 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@93 = private unnamed_addr constant [18 x i8] c"Array[0] after:  \00", align 1
@94 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@95 = private unnamed_addr constant [24 x i8] c"\0A--- F. Object Swap ---\00", align 1
@96 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@97 = private unnamed_addr constant [6 x i8] c"Alpha\00", align 1
@98 = private unnamed_addr constant [5 x i8] c"Beta\00", align 1
@99 = private unnamed_addr constant [7 x i8] c"d1 is \00", align 1
@100 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@101 = private unnamed_addr constant [9 x i8] c", d2 is \00", align 1
@102 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@103 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@104 = private unnamed_addr constant [7 x i8] c"d1 is \00", align 1
@105 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@106 = private unnamed_addr constant [9 x i8] c", d2 is \00", align 1
@107 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@108 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@109 = private unnamed_addr constant [27 x i8] c"\0A--- G. Table Mutation ---\00", align 1
@110 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@111 = private unnamed_addr constant [17 x i8] c"Modified via Ref\00", align 1
@112 = private unnamed_addr constant [7 x i8] c"status\00", align 1
@113 = private unnamed_addr constant [7 x i8] c"status\00", align 1
@114 = private unnamed_addr constant [9 x i8] c"Original\00", align 1
@115 = private unnamed_addr constant [22 x i8] c"Table Status before: \00", align 1
@116 = private unnamed_addr constant [7 x i8] c"status\00", align 1
@117 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@118 = private unnamed_addr constant [22 x i8] c"Table Status after:  \00", align 1
@119 = private unnamed_addr constant [7 x i8] c"status\00", align 1
@120 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@121 = private unnamed_addr constant [34 x i8] c"\0A--- H. Class Field Reference ---\00", align 1
@122 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@123 = private unnamed_addr constant [17 x i8] c"Counter before: \00", align 1
@124 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@125 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@126 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@127 = private unnamed_addr constant [17 x i8] c"Counter after:  \00", align 1
@128 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@129 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@130 = private unnamed_addr constant [25 x i8] c"\0A=== SUITE COMPLETED ===\00", align 1
@131 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %ctr150 = alloca ptr, align 8
  %ctr = alloca ptr, align 8
  %config140 = alloca ptr, align 8
  %config = alloca ptr, align 8
  %d2127 = alloca ptr, align 8
  %d2120 = alloca ptr, align 8
  %d1119 = alloca ptr, align 8
  %d1 = alloca ptr, align 8
  %nums109 = alloca ptr, align 8
  %nums = alloca ptr, align 8
  %user103 = alloca ptr, align 8
  %user = alloca ptr, align 8
  %isActive99 = alloca i1, align 1
  %isActive = alloca i1, align 1
  %pi96 = alloca double, align 8
  %pi = alloca double, align 8
  %y92 = alloca i32, align 4
  %y = alloca i32, align 4
  %x91 = alloca i32, align 4
  %x = alloca i32, align 4
  %f88 = alloca ptr, align 8
  %f = alloca ptr, align 8
  %b80 = alloca ptr, align 8
  %b = alloca ptr, align 8
  %pc63 = alloca ptr, align 8
  %pc = alloca ptr, align 8
  %s343 = alloca ptr, align 8
  %s3 = alloca ptr, align 8
  %s236 = alloca ptr, align 8
  %s2 = alloca ptr, align 8
  %s129 = alloca ptr, align 8
  %s1 = alloca ptr, align 8
  %generic21 = alloca ptr, align 8
  %generic = alloca ptr, align 8
  %c9 = alloca ptr, align 8
  %c = alloca ptr, align 8
  %d2 = alloca ptr, align 8
  %d = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @2)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %print_unbox1)
  %alloc_obj = call ptr @calloc(i64 1, i64 16)
  %4 = getelementptr inbounds nuw %Dog, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Dog, ptr %4, align 8
  %5 = call ptr @moksha_box_string(ptr @16)
  call void @Dog_constructor(ptr %alloc_obj, ptr %5)
  %6 = icmp eq ptr %alloc_obj, null
  br i1 %6, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %entry
  %clone_alloc = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 %alloc_obj, i64 16, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %entry
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %d, align 8
  store ptr %clone_res, ptr %d2, align 8
  %7 = load ptr, ptr %d2, align 8
  %8 = icmp eq ptr %7, null
  br i1 %8, label %call_error, label %call_safe

call_error:                                       ; preds = %clone_merge
  %9 = call ptr @moksha_box_string(ptr @17)
  store ptr %9, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe:                                        ; preds = %clone_merge
  %10 = getelementptr inbounds nuw %Dog, ptr %7, i32 0, i32 0
  %vptr = load ptr, ptr %10, align 8
  %11 = getelementptr inbounds ptr, ptr %vptr, i32 0
  %func_ptr = load ptr, ptr %11, align 8
  call void %func_ptr(ptr %7)
  %alloc_obj3 = call ptr @calloc(i64 1, i64 16)
  %12 = getelementptr inbounds nuw %Cat, ptr %alloc_obj3, i32 0, i32 0
  store ptr @VTable_Cat, ptr %12, align 8
  %13 = call ptr @moksha_box_string(ptr @18)
  call void @Cat_constructor(ptr %alloc_obj3, ptr %13)
  %14 = icmp eq ptr %alloc_obj3, null
  br i1 %14, label %clone_skip5, label %clone_do4

clone_do4:                                        ; preds = %call_safe
  %clone_alloc7 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc7, ptr align 8 %alloc_obj3, i64 16, i1 false)
  br label %clone_merge6

clone_skip5:                                      ; preds = %call_safe
  br label %clone_merge6

clone_merge6:                                     ; preds = %clone_skip5, %clone_do4
  %clone_res8 = phi ptr [ %clone_alloc7, %clone_do4 ], [ null, %clone_skip5 ]
  store ptr %clone_res8, ptr %c, align 8
  store ptr %clone_res8, ptr %c9, align 8
  %15 = load ptr, ptr %c9, align 8
  %16 = icmp eq ptr %15, null
  br i1 %16, label %call_error10, label %call_safe11

call_error10:                                     ; preds = %clone_merge6
  %17 = call ptr @moksha_box_string(ptr @19)
  store ptr %17, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe11:                                      ; preds = %clone_merge6
  %18 = getelementptr inbounds nuw %Cat, ptr %15, i32 0, i32 0
  %vptr12 = load ptr, ptr %18, align 8
  %19 = getelementptr inbounds ptr, ptr %vptr12, i32 0
  %func_ptr13 = load ptr, ptr %19, align 8
  call void %func_ptr13(ptr %15)
  %20 = call ptr @moksha_box_string(ptr @20)
  %print_unbox14 = call ptr @moksha_unbox_string(ptr %20)
  %21 = call i32 (ptr, ...) @printf(ptr @21, ptr %print_unbox14)
  %alloc_obj15 = call ptr @calloc(i64 1, i64 16)
  %22 = getelementptr inbounds nuw %Animal, ptr %alloc_obj15, i32 0, i32 0
  store ptr @VTable_Animal, ptr %22, align 8
  %23 = call ptr @moksha_box_string(ptr @27)
  call void @Animal_constructor(ptr %alloc_obj15, ptr %23)
  %24 = icmp eq ptr %alloc_obj15, null
  br i1 %24, label %clone_skip17, label %clone_do16

clone_do16:                                       ; preds = %call_safe11
  %clone_alloc19 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc19, ptr align 8 %alloc_obj15, i64 16, i1 false)
  br label %clone_merge18

clone_skip17:                                     ; preds = %call_safe11
  br label %clone_merge18

clone_merge18:                                    ; preds = %clone_skip17, %clone_do16
  %clone_res20 = phi ptr [ %clone_alloc19, %clone_do16 ], [ null, %clone_skip17 ]
  store ptr %clone_res20, ptr %generic, align 8
  store ptr %clone_res20, ptr %generic21, align 8
  %25 = load ptr, ptr %generic21, align 8
  call void @makeItSpeak(ptr %25)
  %26 = load ptr, ptr %d2, align 8
  call void @makeItSpeak(ptr %26)
  %27 = load ptr, ptr %c9, align 8
  call void @makeItSpeak(ptr %27)
  %28 = call ptr @moksha_box_string(ptr @28)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %28)
  %29 = call i32 (ptr, ...) @printf(ptr @29, ptr %print_unbox22)
  %alloc_obj23 = call ptr @calloc(i64 1, i64 8)
  %30 = getelementptr inbounds nuw %Shape, ptr %alloc_obj23, i32 0, i32 0
  store ptr @VTable_Shape, ptr %30, align 8
  call void @Shape_constructor(ptr %alloc_obj23)
  %31 = icmp eq ptr %alloc_obj23, null
  br i1 %31, label %clone_skip25, label %clone_do24

clone_do24:                                       ; preds = %clone_merge18
  %clone_alloc27 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc27, ptr align 8 %alloc_obj23, i64 8, i1 false)
  br label %clone_merge26

clone_skip25:                                     ; preds = %clone_merge18
  br label %clone_merge26

clone_merge26:                                    ; preds = %clone_skip25, %clone_do24
  %clone_res28 = phi ptr [ %clone_alloc27, %clone_do24 ], [ null, %clone_skip25 ]
  store ptr %clone_res28, ptr %s1, align 8
  store ptr %clone_res28, ptr %s129, align 8
  %alloc_obj30 = call ptr @calloc(i64 1, i64 8)
  %32 = getelementptr inbounds nuw %Circle, ptr %alloc_obj30, i32 0, i32 0
  store ptr @VTable_Circle, ptr %32, align 8
  call void @Circle_constructor(ptr %alloc_obj30)
  %33 = icmp eq ptr %alloc_obj30, null
  br i1 %33, label %clone_skip32, label %clone_do31

clone_do31:                                       ; preds = %clone_merge26
  %clone_alloc34 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc34, ptr align 8 %alloc_obj30, i64 8, i1 false)
  br label %clone_merge33

clone_skip32:                                     ; preds = %clone_merge26
  br label %clone_merge33

clone_merge33:                                    ; preds = %clone_skip32, %clone_do31
  %clone_res35 = phi ptr [ %clone_alloc34, %clone_do31 ], [ null, %clone_skip32 ]
  store ptr %clone_res35, ptr %s2, align 8
  store ptr %clone_res35, ptr %s236, align 8
  %alloc_obj37 = call ptr @calloc(i64 1, i64 8)
  %34 = getelementptr inbounds nuw %FilledCircle, ptr %alloc_obj37, i32 0, i32 0
  store ptr @VTable_FilledCircle, ptr %34, align 8
  call void @FilledCircle_constructor(ptr %alloc_obj37)
  %35 = icmp eq ptr %alloc_obj37, null
  br i1 %35, label %clone_skip39, label %clone_do38

clone_do38:                                       ; preds = %clone_merge33
  %clone_alloc41 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc41, ptr align 8 %alloc_obj37, i64 8, i1 false)
  br label %clone_merge40

clone_skip39:                                     ; preds = %clone_merge33
  br label %clone_merge40

clone_merge40:                                    ; preds = %clone_skip39, %clone_do38
  %clone_res42 = phi ptr [ %clone_alloc41, %clone_do38 ], [ null, %clone_skip39 ]
  store ptr %clone_res42, ptr %s3, align 8
  store ptr %clone_res42, ptr %s343, align 8
  %36 = load ptr, ptr %s129, align 8
  %37 = icmp eq ptr %36, null
  br i1 %37, label %call_error44, label %call_safe45

call_error44:                                     ; preds = %clone_merge40
  %38 = call ptr @moksha_box_string(ptr @36)
  store ptr %38, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe45:                                      ; preds = %clone_merge40
  %39 = getelementptr inbounds nuw %Shape, ptr %36, i32 0, i32 0
  %vptr46 = load ptr, ptr %39, align 8
  %40 = getelementptr inbounds ptr, ptr %vptr46, i32 0
  %func_ptr47 = load ptr, ptr %40, align 8
  call void %func_ptr47(ptr %36)
  %41 = load ptr, ptr %s236, align 8
  %42 = icmp eq ptr %41, null
  br i1 %42, label %call_error48, label %call_safe49

call_error48:                                     ; preds = %call_safe45
  %43 = call ptr @moksha_box_string(ptr @37)
  store ptr %43, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe49:                                      ; preds = %call_safe45
  %44 = getelementptr inbounds nuw %Shape, ptr %41, i32 0, i32 0
  %vptr50 = load ptr, ptr %44, align 8
  %45 = getelementptr inbounds ptr, ptr %vptr50, i32 0
  %func_ptr51 = load ptr, ptr %45, align 8
  call void %func_ptr51(ptr %41)
  %46 = load ptr, ptr %s343, align 8
  %47 = icmp eq ptr %46, null
  br i1 %47, label %call_error52, label %call_safe53

call_error52:                                     ; preds = %call_safe49
  %48 = call ptr @moksha_box_string(ptr @38)
  store ptr %48, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe53:                                      ; preds = %call_safe49
  %49 = getelementptr inbounds nuw %Shape, ptr %46, i32 0, i32 0
  %vptr54 = load ptr, ptr %49, align 8
  %50 = getelementptr inbounds ptr, ptr %vptr54, i32 0
  %func_ptr55 = load ptr, ptr %50, align 8
  call void %func_ptr55(ptr %46)
  %51 = call ptr @moksha_box_string(ptr @39)
  %print_unbox56 = call ptr @moksha_unbox_string(ptr %51)
  %52 = call i32 (ptr, ...) @printf(ptr @40, ptr %print_unbox56)
  %alloc_obj57 = call ptr @calloc(i64 1, i64 8)
  %53 = getelementptr inbounds nuw %Computer, ptr %alloc_obj57, i32 0, i32 0
  store ptr @VTable_Computer, ptr %53, align 8
  call void @Computer_constructor(ptr %alloc_obj57)
  %54 = icmp eq ptr %alloc_obj57, null
  br i1 %54, label %clone_skip59, label %clone_do58

clone_do58:                                       ; preds = %call_safe53
  %clone_alloc61 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc61, ptr align 8 %alloc_obj57, i64 8, i1 false)
  br label %clone_merge60

clone_skip59:                                     ; preds = %call_safe53
  br label %clone_merge60

clone_merge60:                                    ; preds = %clone_skip59, %clone_do58
  %clone_res62 = phi ptr [ %clone_alloc61, %clone_do58 ], [ null, %clone_skip59 ]
  store ptr %clone_res62, ptr %pc, align 8
  store ptr %clone_res62, ptr %pc63, align 8
  %55 = load ptr, ptr %pc63, align 8
  %56 = icmp eq ptr %55, null
  br i1 %56, label %call_error64, label %call_safe65

call_error64:                                     ; preds = %clone_merge60
  %57 = call ptr @moksha_box_string(ptr @47)
  store ptr %57, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe65:                                      ; preds = %clone_merge60
  %58 = getelementptr inbounds nuw %Device, ptr %55, i32 0, i32 0
  %vptr66 = load ptr, ptr %58, align 8
  %59 = getelementptr inbounds ptr, ptr %vptr66, i32 0
  %func_ptr67 = load ptr, ptr %59, align 8
  call void %func_ptr67(ptr %55)
  %60 = load ptr, ptr %pc63, align 8
  %61 = icmp eq ptr %60, null
  br i1 %61, label %call_error68, label %call_safe69

call_error68:                                     ; preds = %call_safe65
  %62 = call ptr @moksha_box_string(ptr @48)
  store ptr %62, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe69:                                      ; preds = %call_safe65
  %63 = getelementptr inbounds nuw %Device, ptr %60, i32 0, i32 0
  %vptr70 = load ptr, ptr %63, align 8
  %64 = getelementptr inbounds ptr, ptr %vptr70, i32 1
  %func_ptr71 = load ptr, ptr %64, align 8
  call void %func_ptr71(ptr %60)
  %65 = call ptr @moksha_box_string(ptr @49)
  %print_unbox72 = call ptr @moksha_unbox_string(ptr %65)
  %66 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox72)
  %67 = call ptr @moksha_box_string(ptr @57)
  %print_unbox73 = call ptr @moksha_unbox_string(ptr %67)
  %68 = call i32 (ptr, ...) @printf(ptr @58, ptr %print_unbox73)
  %alloc_obj74 = call ptr @calloc(i64 1, i64 8)
  %69 = getelementptr inbounds nuw %BaseLogger, ptr %alloc_obj74, i32 0, i32 0
  store ptr @VTable_BaseLogger, ptr %69, align 8
  call void @BaseLogger_constructor(ptr %alloc_obj74)
  %70 = icmp eq ptr %alloc_obj74, null
  br i1 %70, label %clone_skip76, label %clone_do75

clone_do75:                                       ; preds = %call_safe69
  %clone_alloc78 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc78, ptr align 8 %alloc_obj74, i64 8, i1 false)
  br label %clone_merge77

clone_skip76:                                     ; preds = %call_safe69
  br label %clone_merge77

clone_merge77:                                    ; preds = %clone_skip76, %clone_do75
  %clone_res79 = phi ptr [ %clone_alloc78, %clone_do75 ], [ null, %clone_skip76 ]
  store ptr %clone_res79, ptr %b, align 8
  store ptr %clone_res79, ptr %b80, align 8
  %71 = call ptr @moksha_box_string(ptr @59)
  %print_unbox81 = call ptr @moksha_unbox_string(ptr %71)
  %72 = call i32 (ptr, ...) @printf(ptr @60, ptr %print_unbox81)
  %alloc_obj82 = call ptr @calloc(i64 1, i64 8)
  %73 = getelementptr inbounds nuw %FileLogger, ptr %alloc_obj82, i32 0, i32 0
  store ptr @VTable_FileLogger, ptr %73, align 8
  call void @FileLogger_constructor(ptr %alloc_obj82)
  %74 = icmp eq ptr %alloc_obj82, null
  br i1 %74, label %clone_skip84, label %clone_do83

clone_do83:                                       ; preds = %clone_merge77
  %clone_alloc86 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc86, ptr align 8 %alloc_obj82, i64 8, i1 false)
  br label %clone_merge85

clone_skip84:                                     ; preds = %clone_merge77
  br label %clone_merge85

clone_merge85:                                    ; preds = %clone_skip84, %clone_do83
  %clone_res87 = phi ptr [ %clone_alloc86, %clone_do83 ], [ null, %clone_skip84 ]
  store ptr %clone_res87, ptr %f, align 8
  store ptr %clone_res87, ptr %f88, align 8
  %75 = call ptr @moksha_box_string(ptr @61)
  %print_unbox89 = call ptr @moksha_unbox_string(ptr %75)
  %76 = call i32 (ptr, ...) @printf(ptr @62, ptr %print_unbox89)
  %77 = call ptr @moksha_box_string(ptr @63)
  %print_unbox90 = call ptr @moksha_unbox_string(ptr %77)
  %78 = call i32 (ptr, ...) @printf(ptr @64, ptr %print_unbox90)
  store i32 10, ptr %x, align 4
  store i32 10, ptr %x91, align 4
  store i32 20, ptr %y, align 4
  store i32 20, ptr %y92, align 4
  %79 = call ptr @moksha_box_string(ptr @65)
  %80 = load i32, ptr %x91, align 4
  %81 = call ptr @moksha_int_to_str(i32 %80)
  %82 = call ptr @moksha_box_string(ptr %81)
  %83 = call ptr @moksha_string_concat(ptr %79, ptr %82)
  %84 = call ptr @moksha_box_string(ptr @66)
  %85 = call ptr @moksha_string_concat(ptr %83, ptr %84)
  %86 = load i32, ptr %y92, align 4
  %87 = call ptr @moksha_int_to_str(i32 %86)
  %88 = call ptr @moksha_box_string(ptr %87)
  %89 = call ptr @moksha_string_concat(ptr %85, ptr %88)
  %print_unbox93 = call ptr @moksha_unbox_string(ptr %89)
  %90 = call i32 (ptr, ...) @printf(ptr @67, ptr %print_unbox93)
  call void @swapInt(ptr %x91, ptr %y92)
  %91 = call ptr @moksha_box_string(ptr @68)
  %92 = load i32, ptr %x91, align 4
  %93 = call ptr @moksha_int_to_str(i32 %92)
  %94 = call ptr @moksha_box_string(ptr %93)
  %95 = call ptr @moksha_string_concat(ptr %91, ptr %94)
  %96 = call ptr @moksha_box_string(ptr @69)
  %97 = call ptr @moksha_string_concat(ptr %95, ptr %96)
  %98 = load i32, ptr %y92, align 4
  %99 = call ptr @moksha_int_to_str(i32 %98)
  %100 = call ptr @moksha_box_string(ptr %99)
  %101 = call ptr @moksha_string_concat(ptr %97, ptr %100)
  %print_unbox94 = call ptr @moksha_unbox_string(ptr %101)
  %102 = call i32 (ptr, ...) @printf(ptr @70, ptr %print_unbox94)
  %103 = call ptr @moksha_box_string(ptr @71)
  %print_unbox95 = call ptr @moksha_unbox_string(ptr %103)
  %104 = call i32 (ptr, ...) @printf(ptr @72, ptr %print_unbox95)
  store double 3.141590e+00, ptr %pi, align 8
  store double 3.141590e+00, ptr %pi96, align 8
  call void @doubleValue(ptr %pi96)
  %105 = call ptr @moksha_box_string(ptr @73)
  %106 = load double, ptr %pi96, align 8
  %107 = call ptr @moksha_double_to_str(double %106)
  %108 = call ptr @moksha_box_string(ptr %107)
  %109 = call ptr @moksha_string_concat(ptr %105, ptr %108)
  %print_unbox97 = call ptr @moksha_unbox_string(ptr %109)
  %110 = call i32 (ptr, ...) @printf(ptr @74, ptr %print_unbox97)
  %111 = call ptr @moksha_box_string(ptr @75)
  %print_unbox98 = call ptr @moksha_unbox_string(ptr %111)
  %112 = call i32 (ptr, ...) @printf(ptr @76, ptr %print_unbox98)
  store i1 false, ptr %isActive, align 1
  store i1 false, ptr %isActive99, align 1
  %113 = call ptr @moksha_box_string(ptr @77)
  %114 = load i1, ptr %isActive99, align 1
  %115 = zext i1 %114 to i32
  %116 = call ptr @moksha_box_bool(i32 %115)
  %117 = call ptr @moksha_any_to_str(ptr %116)
  %118 = call ptr @moksha_box_string(ptr %117)
  %119 = call ptr @moksha_string_concat(ptr %113, ptr %118)
  %print_unbox100 = call ptr @moksha_unbox_string(ptr %119)
  %120 = call i32 (ptr, ...) @printf(ptr @78, ptr %print_unbox100)
  call void @toggle(ptr %isActive99)
  %121 = call ptr @moksha_box_string(ptr @79)
  %122 = load i1, ptr %isActive99, align 1
  %123 = zext i1 %122 to i32
  %124 = call ptr @moksha_box_bool(i32 %123)
  %125 = call ptr @moksha_any_to_str(ptr %124)
  %126 = call ptr @moksha_box_string(ptr %125)
  %127 = call ptr @moksha_string_concat(ptr %121, ptr %126)
  %print_unbox101 = call ptr @moksha_unbox_string(ptr %127)
  %128 = call i32 (ptr, ...) @printf(ptr @80, ptr %print_unbox101)
  %129 = call ptr @moksha_box_string(ptr @81)
  %print_unbox102 = call ptr @moksha_unbox_string(ptr %129)
  %130 = call i32 (ptr, ...) @printf(ptr @82, ptr %print_unbox102)
  %131 = call ptr @moksha_box_string(ptr @84)
  store ptr %131, ptr %user, align 8
  store ptr %131, ptr %user103, align 8
  %132 = call ptr @moksha_box_string(ptr @85)
  %133 = load ptr, ptr %user103, align 8
  %134 = call ptr @moksha_string_concat(ptr %132, ptr %133)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %134)
  %135 = call i32 (ptr, ...) @printf(ptr @86, ptr %print_unbox104)
  call void @changeName(ptr %user103)
  %136 = call ptr @moksha_box_string(ptr @87)
  %137 = load ptr, ptr %user103, align 8
  %138 = call ptr @moksha_string_concat(ptr %136, ptr %137)
  %print_unbox105 = call ptr @moksha_unbox_string(ptr %138)
  %139 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox105)
  %140 = call ptr @moksha_box_string(ptr @89)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %140)
  %141 = call i32 (ptr, ...) @printf(ptr @90, ptr %print_unbox106)
  %142 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 1)
  %143 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i)
  %box_i107 = call ptr @moksha_box_int(i32 2)
  %144 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i107)
  %box_i108 = call ptr @moksha_box_int(i32 3)
  %145 = call ptr @moksha_array_push_val(ptr %142, ptr %box_i108)
  store ptr %142, ptr %nums, align 8
  store ptr %142, ptr %nums109, align 8
  %146 = call ptr @moksha_box_string(ptr @91)
  %147 = load ptr, ptr %nums109, align 8
  %148 = call ptr @moksha_array_get(ptr %147, i32 0)
  %149 = call ptr @moksha_any_to_str(ptr %148)
  %150 = call ptr @moksha_box_string(ptr %149)
  %151 = call ptr @moksha_string_concat(ptr %146, ptr %150)
  %print_unbox110 = call ptr @moksha_unbox_string(ptr %151)
  %152 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox110)
  call void @replaceArray(ptr %nums109)
  %153 = call ptr @moksha_box_string(ptr @93)
  %154 = load ptr, ptr %nums109, align 8
  %155 = call ptr @moksha_array_get(ptr %154, i32 0)
  %156 = call ptr @moksha_any_to_str(ptr %155)
  %157 = call ptr @moksha_box_string(ptr %156)
  %158 = call ptr @moksha_string_concat(ptr %153, ptr %157)
  %print_unbox111 = call ptr @moksha_unbox_string(ptr %158)
  %159 = call i32 (ptr, ...) @printf(ptr @94, ptr %print_unbox111)
  %160 = call ptr @moksha_box_string(ptr @95)
  %print_unbox112 = call ptr @moksha_unbox_string(ptr %160)
  %161 = call i32 (ptr, ...) @printf(ptr @96, ptr %print_unbox112)
  %alloc_obj113 = call ptr @calloc(i64 1, i64 16)
  %162 = getelementptr inbounds nuw %Dog, ptr %alloc_obj113, i32 0, i32 0
  store ptr @VTable_Dog, ptr %162, align 8
  %163 = call ptr @moksha_box_string(ptr @97)
  call void @Dog_constructor(ptr %alloc_obj113, ptr %163)
  %164 = icmp eq ptr %alloc_obj113, null
  br i1 %164, label %clone_skip115, label %clone_do114

clone_do114:                                      ; preds = %clone_merge85
  %clone_alloc117 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc117, ptr align 8 %alloc_obj113, i64 16, i1 false)
  br label %clone_merge116

clone_skip115:                                    ; preds = %clone_merge85
  br label %clone_merge116

clone_merge116:                                   ; preds = %clone_skip115, %clone_do114
  %clone_res118 = phi ptr [ %clone_alloc117, %clone_do114 ], [ null, %clone_skip115 ]
  store ptr %clone_res118, ptr %d1, align 8
  store ptr %clone_res118, ptr %d1119, align 8
  %alloc_obj121 = call ptr @calloc(i64 1, i64 16)
  %165 = getelementptr inbounds nuw %Dog, ptr %alloc_obj121, i32 0, i32 0
  store ptr @VTable_Dog, ptr %165, align 8
  %166 = call ptr @moksha_box_string(ptr @98)
  call void @Dog_constructor(ptr %alloc_obj121, ptr %166)
  %167 = icmp eq ptr %alloc_obj121, null
  br i1 %167, label %clone_skip123, label %clone_do122

clone_do122:                                      ; preds = %clone_merge116
  %clone_alloc125 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc125, ptr align 8 %alloc_obj121, i64 16, i1 false)
  br label %clone_merge124

clone_skip123:                                    ; preds = %clone_merge116
  br label %clone_merge124

clone_merge124:                                   ; preds = %clone_skip123, %clone_do122
  %clone_res126 = phi ptr [ %clone_alloc125, %clone_do122 ], [ null, %clone_skip123 ]
  store ptr %clone_res126, ptr %d2120, align 8
  store ptr %clone_res126, ptr %d2127, align 8
  %168 = call ptr @moksha_box_string(ptr @99)
  %loaded_obj_ptr = load ptr, ptr %d1119, align 8
  %169 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %169, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %clone_merge124
  %170 = call ptr @moksha_box_string(ptr @100)
  store ptr %170, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe:                                         ; preds = %clone_merge124
  %171 = getelementptr inbounds nuw %Dog, ptr %loaded_obj_ptr, i32 0, i32 1
  %172 = load ptr, ptr %171, align 8
  %173 = call ptr @moksha_string_concat(ptr %168, ptr %172)
  %174 = call ptr @moksha_box_string(ptr @101)
  %175 = call ptr @moksha_string_concat(ptr %173, ptr %174)
  %loaded_obj_ptr128 = load ptr, ptr %d2127, align 8
  %176 = icmp eq ptr %loaded_obj_ptr128, null
  br i1 %176, label %mem_error129, label %mem_safe130

mem_error129:                                     ; preds = %mem_safe
  %177 = call ptr @moksha_box_string(ptr @102)
  store ptr %177, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe130:                                      ; preds = %mem_safe
  %178 = getelementptr inbounds nuw %Dog, ptr %loaded_obj_ptr128, i32 0, i32 1
  %179 = load ptr, ptr %178, align 8
  %180 = call ptr @moksha_string_concat(ptr %175, ptr %179)
  %print_unbox131 = call ptr @moksha_unbox_string(ptr %180)
  %181 = call i32 (ptr, ...) @printf(ptr @103, ptr %print_unbox131)
  call void @swapDogs(ptr %d1119, ptr %d2127)
  %182 = call ptr @moksha_box_string(ptr @104)
  %loaded_obj_ptr132 = load ptr, ptr %d1119, align 8
  %183 = icmp eq ptr %loaded_obj_ptr132, null
  br i1 %183, label %mem_error133, label %mem_safe134

mem_error133:                                     ; preds = %mem_safe130
  %184 = call ptr @moksha_box_string(ptr @105)
  store ptr %184, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe134:                                      ; preds = %mem_safe130
  %185 = getelementptr inbounds nuw %Dog, ptr %loaded_obj_ptr132, i32 0, i32 1
  %186 = load ptr, ptr %185, align 8
  %187 = call ptr @moksha_string_concat(ptr %182, ptr %186)
  %188 = call ptr @moksha_box_string(ptr @106)
  %189 = call ptr @moksha_string_concat(ptr %187, ptr %188)
  %loaded_obj_ptr135 = load ptr, ptr %d2127, align 8
  %190 = icmp eq ptr %loaded_obj_ptr135, null
  br i1 %190, label %mem_error136, label %mem_safe137

mem_error136:                                     ; preds = %mem_safe134
  %191 = call ptr @moksha_box_string(ptr @107)
  store ptr %191, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe137:                                      ; preds = %mem_safe134
  %192 = getelementptr inbounds nuw %Dog, ptr %loaded_obj_ptr135, i32 0, i32 1
  %193 = load ptr, ptr %192, align 8
  %194 = call ptr @moksha_string_concat(ptr %189, ptr %193)
  %print_unbox138 = call ptr @moksha_unbox_string(ptr %194)
  %195 = call i32 (ptr, ...) @printf(ptr @108, ptr %print_unbox138)
  %196 = call ptr @moksha_box_string(ptr @109)
  %print_unbox139 = call ptr @moksha_unbox_string(ptr %196)
  %197 = call i32 (ptr, ...) @printf(ptr @110, ptr %print_unbox139)
  %198 = call ptr @moksha_alloc_table(i32 1)
  %199 = call ptr @moksha_box_string(ptr @113)
  %200 = call ptr @moksha_box_string(ptr @114)
  %201 = call ptr @moksha_table_set(ptr %198, ptr %199, ptr %200)
  store ptr %198, ptr %config, align 8
  store ptr %198, ptr %config140, align 8
  %202 = call ptr @moksha_box_string(ptr @115)
  %203 = load ptr, ptr %config140, align 8
  %204 = call ptr @moksha_box_string(ptr @116)
  %205 = call ptr @moksha_table_get(ptr %203, ptr %204)
  %206 = call ptr @moksha_any_to_str(ptr %205)
  %207 = call ptr @moksha_box_string(ptr %206)
  %208 = call ptr @moksha_string_concat(ptr %202, ptr %207)
  %print_unbox141 = call ptr @moksha_unbox_string(ptr %208)
  %209 = call i32 (ptr, ...) @printf(ptr @117, ptr %print_unbox141)
  call void @injectKey(ptr %config140)
  %210 = call ptr @moksha_box_string(ptr @118)
  %211 = load ptr, ptr %config140, align 8
  %212 = call ptr @moksha_box_string(ptr @119)
  %213 = call ptr @moksha_table_get(ptr %211, ptr %212)
  %214 = call ptr @moksha_any_to_str(ptr %213)
  %215 = call ptr @moksha_box_string(ptr %214)
  %216 = call ptr @moksha_string_concat(ptr %210, ptr %215)
  %print_unbox142 = call ptr @moksha_unbox_string(ptr %216)
  %217 = call i32 (ptr, ...) @printf(ptr @120, ptr %print_unbox142)
  %218 = call ptr @moksha_box_string(ptr @121)
  %print_unbox143 = call ptr @moksha_unbox_string(ptr %218)
  %219 = call i32 (ptr, ...) @printf(ptr @122, ptr %print_unbox143)
  %alloc_obj144 = call ptr @calloc(i64 1, i64 24)
  call void @Counter_constructor(ptr %alloc_obj144)
  %220 = icmp eq ptr %alloc_obj144, null
  br i1 %220, label %clone_skip146, label %clone_do145

clone_do145:                                      ; preds = %mem_safe137
  %clone_alloc148 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc148, ptr align 8 %alloc_obj144, i64 24, i1 false)
  br label %clone_merge147

clone_skip146:                                    ; preds = %mem_safe137
  br label %clone_merge147

clone_merge147:                                   ; preds = %clone_skip146, %clone_do145
  %clone_res149 = phi ptr [ %clone_alloc148, %clone_do145 ], [ null, %clone_skip146 ]
  store ptr %clone_res149, ptr %ctr, align 8
  store ptr %clone_res149, ptr %ctr150, align 8
  %221 = call ptr @moksha_box_string(ptr @123)
  %loaded_obj_ptr151 = load ptr, ptr %ctr150, align 8
  %222 = icmp eq ptr %loaded_obj_ptr151, null
  br i1 %222, label %mem_error152, label %mem_safe153

mem_error152:                                     ; preds = %clone_merge147
  %223 = call ptr @moksha_box_string(ptr @124)
  store ptr %223, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe153:                                      ; preds = %clone_merge147
  %224 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr151, i32 0, i32 1
  %225 = load i32, ptr %224, align 4
  %226 = call ptr @moksha_int_to_str(i32 %225)
  %227 = call ptr @moksha_box_string(ptr %226)
  %228 = call ptr @moksha_string_concat(ptr %221, ptr %227)
  %print_unbox154 = call ptr @moksha_unbox_string(ptr %228)
  %229 = call i32 (ptr, ...) @printf(ptr @125, ptr %print_unbox154)
  %loaded_obj_ptr155 = load ptr, ptr %ctr150, align 8
  %230 = icmp eq ptr %loaded_obj_ptr155, null
  br i1 %230, label %mem_error156, label %mem_safe157

mem_error156:                                     ; preds = %mem_safe153
  %231 = call ptr @moksha_box_string(ptr @126)
  store ptr %231, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe157:                                      ; preds = %mem_safe153
  %232 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr155, i32 0, i32 1
  call void @incrementRef(ptr %232)
  %233 = call ptr @moksha_box_string(ptr @127)
  %loaded_obj_ptr158 = load ptr, ptr %ctr150, align 8
  %234 = icmp eq ptr %loaded_obj_ptr158, null
  br i1 %234, label %mem_error159, label %mem_safe160

mem_error159:                                     ; preds = %mem_safe157
  %235 = call ptr @moksha_box_string(ptr @128)
  store ptr %235, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe160:                                      ; preds = %mem_safe157
  %236 = getelementptr inbounds nuw %Counter, ptr %loaded_obj_ptr158, i32 0, i32 1
  %237 = load i32, ptr %236, align 4
  %238 = call ptr @moksha_int_to_str(i32 %237)
  %239 = call ptr @moksha_box_string(ptr %238)
  %240 = call ptr @moksha_string_concat(ptr %233, ptr %239)
  %print_unbox161 = call ptr @moksha_unbox_string(ptr %240)
  %241 = call i32 (ptr, ...) @printf(ptr @129, ptr %print_unbox161)
  %242 = call ptr @moksha_box_string(ptr @130)
  %print_unbox162 = call ptr @moksha_unbox_string(ptr %242)
  %243 = call i32 (ptr, ...) @printf(ptr @131, ptr %print_unbox162)
  ret i32 0
}

define void @Animal_speak(ptr %this) {
entry:
  %0 = icmp eq ptr %this, null
  br i1 %0, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @7)
  store ptr %1, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %2 = getelementptr inbounds nuw %Animal, ptr %this, i32 0, i32 1
  %3 = load ptr, ptr %2, align 8
  %4 = call ptr @moksha_box_string(ptr @8)
  %5 = call ptr @moksha_string_concat(ptr %3, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @9, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe
  ret void

unwind:                                           ; preds = %mem_safe
  ret void
}

define void @Animal_constructor(ptr %this, ptr %n) {
entry:
  %n1 = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @4)
  %1 = getelementptr inbounds nuw %Animal, ptr %this, i32 0, i32 1
  store ptr %0, ptr %1, align 8
  store ptr %n, ptr %n1, align 8
  %2 = load ptr, ptr %n1, align 8
  %3 = icmp eq ptr %this, null
  br i1 %3, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %4 = call ptr @moksha_box_string(ptr @5)
  store ptr %4, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %5 = getelementptr inbounds nuw %Animal, ptr %this, i32 0, i32 1
  %6 = icmp eq ptr %this, null
  br i1 %6, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %7 = call ptr @moksha_box_string(ptr @6)
  store ptr %7, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe3:                                        ; preds = %mem_safe
  %8 = getelementptr inbounds nuw %Animal, ptr %this, i32 0, i32 1
  store ptr %2, ptr %5, align 8
  %9 = load i32, ptr @__exception_flag, align 4
  %10 = icmp ne i32 %9, 0
  br i1 %10, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe3
  ret void

unwind:                                           ; preds = %mem_safe3
  ret void
}

define void @Dog_speak(ptr %this) {
entry:
  %0 = icmp eq ptr %this, null
  br i1 %0, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @10)
  store ptr %1, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %2 = getelementptr inbounds nuw %Dog, ptr %this, i32 0, i32 1
  %3 = load ptr, ptr %2, align 8
  %4 = call ptr @moksha_box_string(ptr @11)
  %5 = call ptr @moksha_string_concat(ptr %3, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @12, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe
  ret void

unwind:                                           ; preds = %mem_safe
  ret void
}

define void @Dog_constructor(ptr %this, ptr %0) {
entry:
  call void @Animal_constructor(ptr %this, ptr %0)
  ret void
}

define void @Cat_speak(ptr %this) {
entry:
  %0 = icmp eq ptr %this, null
  br i1 %0, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @13)
  store ptr %1, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %2 = getelementptr inbounds nuw %Cat, ptr %this, i32 0, i32 1
  %3 = load ptr, ptr %2, align 8
  %4 = call ptr @moksha_box_string(ptr @14)
  %5 = call ptr @moksha_string_concat(ptr %3, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @15, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe
  ret void

unwind:                                           ; preds = %mem_safe
  ret void
}

define void @Cat_constructor(ptr %this, ptr %0) {
entry:
  call void @Animal_constructor(ptr %this, ptr %0)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

define void @makeItSpeak(ptr %a) {
entry:
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %0 = call ptr @moksha_box_string(ptr @22)
  %loaded_obj_ptr = load ptr, ptr %a1, align 8
  %1 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @23)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Animal, ptr %loaded_obj_ptr, i32 0, i32 1
  %4 = load ptr, ptr %3, align 8
  %5 = call ptr @moksha_string_concat(ptr %0, ptr %4)
  %6 = call ptr @moksha_box_string(ptr @24)
  %7 = call ptr @moksha_string_concat(ptr %5, ptr %6)
  %print_unbox = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @25, ptr %print_unbox)
  %9 = load i32, ptr @__exception_flag, align 4
  %10 = icmp ne i32 %9, 0
  br i1 %10, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe
  %11 = load ptr, ptr %a1, align 8
  %12 = icmp eq ptr %11, null
  br i1 %12, label %call_error, label %call_safe

unwind:                                           ; preds = %mem_safe
  ret void

call_error:                                       ; preds = %next_stmt
  %13 = call ptr @moksha_box_string(ptr @26)
  store ptr %13, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

call_safe:                                        ; preds = %next_stmt
  %14 = getelementptr inbounds nuw %Animal, ptr %11, i32 0, i32 0
  %vptr = load ptr, ptr %14, align 8
  %15 = getelementptr inbounds ptr, ptr %vptr, i32 0
  %func_ptr = load ptr, ptr %15, align 8
  call void %func_ptr(ptr %11)
  %16 = load i32, ptr @__exception_flag, align 4
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %unwind3, label %next_stmt2

next_stmt2:                                       ; preds = %call_safe
  ret void

unwind3:                                          ; preds = %call_safe
  ret void
}

define void @Shape_draw(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @30)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @31, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Shape_constructor(ptr %this) {
entry:
  ret void
}

define void @Circle_draw(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @32)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Circle_constructor(ptr %this) {
entry:
  call void @Shape_constructor(ptr %this)
  ret void
}

define void @FilledCircle_draw(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @34)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @35, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @FilledCircle_constructor(ptr %this) {
entry:
  call void @Circle_constructor(ptr %this)
  ret void
}

define void @Device_start(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @41)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Device_stop(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @43)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @44, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Device_constructor(ptr %this) {
entry:
  ret void
}

define void @Computer_start(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @45)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @46, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Computer_constructor(ptr %this) {
entry:
  call void @Device_constructor(ptr %this)
  ret void
}

define void @BaseLogger_log(ptr %this, ptr %msg) {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %0 = call ptr @moksha_box_string(ptr @53)
  %1 = load ptr, ptr %msg1, align 8
  %2 = call ptr @moksha_string_concat(ptr %0, ptr %1)
  %print_unbox = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @54, ptr %print_unbox)
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @BaseLogger_constructor(ptr %this) {
entry:
  %0 = icmp eq ptr %this, null
  br i1 %0, label %call_error, label %call_safe

call_error:                                       ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @51)
  store ptr %1, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

call_safe:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @52)
  %3 = getelementptr inbounds nuw %BaseLogger, ptr %this, i32 0, i32 0
  %vptr = load ptr, ptr %3, align 8
  %4 = getelementptr inbounds ptr, ptr %vptr, i32 0
  %func_ptr = load ptr, ptr %4, align 8
  call void %func_ptr(ptr %this, ptr %2)
  %5 = load i32, ptr @__exception_flag, align 4
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %call_safe
  ret void

unwind:                                           ; preds = %call_safe
  ret void
}

define void @FileLogger_log(ptr %this, ptr %msg) {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %0 = call ptr @moksha_box_string(ptr @55)
  %1 = load ptr, ptr %msg1, align 8
  %2 = call ptr @moksha_string_concat(ptr %0, ptr %1)
  %print_unbox = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @56, ptr %print_unbox)
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @FileLogger_constructor(ptr %this) {
entry:
  call void @BaseLogger_constructor(ptr %this)
  ret void
}

define void @swapInt(ptr %a, ptr %b) {
entry:
  %temp1 = alloca i32, align 4
  %temp = alloca i32, align 4
  %0 = load i32, ptr %a, align 4
  store i32 %0, ptr %temp, align 4
  store i32 %0, ptr %temp1, align 4
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  %3 = load i32, ptr %b, align 4
  store i32 %3, ptr %a, align 4
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind3, label %next_stmt2

unwind:                                           ; preds = %entry
  ret void

next_stmt2:                                       ; preds = %next_stmt
  %6 = load i32, ptr %temp1, align 4
  store i32 %6, ptr %b, align 4
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind5, label %next_stmt4

unwind3:                                          ; preds = %next_stmt
  ret void

next_stmt4:                                       ; preds = %next_stmt2
  ret void

unwind5:                                          ; preds = %next_stmt2
  ret void
}

define void @doubleValue(ptr %val) {
entry:
  %0 = load double, ptr %val, align 8
  %fmultmp = fmul double %0, 2.000000e+00
  store double %fmultmp, ptr %val, align 8
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @toggle(ptr %flag) {
entry:
  %0 = load i1, ptr %flag, align 1
  %nottmp = xor i1 %0, true
  store i1 %nottmp, ptr %flag, align 1
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @changeName(ptr %name) {
entry:
  %0 = call ptr @moksha_box_string(ptr @83)
  store ptr %0, ptr %name, align 8
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @replaceArray(ptr %arr) {
entry:
  %0 = call ptr @moksha_alloc_array(i32 3)
  %box_i = call ptr @moksha_box_int(i32 999)
  %1 = call ptr @moksha_array_push_val(ptr %0, ptr %box_i)
  %box_i1 = call ptr @moksha_box_int(i32 888)
  %2 = call ptr @moksha_array_push_val(ptr %0, ptr %box_i1)
  %box_i2 = call ptr @moksha_box_int(i32 777)
  %3 = call ptr @moksha_array_push_val(ptr %0, ptr %box_i2)
  store ptr %0, ptr %arr, align 8
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @swapDogs(ptr %a, ptr %b) {
entry:
  %temp1 = alloca ptr, align 8
  %temp = alloca ptr, align 8
  %0 = load ptr, ptr %a, align 8
  %1 = icmp eq ptr %0, null
  br i1 %1, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %entry
  %clone_alloc = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 %0, i64 16, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %entry
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %temp, align 8
  store ptr %clone_res, ptr %temp1, align 8
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %clone_merge
  %4 = load ptr, ptr %b, align 8
  %5 = icmp eq ptr %4, null
  br i1 %5, label %clone_skip3, label %clone_do2

unwind:                                           ; preds = %clone_merge
  ret void

clone_do2:                                        ; preds = %next_stmt
  %clone_alloc5 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc5, ptr align 8 %4, i64 16, i1 false)
  br label %clone_merge4

clone_skip3:                                      ; preds = %next_stmt
  br label %clone_merge4

clone_merge4:                                     ; preds = %clone_skip3, %clone_do2
  %clone_res6 = phi ptr [ %clone_alloc5, %clone_do2 ], [ null, %clone_skip3 ]
  store ptr %clone_res6, ptr %a, align 8
  %6 = load i32, ptr @__exception_flag, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %unwind8, label %next_stmt7

next_stmt7:                                       ; preds = %clone_merge4
  %8 = load ptr, ptr %temp1, align 8
  %9 = icmp eq ptr %8, null
  br i1 %9, label %clone_skip10, label %clone_do9

unwind8:                                          ; preds = %clone_merge4
  ret void

clone_do9:                                        ; preds = %next_stmt7
  %clone_alloc12 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc12, ptr align 8 %8, i64 16, i1 false)
  br label %clone_merge11

clone_skip10:                                     ; preds = %next_stmt7
  br label %clone_merge11

clone_merge11:                                    ; preds = %clone_skip10, %clone_do9
  %clone_res13 = phi ptr [ %clone_alloc12, %clone_do9 ], [ null, %clone_skip10 ]
  store ptr %clone_res13, ptr %b, align 8
  %10 = load i32, ptr @__exception_flag, align 4
  %11 = icmp ne i32 %10, 0
  br i1 %11, label %unwind15, label %next_stmt14

next_stmt14:                                      ; preds = %clone_merge11
  ret void

unwind15:                                         ; preds = %clone_merge11
  ret void
}

define void @injectKey(ptr %t) {
entry:
  %0 = call ptr @moksha_box_string(ptr @111)
  %1 = load ptr, ptr %t, align 8
  %2 = call ptr @moksha_box_string(ptr @112)
  %3 = call ptr @moksha_table_set(ptr %1, ptr %2, ptr %0)
  %4 = load i32, ptr @__exception_flag, align 4
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Counter_constructor(ptr %this) {
entry:
  %0 = getelementptr inbounds nuw %Counter, ptr %this, i32 0, i32 1
  store i32 0, ptr %0, align 4
  %1 = getelementptr inbounds nuw %Counter, ptr %this, i32 0, i32 2
  store double 1.500000e+00, ptr %1, align 8
  ret void
}

define void @incrementRef(ptr %c) {
entry:
  %0 = load i32, ptr %c, align 4
  %addtmp = add i32 %0, 1
  store i32 %addtmp, ptr %c, align 4
  %1 = load i32, ptr @__exception_flag, align 4
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
