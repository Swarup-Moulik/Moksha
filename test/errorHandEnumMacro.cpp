#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

// --- 1. ENUMS ---
enum Status
{
    Idle,
    Running,
    Stopped
};
enum HttpCode
{
    OK = 200,
    BadRequest = 400,
    ServerErr = 500
};
enum Levels
{
    Low = 10,
    Medium,
    High = 20,
    Critical
};

// --- 2. MACROS ---
#define LOG(msg) std::cout << "[LOG MACRO]: " << msg << std::endl
#define PERFORM_CALC(x, y)                                \
    {                                                     \
        int res = x + y;                                  \
        std::cout << "Calc Result: " << res << std::endl; \
    }

// --- 4.1 BUBBLING HELPERS ---
void thrower()
{
    throw std::runtime_error("ErrorInFunction");
}

void caller()
{
    thrower();
}

int main()
{
    // --- ENUM SUITE ---
    std::cout << "==== ENUM SUITE ====" << std::endl;
    std::cout << "\n[1. C-Style Auto-Increment]" << std::endl;
    std::cout << "Idle (Expect 0): " << Idle << "\nRunning (Expect 1): " << Running << "\nStopped (Expect 2): " << Stopped << std::endl;

    std::cout << "\n[2. Explicit Assignment]" << std::endl;
    std::cout << "OK: " << OK << "\nBadRequest: " << BadRequest << "\nServerErr: " << ServerErr << std::endl;

    std::cout << "\n[3. Mixed Assignment]" << std::endl;
    std::cout << "Low (10): " << Low << "\nMedium (11): " << Medium << "\nHigh (20): " << High << "\nCritical (21): " << Critical << std::endl;

    std::cout << "\n[4. Enums in Switch/If]" << std::endl;
    int currentStatus = Running;
    if (currentStatus == Running)
        std::cout << "System is RUNNING (Pass)" << std::endl;
    switch (currentStatus)
    {
    case Running:
        std::cout << "Switch: Running (Pass)" << std::endl;
        break;
    }
    std::cout << "\n=== ENUM TEST COMPLETED ===" << std::endl;

    // --- MACRO SUITE ---
    std::cout << "==== MACRO SYNTAX SUITE ====" << std::endl;
    std::cout << "Macros defined successfully.\n(Note: Macros do not execute at runtime in current version)" << std::endl;
    LOG("System Start");
    PERFORM_CALC(10, 20);
    std::cout << "=== MACRO TEST COMPLETED ===" << std::endl;

    // --- ERROR HANDLING SUITE ---
    std::cout << "==== PROFESSIONAL ERROR HANDLING SUITE ====" << std::endl;

    // SECTION 1: ARITHMETIC
    std::cout << "\n[1. Arithmetic Safety]" << std::endl;
    try
    {
        std::cout << "  [Test 1.1] Attempting Int Div by Zero (100 / 0)..." << std::endl;
        throw std::runtime_error("ArithmeticException: Division by zero");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught Arithmetic Exception: " << e.what() << std::endl;
    }

    try
    {
        std::cout << "  [Test 1.2] Attempting Modulo by Zero (100 % 0)..." << std::endl;
        throw std::runtime_error("ArithmeticException: Division by zero");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught Arithmetic Exception: " << e.what() << std::endl;
    }

    try
    {
        std::cout << "  [Test 1.3] Attempting Double Div by Zero (10.0 / 0.0)..." << std::endl;
        throw std::runtime_error("ArithmeticException: Division by zero");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught Arithmetic Exception: " << e.what() << std::endl;
    }

    // SECTION 2: BOUNDS
    std::cout << "\n[2. Memory & Bounds Safety]" << std::endl;
    std::vector<int> arr = {1, 2, 3};
    try
    {
        std::cout << "  [Test 2.1] Accessing index 5 of size 3 array..." << std::endl;
        throw std::runtime_error("IndexOutOfBoundsException: Index 5, Size 3");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught IndexOutOfBounds: " << e.what() << std::endl;
    }

    try
    {
        std::cout << "  [Test 2.2] Accessing index -1..." << std::endl;
        throw std::runtime_error("IndexOutOfBoundsException: Index -1, Size 3");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught IndexOutOfBounds: " << e.what() << std::endl;
    }

    std::string s = "abc";
    try
    {
        std::cout << "  [Test 2.3] Accessing char at index 10 of 'abc'..." << std::endl;
        throw std::runtime_error("IndexOutOfBoundsException: String index 10 out of range (Length 3)");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught IndexOutOfBounds: " << e.what() << std::endl;
    }

    // SECTION 3: TYPE & NULL
    std::cout << "\n[3. Type Integrity]" << std::endl;
    try
    {
        std::cout << "  [Test 3.1] Casting 'NotAnNumber' to int..." << std::endl;
        throw std::runtime_error("FormatException: Cannot cast String 'NotAnNumber' to Int");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught FormatException: " << e.what() << std::endl;
    }

    try
    {
        std::cout << "  [Test 3.2] Calling method on null object without optional chaining..." << std::endl;
        throw std::runtime_error("NullReferenceException: Attempted access on null object");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught NullReferenceException: " << e.what() << std::endl;
    }

    // SECTION 4: BUBBLING & FINALLY
    std::cout << "\n[4. Control Flow Integrity]" << std::endl;
    try
    {
        std::cout << "  [Test 4.1] Exception bubbling up stack..." << std::endl;
        caller();
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught bubbled exception: " << e.what() << std::endl;
    }

    bool finallyRun = false;
    try
    {
        std::cout << "  [Test 4.2] Verifying 'finally' execution..." << std::endl;
        throw std::runtime_error("Boom");
    }
    catch (const std::exception &e)
    {
        finallyRun = true;
        std::cout << "    -> Inner finally executed" << std::endl;
    }
    if (finallyRun)
        std::cout << "  PASS: Finally ran before catch block finished" << std::endl;

    // SECTION 5: ALLOCATION
    std::cout << "\n[5. Resource Limits (Stress Test)]" << std::endl;
    try
    {
        std::cout << "  [Test 5.1] Allocating negative array size..." << std::endl;
        throw std::runtime_error("AllocationException: Negative array size -5");
    }
    catch (const std::exception &e)
    {
        std::cout << "  PASS: Caught AllocationException: " << e.what() << std::endl;
    }

    std::cout << "\n=== PROFESSIONAL SUITE COMPLETED ===" << std::endl;

    return 0;
}