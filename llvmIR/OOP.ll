; ModuleID = 'moksha_module'
source_filename = "moksha_module"

%Vector = type { ptr, double, double }
%Manager = type { ptr, i32, ptr }
%ScalarMath = type { ptr, double }
%Entity = type { ptr, ptr }
%Player = type { ptr, ptr, i32 }
%Account = type { ptr, double, ptr, ptr }
%Savings = type { ptr, double, ptr, ptr }
%Bat = type { ptr }
%SportsCar = type { ptr }
%ListNode = type { ptr, i32, ptr }
%Node = type { ptr, i32 }
%Square = type { ptr, double, double }
%Shape = type { ptr, double }

@__exception_flag = global i32 0
@__exception_val = global ptr null
@0 = private unnamed_addr constant [39 x i8] c"==== OOP, MEMORY & OPERATOR SUITE ====\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Vector = constant [4 x ptr] [ptr @"Vector_+", ptr @Vector_-, ptr @"Vector_*", ptr @Vector_printVec]
@2 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@3 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@4 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@5 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@6 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@7 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@8 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@9 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@10 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@11 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@12 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@13 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@14 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@15 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@16 = private unnamed_addr constant [3 x i8] c": \00", align 1
@17 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@18 = private unnamed_addr constant [3 x i8] c", \00", align 1
@19 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@20 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@21 = private unnamed_addr constant [34 x i8] c">> Destructor called for Vector (\00", align 1
@22 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@23 = private unnamed_addr constant [3 x i8] c", \00", align 1
@24 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@25 = private unnamed_addr constant [2 x i8] c")\00", align 1
@26 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@27 = private unnamed_addr constant [36 x i8] c"\0A[1. Normal Class: Value Semantics]\00", align 1
@28 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@29 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@30 = private unnamed_addr constant [3 x i8] c"v1\00", align 1
@31 = private unnamed_addr constant [35 x i8] c"Creating v2 = v1 (Should Clone)...\00", align 1
@32 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@33 = private unnamed_addr constant [27 x i8] c"Modifying v2.x to 999.0...\00", align 1
@34 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@35 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@36 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@37 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@38 = private unnamed_addr constant [26 x i8] c"v1 (Should remain 10, 20)\00", align 1
@39 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@40 = private unnamed_addr constant [23 x i8] c"v2 (Should be 999, 20)\00", align 1
@41 = private unnamed_addr constant [27 x i8] c"\0A[2. Operator Overloading]\00", align 1
@42 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@43 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@44 = private unnamed_addr constant [15 x i8] c"vSum (v1 + v2)\00", align 1
@45 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@46 = private unnamed_addr constant [15 x i8] c"vSub (v1 - v2)\00", align 1
@47 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@48 = private unnamed_addr constant [18 x i8] c"vScale (v1 * 2.5)\00", align 1
@49 = private unnamed_addr constant [37 x i8] c"\0A[3. Ref Class: Reference Semantics]\00", align 1
@50 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@51 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@52 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@53 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@54 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@55 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@56 = private unnamed_addr constant [6 x i8] c"Alice\00", align 1
@57 = private unnamed_addr constant [5 x i8] c"m1: \00", align 1
@58 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@59 = private unnamed_addr constant [3 x i8] c" (\00", align 1
@60 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@61 = private unnamed_addr constant [2 x i8] c")\00", align 1
@62 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@63 = private unnamed_addr constant [32 x i8] c"Creating m2 = m1 (Ref Class)...\00", align 1
@64 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@65 = private unnamed_addr constant [30 x i8] c"Modifying m2.name to 'Bob'...\00", align 1
@66 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@67 = private unnamed_addr constant [4 x i8] c"Bob\00", align 1
@68 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@69 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@70 = private unnamed_addr constant [28 x i8] c"m1 Name (Should be 'Bob'): \00", align 1
@71 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@72 = private unnamed_addr constant [13 x i8] c" , m2 Name: \00", align 1
@73 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@74 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@75 = private unnamed_addr constant [21 x i8] c"\0A[4. Shared Keyword]\00", align 1
@76 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@77 = private unnamed_addr constant [31 x i8] c"Creating origin Vector(0,0)...\00", align 1
@78 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@79 = private unnamed_addr constant [49 x i8] c"Creating 'shared Vector refToOrigin = origin'...\00", align 1
@80 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@81 = private unnamed_addr constant [35 x i8] c"Modifying refToOrigin.x to 55.5...\00", align 1
@82 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@83 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@84 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@85 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@86 = private unnamed_addr constant [29 x i8] c"Origin (Should be 55.5, 0.0)\00", align 1
@87 = private unnamed_addr constant [17 x i8] c"\0A[5. Destructor]\00", align 1
@88 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@89 = private unnamed_addr constant [17 x i8] c"Deleting vSum...\00", align 1
@90 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@91 = private unnamed_addr constant [14 x i8] c"vSum deleted.\00", align 1
@92 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@93 = private unnamed_addr constant [34 x i8] c"\0A[6. Operator Argument Promotion]\00", align 1
@94 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_ScalarMath = constant [1 x ptr] [ptr @"ScalarMath_*"]
@95 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@96 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@97 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@98 = private unnamed_addr constant [35 x i8] c"9.3 * 2 (Int promoted to Double): \00", align 1
@99 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@100 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@101 = private unnamed_addr constant [31 x i8] c"\0A[7. Inheritance & Overriding]\00", align 1
@102 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Entity = constant [1 x ptr] [ptr @Entity_describe]
@103 = private unnamed_addr constant [12 x i8] c"Entity ID: \00", align 1
@104 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@105 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@106 = private unnamed_addr constant [8 x i8] c"GEN-000\00", align 1
@VTable_Player = constant [1 x ptr] [ptr @Player_describe]
@107 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@108 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@109 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@110 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@111 = private unnamed_addr constant [8 x i8] c"Player \00", align 1
@112 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@113 = private unnamed_addr constant [11 x i8] c" is Level \00", align 1
@114 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@115 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@116 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@117 = private unnamed_addr constant [6 x i8] c"P-101\00", align 1
@118 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@119 = private unnamed_addr constant [51 x i8] c"\0A[8. Access Specifiers (Public/Private/Protected)]\00", align 1
@120 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Account = constant [3 x ptr] [ptr @Account_getBalance, ptr @Account_setBalance, ptr @Account_printInfo]
@121 = private unnamed_addr constant [9 x i8] c"Checking\00", align 1
@122 = private unnamed_addr constant [10 x i8] c"Anonymous\00", align 1
@123 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@124 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@125 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@126 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@127 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@128 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@129 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@130 = private unnamed_addr constant [37 x i8] c"Error: Negative balance not allowed!\00", align 1
@131 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@132 = private unnamed_addr constant [10 x i8] c"Account: \00", align 1
@133 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@134 = private unnamed_addr constant [10 x i8] c" | Type: \00", align 1
@135 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@136 = private unnamed_addr constant [9 x i8] c" | Bal: \00", align 1
@137 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@138 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@139 = private unnamed_addr constant [6 x i8] c"Alice\00", align 1
@140 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@141 = private unnamed_addr constant [39 x i8] c"Attempting illegal setBalance(-100)...\00", align 1
@142 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@143 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@144 = private unnamed_addr constant [34 x i8] c"Current Balance (Should be 500): \00", align 1
@145 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@146 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@147 = private unnamed_addr constant [32 x i8] c"Setting valid balance 1000.0...\00", align 1
@148 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@149 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@150 = private unnamed_addr constant [14 x i8] c"New Balance: \00", align 1
@151 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@152 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Savings = constant [3 x ptr] [ptr @Account_getBalance, ptr @Account_setBalance, ptr @Account_printInfo]
@153 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@154 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@155 = private unnamed_addr constant [8 x i8] c"Savings\00", align 1
@156 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@157 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@158 = private unnamed_addr constant [4 x i8] c"Bob\00", align 1
@159 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@160 = private unnamed_addr constant [27 x i8] c"\0A[9. Multiple Inheritance]\00", align 1
@161 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Mamal = constant [1 x ptr] [ptr @Mamal_breathe]
@162 = private unnamed_addr constant [22 x i8] c"Mamal is breathing...\00", align 1
@163 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Winged = constant [1 x ptr] [ptr @Winged_fly]
@164 = private unnamed_addr constant [29 x i8] c"Winged creature is flying...\00", align 1
@165 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Bat = constant [3 x ptr] [ptr @Mamal_breathe, ptr @Winged_fly, ptr @Bat_screech]
@166 = private unnamed_addr constant [15 x i8] c"Bat screeches!\00", align 1
@167 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@168 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@169 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@170 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@171 = private unnamed_addr constant [30 x i8] c"\0A[10. Multilevel Inheritance]\00", align 1
@172 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Vehicle = constant [1 x ptr] [ptr @Vehicle_start]
@173 = private unnamed_addr constant [17 x i8] c"Vehicle started.\00", align 1
@174 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_Car = constant [2 x ptr] [ptr @Vehicle_start, ptr @Car_honk]
@175 = private unnamed_addr constant [11 x i8] c"Car honks.\00", align 1
@176 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@VTable_SportsCar = constant [3 x ptr] [ptr @Vehicle_start, ptr @Car_honk, ptr @SportsCar_turbo]
@177 = private unnamed_addr constant [21 x i8] c"Turbo boost engaged!\00", align 1
@178 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@179 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@180 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@181 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@182 = private unnamed_addr constant [43 x i8] c"\0A[11. Inheritance with Ref Class & Shared]\00", align 1
@183 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@184 = private unnamed_addr constant [48 x i8] c"Creating Linked List (Ref Class Inheritance)...\00", align 1
@185 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@186 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@187 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@188 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@189 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@190 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@191 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@192 = private unnamed_addr constant [7 x i8] c"Head: \00", align 1
@193 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@194 = private unnamed_addr constant [9 x i8] c", Next: \00", align 1
@195 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@196 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@197 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@198 = private unnamed_addr constant [34 x i8] c"Modifying head.next.data to 99...\00", align 1
@199 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@200 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@201 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@202 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@203 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@204 = private unnamed_addr constant [34 x i8] c"Second Node Data (Should be 99): \00", align 1
@205 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@206 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@207 = private unnamed_addr constant [35 x i8] c"\0A[Shared Keyword on Derived Class]\00", align 1
@208 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@209 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@210 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@211 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@212 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@213 = private unnamed_addr constant [40 x i8] c"Creating 'shared Square sqRef = sq1'...\00", align 1
@214 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@215 = private unnamed_addr constant [31 x i8] c"Creating 'Square sq2 = sq1'...\00", align 1
@216 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@217 = private unnamed_addr constant [33 x i8] c"Modifying sqRef.area to 100.0...\00", align 1
@218 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@219 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@220 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@221 = private unnamed_addr constant [36 x i8] c"Original sq1 Area (Should be 100): \00", align 1
@222 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@223 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@224 = private unnamed_addr constant [35 x i8] c"Original sq2 Area (Should be 25): \00", align 1
@225 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@226 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@227 = private unnamed_addr constant [36 x i8] c"Shared sqRef Area (Should be 100): \00", align 1
@228 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@229 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@230 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@231 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@232 = private unnamed_addr constant [52 x i8] c"Shared sqRef Area after Alias mod (Should be 200): \00", align 1
@233 = private unnamed_addr constant [56 x i8] c"NullReferenceException: Attempted access on null object\00", align 1
@234 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@235 = private unnamed_addr constant [24 x i8] c"=== SUITE COMPLETED ===\00", align 1
@236 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

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
  %aliasRef337 = alloca ptr, align 8
  %aliasRef = alloca ptr, align 8
  %sq2313 = alloca ptr, align 8
  %sq2 = alloca ptr, align 8
  %sqRef305 = alloca ptr, align 8
  %sqRef = alloca ptr, align 8
  %sq1291 = alloca ptr, align 8
  %sq1 = alloca ptr, align 8
  %second244 = alloca ptr, align 8
  %second = alloca ptr, align 8
  %head236 = alloca ptr, align 8
  %head = alloca ptr, align 8
  %ferrari220 = alloca ptr, align 8
  %ferrari = alloca ptr, align 8
  %b200 = alloca ptr, align 8
  %b = alloca ptr, align 8
  %save188 = alloca ptr, align 8
  %save = alloca ptr, align 8
  %acc156 = alloca ptr, align 8
  %acc = alloca ptr, align 8
  %p144 = alloca ptr, align 8
  %p = alloca ptr, align 8
  %e133 = alloca ptr, align 8
  %e = alloca ptr, align 8
  %s2121 = alloca ptr, align 8
  %s2 = alloca ptr, align 8
  %s114 = alloca ptr, align 8
  %s = alloca ptr, align 8
  %refToOrigin91 = alloca ptr, align 8
  %refToOrigin = alloca ptr, align 8
  %origin89 = alloca ptr, align 8
  %origin = alloca ptr, align 8
  %m266 = alloca ptr, align 8
  %m2 = alloca ptr, align 8
  %m157 = alloca ptr, align 8
  %m1 = alloca ptr, align 8
  %vScale50 = alloca ptr, align 8
  %vScale = alloca ptr, align 8
  %vSub39 = alloca ptr, align 8
  %vSub = alloca ptr, align 8
  %vSum28 = alloca ptr, align 8
  %vSum = alloca ptr, align 8
  %v29 = alloca ptr, align 8
  %v2 = alloca ptr, align 8
  %v12 = alloca ptr, align 8
  %v1 = alloca ptr, align 8
  %0 = call ptr @moksha_box_string(ptr @0)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr %print_unbox)
  %2 = call ptr @moksha_box_string(ptr @27)
  %print_unbox1 = call ptr @moksha_unbox_string(ptr %2)
  %3 = call i32 (ptr, ...) @printf(ptr @28, ptr %print_unbox1)
  %alloc_obj = call ptr @calloc(i64 1, i64 24)
  %4 = getelementptr inbounds nuw %Vector, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Vector, ptr %4, align 8
  call void @Vector_constructor(ptr %alloc_obj, double 1.000000e+01, double 2.000000e+01)
  %5 = icmp eq ptr %alloc_obj, null
  br i1 %5, label %clone_skip, label %clone_do

clone_do:                                         ; preds = %entry
  %clone_alloc = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc, ptr align 8 %alloc_obj, i64 24, i1 false)
  br label %clone_merge

clone_skip:                                       ; preds = %entry
  br label %clone_merge

clone_merge:                                      ; preds = %clone_skip, %clone_do
  %clone_res = phi ptr [ %clone_alloc, %clone_do ], [ null, %clone_skip ]
  store ptr %clone_res, ptr %v1, align 8
  store ptr %clone_res, ptr %v12, align 8
  %6 = load ptr, ptr %v12, align 8
  %7 = icmp eq ptr %6, null
  br i1 %7, label %call_error, label %call_safe

call_error:                                       ; preds = %clone_merge
  %8 = call ptr @moksha_box_string(ptr @29)
  store ptr %8, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe:                                        ; preds = %clone_merge
  %9 = call ptr @moksha_box_string(ptr @30)
  %10 = getelementptr inbounds nuw %Vector, ptr %6, i32 0, i32 0
  %vptr = load ptr, ptr %10, align 8
  %11 = getelementptr inbounds ptr, ptr %vptr, i32 3
  %func_ptr = load ptr, ptr %11, align 8
  call void %func_ptr(ptr %6, ptr %9)
  %12 = call ptr @moksha_box_string(ptr @31)
  %print_unbox3 = call ptr @moksha_unbox_string(ptr %12)
  %13 = call i32 (ptr, ...) @printf(ptr @32, ptr %print_unbox3)
  %14 = load ptr, ptr %v12, align 8
  %15 = icmp eq ptr %14, null
  br i1 %15, label %clone_skip5, label %clone_do4

clone_do4:                                        ; preds = %call_safe
  %clone_alloc7 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc7, ptr align 8 %14, i64 24, i1 false)
  br label %clone_merge6

clone_skip5:                                      ; preds = %call_safe
  br label %clone_merge6

clone_merge6:                                     ; preds = %clone_skip5, %clone_do4
  %clone_res8 = phi ptr [ %clone_alloc7, %clone_do4 ], [ null, %clone_skip5 ]
  store ptr %clone_res8, ptr %v2, align 8
  store ptr %clone_res8, ptr %v29, align 8
  %16 = call ptr @moksha_box_string(ptr @33)
  %print_unbox10 = call ptr @moksha_unbox_string(ptr %16)
  %17 = call i32 (ptr, ...) @printf(ptr @34, ptr %print_unbox10)
  %loaded_obj_ptr = load ptr, ptr %v29, align 8
  %18 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %18, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %clone_merge6
  %19 = call ptr @moksha_box_string(ptr @35)
  store ptr %19, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe:                                         ; preds = %clone_merge6
  %20 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr, i32 0, i32 1
  %loaded_obj_ptr11 = load ptr, ptr %v29, align 8
  %21 = icmp eq ptr %loaded_obj_ptr11, null
  br i1 %21, label %mem_error12, label %mem_safe13

mem_error12:                                      ; preds = %mem_safe
  %22 = call ptr @moksha_box_string(ptr @36)
  store ptr %22, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe13:                                       ; preds = %mem_safe
  %23 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr11, i32 0, i32 1
  store double 9.990000e+02, ptr %20, align 8
  %24 = load ptr, ptr %v12, align 8
  %25 = icmp eq ptr %24, null
  br i1 %25, label %call_error14, label %call_safe15

call_error14:                                     ; preds = %mem_safe13
  %26 = call ptr @moksha_box_string(ptr @37)
  store ptr %26, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe15:                                      ; preds = %mem_safe13
  %27 = call ptr @moksha_box_string(ptr @38)
  %28 = getelementptr inbounds nuw %Vector, ptr %24, i32 0, i32 0
  %vptr16 = load ptr, ptr %28, align 8
  %29 = getelementptr inbounds ptr, ptr %vptr16, i32 3
  %func_ptr17 = load ptr, ptr %29, align 8
  call void %func_ptr17(ptr %24, ptr %27)
  %30 = load ptr, ptr %v29, align 8
  %31 = icmp eq ptr %30, null
  br i1 %31, label %call_error18, label %call_safe19

call_error18:                                     ; preds = %call_safe15
  %32 = call ptr @moksha_box_string(ptr @39)
  store ptr %32, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe19:                                      ; preds = %call_safe15
  %33 = call ptr @moksha_box_string(ptr @40)
  %34 = getelementptr inbounds nuw %Vector, ptr %30, i32 0, i32 0
  %vptr20 = load ptr, ptr %34, align 8
  %35 = getelementptr inbounds ptr, ptr %vptr20, i32 3
  %func_ptr21 = load ptr, ptr %35, align 8
  call void %func_ptr21(ptr %30, ptr %33)
  %36 = call ptr @moksha_box_string(ptr @41)
  %print_unbox22 = call ptr @moksha_unbox_string(ptr %36)
  %37 = call i32 (ptr, ...) @printf(ptr @42, ptr %print_unbox22)
  %38 = load ptr, ptr %v12, align 8
  %39 = load ptr, ptr %v29, align 8
  %op_call = call ptr @"Vector_+"(ptr %38, ptr %39)
  %40 = icmp eq ptr %op_call, null
  br i1 %40, label %clone_skip24, label %clone_do23

clone_do23:                                       ; preds = %call_safe19
  %clone_alloc26 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc26, ptr align 8 %op_call, i64 24, i1 false)
  br label %clone_merge25

clone_skip24:                                     ; preds = %call_safe19
  br label %clone_merge25

clone_merge25:                                    ; preds = %clone_skip24, %clone_do23
  %clone_res27 = phi ptr [ %clone_alloc26, %clone_do23 ], [ null, %clone_skip24 ]
  store ptr %clone_res27, ptr %vSum, align 8
  store ptr %clone_res27, ptr %vSum28, align 8
  %41 = load ptr, ptr %vSum28, align 8
  %42 = icmp eq ptr %41, null
  br i1 %42, label %call_error29, label %call_safe30

call_error29:                                     ; preds = %clone_merge25
  %43 = call ptr @moksha_box_string(ptr @43)
  store ptr %43, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe30:                                      ; preds = %clone_merge25
  %44 = call ptr @moksha_box_string(ptr @44)
  %45 = getelementptr inbounds nuw %Vector, ptr %41, i32 0, i32 0
  %vptr31 = load ptr, ptr %45, align 8
  %46 = getelementptr inbounds ptr, ptr %vptr31, i32 3
  %func_ptr32 = load ptr, ptr %46, align 8
  call void %func_ptr32(ptr %41, ptr %44)
  %47 = load ptr, ptr %v12, align 8
  %48 = load ptr, ptr %v29, align 8
  %op_call33 = call ptr @Vector_-(ptr %47, ptr %48)
  %49 = icmp eq ptr %op_call33, null
  br i1 %49, label %clone_skip35, label %clone_do34

clone_do34:                                       ; preds = %call_safe30
  %clone_alloc37 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc37, ptr align 8 %op_call33, i64 24, i1 false)
  br label %clone_merge36

clone_skip35:                                     ; preds = %call_safe30
  br label %clone_merge36

clone_merge36:                                    ; preds = %clone_skip35, %clone_do34
  %clone_res38 = phi ptr [ %clone_alloc37, %clone_do34 ], [ null, %clone_skip35 ]
  store ptr %clone_res38, ptr %vSub, align 8
  store ptr %clone_res38, ptr %vSub39, align 8
  %50 = load ptr, ptr %vSub39, align 8
  %51 = icmp eq ptr %50, null
  br i1 %51, label %call_error40, label %call_safe41

call_error40:                                     ; preds = %clone_merge36
  %52 = call ptr @moksha_box_string(ptr @45)
  store ptr %52, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe41:                                      ; preds = %clone_merge36
  %53 = call ptr @moksha_box_string(ptr @46)
  %54 = getelementptr inbounds nuw %Vector, ptr %50, i32 0, i32 0
  %vptr42 = load ptr, ptr %54, align 8
  %55 = getelementptr inbounds ptr, ptr %vptr42, i32 3
  %func_ptr43 = load ptr, ptr %55, align 8
  call void %func_ptr43(ptr %50, ptr %53)
  %56 = load ptr, ptr %v12, align 8
  %op_call44 = call ptr @"Vector_*"(ptr %56, double 2.500000e+00)
  %57 = icmp eq ptr %op_call44, null
  br i1 %57, label %clone_skip46, label %clone_do45

clone_do45:                                       ; preds = %call_safe41
  %clone_alloc48 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc48, ptr align 8 %op_call44, i64 24, i1 false)
  br label %clone_merge47

clone_skip46:                                     ; preds = %call_safe41
  br label %clone_merge47

clone_merge47:                                    ; preds = %clone_skip46, %clone_do45
  %clone_res49 = phi ptr [ %clone_alloc48, %clone_do45 ], [ null, %clone_skip46 ]
  store ptr %clone_res49, ptr %vScale, align 8
  store ptr %clone_res49, ptr %vScale50, align 8
  %58 = load ptr, ptr %vScale50, align 8
  %59 = icmp eq ptr %58, null
  br i1 %59, label %call_error51, label %call_safe52

call_error51:                                     ; preds = %clone_merge47
  %60 = call ptr @moksha_box_string(ptr @47)
  store ptr %60, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe52:                                      ; preds = %clone_merge47
  %61 = call ptr @moksha_box_string(ptr @48)
  %62 = getelementptr inbounds nuw %Vector, ptr %58, i32 0, i32 0
  %vptr53 = load ptr, ptr %62, align 8
  %63 = getelementptr inbounds ptr, ptr %vptr53, i32 3
  %func_ptr54 = load ptr, ptr %63, align 8
  call void %func_ptr54(ptr %58, ptr %61)
  %64 = call ptr @moksha_box_string(ptr @49)
  %print_unbox55 = call ptr @moksha_unbox_string(ptr %64)
  %65 = call i32 (ptr, ...) @printf(ptr @50, ptr %print_unbox55)
  %alloc_obj56 = call ptr @calloc(i64 1, i64 24)
  %66 = call ptr @moksha_box_string(ptr @56)
  call void @Manager_constructor(ptr %alloc_obj56, i32 101, ptr %66)
  store ptr %alloc_obj56, ptr %m1, align 8
  store ptr %alloc_obj56, ptr %m157, align 8
  %67 = call ptr @moksha_box_string(ptr @57)
  %loaded_obj_ptr58 = load ptr, ptr %m157, align 8
  %68 = icmp eq ptr %loaded_obj_ptr58, null
  br i1 %68, label %mem_error59, label %mem_safe60

mem_error59:                                      ; preds = %call_safe52
  %69 = call ptr @moksha_box_string(ptr @58)
  store ptr %69, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe60:                                       ; preds = %call_safe52
  %70 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr58, i32 0, i32 2
  %71 = load ptr, ptr %70, align 8
  %72 = call ptr @moksha_string_concat(ptr %67, ptr %71)
  %73 = call ptr @moksha_box_string(ptr @59)
  %74 = call ptr @moksha_string_concat(ptr %72, ptr %73)
  %loaded_obj_ptr61 = load ptr, ptr %m157, align 8
  %75 = icmp eq ptr %loaded_obj_ptr61, null
  br i1 %75, label %mem_error62, label %mem_safe63

mem_error62:                                      ; preds = %mem_safe60
  %76 = call ptr @moksha_box_string(ptr @60)
  store ptr %76, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe63:                                       ; preds = %mem_safe60
  %77 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr61, i32 0, i32 1
  %78 = load i32, ptr %77, align 4
  %79 = call ptr @moksha_int_to_str(i32 %78)
  %80 = call ptr @moksha_box_string(ptr %79)
  %81 = call ptr @moksha_string_concat(ptr %74, ptr %80)
  %82 = call ptr @moksha_box_string(ptr @61)
  %83 = call ptr @moksha_string_concat(ptr %81, ptr %82)
  %print_unbox64 = call ptr @moksha_unbox_string(ptr %83)
  %84 = call i32 (ptr, ...) @printf(ptr @62, ptr %print_unbox64)
  %85 = call ptr @moksha_box_string(ptr @63)
  %print_unbox65 = call ptr @moksha_unbox_string(ptr %85)
  %86 = call i32 (ptr, ...) @printf(ptr @64, ptr %print_unbox65)
  %87 = load ptr, ptr %m157, align 8
  store ptr %87, ptr %m2, align 8
  store ptr %87, ptr %m266, align 8
  %88 = call ptr @moksha_box_string(ptr @65)
  %print_unbox67 = call ptr @moksha_unbox_string(ptr %88)
  %89 = call i32 (ptr, ...) @printf(ptr @66, ptr %print_unbox67)
  %90 = call ptr @moksha_box_string(ptr @67)
  %loaded_obj_ptr68 = load ptr, ptr %m266, align 8
  %91 = icmp eq ptr %loaded_obj_ptr68, null
  br i1 %91, label %mem_error69, label %mem_safe70

mem_error69:                                      ; preds = %mem_safe63
  %92 = call ptr @moksha_box_string(ptr @68)
  store ptr %92, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe70:                                       ; preds = %mem_safe63
  %93 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr68, i32 0, i32 2
  %loaded_obj_ptr71 = load ptr, ptr %m266, align 8
  %94 = icmp eq ptr %loaded_obj_ptr71, null
  br i1 %94, label %mem_error72, label %mem_safe73

mem_error72:                                      ; preds = %mem_safe70
  %95 = call ptr @moksha_box_string(ptr @69)
  store ptr %95, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe73:                                       ; preds = %mem_safe70
  %96 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr71, i32 0, i32 2
  store ptr %90, ptr %93, align 8
  %97 = call ptr @moksha_box_string(ptr @70)
  %loaded_obj_ptr74 = load ptr, ptr %m157, align 8
  %98 = icmp eq ptr %loaded_obj_ptr74, null
  br i1 %98, label %mem_error75, label %mem_safe76

mem_error75:                                      ; preds = %mem_safe73
  %99 = call ptr @moksha_box_string(ptr @71)
  store ptr %99, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe76:                                       ; preds = %mem_safe73
  %100 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr74, i32 0, i32 2
  %101 = load ptr, ptr %100, align 8
  %102 = call ptr @moksha_string_concat(ptr %97, ptr %101)
  %103 = call ptr @moksha_box_string(ptr @72)
  %104 = call ptr @moksha_string_concat(ptr %102, ptr %103)
  %loaded_obj_ptr77 = load ptr, ptr %m266, align 8
  %105 = icmp eq ptr %loaded_obj_ptr77, null
  br i1 %105, label %mem_error78, label %mem_safe79

mem_error78:                                      ; preds = %mem_safe76
  %106 = call ptr @moksha_box_string(ptr @73)
  store ptr %106, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe79:                                       ; preds = %mem_safe76
  %107 = getelementptr inbounds nuw %Manager, ptr %loaded_obj_ptr77, i32 0, i32 2
  %108 = load ptr, ptr %107, align 8
  %109 = call ptr @moksha_string_concat(ptr %104, ptr %108)
  %print_unbox80 = call ptr @moksha_unbox_string(ptr %109)
  %110 = call i32 (ptr, ...) @printf(ptr @74, ptr %print_unbox80)
  %111 = call ptr @moksha_box_string(ptr @75)
  %print_unbox81 = call ptr @moksha_unbox_string(ptr %111)
  %112 = call i32 (ptr, ...) @printf(ptr @76, ptr %print_unbox81)
  %113 = call ptr @moksha_box_string(ptr @77)
  %print_unbox82 = call ptr @moksha_unbox_string(ptr %113)
  %114 = call i32 (ptr, ...) @printf(ptr @78, ptr %print_unbox82)
  %alloc_obj83 = call ptr @calloc(i64 1, i64 24)
  %115 = getelementptr inbounds nuw %Vector, ptr %alloc_obj83, i32 0, i32 0
  store ptr @VTable_Vector, ptr %115, align 8
  call void @Vector_constructor(ptr %alloc_obj83, double 0.000000e+00, double 0.000000e+00)
  %116 = icmp eq ptr %alloc_obj83, null
  br i1 %116, label %clone_skip85, label %clone_do84

clone_do84:                                       ; preds = %mem_safe79
  %clone_alloc87 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc87, ptr align 8 %alloc_obj83, i64 24, i1 false)
  br label %clone_merge86

clone_skip85:                                     ; preds = %mem_safe79
  br label %clone_merge86

clone_merge86:                                    ; preds = %clone_skip85, %clone_do84
  %clone_res88 = phi ptr [ %clone_alloc87, %clone_do84 ], [ null, %clone_skip85 ]
  store ptr %clone_res88, ptr %origin, align 8
  store ptr %clone_res88, ptr %origin89, align 8
  %117 = call ptr @moksha_box_string(ptr @79)
  %print_unbox90 = call ptr @moksha_unbox_string(ptr %117)
  %118 = call i32 (ptr, ...) @printf(ptr @80, ptr %print_unbox90)
  %119 = load ptr, ptr %origin89, align 8
  store ptr %119, ptr %refToOrigin, align 8
  %heap_var = call ptr @malloc(i64 8)
  store ptr %heap_var, ptr %refToOrigin91, align 8
  store ptr %119, ptr %heap_var, align 8
  %120 = call ptr @moksha_box_string(ptr @81)
  %print_unbox92 = call ptr @moksha_unbox_string(ptr %120)
  %121 = call i32 (ptr, ...) @printf(ptr @82, ptr %print_unbox92)
  %heap_addr = load ptr, ptr %refToOrigin91, align 8
  %loaded_obj_ptr93 = load ptr, ptr %heap_addr, align 8
  %122 = icmp eq ptr %loaded_obj_ptr93, null
  br i1 %122, label %mem_error94, label %mem_safe95

mem_error94:                                      ; preds = %clone_merge86
  %123 = call ptr @moksha_box_string(ptr @83)
  store ptr %123, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe95:                                       ; preds = %clone_merge86
  %124 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr93, i32 0, i32 1
  %heap_addr96 = load ptr, ptr %refToOrigin91, align 8
  %loaded_obj_ptr97 = load ptr, ptr %heap_addr96, align 8
  %125 = icmp eq ptr %loaded_obj_ptr97, null
  br i1 %125, label %mem_error98, label %mem_safe99

mem_error98:                                      ; preds = %mem_safe95
  %126 = call ptr @moksha_box_string(ptr @84)
  store ptr %126, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe99:                                       ; preds = %mem_safe95
  %127 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr97, i32 0, i32 1
  store double 5.550000e+01, ptr %124, align 8
  %128 = load ptr, ptr %origin89, align 8
  %129 = icmp eq ptr %128, null
  br i1 %129, label %call_error100, label %call_safe101

call_error100:                                    ; preds = %mem_safe99
  %130 = call ptr @moksha_box_string(ptr @85)
  store ptr %130, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe101:                                     ; preds = %mem_safe99
  %131 = call ptr @moksha_box_string(ptr @86)
  %132 = getelementptr inbounds nuw %Vector, ptr %128, i32 0, i32 0
  %vptr102 = load ptr, ptr %132, align 8
  %133 = getelementptr inbounds ptr, ptr %vptr102, i32 3
  %func_ptr103 = load ptr, ptr %133, align 8
  call void %func_ptr103(ptr %128, ptr %131)
  %134 = call ptr @moksha_box_string(ptr @87)
  %print_unbox104 = call ptr @moksha_unbox_string(ptr %134)
  %135 = call i32 (ptr, ...) @printf(ptr @88, ptr %print_unbox104)
  %136 = call ptr @moksha_box_string(ptr @89)
  %print_unbox105 = call ptr @moksha_unbox_string(ptr %136)
  %137 = call i32 (ptr, ...) @printf(ptr @90, ptr %print_unbox105)
  %138 = load ptr, ptr %vSum28, align 8
  call void @Vector_destructor(ptr %138)
  call void @moksha_free(ptr %138)
  %139 = call ptr @moksha_box_string(ptr @91)
  %print_unbox106 = call ptr @moksha_unbox_string(ptr %139)
  %140 = call i32 (ptr, ...) @printf(ptr @92, ptr %print_unbox106)
  %141 = call ptr @moksha_box_string(ptr @93)
  %print_unbox107 = call ptr @moksha_unbox_string(ptr %141)
  %142 = call i32 (ptr, ...) @printf(ptr @94, ptr %print_unbox107)
  %alloc_obj108 = call ptr @calloc(i64 1, i64 16)
  %143 = getelementptr inbounds nuw %ScalarMath, ptr %alloc_obj108, i32 0, i32 0
  store ptr @VTable_ScalarMath, ptr %143, align 8
  call void @ScalarMath_constructor(ptr %alloc_obj108, double 9.300000e+00)
  %144 = icmp eq ptr %alloc_obj108, null
  br i1 %144, label %clone_skip110, label %clone_do109

clone_do109:                                      ; preds = %call_safe101
  %clone_alloc112 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc112, ptr align 8 %alloc_obj108, i64 16, i1 false)
  br label %clone_merge111

clone_skip110:                                    ; preds = %call_safe101
  br label %clone_merge111

clone_merge111:                                   ; preds = %clone_skip110, %clone_do109
  %clone_res113 = phi ptr [ %clone_alloc112, %clone_do109 ], [ null, %clone_skip110 ]
  store ptr %clone_res113, ptr %s, align 8
  store ptr %clone_res113, ptr %s114, align 8
  %145 = load ptr, ptr %s114, align 8
  %op_call115 = call ptr @"ScalarMath_*"(ptr %145, double 2.000000e+00)
  %146 = icmp eq ptr %op_call115, null
  br i1 %146, label %clone_skip117, label %clone_do116

clone_do116:                                      ; preds = %clone_merge111
  %clone_alloc119 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc119, ptr align 8 %op_call115, i64 16, i1 false)
  br label %clone_merge118

clone_skip117:                                    ; preds = %clone_merge111
  br label %clone_merge118

clone_merge118:                                   ; preds = %clone_skip117, %clone_do116
  %clone_res120 = phi ptr [ %clone_alloc119, %clone_do116 ], [ null, %clone_skip117 ]
  store ptr %clone_res120, ptr %s2, align 8
  store ptr %clone_res120, ptr %s2121, align 8
  %147 = call ptr @moksha_box_string(ptr @98)
  %loaded_obj_ptr122 = load ptr, ptr %s2121, align 8
  %148 = icmp eq ptr %loaded_obj_ptr122, null
  br i1 %148, label %mem_error123, label %mem_safe124

mem_error123:                                     ; preds = %clone_merge118
  %149 = call ptr @moksha_box_string(ptr @99)
  store ptr %149, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe124:                                      ; preds = %clone_merge118
  %150 = getelementptr inbounds nuw %ScalarMath, ptr %loaded_obj_ptr122, i32 0, i32 1
  %151 = load double, ptr %150, align 8
  %152 = call ptr @moksha_double_to_str(double %151)
  %153 = call ptr @moksha_box_string(ptr %152)
  %154 = call ptr @moksha_string_concat(ptr %147, ptr %153)
  %print_unbox125 = call ptr @moksha_unbox_string(ptr %154)
  %155 = call i32 (ptr, ...) @printf(ptr @100, ptr %print_unbox125)
  %156 = call ptr @moksha_box_string(ptr @101)
  %print_unbox126 = call ptr @moksha_unbox_string(ptr %156)
  %157 = call i32 (ptr, ...) @printf(ptr @102, ptr %print_unbox126)
  %alloc_obj127 = call ptr @calloc(i64 1, i64 16)
  %158 = getelementptr inbounds nuw %Entity, ptr %alloc_obj127, i32 0, i32 0
  store ptr @VTable_Entity, ptr %158, align 8
  call void @Entity_constructor(ptr %alloc_obj127)
  %159 = icmp eq ptr %alloc_obj127, null
  br i1 %159, label %clone_skip129, label %clone_do128

clone_do128:                                      ; preds = %mem_safe124
  %clone_alloc131 = call ptr @malloc(i64 16)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc131, ptr align 8 %alloc_obj127, i64 16, i1 false)
  br label %clone_merge130

clone_skip129:                                    ; preds = %mem_safe124
  br label %clone_merge130

clone_merge130:                                   ; preds = %clone_skip129, %clone_do128
  %clone_res132 = phi ptr [ %clone_alloc131, %clone_do128 ], [ null, %clone_skip129 ]
  store ptr %clone_res132, ptr %e, align 8
  store ptr %clone_res132, ptr %e133, align 8
  %160 = load ptr, ptr %e133, align 8
  %161 = icmp eq ptr %160, null
  br i1 %161, label %call_error134, label %call_safe135

call_error134:                                    ; preds = %clone_merge130
  %162 = call ptr @moksha_box_string(ptr @116)
  store ptr %162, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe135:                                     ; preds = %clone_merge130
  %163 = getelementptr inbounds nuw %Entity, ptr %160, i32 0, i32 0
  %vptr136 = load ptr, ptr %163, align 8
  %164 = getelementptr inbounds ptr, ptr %vptr136, i32 0
  %func_ptr137 = load ptr, ptr %164, align 8
  call void %func_ptr137(ptr %160)
  %alloc_obj138 = call ptr @calloc(i64 1, i64 24)
  %165 = getelementptr inbounds nuw %Player, ptr %alloc_obj138, i32 0, i32 0
  store ptr @VTable_Player, ptr %165, align 8
  %166 = call ptr @moksha_box_string(ptr @117)
  call void @Player_constructor(ptr %alloc_obj138, ptr %166, i32 5)
  %167 = icmp eq ptr %alloc_obj138, null
  br i1 %167, label %clone_skip140, label %clone_do139

clone_do139:                                      ; preds = %call_safe135
  %clone_alloc142 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc142, ptr align 8 %alloc_obj138, i64 24, i1 false)
  br label %clone_merge141

clone_skip140:                                    ; preds = %call_safe135
  br label %clone_merge141

clone_merge141:                                   ; preds = %clone_skip140, %clone_do139
  %clone_res143 = phi ptr [ %clone_alloc142, %clone_do139 ], [ null, %clone_skip140 ]
  store ptr %clone_res143, ptr %p, align 8
  store ptr %clone_res143, ptr %p144, align 8
  %168 = load ptr, ptr %p144, align 8
  %169 = icmp eq ptr %168, null
  br i1 %169, label %call_error145, label %call_safe146

call_error145:                                    ; preds = %clone_merge141
  %170 = call ptr @moksha_box_string(ptr @118)
  store ptr %170, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe146:                                     ; preds = %clone_merge141
  %171 = getelementptr inbounds nuw %Player, ptr %168, i32 0, i32 0
  %vptr147 = load ptr, ptr %171, align 8
  %172 = getelementptr inbounds ptr, ptr %vptr147, i32 0
  %func_ptr148 = load ptr, ptr %172, align 8
  call void %func_ptr148(ptr %168)
  %173 = call ptr @moksha_box_string(ptr @119)
  %print_unbox149 = call ptr @moksha_unbox_string(ptr %173)
  %174 = call i32 (ptr, ...) @printf(ptr @120, ptr %print_unbox149)
  %alloc_obj150 = call ptr @calloc(i64 1, i64 32)
  %175 = getelementptr inbounds nuw %Account, ptr %alloc_obj150, i32 0, i32 0
  store ptr @VTable_Account, ptr %175, align 8
  %176 = call ptr @moksha_box_string(ptr @139)
  call void @Account_constructor(ptr %alloc_obj150, ptr %176, double 5.000000e+02)
  %177 = icmp eq ptr %alloc_obj150, null
  br i1 %177, label %clone_skip152, label %clone_do151

clone_do151:                                      ; preds = %call_safe146
  %clone_alloc154 = call ptr @malloc(i64 32)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc154, ptr align 8 %alloc_obj150, i64 32, i1 false)
  br label %clone_merge153

clone_skip152:                                    ; preds = %call_safe146
  br label %clone_merge153

clone_merge153:                                   ; preds = %clone_skip152, %clone_do151
  %clone_res155 = phi ptr [ %clone_alloc154, %clone_do151 ], [ null, %clone_skip152 ]
  store ptr %clone_res155, ptr %acc, align 8
  store ptr %clone_res155, ptr %acc156, align 8
  %178 = load ptr, ptr %acc156, align 8
  %179 = icmp eq ptr %178, null
  br i1 %179, label %call_error157, label %call_safe158

call_error157:                                    ; preds = %clone_merge153
  %180 = call ptr @moksha_box_string(ptr @140)
  store ptr %180, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe158:                                     ; preds = %clone_merge153
  %181 = getelementptr inbounds nuw %Account, ptr %178, i32 0, i32 0
  %vptr159 = load ptr, ptr %181, align 8
  %182 = getelementptr inbounds ptr, ptr %vptr159, i32 2
  %func_ptr160 = load ptr, ptr %182, align 8
  call void %func_ptr160(ptr %178)
  %183 = call ptr @moksha_box_string(ptr @141)
  %print_unbox161 = call ptr @moksha_unbox_string(ptr %183)
  %184 = call i32 (ptr, ...) @printf(ptr @142, ptr %print_unbox161)
  %185 = load ptr, ptr %acc156, align 8
  %186 = icmp eq ptr %185, null
  br i1 %186, label %call_error162, label %call_safe163

call_error162:                                    ; preds = %call_safe158
  %187 = call ptr @moksha_box_string(ptr @143)
  store ptr %187, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe163:                                     ; preds = %call_safe158
  %188 = getelementptr inbounds nuw %Account, ptr %185, i32 0, i32 0
  %vptr164 = load ptr, ptr %188, align 8
  %189 = getelementptr inbounds ptr, ptr %vptr164, i32 1
  %func_ptr165 = load ptr, ptr %189, align 8
  call void %func_ptr165(ptr %185, double -1.000000e+02)
  %190 = call ptr @moksha_box_string(ptr @144)
  %191 = load ptr, ptr %acc156, align 8
  %192 = icmp eq ptr %191, null
  br i1 %192, label %call_error166, label %call_safe167

call_error166:                                    ; preds = %call_safe163
  %193 = call ptr @moksha_box_string(ptr @145)
  store ptr %193, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe167:                                     ; preds = %call_safe163
  %194 = getelementptr inbounds nuw %Account, ptr %191, i32 0, i32 0
  %vptr168 = load ptr, ptr %194, align 8
  %195 = getelementptr inbounds ptr, ptr %vptr168, i32 0
  %func_ptr169 = load ptr, ptr %195, align 8
  %virt_call = call double %func_ptr169(ptr %191)
  %196 = call ptr @moksha_double_to_str(double %virt_call)
  %197 = call ptr @moksha_box_string(ptr %196)
  %198 = call ptr @moksha_string_concat(ptr %190, ptr %197)
  %print_unbox170 = call ptr @moksha_unbox_string(ptr %198)
  %199 = call i32 (ptr, ...) @printf(ptr @146, ptr %print_unbox170)
  %200 = call ptr @moksha_box_string(ptr @147)
  %print_unbox171 = call ptr @moksha_unbox_string(ptr %200)
  %201 = call i32 (ptr, ...) @printf(ptr @148, ptr %print_unbox171)
  %202 = load ptr, ptr %acc156, align 8
  %203 = icmp eq ptr %202, null
  br i1 %203, label %call_error172, label %call_safe173

call_error172:                                    ; preds = %call_safe167
  %204 = call ptr @moksha_box_string(ptr @149)
  store ptr %204, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe173:                                     ; preds = %call_safe167
  %205 = getelementptr inbounds nuw %Account, ptr %202, i32 0, i32 0
  %vptr174 = load ptr, ptr %205, align 8
  %206 = getelementptr inbounds ptr, ptr %vptr174, i32 1
  %func_ptr175 = load ptr, ptr %206, align 8
  call void %func_ptr175(ptr %202, double 1.000000e+03)
  %207 = call ptr @moksha_box_string(ptr @150)
  %208 = load ptr, ptr %acc156, align 8
  %209 = icmp eq ptr %208, null
  br i1 %209, label %call_error176, label %call_safe177

call_error176:                                    ; preds = %call_safe173
  %210 = call ptr @moksha_box_string(ptr @151)
  store ptr %210, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe177:                                     ; preds = %call_safe173
  %211 = getelementptr inbounds nuw %Account, ptr %208, i32 0, i32 0
  %vptr178 = load ptr, ptr %211, align 8
  %212 = getelementptr inbounds ptr, ptr %vptr178, i32 0
  %func_ptr179 = load ptr, ptr %212, align 8
  %virt_call180 = call double %func_ptr179(ptr %208)
  %213 = call ptr @moksha_double_to_str(double %virt_call180)
  %214 = call ptr @moksha_box_string(ptr %213)
  %215 = call ptr @moksha_string_concat(ptr %207, ptr %214)
  %print_unbox181 = call ptr @moksha_unbox_string(ptr %215)
  %216 = call i32 (ptr, ...) @printf(ptr @152, ptr %print_unbox181)
  %alloc_obj182 = call ptr @calloc(i64 1, i64 32)
  %217 = getelementptr inbounds nuw %Savings, ptr %alloc_obj182, i32 0, i32 0
  store ptr @VTable_Savings, ptr %217, align 8
  %218 = call ptr @moksha_box_string(ptr @158)
  call void @Savings_constructor(ptr %alloc_obj182, ptr %218)
  %219 = icmp eq ptr %alloc_obj182, null
  br i1 %219, label %clone_skip184, label %clone_do183

clone_do183:                                      ; preds = %call_safe177
  %clone_alloc186 = call ptr @malloc(i64 32)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc186, ptr align 8 %alloc_obj182, i64 32, i1 false)
  br label %clone_merge185

clone_skip184:                                    ; preds = %call_safe177
  br label %clone_merge185

clone_merge185:                                   ; preds = %clone_skip184, %clone_do183
  %clone_res187 = phi ptr [ %clone_alloc186, %clone_do183 ], [ null, %clone_skip184 ]
  store ptr %clone_res187, ptr %save, align 8
  store ptr %clone_res187, ptr %save188, align 8
  %220 = load ptr, ptr %save188, align 8
  %221 = icmp eq ptr %220, null
  br i1 %221, label %call_error189, label %call_safe190

call_error189:                                    ; preds = %clone_merge185
  %222 = call ptr @moksha_box_string(ptr @159)
  store ptr %222, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe190:                                     ; preds = %clone_merge185
  %223 = getelementptr inbounds nuw %Savings, ptr %220, i32 0, i32 0
  %vptr191 = load ptr, ptr %223, align 8
  %224 = getelementptr inbounds ptr, ptr %vptr191, i32 2
  %func_ptr192 = load ptr, ptr %224, align 8
  call void %func_ptr192(ptr %220)
  %225 = call ptr @moksha_box_string(ptr @160)
  %print_unbox193 = call ptr @moksha_unbox_string(ptr %225)
  %226 = call i32 (ptr, ...) @printf(ptr @161, ptr %print_unbox193)
  %alloc_obj194 = call ptr @calloc(i64 1, i64 8)
  %227 = getelementptr inbounds nuw %Bat, ptr %alloc_obj194, i32 0, i32 0
  store ptr @VTable_Bat, ptr %227, align 8
  call void @Bat_constructor(ptr %alloc_obj194)
  %228 = icmp eq ptr %alloc_obj194, null
  br i1 %228, label %clone_skip196, label %clone_do195

clone_do195:                                      ; preds = %call_safe190
  %clone_alloc198 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc198, ptr align 8 %alloc_obj194, i64 8, i1 false)
  br label %clone_merge197

clone_skip196:                                    ; preds = %call_safe190
  br label %clone_merge197

clone_merge197:                                   ; preds = %clone_skip196, %clone_do195
  %clone_res199 = phi ptr [ %clone_alloc198, %clone_do195 ], [ null, %clone_skip196 ]
  store ptr %clone_res199, ptr %b, align 8
  store ptr %clone_res199, ptr %b200, align 8
  %229 = load ptr, ptr %b200, align 8
  %230 = icmp eq ptr %229, null
  br i1 %230, label %call_error201, label %call_safe202

call_error201:                                    ; preds = %clone_merge197
  %231 = call ptr @moksha_box_string(ptr @168)
  store ptr %231, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe202:                                     ; preds = %clone_merge197
  %232 = getelementptr inbounds nuw %Bat, ptr %229, i32 0, i32 0
  %vptr203 = load ptr, ptr %232, align 8
  %233 = getelementptr inbounds ptr, ptr %vptr203, i32 0
  %func_ptr204 = load ptr, ptr %233, align 8
  call void %func_ptr204(ptr %229)
  %234 = load ptr, ptr %b200, align 8
  %235 = icmp eq ptr %234, null
  br i1 %235, label %call_error205, label %call_safe206

call_error205:                                    ; preds = %call_safe202
  %236 = call ptr @moksha_box_string(ptr @169)
  store ptr %236, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe206:                                     ; preds = %call_safe202
  %237 = getelementptr inbounds nuw %Bat, ptr %234, i32 0, i32 0
  %vptr207 = load ptr, ptr %237, align 8
  %238 = getelementptr inbounds ptr, ptr %vptr207, i32 1
  %func_ptr208 = load ptr, ptr %238, align 8
  call void %func_ptr208(ptr %234)
  %239 = load ptr, ptr %b200, align 8
  %240 = icmp eq ptr %239, null
  br i1 %240, label %call_error209, label %call_safe210

call_error209:                                    ; preds = %call_safe206
  %241 = call ptr @moksha_box_string(ptr @170)
  store ptr %241, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe210:                                     ; preds = %call_safe206
  %242 = getelementptr inbounds nuw %Bat, ptr %239, i32 0, i32 0
  %vptr211 = load ptr, ptr %242, align 8
  %243 = getelementptr inbounds ptr, ptr %vptr211, i32 2
  %func_ptr212 = load ptr, ptr %243, align 8
  call void %func_ptr212(ptr %239)
  %244 = call ptr @moksha_box_string(ptr @171)
  %print_unbox213 = call ptr @moksha_unbox_string(ptr %244)
  %245 = call i32 (ptr, ...) @printf(ptr @172, ptr %print_unbox213)
  %alloc_obj214 = call ptr @calloc(i64 1, i64 8)
  %246 = getelementptr inbounds nuw %SportsCar, ptr %alloc_obj214, i32 0, i32 0
  store ptr @VTable_SportsCar, ptr %246, align 8
  call void @SportsCar_constructor(ptr %alloc_obj214)
  %247 = icmp eq ptr %alloc_obj214, null
  br i1 %247, label %clone_skip216, label %clone_do215

clone_do215:                                      ; preds = %call_safe210
  %clone_alloc218 = call ptr @malloc(i64 8)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc218, ptr align 8 %alloc_obj214, i64 8, i1 false)
  br label %clone_merge217

clone_skip216:                                    ; preds = %call_safe210
  br label %clone_merge217

clone_merge217:                                   ; preds = %clone_skip216, %clone_do215
  %clone_res219 = phi ptr [ %clone_alloc218, %clone_do215 ], [ null, %clone_skip216 ]
  store ptr %clone_res219, ptr %ferrari, align 8
  store ptr %clone_res219, ptr %ferrari220, align 8
  %248 = load ptr, ptr %ferrari220, align 8
  %249 = icmp eq ptr %248, null
  br i1 %249, label %call_error221, label %call_safe222

call_error221:                                    ; preds = %clone_merge217
  %250 = call ptr @moksha_box_string(ptr @179)
  store ptr %250, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe222:                                     ; preds = %clone_merge217
  %251 = getelementptr inbounds nuw %SportsCar, ptr %248, i32 0, i32 0
  %vptr223 = load ptr, ptr %251, align 8
  %252 = getelementptr inbounds ptr, ptr %vptr223, i32 0
  %func_ptr224 = load ptr, ptr %252, align 8
  call void %func_ptr224(ptr %248)
  %253 = load ptr, ptr %ferrari220, align 8
  %254 = icmp eq ptr %253, null
  br i1 %254, label %call_error225, label %call_safe226

call_error225:                                    ; preds = %call_safe222
  %255 = call ptr @moksha_box_string(ptr @180)
  store ptr %255, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe226:                                     ; preds = %call_safe222
  %256 = getelementptr inbounds nuw %SportsCar, ptr %253, i32 0, i32 0
  %vptr227 = load ptr, ptr %256, align 8
  %257 = getelementptr inbounds ptr, ptr %vptr227, i32 1
  %func_ptr228 = load ptr, ptr %257, align 8
  call void %func_ptr228(ptr %253)
  %258 = load ptr, ptr %ferrari220, align 8
  %259 = icmp eq ptr %258, null
  br i1 %259, label %call_error229, label %call_safe230

call_error229:                                    ; preds = %call_safe226
  %260 = call ptr @moksha_box_string(ptr @181)
  store ptr %260, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

call_safe230:                                     ; preds = %call_safe226
  %261 = getelementptr inbounds nuw %SportsCar, ptr %258, i32 0, i32 0
  %vptr231 = load ptr, ptr %261, align 8
  %262 = getelementptr inbounds ptr, ptr %vptr231, i32 2
  %func_ptr232 = load ptr, ptr %262, align 8
  call void %func_ptr232(ptr %258)
  %263 = call ptr @moksha_box_string(ptr @182)
  %print_unbox233 = call ptr @moksha_unbox_string(ptr %263)
  %264 = call i32 (ptr, ...) @printf(ptr @183, ptr %print_unbox233)
  %265 = call ptr @moksha_box_string(ptr @184)
  %print_unbox234 = call ptr @moksha_unbox_string(ptr %265)
  %266 = call i32 (ptr, ...) @printf(ptr @185, ptr %print_unbox234)
  %alloc_obj235 = call ptr @calloc(i64 1, i64 24)
  call void @ListNode_constructor(ptr %alloc_obj235)
  store ptr %alloc_obj235, ptr %head, align 8
  store ptr %alloc_obj235, ptr %head236, align 8
  %loaded_obj_ptr237 = load ptr, ptr %head236, align 8
  %267 = icmp eq ptr %loaded_obj_ptr237, null
  br i1 %267, label %mem_error238, label %mem_safe239

mem_error238:                                     ; preds = %call_safe230
  %268 = call ptr @moksha_box_string(ptr @186)
  store ptr %268, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe239:                                      ; preds = %call_safe230
  %269 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr237, i32 0, i32 1
  %loaded_obj_ptr240 = load ptr, ptr %head236, align 8
  %270 = icmp eq ptr %loaded_obj_ptr240, null
  br i1 %270, label %mem_error241, label %mem_safe242

mem_error241:                                     ; preds = %mem_safe239
  %271 = call ptr @moksha_box_string(ptr @187)
  store ptr %271, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe242:                                      ; preds = %mem_safe239
  %272 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr240, i32 0, i32 1
  store i32 10, ptr %269, align 4
  %alloc_obj243 = call ptr @calloc(i64 1, i64 24)
  call void @ListNode_constructor(ptr %alloc_obj243)
  store ptr %alloc_obj243, ptr %second, align 8
  store ptr %alloc_obj243, ptr %second244, align 8
  %loaded_obj_ptr245 = load ptr, ptr %second244, align 8
  %273 = icmp eq ptr %loaded_obj_ptr245, null
  br i1 %273, label %mem_error246, label %mem_safe247

mem_error246:                                     ; preds = %mem_safe242
  %274 = call ptr @moksha_box_string(ptr @188)
  store ptr %274, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe247:                                      ; preds = %mem_safe242
  %275 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr245, i32 0, i32 1
  %loaded_obj_ptr248 = load ptr, ptr %second244, align 8
  %276 = icmp eq ptr %loaded_obj_ptr248, null
  br i1 %276, label %mem_error249, label %mem_safe250

mem_error249:                                     ; preds = %mem_safe247
  %277 = call ptr @moksha_box_string(ptr @189)
  store ptr %277, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe250:                                      ; preds = %mem_safe247
  %278 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr248, i32 0, i32 1
  store i32 20, ptr %275, align 4
  %279 = load ptr, ptr %second244, align 8
  %loaded_obj_ptr251 = load ptr, ptr %head236, align 8
  %280 = icmp eq ptr %loaded_obj_ptr251, null
  br i1 %280, label %mem_error252, label %mem_safe253

mem_error252:                                     ; preds = %mem_safe250
  %281 = call ptr @moksha_box_string(ptr @190)
  store ptr %281, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe253:                                      ; preds = %mem_safe250
  %282 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr251, i32 0, i32 2
  %loaded_obj_ptr254 = load ptr, ptr %head236, align 8
  %283 = icmp eq ptr %loaded_obj_ptr254, null
  br i1 %283, label %mem_error255, label %mem_safe256

mem_error255:                                     ; preds = %mem_safe253
  %284 = call ptr @moksha_box_string(ptr @191)
  store ptr %284, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe256:                                      ; preds = %mem_safe253
  %285 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr254, i32 0, i32 2
  store ptr %279, ptr %282, align 8
  %286 = call ptr @moksha_box_string(ptr @192)
  %loaded_obj_ptr257 = load ptr, ptr %head236, align 8
  %287 = icmp eq ptr %loaded_obj_ptr257, null
  br i1 %287, label %mem_error258, label %mem_safe259

mem_error258:                                     ; preds = %mem_safe256
  %288 = call ptr @moksha_box_string(ptr @193)
  store ptr %288, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe259:                                      ; preds = %mem_safe256
  %289 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr257, i32 0, i32 1
  %290 = load i32, ptr %289, align 4
  %291 = call ptr @moksha_int_to_str(i32 %290)
  %292 = call ptr @moksha_box_string(ptr %291)
  %293 = call ptr @moksha_string_concat(ptr %286, ptr %292)
  %294 = call ptr @moksha_box_string(ptr @194)
  %295 = call ptr @moksha_string_concat(ptr %293, ptr %294)
  %loaded_obj_ptr260 = load ptr, ptr %head236, align 8
  %296 = icmp eq ptr %loaded_obj_ptr260, null
  br i1 %296, label %mem_error261, label %mem_safe262

mem_error261:                                     ; preds = %mem_safe259
  %297 = call ptr @moksha_box_string(ptr @195)
  store ptr %297, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe262:                                      ; preds = %mem_safe259
  %298 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr260, i32 0, i32 2
  %loaded_obj_ptr263 = load ptr, ptr %298, align 8
  %299 = icmp eq ptr %loaded_obj_ptr263, null
  br i1 %299, label %mem_error264, label %mem_safe265

mem_error264:                                     ; preds = %mem_safe262
  %300 = call ptr @moksha_box_string(ptr @196)
  store ptr %300, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe265:                                      ; preds = %mem_safe262
  %301 = getelementptr inbounds nuw %Node, ptr %loaded_obj_ptr263, i32 0, i32 1
  %302 = load i32, ptr %301, align 4
  %303 = call ptr @moksha_int_to_str(i32 %302)
  %304 = call ptr @moksha_box_string(ptr %303)
  %305 = call ptr @moksha_string_concat(ptr %295, ptr %304)
  %print_unbox266 = call ptr @moksha_unbox_string(ptr %305)
  %306 = call i32 (ptr, ...) @printf(ptr @197, ptr %print_unbox266)
  %307 = call ptr @moksha_box_string(ptr @198)
  %print_unbox267 = call ptr @moksha_unbox_string(ptr %307)
  %308 = call i32 (ptr, ...) @printf(ptr @199, ptr %print_unbox267)
  %loaded_obj_ptr268 = load ptr, ptr %head236, align 8
  %309 = icmp eq ptr %loaded_obj_ptr268, null
  br i1 %309, label %mem_error269, label %mem_safe270

mem_error269:                                     ; preds = %mem_safe265
  %310 = call ptr @moksha_box_string(ptr @200)
  store ptr %310, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe270:                                      ; preds = %mem_safe265
  %311 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr268, i32 0, i32 2
  %loaded_obj_ptr271 = load ptr, ptr %311, align 8
  %312 = icmp eq ptr %loaded_obj_ptr271, null
  br i1 %312, label %mem_error272, label %mem_safe273

mem_error272:                                     ; preds = %mem_safe270
  %313 = call ptr @moksha_box_string(ptr @201)
  store ptr %313, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe273:                                      ; preds = %mem_safe270
  %314 = getelementptr inbounds nuw %Node, ptr %loaded_obj_ptr271, i32 0, i32 1
  %loaded_obj_ptr274 = load ptr, ptr %head236, align 8
  %315 = icmp eq ptr %loaded_obj_ptr274, null
  br i1 %315, label %mem_error275, label %mem_safe276

mem_error275:                                     ; preds = %mem_safe273
  %316 = call ptr @moksha_box_string(ptr @202)
  store ptr %316, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe276:                                      ; preds = %mem_safe273
  %317 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr274, i32 0, i32 2
  %loaded_obj_ptr277 = load ptr, ptr %317, align 8
  %318 = icmp eq ptr %loaded_obj_ptr277, null
  br i1 %318, label %mem_error278, label %mem_safe279

mem_error278:                                     ; preds = %mem_safe276
  %319 = call ptr @moksha_box_string(ptr @203)
  store ptr %319, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe279:                                      ; preds = %mem_safe276
  %320 = getelementptr inbounds nuw %Node, ptr %loaded_obj_ptr277, i32 0, i32 1
  store i32 99, ptr %314, align 4
  %321 = call ptr @moksha_box_string(ptr @204)
  %loaded_obj_ptr280 = load ptr, ptr %second244, align 8
  %322 = icmp eq ptr %loaded_obj_ptr280, null
  br i1 %322, label %mem_error281, label %mem_safe282

mem_error281:                                     ; preds = %mem_safe279
  %323 = call ptr @moksha_box_string(ptr @205)
  store ptr %323, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe282:                                      ; preds = %mem_safe279
  %324 = getelementptr inbounds nuw %ListNode, ptr %loaded_obj_ptr280, i32 0, i32 1
  %325 = load i32, ptr %324, align 4
  %326 = call ptr @moksha_int_to_str(i32 %325)
  %327 = call ptr @moksha_box_string(ptr %326)
  %328 = call ptr @moksha_string_concat(ptr %321, ptr %327)
  %print_unbox283 = call ptr @moksha_unbox_string(ptr %328)
  %329 = call i32 (ptr, ...) @printf(ptr @206, ptr %print_unbox283)
  %330 = call ptr @moksha_box_string(ptr @207)
  %print_unbox284 = call ptr @moksha_unbox_string(ptr %330)
  %331 = call i32 (ptr, ...) @printf(ptr @208, ptr %print_unbox284)
  %alloc_obj285 = call ptr @calloc(i64 1, i64 24)
  call void @Square_constructor(ptr %alloc_obj285)
  %332 = icmp eq ptr %alloc_obj285, null
  br i1 %332, label %clone_skip287, label %clone_do286

clone_do286:                                      ; preds = %mem_safe282
  %clone_alloc289 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc289, ptr align 8 %alloc_obj285, i64 24, i1 false)
  br label %clone_merge288

clone_skip287:                                    ; preds = %mem_safe282
  br label %clone_merge288

clone_merge288:                                   ; preds = %clone_skip287, %clone_do286
  %clone_res290 = phi ptr [ %clone_alloc289, %clone_do286 ], [ null, %clone_skip287 ]
  store ptr %clone_res290, ptr %sq1, align 8
  store ptr %clone_res290, ptr %sq1291, align 8
  %loaded_obj_ptr292 = load ptr, ptr %sq1291, align 8
  %333 = icmp eq ptr %loaded_obj_ptr292, null
  br i1 %333, label %mem_error293, label %mem_safe294

mem_error293:                                     ; preds = %clone_merge288
  %334 = call ptr @moksha_box_string(ptr @209)
  store ptr %334, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe294:                                      ; preds = %clone_merge288
  %335 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr292, i32 0, i32 2
  %loaded_obj_ptr295 = load ptr, ptr %sq1291, align 8
  %336 = icmp eq ptr %loaded_obj_ptr295, null
  br i1 %336, label %mem_error296, label %mem_safe297

mem_error296:                                     ; preds = %mem_safe294
  %337 = call ptr @moksha_box_string(ptr @210)
  store ptr %337, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe297:                                      ; preds = %mem_safe294
  %338 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr295, i32 0, i32 2
  store double 5.000000e+00, ptr %335, align 8
  %loaded_obj_ptr298 = load ptr, ptr %sq1291, align 8
  %339 = icmp eq ptr %loaded_obj_ptr298, null
  br i1 %339, label %mem_error299, label %mem_safe300

mem_error299:                                     ; preds = %mem_safe297
  %340 = call ptr @moksha_box_string(ptr @211)
  store ptr %340, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe300:                                      ; preds = %mem_safe297
  %341 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr298, i32 0, i32 1
  %loaded_obj_ptr301 = load ptr, ptr %sq1291, align 8
  %342 = icmp eq ptr %loaded_obj_ptr301, null
  br i1 %342, label %mem_error302, label %mem_safe303

mem_error302:                                     ; preds = %mem_safe300
  %343 = call ptr @moksha_box_string(ptr @212)
  store ptr %343, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe303:                                      ; preds = %mem_safe300
  %344 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr301, i32 0, i32 1
  store double 2.500000e+01, ptr %341, align 8
  %345 = call ptr @moksha_box_string(ptr @213)
  %print_unbox304 = call ptr @moksha_unbox_string(ptr %345)
  %346 = call i32 (ptr, ...) @printf(ptr @214, ptr %print_unbox304)
  %347 = load ptr, ptr %sq1291, align 8
  store ptr %347, ptr %sqRef, align 8
  %heap_var306 = call ptr @malloc(i64 8)
  store ptr %heap_var306, ptr %sqRef305, align 8
  store ptr %347, ptr %heap_var306, align 8
  %348 = call ptr @moksha_box_string(ptr @215)
  %print_unbox307 = call ptr @moksha_unbox_string(ptr %348)
  %349 = call i32 (ptr, ...) @printf(ptr @216, ptr %print_unbox307)
  %350 = load ptr, ptr %sq1291, align 8
  %351 = icmp eq ptr %350, null
  br i1 %351, label %clone_skip309, label %clone_do308

clone_do308:                                      ; preds = %mem_safe303
  %clone_alloc311 = call ptr @malloc(i64 24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %clone_alloc311, ptr align 8 %350, i64 24, i1 false)
  br label %clone_merge310

clone_skip309:                                    ; preds = %mem_safe303
  br label %clone_merge310

clone_merge310:                                   ; preds = %clone_skip309, %clone_do308
  %clone_res312 = phi ptr [ %clone_alloc311, %clone_do308 ], [ null, %clone_skip309 ]
  store ptr %clone_res312, ptr %sq2, align 8
  store ptr %clone_res312, ptr %sq2313, align 8
  %352 = call ptr @moksha_box_string(ptr @217)
  %print_unbox314 = call ptr @moksha_unbox_string(ptr %352)
  %353 = call i32 (ptr, ...) @printf(ptr @218, ptr %print_unbox314)
  %heap_addr315 = load ptr, ptr %sqRef305, align 8
  %loaded_obj_ptr316 = load ptr, ptr %heap_addr315, align 8
  %354 = icmp eq ptr %loaded_obj_ptr316, null
  br i1 %354, label %mem_error317, label %mem_safe318

mem_error317:                                     ; preds = %clone_merge310
  %355 = call ptr @moksha_box_string(ptr @219)
  store ptr %355, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe318:                                      ; preds = %clone_merge310
  %356 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr316, i32 0, i32 1
  %heap_addr319 = load ptr, ptr %sqRef305, align 8
  %loaded_obj_ptr320 = load ptr, ptr %heap_addr319, align 8
  %357 = icmp eq ptr %loaded_obj_ptr320, null
  br i1 %357, label %mem_error321, label %mem_safe322

mem_error321:                                     ; preds = %mem_safe318
  %358 = call ptr @moksha_box_string(ptr @220)
  store ptr %358, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe322:                                      ; preds = %mem_safe318
  %359 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr320, i32 0, i32 1
  store double 1.000000e+02, ptr %356, align 8
  %360 = call ptr @moksha_box_string(ptr @221)
  %loaded_obj_ptr323 = load ptr, ptr %sq1291, align 8
  %361 = icmp eq ptr %loaded_obj_ptr323, null
  br i1 %361, label %mem_error324, label %mem_safe325

mem_error324:                                     ; preds = %mem_safe322
  %362 = call ptr @moksha_box_string(ptr @222)
  store ptr %362, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe325:                                      ; preds = %mem_safe322
  %363 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr323, i32 0, i32 1
  %364 = load double, ptr %363, align 8
  %365 = call ptr @moksha_double_to_str(double %364)
  %366 = call ptr @moksha_box_string(ptr %365)
  %367 = call ptr @moksha_string_concat(ptr %360, ptr %366)
  %print_unbox326 = call ptr @moksha_unbox_string(ptr %367)
  %368 = call i32 (ptr, ...) @printf(ptr @223, ptr %print_unbox326)
  %369 = call ptr @moksha_box_string(ptr @224)
  %loaded_obj_ptr327 = load ptr, ptr %sq2313, align 8
  %370 = icmp eq ptr %loaded_obj_ptr327, null
  br i1 %370, label %mem_error328, label %mem_safe329

mem_error328:                                     ; preds = %mem_safe325
  %371 = call ptr @moksha_box_string(ptr @225)
  store ptr %371, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe329:                                      ; preds = %mem_safe325
  %372 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr327, i32 0, i32 1
  %373 = load double, ptr %372, align 8
  %374 = call ptr @moksha_double_to_str(double %373)
  %375 = call ptr @moksha_box_string(ptr %374)
  %376 = call ptr @moksha_string_concat(ptr %369, ptr %375)
  %print_unbox330 = call ptr @moksha_unbox_string(ptr %376)
  %377 = call i32 (ptr, ...) @printf(ptr @226, ptr %print_unbox330)
  %378 = call ptr @moksha_box_string(ptr @227)
  %heap_addr331 = load ptr, ptr %sqRef305, align 8
  %loaded_obj_ptr332 = load ptr, ptr %heap_addr331, align 8
  %379 = icmp eq ptr %loaded_obj_ptr332, null
  br i1 %379, label %mem_error333, label %mem_safe334

mem_error333:                                     ; preds = %mem_safe329
  %380 = call ptr @moksha_box_string(ptr @228)
  store ptr %380, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe334:                                      ; preds = %mem_safe329
  %381 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr332, i32 0, i32 1
  %382 = load double, ptr %381, align 8
  %383 = call ptr @moksha_double_to_str(double %382)
  %384 = call ptr @moksha_box_string(ptr %383)
  %385 = call ptr @moksha_string_concat(ptr %378, ptr %384)
  %print_unbox335 = call ptr @moksha_unbox_string(ptr %385)
  %386 = call i32 (ptr, ...) @printf(ptr @229, ptr %print_unbox335)
  %heap_addr336 = load ptr, ptr %sqRef305, align 8
  %387 = load ptr, ptr %heap_addr336, align 8
  store ptr %387, ptr %aliasRef, align 8
  %heap_var338 = call ptr @malloc(i64 8)
  store ptr %heap_var338, ptr %aliasRef337, align 8
  store ptr %387, ptr %heap_var338, align 8
  %heap_addr339 = load ptr, ptr %aliasRef337, align 8
  %loaded_obj_ptr340 = load ptr, ptr %heap_addr339, align 8
  %388 = icmp eq ptr %loaded_obj_ptr340, null
  br i1 %388, label %mem_error341, label %mem_safe342

mem_error341:                                     ; preds = %mem_safe334
  %389 = call ptr @moksha_box_string(ptr @230)
  store ptr %389, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe342:                                      ; preds = %mem_safe334
  %390 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr340, i32 0, i32 1
  %heap_addr343 = load ptr, ptr %aliasRef337, align 8
  %loaded_obj_ptr344 = load ptr, ptr %heap_addr343, align 8
  %391 = icmp eq ptr %loaded_obj_ptr344, null
  br i1 %391, label %mem_error345, label %mem_safe346

mem_error345:                                     ; preds = %mem_safe342
  %392 = call ptr @moksha_box_string(ptr @231)
  store ptr %392, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe346:                                      ; preds = %mem_safe342
  %393 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr344, i32 0, i32 1
  store double 2.000000e+02, ptr %390, align 8
  %394 = call ptr @moksha_box_string(ptr @232)
  %heap_addr347 = load ptr, ptr %sqRef305, align 8
  %loaded_obj_ptr348 = load ptr, ptr %heap_addr347, align 8
  %395 = icmp eq ptr %loaded_obj_ptr348, null
  br i1 %395, label %mem_error349, label %mem_safe350

mem_error349:                                     ; preds = %mem_safe346
  %396 = call ptr @moksha_box_string(ptr @233)
  store ptr %396, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret i32 0

mem_safe350:                                      ; preds = %mem_safe346
  %397 = getelementptr inbounds nuw %Square, ptr %loaded_obj_ptr348, i32 0, i32 1
  %398 = load double, ptr %397, align 8
  %399 = call ptr @moksha_double_to_str(double %398)
  %400 = call ptr @moksha_box_string(ptr %399)
  %401 = call ptr @moksha_string_concat(ptr %394, ptr %400)
  %print_unbox351 = call ptr @moksha_unbox_string(ptr %401)
  %402 = call i32 (ptr, ...) @printf(ptr @234, ptr %print_unbox351)
  %403 = call ptr @moksha_box_string(ptr @235)
  %print_unbox352 = call ptr @moksha_unbox_string(ptr %403)
  %404 = call i32 (ptr, ...) @printf(ptr @236, ptr %print_unbox352)
  ret i32 0
}

define ptr @"Vector_+"(ptr %this, ptr %other) {
entry:
  %other1 = alloca ptr, align 8
  store ptr %other, ptr %other1, align 8
  %alloc_obj = call ptr @calloc(i64 1, i64 24)
  %0 = getelementptr inbounds nuw %Vector, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Vector, ptr %0, align 8
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @6)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %4 = load double, ptr %3, align 8
  %loaded_obj_ptr = load ptr, ptr %other1, align 8
  %5 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %5, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %6 = call ptr @moksha_box_string(ptr @7)
  store ptr %6, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe3:                                        ; preds = %mem_safe
  %7 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr, i32 0, i32 1
  %8 = load double, ptr %7, align 8
  %faddtmp = fadd double %4, %8
  %9 = icmp eq ptr %this, null
  br i1 %9, label %mem_error4, label %mem_safe5

mem_error4:                                       ; preds = %mem_safe3
  %10 = call ptr @moksha_box_string(ptr @8)
  store ptr %10, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe5:                                        ; preds = %mem_safe3
  %11 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %12 = load double, ptr %11, align 8
  %loaded_obj_ptr6 = load ptr, ptr %other1, align 8
  %13 = icmp eq ptr %loaded_obj_ptr6, null
  br i1 %13, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe5
  %14 = call ptr @moksha_box_string(ptr @9)
  store ptr %14, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe8:                                        ; preds = %mem_safe5
  %15 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr6, i32 0, i32 2
  %16 = load double, ptr %15, align 8
  %faddtmp9 = fadd double %12, %16
  call void @Vector_constructor(ptr %alloc_obj, double %faddtmp, double %faddtmp9)
  ret ptr %alloc_obj
}

define ptr @Vector_-(ptr %this, ptr %other) {
entry:
  %other1 = alloca ptr, align 8
  store ptr %other, ptr %other1, align 8
  %alloc_obj = call ptr @calloc(i64 1, i64 24)
  %0 = getelementptr inbounds nuw %Vector, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Vector, ptr %0, align 8
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @10)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %4 = load double, ptr %3, align 8
  %loaded_obj_ptr = load ptr, ptr %other1, align 8
  %5 = icmp eq ptr %loaded_obj_ptr, null
  br i1 %5, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %6 = call ptr @moksha_box_string(ptr @11)
  store ptr %6, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe3:                                        ; preds = %mem_safe
  %7 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr, i32 0, i32 1
  %8 = load double, ptr %7, align 8
  %fsubtmp = fsub double %4, %8
  %9 = icmp eq ptr %this, null
  br i1 %9, label %mem_error4, label %mem_safe5

mem_error4:                                       ; preds = %mem_safe3
  %10 = call ptr @moksha_box_string(ptr @12)
  store ptr %10, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe5:                                        ; preds = %mem_safe3
  %11 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %12 = load double, ptr %11, align 8
  %loaded_obj_ptr6 = load ptr, ptr %other1, align 8
  %13 = icmp eq ptr %loaded_obj_ptr6, null
  br i1 %13, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe5
  %14 = call ptr @moksha_box_string(ptr @13)
  store ptr %14, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe8:                                        ; preds = %mem_safe5
  %15 = getelementptr inbounds nuw %Vector, ptr %loaded_obj_ptr6, i32 0, i32 2
  %16 = load double, ptr %15, align 8
  %fsubtmp9 = fsub double %12, %16
  call void @Vector_constructor(ptr %alloc_obj, double %fsubtmp, double %fsubtmp9)
  ret ptr %alloc_obj
}

define ptr @"Vector_*"(ptr %this, double %scalar) {
entry:
  %scalar1 = alloca double, align 8
  store double %scalar, ptr %scalar1, align 8
  %alloc_obj = call ptr @calloc(i64 1, i64 24)
  %0 = getelementptr inbounds nuw %Vector, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_Vector, ptr %0, align 8
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @14)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %4 = load double, ptr %3, align 8
  %5 = load double, ptr %scalar1, align 8
  %fmultmp = fmul double %4, %5
  %6 = icmp eq ptr %this, null
  br i1 %6, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %7 = call ptr @moksha_box_string(ptr @15)
  store ptr %7, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe3:                                        ; preds = %mem_safe
  %8 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %9 = load double, ptr %8, align 8
  %10 = load double, ptr %scalar1, align 8
  %fmultmp4 = fmul double %9, %10
  call void @Vector_constructor(ptr %alloc_obj, double %fmultmp, double %fmultmp4)
  ret ptr %alloc_obj
}

define void @Vector_printVec(ptr %this, ptr %label) {
entry:
  %label1 = alloca ptr, align 8
  store ptr %label, ptr %label1, align 8
  %0 = load ptr, ptr %label1, align 8
  %1 = call ptr @moksha_box_string(ptr @16)
  %2 = call ptr @moksha_string_concat(ptr %0, ptr %1)
  %3 = icmp eq ptr %this, null
  br i1 %3, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %4 = call ptr @moksha_box_string(ptr @17)
  store ptr %4, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %5 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %6 = load double, ptr %5, align 8
  %7 = call ptr @moksha_double_to_str(double %6)
  %8 = call ptr @moksha_box_string(ptr %7)
  %9 = call ptr @moksha_string_concat(ptr %2, ptr %8)
  %10 = call ptr @moksha_box_string(ptr @18)
  %11 = call ptr @moksha_string_concat(ptr %9, ptr %10)
  %12 = icmp eq ptr %this, null
  br i1 %12, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %13 = call ptr @moksha_box_string(ptr @19)
  store ptr %13, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe3:                                        ; preds = %mem_safe
  %14 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %15 = load double, ptr %14, align 8
  %16 = call ptr @moksha_double_to_str(double %15)
  %17 = call ptr @moksha_box_string(ptr %16)
  %18 = call ptr @moksha_string_concat(ptr %11, ptr %17)
  %print_unbox = call ptr @moksha_unbox_string(ptr %18)
  %19 = call i32 (ptr, ...) @printf(ptr @20, ptr %print_unbox)
  %20 = load i32, ptr @__exception_flag, align 4
  %21 = icmp ne i32 %20, 0
  br i1 %21, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe3
  ret void

unwind:                                           ; preds = %mem_safe3
  ret void
}

define void @Vector_constructor(ptr %this, double %x, double %y) {
entry:
  %y2 = alloca double, align 8
  %x1 = alloca double, align 8
  %0 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  store double 0.000000e+00, ptr %0, align 8
  %1 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  store double 0.000000e+00, ptr %1, align 8
  store double %x, ptr %x1, align 8
  store double %y, ptr %y2, align 8
  %2 = load double, ptr %x1, align 8
  %3 = icmp eq ptr %this, null
  br i1 %3, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %4 = call ptr @moksha_box_string(ptr @2)
  store ptr %4, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %5 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %6 = icmp eq ptr %this, null
  br i1 %6, label %mem_error3, label %mem_safe4

mem_error3:                                       ; preds = %mem_safe
  %7 = call ptr @moksha_box_string(ptr @3)
  store ptr %7, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe4:                                        ; preds = %mem_safe
  %8 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  store double %2, ptr %5, align 8
  %9 = load i32, ptr @__exception_flag, align 4
  %10 = icmp ne i32 %9, 0
  br i1 %10, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe4
  %11 = load double, ptr %y2, align 8
  %12 = icmp eq ptr %this, null
  br i1 %12, label %mem_error5, label %mem_safe6

unwind:                                           ; preds = %mem_safe4
  ret void

mem_error5:                                       ; preds = %next_stmt
  %13 = call ptr @moksha_box_string(ptr @4)
  store ptr %13, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe6:                                        ; preds = %next_stmt
  %14 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %15 = icmp eq ptr %this, null
  br i1 %15, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe6
  %16 = call ptr @moksha_box_string(ptr @5)
  store ptr %16, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe8:                                        ; preds = %mem_safe6
  %17 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  store double %11, ptr %14, align 8
  %18 = load i32, ptr @__exception_flag, align 4
  %19 = icmp ne i32 %18, 0
  br i1 %19, label %unwind10, label %next_stmt9

next_stmt9:                                       ; preds = %mem_safe8
  ret void

unwind10:                                         ; preds = %mem_safe8
  ret void
}

define void @Vector_destructor(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @21)
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @22)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 1
  %4 = load double, ptr %3, align 8
  %5 = call ptr @moksha_double_to_str(double %4)
  %6 = call ptr @moksha_box_string(ptr %5)
  %7 = call ptr @moksha_string_concat(ptr %0, ptr %6)
  %8 = call ptr @moksha_box_string(ptr @23)
  %9 = call ptr @moksha_string_concat(ptr %7, ptr %8)
  %10 = icmp eq ptr %this, null
  br i1 %10, label %mem_error1, label %mem_safe2

mem_error1:                                       ; preds = %mem_safe
  %11 = call ptr @moksha_box_string(ptr @24)
  store ptr %11, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe2:                                        ; preds = %mem_safe
  %12 = getelementptr inbounds nuw %Vector, ptr %this, i32 0, i32 2
  %13 = load double, ptr %12, align 8
  %14 = call ptr @moksha_double_to_str(double %13)
  %15 = call ptr @moksha_box_string(ptr %14)
  %16 = call ptr @moksha_string_concat(ptr %9, ptr %15)
  %17 = call ptr @moksha_box_string(ptr @25)
  %18 = call ptr @moksha_string_concat(ptr %16, ptr %17)
  %print_unbox = call ptr @moksha_unbox_string(ptr %18)
  %19 = call i32 (ptr, ...) @printf(ptr @26, ptr %print_unbox)
  %20 = load i32, ptr @__exception_flag, align 4
  %21 = icmp ne i32 %20, 0
  br i1 %21, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe2
  ret void

unwind:                                           ; preds = %mem_safe2
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

define void @Manager_constructor(ptr %this, i32 %id, ptr %name) {
entry:
  %name2 = alloca ptr, align 8
  %id1 = alloca i32, align 4
  %0 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 1
  store i32 0, ptr %0, align 4
  %1 = call ptr @moksha_box_string(ptr @51)
  %2 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 2
  store ptr %1, ptr %2, align 8
  store i32 %id, ptr %id1, align 4
  store ptr %name, ptr %name2, align 8
  %3 = load i32, ptr %id1, align 4
  %4 = icmp eq ptr %this, null
  br i1 %4, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %5 = call ptr @moksha_box_string(ptr @52)
  store ptr %5, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %6 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 1
  %7 = icmp eq ptr %this, null
  br i1 %7, label %mem_error3, label %mem_safe4

mem_error3:                                       ; preds = %mem_safe
  %8 = call ptr @moksha_box_string(ptr @53)
  store ptr %8, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe4:                                        ; preds = %mem_safe
  %9 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 1
  store i32 %3, ptr %6, align 4
  %10 = load i32, ptr @__exception_flag, align 4
  %11 = icmp ne i32 %10, 0
  br i1 %11, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe4
  %12 = load ptr, ptr %name2, align 8
  %13 = icmp eq ptr %this, null
  br i1 %13, label %mem_error5, label %mem_safe6

unwind:                                           ; preds = %mem_safe4
  ret void

mem_error5:                                       ; preds = %next_stmt
  %14 = call ptr @moksha_box_string(ptr @54)
  store ptr %14, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe6:                                        ; preds = %next_stmt
  %15 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 2
  %16 = icmp eq ptr %this, null
  br i1 %16, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe6
  %17 = call ptr @moksha_box_string(ptr @55)
  store ptr %17, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe8:                                        ; preds = %mem_safe6
  %18 = getelementptr inbounds nuw %Manager, ptr %this, i32 0, i32 2
  store ptr %12, ptr %15, align 8
  %19 = load i32, ptr @__exception_flag, align 4
  %20 = icmp ne i32 %19, 0
  br i1 %20, label %unwind10, label %next_stmt9

next_stmt9:                                       ; preds = %mem_safe8
  ret void

unwind10:                                         ; preds = %mem_safe8
  ret void
}

define ptr @"ScalarMath_*"(ptr %this, double %factor) {
entry:
  %factor1 = alloca double, align 8
  store double %factor, ptr %factor1, align 8
  %alloc_obj = call ptr @calloc(i64 1, i64 16)
  %0 = getelementptr inbounds nuw %ScalarMath, ptr %alloc_obj, i32 0, i32 0
  store ptr @VTable_ScalarMath, ptr %0, align 8
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @97)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret ptr null

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %ScalarMath, ptr %this, i32 0, i32 1
  %4 = load double, ptr %3, align 8
  %5 = load double, ptr %factor1, align 8
  %fmultmp = fmul double %4, %5
  call void @ScalarMath_constructor(ptr %alloc_obj, double %fmultmp)
  ret ptr %alloc_obj
}

define void @ScalarMath_constructor(ptr %this, double %v) {
entry:
  %v1 = alloca double, align 8
  %0 = getelementptr inbounds nuw %ScalarMath, ptr %this, i32 0, i32 1
  store double 0.000000e+00, ptr %0, align 8
  store double %v, ptr %v1, align 8
  %1 = load double, ptr %v1, align 8
  %2 = icmp eq ptr %this, null
  br i1 %2, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %3 = call ptr @moksha_box_string(ptr @95)
  store ptr %3, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %4 = getelementptr inbounds nuw %ScalarMath, ptr %this, i32 0, i32 1
  %5 = icmp eq ptr %this, null
  br i1 %5, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %6 = call ptr @moksha_box_string(ptr @96)
  store ptr %6, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe3:                                        ; preds = %mem_safe
  %7 = getelementptr inbounds nuw %ScalarMath, ptr %this, i32 0, i32 1
  store double %1, ptr %4, align 8
  %8 = load i32, ptr @__exception_flag, align 4
  %9 = icmp ne i32 %8, 0
  br i1 %9, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe3
  ret void

unwind:                                           ; preds = %mem_safe3
  ret void
}

define void @Entity_describe(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @103)
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @104)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Entity, ptr %this, i32 0, i32 1
  %4 = load ptr, ptr %3, align 8
  %5 = call ptr @moksha_string_concat(ptr %0, ptr %4)
  %print_unbox = call ptr @moksha_unbox_string(ptr %5)
  %6 = call i32 (ptr, ...) @printf(ptr @105, ptr %print_unbox)
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe
  ret void

unwind:                                           ; preds = %mem_safe
  ret void
}

define void @Entity_constructor(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @106)
  %1 = getelementptr inbounds nuw %Entity, ptr %this, i32 0, i32 1
  store ptr %0, ptr %1, align 8
  ret void
}

define void @Player_describe(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @111)
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @112)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 1
  %4 = load ptr, ptr %3, align 8
  %5 = call ptr @moksha_string_concat(ptr %0, ptr %4)
  %6 = call ptr @moksha_box_string(ptr @113)
  %7 = call ptr @moksha_string_concat(ptr %5, ptr %6)
  %8 = icmp eq ptr %this, null
  br i1 %8, label %mem_error1, label %mem_safe2

mem_error1:                                       ; preds = %mem_safe
  %9 = call ptr @moksha_box_string(ptr @114)
  store ptr %9, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe2:                                        ; preds = %mem_safe
  %10 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 2
  %11 = load i32, ptr %10, align 4
  %12 = call ptr @moksha_int_to_str(i32 %11)
  %13 = call ptr @moksha_box_string(ptr %12)
  %14 = call ptr @moksha_string_concat(ptr %7, ptr %13)
  %print_unbox = call ptr @moksha_unbox_string(ptr %14)
  %15 = call i32 (ptr, ...) @printf(ptr @115, ptr %print_unbox)
  %16 = load i32, ptr @__exception_flag, align 4
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe2
  ret void

unwind:                                           ; preds = %mem_safe2
  ret void
}

define void @Player_constructor(ptr %this, ptr %id, i32 %lvl) {
entry:
  %lvl2 = alloca i32, align 4
  %id1 = alloca ptr, align 8
  %0 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 2
  store i32 1, ptr %0, align 4
  store ptr %id, ptr %id1, align 8
  store i32 %lvl, ptr %lvl2, align 4
  %1 = load ptr, ptr %id1, align 8
  %2 = icmp eq ptr %this, null
  br i1 %2, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %3 = call ptr @moksha_box_string(ptr @107)
  store ptr %3, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %4 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 1
  %5 = icmp eq ptr %this, null
  br i1 %5, label %mem_error3, label %mem_safe4

mem_error3:                                       ; preds = %mem_safe
  %6 = call ptr @moksha_box_string(ptr @108)
  store ptr %6, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe4:                                        ; preds = %mem_safe
  %7 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 1
  store ptr %1, ptr %4, align 8
  %8 = load i32, ptr @__exception_flag, align 4
  %9 = icmp ne i32 %8, 0
  br i1 %9, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe4
  %10 = load i32, ptr %lvl2, align 4
  %11 = icmp eq ptr %this, null
  br i1 %11, label %mem_error5, label %mem_safe6

unwind:                                           ; preds = %mem_safe4
  ret void

mem_error5:                                       ; preds = %next_stmt
  %12 = call ptr @moksha_box_string(ptr @109)
  store ptr %12, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe6:                                        ; preds = %next_stmt
  %13 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 2
  %14 = icmp eq ptr %this, null
  br i1 %14, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe6
  %15 = call ptr @moksha_box_string(ptr @110)
  store ptr %15, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe8:                                        ; preds = %mem_safe6
  %16 = getelementptr inbounds nuw %Player, ptr %this, i32 0, i32 2
  store i32 %10, ptr %13, align 4
  %17 = load i32, ptr @__exception_flag, align 4
  %18 = icmp ne i32 %17, 0
  br i1 %18, label %unwind10, label %next_stmt9

next_stmt9:                                       ; preds = %mem_safe8
  ret void

unwind10:                                         ; preds = %mem_safe8
  ret void
}

define double @Account_getBalance(ptr %this) {
entry:
  %0 = icmp eq ptr %this, null
  br i1 %0, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %1 = call ptr @moksha_box_string(ptr @127)
  store ptr %1, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret double 0.000000e+00

mem_safe:                                         ; preds = %entry
  %2 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  %3 = load double, ptr %2, align 8
  ret double %3
}

define void @Account_setBalance(ptr %this, double %val) {
entry:
  %val1 = alloca double, align 8
  store double %val, ptr %val1, align 8
  %0 = load double, ptr %val1, align 8
  %fcmptmp = fcmp oge double %0, 0.000000e+00
  br i1 %fcmptmp, label %then, label %else

then:                                             ; preds = %entry
  %1 = load double, ptr %val1, align 8
  %2 = icmp eq ptr %this, null
  br i1 %2, label %mem_error, label %mem_safe

else:                                             ; preds = %entry
  %3 = call ptr @moksha_box_string(ptr @130)
  %print_unbox = call ptr @moksha_unbox_string(ptr %3)
  %4 = call i32 (ptr, ...) @printf(ptr @131, ptr %print_unbox)
  %5 = load i32, ptr @__exception_flag, align 4
  %6 = icmp ne i32 %5, 0
  br i1 %6, label %unwind5, label %next_stmt4

ifcont:                                           ; preds = %next_stmt4, %next_stmt
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind7, label %next_stmt6

mem_error:                                        ; preds = %then
  %9 = call ptr @moksha_box_string(ptr @128)
  store ptr %9, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %then
  %10 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  %11 = icmp eq ptr %this, null
  br i1 %11, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %12 = call ptr @moksha_box_string(ptr @129)
  store ptr %12, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe3:                                        ; preds = %mem_safe
  %13 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  store double %1, ptr %10, align 8
  %14 = load i32, ptr @__exception_flag, align 4
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe3
  br label %ifcont

unwind:                                           ; preds = %mem_safe3
  ret void

next_stmt4:                                       ; preds = %else
  br label %ifcont

unwind5:                                          ; preds = %else
  ret void

next_stmt6:                                       ; preds = %ifcont
  ret void

unwind7:                                          ; preds = %ifcont
  ret void
}

define void @Account_printInfo(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @132)
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @133)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 3
  %4 = load ptr, ptr %3, align 8
  %5 = call ptr @moksha_string_concat(ptr %0, ptr %4)
  %6 = call ptr @moksha_box_string(ptr @134)
  %7 = call ptr @moksha_string_concat(ptr %5, ptr %6)
  %8 = icmp eq ptr %this, null
  br i1 %8, label %mem_error1, label %mem_safe2

mem_error1:                                       ; preds = %mem_safe
  %9 = call ptr @moksha_box_string(ptr @135)
  store ptr %9, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe2:                                        ; preds = %mem_safe
  %10 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 2
  %11 = load ptr, ptr %10, align 8
  %12 = call ptr @moksha_string_concat(ptr %7, ptr %11)
  %13 = call ptr @moksha_box_string(ptr @136)
  %14 = call ptr @moksha_string_concat(ptr %12, ptr %13)
  %15 = icmp eq ptr %this, null
  br i1 %15, label %mem_error3, label %mem_safe4

mem_error3:                                       ; preds = %mem_safe2
  %16 = call ptr @moksha_box_string(ptr @137)
  store ptr %16, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe4:                                        ; preds = %mem_safe2
  %17 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  %18 = load double, ptr %17, align 8
  %19 = call ptr @moksha_double_to_str(double %18)
  %20 = call ptr @moksha_box_string(ptr %19)
  %21 = call ptr @moksha_string_concat(ptr %14, ptr %20)
  %print_unbox = call ptr @moksha_unbox_string(ptr %21)
  %22 = call i32 (ptr, ...) @printf(ptr @138, ptr %print_unbox)
  %23 = load i32, ptr @__exception_flag, align 4
  %24 = icmp ne i32 %23, 0
  br i1 %24, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe4
  ret void

unwind:                                           ; preds = %mem_safe4
  ret void
}

define void @Account_constructor(ptr %this, ptr %owner, double %initial) {
entry:
  %initial2 = alloca double, align 8
  %owner1 = alloca ptr, align 8
  %0 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  store double 0.000000e+00, ptr %0, align 8
  %1 = call ptr @moksha_box_string(ptr @121)
  %2 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 2
  store ptr %1, ptr %2, align 8
  %3 = call ptr @moksha_box_string(ptr @122)
  %4 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 3
  store ptr %3, ptr %4, align 8
  store ptr %owner, ptr %owner1, align 8
  store double %initial, ptr %initial2, align 8
  %5 = load ptr, ptr %owner1, align 8
  %6 = icmp eq ptr %this, null
  br i1 %6, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %7 = call ptr @moksha_box_string(ptr @123)
  store ptr %7, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %8 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 3
  %9 = icmp eq ptr %this, null
  br i1 %9, label %mem_error3, label %mem_safe4

mem_error3:                                       ; preds = %mem_safe
  %10 = call ptr @moksha_box_string(ptr @124)
  store ptr %10, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe4:                                        ; preds = %mem_safe
  %11 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 3
  store ptr %5, ptr %8, align 8
  %12 = load i32, ptr @__exception_flag, align 4
  %13 = icmp ne i32 %12, 0
  br i1 %13, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe4
  %14 = load double, ptr %initial2, align 8
  %15 = icmp eq ptr %this, null
  br i1 %15, label %mem_error5, label %mem_safe6

unwind:                                           ; preds = %mem_safe4
  ret void

mem_error5:                                       ; preds = %next_stmt
  %16 = call ptr @moksha_box_string(ptr @125)
  store ptr %16, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe6:                                        ; preds = %next_stmt
  %17 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  %18 = icmp eq ptr %this, null
  br i1 %18, label %mem_error7, label %mem_safe8

mem_error7:                                       ; preds = %mem_safe6
  %19 = call ptr @moksha_box_string(ptr @126)
  store ptr %19, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe8:                                        ; preds = %mem_safe6
  %20 = getelementptr inbounds nuw %Account, ptr %this, i32 0, i32 1
  store double %14, ptr %17, align 8
  %21 = load i32, ptr @__exception_flag, align 4
  %22 = icmp ne i32 %21, 0
  br i1 %22, label %unwind10, label %next_stmt9

next_stmt9:                                       ; preds = %mem_safe8
  ret void

unwind10:                                         ; preds = %mem_safe8
  ret void
}

define void @Savings_constructor(ptr %this, ptr %owner) {
entry:
  %owner1 = alloca ptr, align 8
  store ptr %owner, ptr %owner1, align 8
  %0 = load ptr, ptr %owner1, align 8
  %1 = icmp eq ptr %this, null
  br i1 %1, label %mem_error, label %mem_safe

mem_error:                                        ; preds = %entry
  %2 = call ptr @moksha_box_string(ptr @153)
  store ptr %2, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe:                                         ; preds = %entry
  %3 = getelementptr inbounds nuw %Savings, ptr %this, i32 0, i32 3
  %4 = icmp eq ptr %this, null
  br i1 %4, label %mem_error2, label %mem_safe3

mem_error2:                                       ; preds = %mem_safe
  %5 = call ptr @moksha_box_string(ptr @154)
  store ptr %5, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe3:                                        ; preds = %mem_safe
  %6 = getelementptr inbounds nuw %Savings, ptr %this, i32 0, i32 3
  store ptr %0, ptr %3, align 8
  %7 = load i32, ptr @__exception_flag, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %mem_safe3
  %9 = call ptr @moksha_box_string(ptr @155)
  %10 = icmp eq ptr %this, null
  br i1 %10, label %mem_error4, label %mem_safe5

unwind:                                           ; preds = %mem_safe3
  ret void

mem_error4:                                       ; preds = %next_stmt
  %11 = call ptr @moksha_box_string(ptr @156)
  store ptr %11, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe5:                                        ; preds = %next_stmt
  %12 = getelementptr inbounds nuw %Savings, ptr %this, i32 0, i32 2
  %13 = icmp eq ptr %this, null
  br i1 %13, label %mem_error6, label %mem_safe7

mem_error6:                                       ; preds = %mem_safe5
  %14 = call ptr @moksha_box_string(ptr @157)
  store ptr %14, ptr @__exception_val, align 8
  store i32 1, ptr @__exception_flag, align 4
  ret void

mem_safe7:                                        ; preds = %mem_safe5
  %15 = getelementptr inbounds nuw %Savings, ptr %this, i32 0, i32 2
  store ptr %9, ptr %12, align 8
  %16 = load i32, ptr @__exception_flag, align 4
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %unwind9, label %next_stmt8

next_stmt8:                                       ; preds = %mem_safe7
  ret void

unwind9:                                          ; preds = %mem_safe7
  ret void
}

define void @Mamal_breathe(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @162)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @163, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Mamal_constructor(ptr %this) {
entry:
  ret void
}

define void @Winged_fly(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @164)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @165, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Winged_constructor(ptr %this) {
entry:
  ret void
}

define void @Bat_screech(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @166)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @167, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Bat_constructor(ptr %this) {
entry:
  call void @Mamal_constructor(ptr %this)
  ret void
}

define void @Vehicle_start(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @173)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @174, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Vehicle_constructor(ptr %this) {
entry:
  ret void
}

define void @Car_honk(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @175)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @176, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @Car_constructor(ptr %this) {
entry:
  call void @Vehicle_constructor(ptr %this)
  ret void
}

define void @SportsCar_turbo(ptr %this) {
entry:
  %0 = call ptr @moksha_box_string(ptr @177)
  %print_unbox = call ptr @moksha_unbox_string(ptr %0)
  %1 = call i32 (ptr, ...) @printf(ptr @178, ptr %print_unbox)
  %2 = load i32, ptr @__exception_flag, align 4
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %unwind, label %next_stmt

next_stmt:                                        ; preds = %entry
  ret void

unwind:                                           ; preds = %entry
  ret void
}

define void @SportsCar_constructor(ptr %this) {
entry:
  call void @Car_constructor(ptr %this)
  ret void
}

define void @Node_constructor(ptr %this) {
entry:
  %0 = getelementptr inbounds nuw %Node, ptr %this, i32 0, i32 1
  store i32 0, ptr %0, align 4
  ret void
}

define void @ListNode_constructor(ptr %this) {
entry:
  call void @Node_constructor(ptr %this)
  %0 = getelementptr inbounds nuw %ListNode, ptr %this, i32 0, i32 2
  store ptr null, ptr %0, align 8
  ret void
}

define void @Shape_constructor(ptr %this) {
entry:
  %0 = getelementptr inbounds nuw %Shape, ptr %this, i32 0, i32 1
  store double 0.000000e+00, ptr %0, align 8
  ret void
}

define void @Square_constructor(ptr %this) {
entry:
  call void @Shape_constructor(ptr %this)
  %0 = getelementptr inbounds nuw %Square, ptr %this, i32 0, i32 2
  store double 0.000000e+00, ptr %0, align 8
  ret void
}

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
