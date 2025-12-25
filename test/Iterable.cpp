#include <iostream>
#include <vector>
#include <string>
#include <utility> // for std::pair

static constexpr int ITERATIONS = 100000;

int main() {
    // =====================
    // 1. INPUT (ONCE)
    // =====================
    std::string w;
    std::cout << "\nEnter a string: ";
    std::cin >> w;

    int size;
    std::cout << "Enter array size: ";
    std::cin >> size;

    std::vector<std::string> arInput(size);
    std::cout << "Enter " << size << " words for the array:\n";
    for (int i = 0; i < size; i++) {
        std::cout << "ar[" << i << "]: ";
        std::cin >> arInput[i];
    }

    std::string k;
    std::string kValue;

    std::cout << "\nEnter a table key: ";
    std::cin >> k;
    std::cout << "Enter value for " << k << ": ";
    std::cin >> kValue;


    // =====================
    // 2. BENCHMARK LOOP
    // =====================
    for (int iter = 0; iter < ITERATIONS; ++iter) {

        // --- Fresh copies ---
        // std::vector copy is efficient (contiguous memory)
        std::vector<std::string> ar(arInput);
        
        // Optimization: Use vector<pair> instead of unordered_map.
        // The C code uses a struct array (linear memory). 
        // For small N, this is MUCH faster than hashing.
        std::vector<std::pair<std::string, std::string>> tab;
        tab.reserve(4); // Pre-allocate small stack-like buffer

        tab.emplace_back(k, kValue);
        tab.emplace_back("fixed", "unchangeable");

        // --- Array loops ---
        // Optimization: Removed 'volatile' to match C code.
        // If the C code loops are empty, the compiler deletes them.
        // C++ must be allowed to do the same to be comparable.
        for (const auto& v : ar) { }

        for (size_t i = 0; i < ar.size(); ++i) { }

        for (const auto& v : ar) { }

        // --- Table loops ---
        for (const auto& entry : tab) { }

        for (const auto& entry : tab) { }

        // --- String iteration ---
        if (!w.empty()) {
            for (char ch : w) { }
        }

        // --- Complex assignment ---
        if (w.size() > 3 && ar.size() > 3) {
            ar[3] = w[3]; // Direct assignment
        }

        if (ar.size() > 3) {
            // Linear push is faster than hash lookup for insertion
            tab.emplace_back("fromArray", ar[3]);
        }

        auto len = ar.size();
    }


    // =====================
    // 3. FINAL OUTPUT
    // =====================
    std::cout << "\n=== TEST COMPLETED ===\n";
    std::cout << "Iterations executed: " << ITERATIONS << "\n";

    return 0;
}