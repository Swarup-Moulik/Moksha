; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Color = type { i32 }
%Outer = type <{ i8, %Inner, i32 }>
%Inner = type <{ i8, i8 }>
%Converter = type { i32 }
%HardwarePacket = type <{ i8, i32, i8 }>
%BitfieldTest = type { i32 }

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [7 x i8] c"HOSTED\00", align 1
@1 = private unnamed_addr constant [39 x i8] c"--- Consolidated Moksha Test Suite ---\00", align 1
@2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@3 = private unnamed_addr constant [23 x i8] c"Size of BitfieldTest: \00", align 1
@4 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@5 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@6 = private unnamed_addr constant [20 x i8] c"Size of int alias: \00", align 1
@7 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@8 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@9 = private unnamed_addr constant [47 x i8] c"Static If: Successfully compiled HOSTED block.\00", align 1
@10 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@11 = private unnamed_addr constant [17 x i8] c"Bitfield Values:\00", align 1
@12 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@13 = private unnamed_addr constant [19 x i8] c"  First (3 bits): \00", align 1
@14 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@16 = private unnamed_addr constant [20 x i8] c"  Second (5 bits): \00", align 1
@17 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@18 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@19 = private unnamed_addr constant [19 x i8] c"  Third (8 bits): \00", align 1
@20 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@21 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@22 = private unnamed_addr constant [19 x i8] c"Raw Memory Value: \00", align 1
@23 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@24 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@25 = private unnamed_addr constant [39 x i8] c"Offset of 'payload' in packed struct: \00", align 1
@26 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@27 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@28 = private unnamed_addr constant [25 x i8] c"Union raw hex as float: \00", align 1
@29 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@30 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@31 = private unnamed_addr constant [12 x i8] c"Hello World\00", align 1
@32 = private unnamed_addr constant [20 x i8] c"Modified C-Buffer: \00", align 1
@33 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@35 = private unnamed_addr constant [30 x i8] c"Nested struct footer offset: \00", align 1
@36 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@37 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@38 = private unnamed_addr constant [20 x i8] c"Pointer index [1]: \00", align 1
@39 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@40 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@41 = private unnamed_addr constant [25 x i8] c"Dereferenced offset +2: \00", align 1
@42 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@43 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@44 = private unnamed_addr constant [26 x i8] c"Union Component 0 (LSB): \00", align 1
@45 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@46 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@47 = private unnamed_addr constant [26 x i8] c"Union Component 3 (MSB): \00", align 1
@48 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@49 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@50 = private unnamed_addr constant [31 x i8] c"Variadic Printf: %d + %d = %d\0A\00", align 1

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
  %b74 = alloca i32, align 4
  %b = alloca i32, align 4
  %a73 = alloca i32, align 4
  %a = alloca i32, align 4
  %pixel67 = alloca %Color, align 8
  %pixel66 = alloca %Color, align 8
  %pixel = alloca %Color, align 8
  %data_ptr60 = alloca ptr, align 8
  %data_ptr = alloca ptr, align 8
  %nested_offset57 = alloca i32, align 4
  %nested_offset = alloca i32, align 4
  %footer_ptr53 = alloca ptr, align 8
  %footer_ptr = alloca ptr, align 8
  %base51 = alloca ptr, align 8
  %base = alloca ptr, align 8
  %obj50 = alloca %Outer, align 8
  %obj49 = alloca %Outer, align 8
  %obj = alloca %Outer, align 8
  %p43 = alloca ptr, align 8
  %p = alloca ptr, align 8
  %buffer41 = alloca ptr, align 8
  %buffer = alloca ptr, align 8
  %conv39 = alloca %Converter, align 8
  %conv38 = alloca %Converter, align 8
  %conv = alloca %Converter, align 8
  %offset36 = alloca i32, align 4
  %offset = alloca i32, align 4
  %payload_addr34 = alloca ptr, align 8
  %payload_addr = alloca ptr, align 8
  %base_addr32 = alloca ptr, align 8
  %base_addr = alloca ptr, align 8
  %packet31 = alloca %HardwarePacket, align 8
  %packet30 = alloca %HardwarePacket, align 8
  %packet = alloca %HardwarePacket, align 8
  %raw28 = alloca ptr, align 8
  %raw = alloca ptr, align 8
  %bf8 = alloca %BitfieldTest, align 8
  %bf7 = alloca %BitfieldTest, align 8
  %bf = alloca %BitfieldTest, align 8
  %s23 = alloca i32, align 4
  %s2 = alloca i32, align 4
  %s12 = alloca i32, align 4
  %s1 = alloca i32, align 4
  %TEST_PLATFORM1 = alloca ptr, align 8
  %TEST_PLATFORM = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @0)
  store ptr %0, ptr %TEST_PLATFORM, align 8
  store ptr %0, ptr %TEST_PLATFORM1, align 8
  %1 = call ptr @moksha_box_string(ptr @1)
  %print_unbox = call ptr @moksha_unbox_string(ptr %1)
  %2 = call i32 (ptr, ...) @printf(ptr @2, ptr %print_unbox)
  store i32 4, ptr %s1, align 4
  store i32 4, ptr %s12, align 4
  store i32 4, ptr %s2, align 4
  store i32 4, ptr %s23, align 4
  %3 = call ptr @moksha_box_string(ptr @3)
  %print_unbox4 = call ptr @moksha_unbox_string(ptr %3)
  %4 = call i32 (ptr, ...) @printf(ptr @4, ptr %print_unbox4)
  %5 = load i32, ptr %s12, align 4
  %6 = call i32 (ptr, ...) @printf(ptr @5, i32 %5)
  %7 = call ptr @moksha_box_string(ptr @6)
  %print_unbox5 = call ptr @moksha_unbox_string(ptr %7)
  %8 = call i32 (ptr, ...) @printf(ptr @7, ptr %print_unbox5)
  %9 = load i32, ptr %s23, align 4
  %10 = call i32 (ptr, ...) @printf(ptr @8, i32 %9)
  %11 = call ptr @moksha_box_string(ptr @9)
  %print_unbox6 = call ptr @moksha_unbox_string(ptr %11)
  %12 = call i32 (ptr, ...) @printf(ptr @10, ptr %print_unbox6)
  %13 = load i32, ptr @__exception_flag, align 4
  %14 = icmp ne i32 %13, 0
  br i1 %14, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  store %BitfieldTest zeroinitializer, ptr %bf7, align 4
  store %BitfieldTest zeroinitializer, ptr %bf8, align 4
  %member_ptr = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %member_ptr9 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_old = load i32, ptr %member_ptr9, align 4
  %15 = and i32 %bf_old, -8
  %16 = or i32 %15, 5
  store i32 %16, ptr %member_ptr9, align 4
  %member_ptr10 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %member_ptr11 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_old12 = load i32, ptr %member_ptr11, align 4
  %17 = and i32 %bf_old12, -249
  %18 = or i32 %17, 168
  store i32 %18, ptr %member_ptr11, align 4
  %member_ptr13 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %member_ptr14 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_old15 = load i32, ptr %member_ptr14, align 4
  %19 = and i32 %bf_old15, -65281
  %20 = or i32 %19, 65280
  store i32 %20, ptr %member_ptr14, align 4
  %member_ptr16 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %member_ptr17 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_old18 = load i32, ptr %member_ptr17, align 4
  %21 = and i32 %bf_old18, 65535
  %22 = or i32 %21, 80871424
  store i32 %22, ptr %member_ptr17, align 4
  %23 = call ptr @moksha_box_string(ptr @11)
  %print_unbox19 = call ptr @moksha_unbox_string(ptr %23)
  %24 = call i32 (ptr, ...) @printf(ptr @12, ptr %print_unbox19)
  %25 = call ptr @moksha_box_string(ptr @13)
  %print_unbox20 = call ptr @moksha_unbox_string(ptr %25)
  %26 = call i32 (ptr, ...) @printf(ptr @14, ptr %print_unbox20)
  %member_ptr21 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_container = load i32, ptr %member_ptr21, align 4
  %27 = lshr i32 %bf_container, 0
  %28 = and i32 %27, 7
  %29 = call i32 (ptr, ...) @printf(ptr @15, i32 %28)
  %30 = call ptr @moksha_box_string(ptr @16)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %30)
  %31 = call i32 (ptr, ...) @printf(ptr @17, ptr %print_unbox22)
  %member_ptr23 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_container24 = load i32, ptr %member_ptr23, align 4
  %32 = lshr i32 %bf_container24, 3
  %33 = and i32 %32, 31
  %34 = call i32 (ptr, ...) @printf(ptr @18, i32 %33)
  %35 = call ptr @moksha_box_string(ptr @19)
  %print_unbox25 = call ptr @moksha_unbox_string(ptr %35)
  %36 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox25)
  %member_ptr26 = getelementptr inbounds nuw %BitfieldTest, ptr %bf8, i32 0, i32 0
  %bf_container27 = load i32, ptr %member_ptr26, align 4
  %37 = lshr i32 %bf_container27, 8
  %38 = and i32 %37, 255
  %39 = call i32 (ptr, ...) @printf(ptr @21, i32 %38)
  store ptr %bf8, ptr %raw, align 8
  store ptr %bf8, ptr %raw28, align 8
  %40 = call ptr @moksha_box_string(ptr @22)
  %print_unbox29 = call ptr @moksha_unbox_string(ptr %40)
  %41 = call i32 (ptr, ...) @printf(ptr @23, ptr %print_unbox29)
  %42 = load ptr, ptr %raw28, align 8
  %43 = load i32, ptr %42, align 4
  %44 = call i32 (ptr, ...) @printf(ptr @24, i32 %43)
  store %HardwarePacket zeroinitializer, ptr %packet30, align 1
  store %HardwarePacket zeroinitializer, ptr %packet31, align 1
  store ptr %packet31, ptr %base_addr, align 8
  store ptr %packet31, ptr %base_addr32, align 8
  %member_ptr33 = getelementptr inbounds nuw %HardwarePacket, ptr %packet31, i32 0, i32 1
  store ptr %member_ptr33, ptr %payload_addr, align 8
  store ptr %member_ptr33, ptr %payload_addr34, align 8
  %45 = load ptr, ptr %payload_addr34, align 8
  %ptr_to_int = ptrtoint ptr %45 to i32
  %46 = load ptr, ptr %base_addr32, align 8
  %ptr_to_int35 = ptrtoint ptr %46 to i32
  %subtmp = sub i32 %ptr_to_int, %ptr_to_int35
  store i32 %subtmp, ptr %offset, align 4
  store i32 %subtmp, ptr %offset36, align 4
  %47 = call ptr @moksha_box_string(ptr @25)
  %print_unbox37 = call ptr @moksha_unbox_string(ptr %47)
  %48 = call i32 (ptr, ...) @printf(ptr @26, ptr %print_unbox37)
  %49 = load i32, ptr %offset36, align 4
  %50 = call i32 (ptr, ...) @printf(ptr @27, i32 %49)
  store %Converter zeroinitializer, ptr %conv38, align 4
  store %Converter zeroinitializer, ptr %conv39, align 4
  store i32 1065353216, ptr %conv39, align 4
  %51 = call ptr @moksha_box_string(ptr @28)
  %print_unbox40 = call ptr @moksha_unbox_string(ptr %51)
  %52 = call i32 (ptr, ...) @printf(ptr @29, ptr %print_unbox40)
  %member_val = load float, ptr %conv39, align 4
  %float_to_double = fpext float %member_val to double
  %53 = call i32 (ptr, ...) @printf(ptr @30, double %float_to_double)
  %call = call ptr @malloc(i64 12)
  store ptr %call, ptr %buffer, align 8
  store ptr %call, ptr %buffer41, align 8
  %54 = load ptr, ptr %buffer41, align 8
  %55 = call ptr @moksha_box_string(ptr @31)
  %auto_unbox_str = call ptr @moksha_unbox_string(ptr %55)
  %call42 = call ptr @strcpy(ptr %54, ptr %auto_unbox_str)
  %56 = load ptr, ptr %buffer41, align 8
  store ptr %56, ptr %p, align 8
  store ptr %56, ptr %p43, align 8
  %ptr_idx = getelementptr i8, ptr %p43, i32 6
  store i8 77, ptr %ptr_idx, align 1
  %ptr_idx44 = getelementptr i8, ptr %p43, i32 7
  store i8 111, ptr %ptr_idx44, align 1
  %ptr_idx45 = getelementptr i8, ptr %p43, i32 8
  store i8 107, ptr %ptr_idx45, align 1
  %ptr_idx46 = getelementptr i8, ptr %p43, i32 9
  store i8 115, ptr %ptr_idx46, align 1
  %ptr_idx47 = getelementptr i8, ptr %p43, i32 10
  store i8 104, ptr %ptr_idx47, align 1
  %57 = call ptr @moksha_box_string(ptr @32)
  %print_unbox48 = call ptr @moksha_unbox_string(ptr %57)
  %58 = call i32 (ptr, ...) @printf(ptr @33, ptr %print_unbox48)
  %59 = load ptr, ptr %buffer41, align 8
  %60 = call i32 (ptr, ...) @printf(ptr @34, ptr %59)
  store %Outer zeroinitializer, ptr %obj49, align 1
  store %Outer zeroinitializer, ptr %obj50, align 1
  store ptr %obj50, ptr %base, align 8
  store ptr %obj50, ptr %base51, align 8
  %member_ptr52 = getelementptr inbounds nuw %Outer, ptr %obj50, i32 0, i32 2
  store ptr %member_ptr52, ptr %footer_ptr, align 8
  store ptr %member_ptr52, ptr %footer_ptr53, align 8
  %61 = load ptr, ptr %footer_ptr53, align 8
  %ptr_to_int54 = ptrtoint ptr %61 to i32
  %62 = load ptr, ptr %base51, align 8
  %ptr_to_int55 = ptrtoint ptr %62 to i32
  %subtmp56 = sub i32 %ptr_to_int54, %ptr_to_int55
  store i32 %subtmp56, ptr %nested_offset, align 4
  store i32 %subtmp56, ptr %nested_offset57, align 4
  %63 = call ptr @moksha_box_string(ptr @35)
  %print_unbox58 = call ptr @moksha_unbox_string(ptr %63)
  %64 = call i32 (ptr, ...) @printf(ptr @36, ptr %print_unbox58)
  %65 = load i32, ptr %nested_offset57, align 4
  %66 = call i32 (ptr, ...) @printf(ptr @37, i32 %65)
  %call59 = call ptr @malloc(i64 12)
  store ptr %call59, ptr %data_ptr, align 8
  store ptr %call59, ptr %data_ptr60, align 8
  %67 = load ptr, ptr %data_ptr60, align 8
  store i32 100, ptr %67, align 4
  %68 = load ptr, ptr %data_ptr60, align 8
  %ptr_math = getelementptr i32, ptr %68, i32 1
  store i32 200, ptr %ptr_math, align 4
  %69 = load ptr, ptr %data_ptr60, align 8
  %ptr_math61 = getelementptr i32, ptr %69, i32 2
  store i32 300, ptr %ptr_math61, align 4
  %70 = call ptr @moksha_box_string(ptr @38)
  %print_unbox62 = call ptr @moksha_unbox_string(ptr %70)
  %71 = call i32 (ptr, ...) @printf(ptr @39, ptr %print_unbox62)
  %72 = load ptr, ptr %data_ptr60, align 8
  %ptr_idx63 = getelementptr i32, ptr %72, i32 1
  %ptr_val = load i32, ptr %ptr_idx63, align 4
  %73 = call i32 (ptr, ...) @printf(ptr @40, i32 %ptr_val)
  %74 = call ptr @moksha_box_string(ptr @41)
  %print_unbox64 = call ptr @moksha_unbox_string(ptr %74)
  %75 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox64)
  %76 = load ptr, ptr %data_ptr60, align 8
  %ptr_math65 = getelementptr i32, ptr %76, i32 2
  %77 = load i32, ptr %ptr_math65, align 4
  %78 = call i32 (ptr, ...) @printf(ptr @43, i32 %77)
  store %Color zeroinitializer, ptr %pixel66, align 4
  store %Color zeroinitializer, ptr %pixel67, align 4
  store i32 287454020, ptr %pixel67, align 4
  %79 = call ptr @moksha_box_string(ptr @44)
  %print_unbox68 = call ptr @moksha_unbox_string(ptr %79)
  %80 = call i32 (ptr, ...) @printf(ptr @45, ptr %print_unbox68)
  %fixed_idx = getelementptr i8, ptr %pixel67, i32 0
  %fixed_val = load i8, ptr %fixed_idx, align 1
  %to_i32 = sext i8 %fixed_val to i32
  %81 = call i32 (ptr, ...) @printf(ptr @46, i32 %to_i32)
  %82 = call ptr @moksha_box_string(ptr @47)
  %print_unbox69 = call ptr @moksha_unbox_string(ptr %82)
  %83 = call i32 (ptr, ...) @printf(ptr @48, ptr %print_unbox69)
  %fixed_idx70 = getelementptr i8, ptr %pixel67, i32 3
  %fixed_val71 = load i8, ptr %fixed_idx70, align 1
  %to_i3272 = sext i8 %fixed_val71 to i32
  %84 = call i32 (ptr, ...) @printf(ptr @49, i32 %to_i3272)
  store i32 10, ptr %a, align 4
  store i32 10, ptr %a73, align 4
  store i32 20, ptr %b, align 4
  store i32 20, ptr %b74, align 4
  %85 = call ptr @moksha_box_string(ptr @50)
  %auto_unbox_str75 = call ptr @moksha_unbox_string(ptr %85)
  %86 = load i32, ptr %a73, align 4
  %87 = load i32, ptr %b74, align 4
  %88 = load i32, ptr %a73, align 4
  %89 = load i32, ptr %b74, align 4
  %addtmp = add i32 %88, %89
  %call76 = call i32 (ptr, ...) @printf(ptr %auto_unbox_str75, i32 %86, i32 %87, i32 %addtmp)
  ret i32 0

unwind:                                           ; preds = %entry
  ret i32 0
}

define x86_intrcc void @dummy_interrupt(ptr byval(i64) %frame) {
entry:
  %frame1 = alloca ptr, align 8
  store ptr %frame, ptr %frame1, align 8
  ret void
}
