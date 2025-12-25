#include <iostream>
#include <vector>
#include <string>
#include <map>

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

        // --- fresh copies ---
        std::vector<std::string> ar = arInput;
        std::map<std::string, std::string> tab;

        tab[k] = kValue;
        tab["fixed"] = "unchangeable";

        // --- Array loops ---
        for (const auto& v : ar) { }

        for (size_t i = 0; i < ar.size(); ++i) { }

        for (const auto& v : ar) { }

        // --- Table loops ---
        for (const auto& [key, val] : tab) { }

        for (const auto& [key, val] : tab) { }

        // --- String iteration ---
        if (!w.empty()) {
            for (char ch : w) { }
        }

        // --- Complex assignment ---
        if (w.size() > 3 && ar.size() > 3) {
            ar[3] = std::string(1, w[3]);
        }

        if (ar.size() > 3) {
            tab["fromArray"] = ar[3];
        }

        auto len = ar.size();

        // --- Deletion (COMMENTED OUT) ---
        /*
        if (ar.size() > 3) {
            ar.erase(ar.begin() + 3);
        }

        tab.erase(k);
        */
    }


    // =====================
    // 3. FINAL OUTPUT
    // =====================
    std::cout << "\n=== TEST COMPLETED ===\n";
    std::cout << "Iterations executed: " << ITERATIONS << std::endl;

    return 0;
}
