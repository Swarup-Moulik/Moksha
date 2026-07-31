# Moksha Language Handbook

> **Moksha** is a statically typed systems programming language with a custom multi-stage compilation pipeline, bridging performance with flexibility and safety.

> **Status:** Active development. Features may change; incomplete implementations may exist.

---

## Table of Contents

1. [Lexical & Syntax](#1-lexical--syntax)
2. [Types & Layout](#2-types--layout)
3. [Control Flow](#3-control-flow)
4. [Functions & Closures](#4-functions--closures)
5. [Memory & Ownership](#5-memory--ownership)
6. [Concurrency & Async](#6-concurrency--async)
7. [Exceptions & Panics](#7-exceptions--panics)
8. [FFI & System](#8-ffi--system)
9. [Modules & Macros](#9-modules--macros)
10. [Built-in Functions](#10-built-in-functions)

---

## 1. Lexical & Syntax

### 1.1 Comments

```moksha
// Line comment

/* Block comment */

/* Block comments support nesting
   /* Like this */
   The outer comment continues after the inner closes. */
```

### 1.2 Integer Literals

Integers support decimal, hexadecimal, octal, and binary forms, with optional type suffixes and underscore separators for readability.

```moksha
int decimal = 42;
int hex = 0xFF;
int octal = 0o755;
int binary = 0b1010_1010;

// Underscore separators
int million = 1_000_000;

// Type suffixes
char a = 123i8;
unsigned short b = 0xFFu16;
unsigned int c = 0b1010_1010u32;
long d = 1_000_000i64;
```

### 1.3 Float Literals

```moksha
float pi = 3.14159;
double sci_pos = 1.23e10;
double sci_neg = 4.56E-8;
float trailing = 10.;
float leading = .5;
float separated = 1_000.50_5;
```

### 1.4 String Literals

```moksha
string basic = "Hello\nWorld\t\r\\"";

// UTF-8 and emoji are native
string unicode = "Hello, 世界! 🌍🚀";

// Hex and Unicode escapes
string hex = "\x41\x42\x43";       // "ABC"
string emoji_esc = "\u{1F600}";     // Grinning face

// Raw strings (no escape processing)
string raw = "C:\\dev\\moksha\\tests";

// Template strings (string interpolation)
int val = 20;
string templated = `Value: ${val}`;
string computed = `Calculation: ${10 + 20}`;
```

### 1.5 Operators

```moksha
// Arithmetic: +  -  *  /  %  **
// Bitwise:    &  |  ^  ~  <<  >>
// Comparison: ==  !=  <  <=  >  >=
// Logical:    &&  ||  !
// Assignment: =  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
// Increment/Decrement:  a++  a--
```

### 1.6 Pipe Operator

The pipe operator `|>` passes the left-hand expression as the first argument to the right-hand function call. Chains evaluate strictly left-to-right.

```moksha
"hello" |> mock_length() |> println();

int res = (true ? 100 : 200) |> processNumber();
```

**Rules:**

- The right side must be a callable (function call or identifier).
- Type mismatch, arity mismatch, or void propagation causes a compile-time error.
- Precedence: `10 + 5 |> process()` lowers to `process(15)`.

---

## 2. Types & Layout

### 2.1 Primitive Types

| Type                                              | Description                               |
| ------------------------------------------------- | ----------------------------------------- |
| `int`, `long`, `char`, `short`                    | Signed integers                           |
| `unsigned int`, `unsigned char`, `unsigned short` | Unsigned integers                         |
| `float`, `double`, `half`, `quarter`              | Floating-point                            |
| `bool`                                            | Boolean (`true` / `false`)                |
| `string`                                          | UTF-8 string                              |
| `usize`, `isize`                                  | Platform-sized unsigned/signed integer    |
| `any`                                             | Dynamic type (holds any value at runtime) |

### 2.2 Nullable Types

Append `?` to any type to make it nullable.

```moksha
int? maybe = 42;
int? nothing = null;

// Explicit unwrap required; no implicit narrowing
int definite = maybe;          // TypeError: T? and T are distinct types
int unwrapped = maybe ?? 0;    // Null-coalescing operator
```

### 2.3 Arrays

```moksha
// Fixed-size arrays (exact compile-time length required)
int[5] fixed = [1, 2, 3, 4, 5];
int[2][2] matrix = [[1, 0], [0, 1]];

// Dynamic arrays (slices)
int[] dynamic = [10, 20, 30];
string[] names = ["Alice", "Bob"];

// Array operations
dynamic.push(40);
dynamic.pop();
dynamic.insert(1, 15);
dynamic.remove(0);
dynamic.sort();
dynamic.reverse();
dynamic.fill(0);
int[] cloned = dynamic.clone();
int[] sliced = dynamic.slice(1, 3);
dynamic.clear();
```

### 2.4 Tables (Maps)

```moksha
table<string, int> scores = {
    "Alice": 100,
    "Bob": 95
};

println(scores["Alice"]);

// Built-ins
scores.has("Alice");   // true
scores.length();        // 2
scores.remove("Bob");
scores.clear();
```

### 2.5 Structs

Stack-allocated, C-compatible memory layout.

```moksha
struct StandardHeader {
    int version;
    unsigned int id;
}

packed struct metadata {       // No padding between fields
    unsigned int sensor_id;
    int timestamp;
    unsigned char active;
}

align(4) struct aligned_data { // Custom alignment
    int value;
}
```

### 2.6 Unions

Tagless memory overlays. Reading an inactive field is undefined behavior; requires `unsafe`.

```moksha
union ValueOverlay {
    int int_val;
    float float_val;
}

unsafe {
    ValueOverlay data;
    data.int_val = 0x3F800000;  // Bit pattern for 1.0f
    float check = data.float_val;  // 1.0
}
```

### 2.7 Bitfields

Width must not exceed the base integer type's bit width.

```moksha
struct Flags {
    int present : 1;
    int mode    : 3;
    int type    : 4;
}
```

### 2.8 Classes

```moksha
// Value class (stack-allocated by default)
class Vector2 {
    public int x;
    public int y;

    constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

// Reference class (heap-allocated, ARC-managed)
ref class Player {
    public string name;

    constructor(string name) {
        this.name = name;
    }
}

Vector2 v = new Vector2(10, 20);        // Stack
Player p = new Player("Hero");          // Heap (ARC)
shared Vector2 heapV = new Vector2(30, 40);  // Forced heap (ARC)

// Zero-initialization: new with no args zeroes all fields
Vector2 zeroed = new Vector2();
println(zeroed.x); // 0
```

### 2.9 Inheritance

Only `ref class` can inherit from `ref class`. Value classes cannot inherit from ref classes and vice versa.

```moksha
ref class Entity {
    public string id;
    constructor(string id) { this.id = id; }
}

ref class Player(Entity) {
    public int health;
    constructor(string name, int hp) {
        super(name);
        this.health = hp;
    }
    void takeDamage(int amount) { this.health -= amount; }
}

// Generic inheritance
generic <T>
ref class Box(BaseBox) {
    public T item;
    constructor(int cap, T item) {
        super(cap);
        this.item = item;
    }
    T getItem() { return this.item; }
}
```

### 2.10 Enums

```moksha
enum DeviceState { IDLE, BUSY, ERROR, DISCONNECTED }
enum Status { Ready = 5, Busy = 10 }

Status s = Status.Ready;

// Comparison and arithmetic require explicit cast
if (cast<int>(s) == 5) { println("Match"); }
int offset = cast<int>(Status.Busy) + 2;  // 12
```

### 2.11 Generics

```moksha
generic Box<T> {
    T value;
    void update(T newValue) {
        this.value = newValue;
    }
}

Box<int> intBox = new Box<int>();
intBox.update(100);

Box<string> strBox = new Box<string>();
strBox.update("Hello!");
```

Generics are strictly **invariant** by default: `Box<Dog>` is not assignable to `Box<Animal>`.

### 2.12 The `any` Type

A dynamic type that can hold any value at runtime.

```moksha
any dyn_var;

dyn_var = null;
dyn_var = true;
dyn_var = 42;
dyn_var = 3.14;
dyn_var = "Hello";
dyn_var = [1, 2, 3];

// Casting from `any` back to concrete type
any val = 10;
int x = int(val);        // Runtime cast; tag mismatch causes CastException
```

### 2.13 Bitfields

```moksha
struct BitfieldTest {
    int present : 1;
    int mode    : 3;
}
```

Width must not exceed the base type's bit count. Floats are not allowed as base types.

---

## 3. Control Flow

### 3.1 If / Else

```moksha
if (condition) {
    // ...
} else if (otherCondition) {
    // ...
} else {
    // ...
}
```

### 3.2 Ternary

```moksha
int result = (x > 10) ? x : 10;
```

### 3.3 Switch

Requires explicit `break` per case (no implicit fallthrough). Supports ranges and enum exhaustiveness checking.

```moksha
switch (val) {
    case 1: println("One"); break;
    case 10:20: println("Range 10 to 20"); break;  // Range syntax
    default: println("Other"); break;
}

// Enum switch with exhaustiveness
enum State { Start, Processing, End }
switch (state) {
    case State.Start:      println("Starting"); break;
    case State.Processing: println("Working"); break;
    case State.End:        println("Done"); break;
}
```

Empty cases fall through to the next case.

```moksha
switch (state) {
    case DeviceState.IDLE:
        println("Ready");
        break;
    case DeviceState.BUSY:
    case DeviceState.ERROR:    // Falls through from BUSY
        println("Action required");
        break;
}
```

### 3.4 While Loop

```moksha
while (condition) {
    // ...
}
```

### 3.5 For Loop

```moksha
for (int i = 0; i < 10; i++) {
    println(i);
}
```

### 3.6 For-In Loop

```moksha
int[] list = [1, 2, 3];
for (int x in list) {
    println(x);
}
```

**Iteration behavior by collection type and variable count:**

**String** (1 variable only — yields characters):

```moksha
for (char c in "ABC") { println(c); }  // A, B, C
```

**Array** (1 variable = value, 2 variables = index + value):

```moksha
int[] nums = [10, 20, 30];

// 1 variable: values only
for (int val in nums) { println(val); }        // 10, 20, 30

// 2 variables: index + value
for (int idx, int val in nums) { println(idx); } // 0, 1, 2
```

**Table** (1 variable = key only, 2 variables = key + value):

```moksha
table<string, int> scores = {"Alice": 90, "Bob": 85};

// 1 variable: keys only
for (string name in scores) { println(name); }        // Alice, Bob

// 2 variables: key + value
for (string key, any val in scores) { println(val); }  // 90, 85
```

### 3.7 Break & Continue

```moksha
while (true) {
    if (done) break;
    if (skip) continue;
}
```

### 3.8 Defer

`defer` registers a cleanup statement that executes before the function returns. Defers execute in LIFO (Last-In, First-Out) order.

```moksha
void test_defer() {
    int x = 10;
    defer println("Second");   // Executes last
    defer println("First");    // Executes second
    println("Normal");         // Executes first

    // Also runs on early return
    if (x > 5) return;
    println("Unreachable");
}
```

### 3.9 Definite Assignment

Variables must be definitely assigned on all paths before use. The compiler traces through control flow graphs including `if`, `switch`, `try/catch`, and loops.

```moksha
int x;
if (cond) {
    x = 10;
} else {
    x = 20;
}
println(x);  // OK: assigned on all paths
```

---

## 4. Functions & Closures

### 4.1 Function Declarations

```moksha
// Named function
int add(int a, int b) {
    return a + b;
}

// Void function
void greet(string name) {
    println(`Hello, ${name}!`);
}

// Default parameters
void processData(string data, int retries = 3, int timeout = 5000) {
    // ...
}
```

### 4.2 Closures

```moksha
// Typed closure
closure(int, int) -> int addFunc = (int a, int b) => {
    return a + b;
};

// Short-form closure
closure() -> int counter = &() => {
    return 42;
};

println(addFunc(50, 50));  // 100
```

Closures capture variables by reference. Mutating a captured variable from both the closure and the outer scope is a compile-time borrow error.

### 4.3 Operator Overloading

```moksha
class Vector2 {
    public int x;
    public int y;
    constructor(int x, int y) { this.x = x; this.y = y; }

    Vector2 operator+(Vector2 other) {
        return new Vector2(this.x + other.x, this.y + other.y);
    }

    bool operator==(Vector2 other) {
        return this.x == other.x && this.y == other.y;
    }

    Vector2 operator-() {  // Unary negation
        return new Vector2(-this.x, -this.y);
    }
}

Vector2 v3 = v1 + v2;
Vector2 neg = -v1;
```

### 4.4 Method Overloading

Methods can be overloaded by parameter types or count.

```moksha
void process(int x) { println(x); }
void process(string s) { println(s); }

process(42);       // Calls int version
process("hello");  // Calls string version
```

### 4.5 Higher-Order Functions & Recursion

Functions are first-class citizens and can be passed as arguments or returned from other functions. Mutual recursion is supported.

```moksha
int apply(int x, closure(int) -> int fn) {
    return fn(x);
}

int double(int x) { return x * 2; }

println(apply(5, double));  // 10
```

---

## 5. Memory & Ownership

Moksha uses **Automatic Reference Counting (ARC)** paired with **Non-Lexical Lifetime (NLL) borrow checking** for deterministic memory management. There is no garbage collector.

### 5.1 Storage Qualifiers

```moksha
int x = 10;           // Mutable
lock int y = 30;      // Thread-safe (exclusive lock access)
```

### 5.2 Pointer Types

Pointers are immutable by default, but can be made mutable using `*mut`.

| Syntax    | Meaning                              |
| --------- | ------------------------------------ |
| `*mut T`  | Mutable raw pointer                  |
| `*view T` | Immutable view pointer (read-only)   |
| `*lock T` | Thread-safe pointer (exclusive lock) |

```moksha
int val = 42;
*mut int ptr = &val;

unsafe {
    *ptr = 100;
    println(*ptr);
}

*view int viewPtr = &val;     // Read-only
*lock int lockPtr = &val;     // Thread-safe
```

### 5.3 Reference Types

| Syntax   | Meaning             |
| -------- | ------------------- |
| `&T`     | Immutable reference |
| `&mut T` | Mutable reference   |

```moksha
int val = 100;
*mut int mutPtr = &mut val;
*view int viewPtr = &val;
```

### 5.4 NLL Borrow Rules

- **Single-writer OR multiple-readers** at any given time (no overlapping `*mut` and `*view` on the same data).
- Borrows end at their last use, not at scope exit.
- Returning a pointer to a local variable is a compile error.
- Mutable pointers across `await` suspension points are rejected.

```moksha
int data = 10;
*mut int m1 = &data;
*mut int m2 = &data;  // ERROR: second mutable borrow while m1 is alive
```

### 5.5 Lock Blocks

`lock` blocks elevate a `*lock` pointer to `*mut` for the duration of the block.

```moksha
lock int shared_data = 300;
*lock int shared_ptr = &shared_data;

lock (shared_ptr) {
    *shared_ptr = *shared_ptr + 1;  // Elevated to *mut inside block
}
```

Reentrancy is forbidden: locking an already-locked pointer causes a compile error.

### 5.6 Scope Release & Drop Order

Destructors and `defer` statements execute in reverse order (LIFO). NLL ensures resources are released at the point of last use.

```moksha
void test_lifetime() {
    string s = "hello";
    *view string v = &s;
    int read = *v;   // v used here
    // v is no longer alive after this point
    consume(s);       // OK: s can be moved here
}
```

### 5.7 ARC (Automatic Reference Counting)

ARC manages the lifetime of heap-allocated objects. The compiler inserts retain/release calls. ARC optimizations include:

- **Pair elision**: adjacent retain+release pairs are optimized out.
- **Loop elision**: retain/release inside tight loops are hoisted.
- **Early return**: ARC correctly cleans up on early returns.
- **Exception paths**: ARC handles unwinding correctly.

```moksha
// Shared heap value with ARC
shared Vector2 heapV = new Vector2(30, 40);
```

### 5.8 Weak References

`weak` prevents reference cycles in ARC graphs.

```moksha
// Weak async references do not create strong reference cycles
weak async string fetchWeakData() {
    return "Weak Data";
}
```

---

## 6. Concurrency & Async

### 6.1 Async Functions

```moksha
async string fetchData() {
    return "Data fetched";
}

async void run() {
    string data = await fetchData();
    println(data);
}

run();
```

### 6.2 Weak Async

Weak async functions return promises that do not create strong reference cycles.

```moksha
weak async string fetchWeakData() {
    return "Weak Data";
}
```

### 6.3 Threads

```moksha
new thread(() => {
    println("Background thread");
});

new weak thread(() => {
    println("Weak background thread");
});
```

### 6.4 Thread-Local Storage

```moksha
static thread_local int cpu_id = 0;
static thread_local int[10] kernel_gs_base;
```

### 6.5 Atomics

```moksha
unsigned int data = 0_u32;
*mut unsigned int ptr = &data;

atomic_store(ptr, 100_u32);
unsigned int val = atomic_load(ptr);

unsigned int old = atomic_add(ptr, 10_u32);
unsigned int current = atomic_cas(ptr, 110_u32, 200_u32);

atomic_fence_acquire();
atomic_fence_release();
atomic_fence_seqcst();
```

### 6.6 Lock Elevation

Inside a `lock` block, `*lock` pointers are elevated to `*mut` capabilities.

```moksha
lock int shared_data = 100;
*lock int p = &shared_data;

lock (p) {
    *p = 200;              // Elevated to *mut
    *view int v = p;       // Safe to take a view inside the lock
}
```

### 6.7 Channels

```moksha
shared Channel<int> ch = new Channel<int>(2);  // Ring buffer, capacity 2
shared Channel<int> ch_alias = ch;

spawn(async () => {
    await ch_alias.send(100);
    await ch_alias.send(200);
});

int val1 = await ch.recv();  // Suspends if queue is empty
int val2 = await ch.recv();
```

### 6.8 Promises, Spawn, Join, Select

```moksha
// Spawn returns a promise
promise<void> t1 = spawn(async () => {
    await sleep(10);
});

promise<int> a = spawn(async () => { await sleep(100); return 1; });
promise<int> b = spawn(async () => { await sleep(10); return 2; });

// Join: wait for all promises
int[] results = await join(a, b);

// Select: wait for first promise to complete
int winner = await select(a, b);

// Cancel a running task
promise<void> task = spawn(infinite_task());
cancel(task);
```

### 6.9 Async Mutex

```moksha
AsyncMutex mtx = new AsyncMutex();
int shared_resource = 0;

spawn(async () => {
    async lock(mtx) {
        shared_resource = shared_resource + 1;
        await sleep(10);  // Safe to suspend while holding AsyncMutex
    }
});
```

### 6.10 Sleep, Yield, Timeout

```moksha
await sleep(50);     // Suspend for 50ms
await yield();       // Yield to scheduler (cooperative)

promise<int> result = await timeout(some_long_task(), 100);  // 100ms timeout
```

> **Note:** `async_scope` / `TaskGroup` and structured concurrency patterns are not yet implemented.

---

## 7. Exceptions & Panics

### 7.1 Throw & Try/Catch/Finally

Throw any value. Catch by type. `any` is the catch-all.

```moksha
void riskyOperation() {
    if (true) throw "Something went wrong";
}

try {
    riskyOperation();
} catch (string e) {
    println(`Caught: ${e}`);
} catch (int e) {
    println(`Caught int: ${e}`);
} catch (any e) {
    println(`Caught any: ${e}`);
} finally {
    println("Always executes");
}
```

**Rules:**

- `catch (any e)` must come last; placing it before more specific handlers is a compile error.
- Throwing inside a `finally` block overrides the in-flight exception.
- Double-fault (throwing while unwinding) causes an immediate runtime panic.
- Uncaught exceptions print a stack trace and exit with non-zero status.

> **Note:** `throw new Exception(...)` and `catch (Exception e)` are not yet implemented. Exceptions are thrown as plain values (`throw 404`, `throw "error"`) and caught by their runtime type.

### 7.2 Panic

Runtime panics trigger immediate termination with a diagnostic message.

```moksha
// Index out of bounds
int[] arr = [1, 2, 3];
// arr[100];   // Runtime panic: index out of bounds

// Null pointer dereference
int?[] null_arr = null;
// null_arr[0];  // Runtime panic

// Key not found
table<string, int> scores = {"Alice": 100};
// scores["Unknown"];  // Runtime panic
```

### 7.3 Unhandled Async Rejections

When an unawaited async promise is rejected and its ARC handle drops, the runtime panics with "Unhandled Promise Rejection".

---

## 8. FFI & System

### 8.1 External Function Binding

```moksha
// Link against the C standard library
extern "C" {
    int printf(char* fmt, ...);
    void* malloc(usize size);
    void free(void* ptr);
}

// Link against a specific library
extern "libcurl" {
    int curl_easy_init();
}
```

### 8.2 Calling Conventions

```moksha
extern "C" void c_func();
extern "stdcall" void win32_func();
extern "fastcall" void perf_func();
```

### 8.3 Unsafe Blocks

Raw pointer dereferences and certain operations require `unsafe` blocks.

```moksha
unsafe {
    *mut int ptr = cast<*mut int>(0x1000);
    *ptr = 42;
}
```

### 8.4 Volatile Pointers

For memory-mapped I/O (MMIO) and hardware register access.

```moksha
// Cast a hardware address to a volatile pointer
*mut volatile unsigned int uart0 = cast<*mut volatile unsigned int>(hardware_addr);
*uart0 = 0x01;  // Volatile write prevents compiler reordering

// Or volatile on the pointer itself
volatile *mut int hw_reg = cast<volatile *mut int>(0x40022000);
*hw_reg = 0x01;

// Volatile local variable with non-volatile pointer
volatile int target = 0;
*mut volatile int p_vol = &target;
```

### 8.5 Linker Attributes

```moksha
section(".text") void in_custom_section() {}

weak int optional_symbol = 0;

used void ensure_emitted() {}

pure int compute(int x) { return x * 2; }  // No side effects; optimizer can fold
```

### 8.6 Interrupt / ISR / Naked Functions

```moksha
interrupt void timer_isr() {
    // Interrupt handler body
}

naked void bare_entry() {
    asm("ret");
}

noreturn void halt() {
    while (true) {}
}
```

### 8.7 Inline Assembly

```moksha
// Basic asm
asm("cli");
asm("1: hlt; jmp 1b");

// With clobbers
asm("mov $$42, %rax") clobber("rax", "rbx") volatile;

// With inout operands
int x = 10;
asm("add $$1, $0") inout("+r"(x)) clobber("cc") volatile;
println(x);  // 11

// With in-only operands
int in_val = 5;
int out_val = 0;
asm("mov $1, ($0)") in("r"(&out_val)) in("r"(in_val));
println(out_val);  // 5
```

### 8.8 Intrinsics

```moksha
unsigned int data = 0x000000FF_u32;
unsigned int reversed = bswap32(data);       // Byte-swap
int leading = clz(data);                     // Count leading zeros
usize size = sizeof(int);                    // Type size in bytes
float f = bitcast<float>(raw_bits);          // Reinterpret bits (no conversion)
```

### 8.9 Cross-Compilation

```bash
mokshac source.mox -target x86_64-pc-linux-gnu -o output
mokshac source.mox -target aarch64-linux-android -o output_android
mokshac source.mox -target wasm32-wasi -o output.wasm
```

---

## 9. Modules & Macros

### 9.1 Module Imports

```moksha
// Full module import (unquoted)
import test
println(test.calculate_magic(10, 20));

// Full module import with an alias
import test as t
println(t.calculate_magic(10, 20));

// Import specific symbols from a file
import { test_multiplier, calculate_magic, TestConfig } from "test"

// Cross-folder imports
import { Helper } from "../utils/helper"
```

### 9.2 Exporting Symbols

Mark symbols with `public` to make them importable.

```moksha
// test.mox
public int test_multiplier = 5;

public int calculate_magic(int a, int b) {
    return (a + b) * test_multiplier;
}

public class TestConfig {
    public int version;
    constructor(int v) { this.version = v; }
}
```

### 9.3 Visibility Modifiers

| Modifier    | Meaning                                             |
| ----------- | --------------------------------------------------- |
| `public`    | Accessible from any module                          |
| `protected` | Accessible only within the class and its subclasses |
| `private`   | Accessible only within the defining class           |

```moksha
ref class Base {
    public int publicField = 1;
    protected int protectedField = 2;
    private int privateField = 3;
}

ref class Derived(Base) {
    public void test() {
        println(this.publicField);     // OK
        println(this.protectedField);  // OK: visible to subclasses
        // this.privateField;          // ERROR: not visible
    }
}
```

### 9.4 Macros

Macros are hygienic text-transformations that run before semantic analysis. Internal variables are renamed to prevent name collisions with caller code.

```moksha
macro setDouble(target, val) {
    int internal_temp = val;
    target = internal_temp * 2;
}

int result = 0;
setDouble(result, 5);
println(result);  // 10

// 'internal_temp' in the caller's scope is NOT affected
int internal_temp = 100;
println(internal_temp);  // 100
```

**Macro rules:**

- Parameters must be raw identifiers (no types, no expressions).
- Duplicate parameter names are rejected.
- Bodies must form valid AST nodes.
- Recursive macros are caught at expansion time.
- Expanded bindings respect the block scope they expand into (no cross-arm leakage in `switch`).

### 9.5 Enum & Macro Interaction

```moksha
macro swap(a, b) {
    any temp = a;
    a = b;
    b = temp;
}

enum Color { Red, Green, Blue }
Color x = Color.Red;
Color y = Color.Green;
swap(x, y);  // x is now Green, y is now Red
```

---

## 10. Built-in Functions

All built-ins use standalone function-call syntax (not method syntax).

### 10.1 String Built-ins

```moksha
string s = "hello world";

at(s, 0);                        // 'h'
length(s);                        // 11
substring(s, 0, 5);              // "hello"
contains(s, "world");            // true
index(s, "world");               // 6
trim("  hello  ");               // "hello"
replace("a-b-c", "-", ":");     // "a:b:c"
split("a,b,c", ",");            // ["a", "b", "c"]
join(["x", "y", "z"], "-");     // "x-y-z"
```

### 10.2 Char Built-ins

```moksha
char c = '5';
is_digit(c);       // true
is_alpha(c);       // false
is_whitespace(c);  // false
```

### 10.3 Math Built-ins

```moksha
abs(-5.0);              // 5.0
sqrt(16.0);             // 4.0
floor(3.7);             // 3.0
ceil(3.2);              // 4.0
round(3.6);             // 4.0
sin(0.0);               // 0.0
cos(0.0);               // 1.0
log10(100.0);           // 2.0
log2(8.0);              // 3.0
is_close(sqrt(16.0), 4.0, 1e-6);  // true

// Constants
PI;    // 3.14159...
TAU;   // 6.28318... (2 * PI)
E;     // 2.71828...
NAN;
INF;
```

### 10.4 Array Built-ins

```moksha
int[] arr = [1, 2, 3];

length(arr);            // 3
at(arr, 0);             // 1
push(arr, 10);          // arr is now [1, 2, 3, 10]
pop(arr);               // returns 10
insert(arr, 1, 15);     // [1, 15, 2, 3]
remove(arr, 0);         // returns 1
contains(arr, 3);       // true
sort(arr);              // sorted in-place
reverse(arr);           // reversed in-place
clear(arr);             // []
```

### 10.5 Table (Map) Built-ins

```moksha
table<string, int> scores = {"Alice": 100, "Bob": 95};

has(scores, "Alice");       // true
length(scores);              // 2
remove(scores, "Bob");
clear(scores);
```

### 10.6 File I/O

```moksha
// Low-level
any f = open("test.txt", WRITE | CREATE);
write(f, "data");
close(f);
any data = read(f);
int s = size(f);

// High-level text
writeText("hello.txt", "Hello\n");
appendText("hello.txt", "World");
string content = readText("hello.txt");
string[] lines = readLines("hello.txt");

// High-level binary
writeBytes("data.bin", byte_array);
appendBytes("data.bin", more_bytes);
int[] bytes = readBytes("data.bin");

// Structured data
writeJson("config.json", obj);
any data = readJson("config.json");
writeCsv("data.csv", rows);
string[][] table = readCsv("data.csv");

// Filesystem
exists("data.txt");             // true
createDir("new_dir");
isDir("new_dir");               // true
copy("src.txt", "dst.txt");
move_file("old.txt", "new.txt");
remove("tmp.txt");
removeDir("dir_to_remove");
string[] files = listDir("some_dir");
```

### 10.7 Manual Memory Management

```moksha
unsafe {
    void* ptr = malloc(1024);
    void* zeroed = calloc(10, sizeof(int));
    void* resized = realloc(ptr, 2048);
    free(resized);
}
```

---

## Quick Reference: Compilation

```bash
# Compile a source file
mokshac source.mox -o output

# Cross-compile
mokshac source.mox -target x86_64-pc-linux-gnu -o output_linux

# Optimize (uses moksha-opt)
mokshac source.mox -o output
```

---

## Language Design Principles

1. **Deterministic Memory** -- ARC + NLL borrow checking. No GC pauses. Resources released at last use.
2. **Zero-Cost Abstractions** -- Value types are stack-allocated by default. `ref class` for heap when needed.
3. **Hygienic Macros** -- Textual expansion with scope-safe variable renaming.
4. **Safe Borrowing** -- `*mut`, `*view`, `*lock` pointers with compile-time alias analysis.
5. **FFI-Native** -- Direct C interop with `extern "C"`, calling conventions, inline assembly, and volatile pointers.
6. **Cross-Platform** -- First-class targets for Linux, Windows, macOS, Android, iOS, WebAssembly (WASI + Browser).
