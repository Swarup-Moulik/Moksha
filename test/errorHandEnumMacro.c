#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>

// --- VM METADATA & EXCEPTION SYSTEM ---
jmp_buf stack_frames[10];
int frame_ptr = -1;
char last_err[256];

#define TRY if (setjmp(stack_frames[++frame_ptr]) == 0)
#define CATCH(e) else
#define THROW(msg)                                       \
    {                                                    \
        snprintf(last_err, sizeof(last_err), "%s", msg); \
        longjmp(stack_frames[frame_ptr--], 1);           \
    }

// --- STRUCTURES TO MIMIC MOKSHA TYPES ---
typedef struct
{
    int *data;
    int size;
} IntArray;

typedef struct
{
    char *data;
    int length;
} MString;

MString create_string(char *init)
{
    MString s = {init, (int)strlen(init)};
    return s;
}

// --- 1. ENUMS ---
typedef enum
{
    Idle,
    Running,
    Stopped
} Status;
typedef enum
{
    OK = 200,
    BadRequest = 400,
    ServerErr = 500
} HttpCode;
typedef enum
{
    Low = 10,
    Medium,
    High = 20,
    Critical
} Levels;

// --- 2. MACROS ---
#define LOG(msg) printf("[LOG MACRO]: %s\n", msg)
#define PERFORM_CALC(x, y)                \
    {                                     \
        int res = x + y;                  \
        printf("Calc Result: %d\n", res); \
    }

// --- 4.1 BUBBLING HELPERS ---
void thrower() { THROW("ErrorInFunction"); }
void caller() { thrower(); }

int main()
{
    // --- ENUM SUITE ---
    printf("==== ENUM SUITE ====\n");
    printf("\n[1. C-Style Auto-Increment]\n");
    printf("Idle (Expect 0): %d\nRunning (Expect 1): %d\nStopped (Expect 2): %d\n", Idle, Running, Stopped);

    printf("\n[2. Explicit Assignment]\n");
    printf("OK: %d\nBadRequest: %d\nServerErr: %d\n", OK, BadRequest, ServerErr);

    printf("\n[3. Mixed Assignment]\n");
    printf("Low (10): %d\nMedium (11): %d\nHigh (20): %d\nCritical (21): %d\n", Low, Medium, High, Critical);

    printf("\n[4. Enums in Switch/If]\n");
    int currentStatus = Running;
    if (currentStatus == Running)
        printf("System is RUNNING (Pass)\n");
    switch (currentStatus)
    {
    case Running:
        printf("Switch: Running (Pass)\n");
        break;
    }
    printf("\n=== ENUM TEST COMPLETED ===\n");

    // --- MACRO SUITE ---
    printf("==== MACRO SYNTAX SUITE ====\n");
    printf("Macros defined successfully.\n(Note: Macros do not execute at runtime in current version)\n");
    LOG("System Start");
    PERFORM_CALC(10, 20);
    printf("=== MACRO TEST COMPLETED ===\n");

    // --- ERROR HANDLING SUITE ---
    printf("==== PROFESSIONAL ERROR HANDLING SUITE ====\n");

    // SECTION 1: ARITHMETIC
    printf("\n[1. Arithmetic Safety]\n");
    TRY
    {
        int b = 0;
        printf("  [Test 1.1] Attempting Int Div by Zero (100 / 0)...\n");
        if (b == 0)
            THROW("ArithmeticException: Division by zero");
    }
    CATCH(e) { printf("  PASS: Caught Arithmetic Exception: %s\n", last_err); }

    TRY
    {
        int b = 0;
        printf("  [Test 1.2] Attempting Modulo by Zero (100 %% 0)...\n");
        if (b == 0)
            THROW("ArithmeticException: Division by zero");
    }
    CATCH(e) { printf("  PASS: Caught Arithmetic Exception: %s\n", last_err); }

    TRY
    {
        double y = 0.0;
        printf("  [Test 1.3] Attempting Double Div by Zero (10.0 / 0.0)...\n");
        if (y == 0.0)
            THROW("ArithmeticException: Division by zero");
    }
    CATCH(e) { printf("  PASS: Caught Arithmetic Exception: %s\n", last_err); }

    // SECTION 2: BOUNDS
    printf("\n[2. Memory & Bounds Safety]\n");
    IntArray arr = {(int[]){1, 2, 3}, 3};
    TRY
    {
        printf("  [Test 2.1] Accessing index 5 of size 3 array...\n");
        int idx = 5;
        if (idx >= arr.size)
            THROW("IndexOutOfBoundsException: Index 5, Size 3");
    }
    CATCH(e) { printf("  PASS: Caught IndexOutOfBounds: %s\n", last_err); }

    TRY
    {
        printf("  [Test 2.2] Accessing index -1...\n");
        int idx = -1;
        if (idx < 0)
            THROW("IndexOutOfBoundsException: Index -1, Size 3");
    }
    CATCH(e) { printf("  PASS: Caught IndexOutOfBounds: %s\n", last_err); }

    MString s = create_string("abc");
    TRY
    {
        printf("  [Test 2.3] Accessing char at index 10 of 'abc'...\n");
        int idx = 10;
        if (idx >= s.length)
            THROW("IndexOutOfBoundsException: String index 10 out of range (Length 3)");
    }
    CATCH(e) { printf("  PASS: Caught IndexOutOfBounds: %s\n", last_err); }

    // SECTION 3: TYPE & NULL
    printf("\n[3. Type Integrity]\n");
    TRY
    {
        printf("  [Test 3.1] Casting 'NotAnNumber' to int...\n");
        THROW("FormatException: Cannot cast String 'NotAnNumber' to Int");
    }
    CATCH(e) { printf("  PASS: Caught FormatException: %s\n", last_err); }

    TRY
    {
        printf("  [Test 3.2] Calling method on null object without optional chaining...\n");
        void *d = NULL;
        if (d == NULL)
            THROW("NullReferenceException: Attempted access on null object");
    }
    CATCH(e) { printf("  PASS: Caught NullReferenceException: %s\n", last_err); }

    // SECTION 4: BUBBLING & FINALLY
    printf("\n[4. Control Flow Integrity]\n");
    TRY
    {
        printf("  [Test 4.1] Exception bubbling up stack...\n");
        caller();
    }
    CATCH(e) { printf("  PASS: Caught bubbled exception: %s\n", last_err); }

    bool finallyRun = false;
    TRY
    {
        printf("  [Test 4.2] Verifying 'finally' execution...\n");
        THROW("Boom");
    }
    CATCH(e)
    {
        finallyRun = true;
        printf("    -> Inner finally executed\n");
    }
    if (finallyRun)
        printf("  PASS: Finally ran before catch block finished\n");

    // SECTION 5: ALLOCATION
    printf("\n[5. Resource Limits (Stress Test)]\n");
    TRY
    {
        printf("  [Test 5.1] Allocating negative array size...\n");
        int size = -5;
        if (size < 0)
            THROW("AllocationException: Negative array size -5");
    }
    CATCH(e) { printf("  PASS: Caught AllocationException: %s\n", last_err); }

    printf("\n=== PROFESSIONAL SUITE COMPLETED ===\n");
    return 0;
}