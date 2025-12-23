#include <iostream>
#include <vector>
#include <string>
#include <map>

// --- HELPER FUNCTIONS ---

// Replicates "Pretty Print Array :- [val1, val2...]"
void pretty_print_array(const std::vector<std::string>& ar) {
    std::cout << "[";
    for (size_t i = 0; i < ar.size(); ++i) {
        std::cout << ar[i];
        if (i < ar.size() - 1) std::cout << ", ";
    }
    std::cout << "]";
}

// Replicates "Table : {key1: val1, key2: val2...}"
void pretty_print_table(const std::map<std::string, std::string>& tab) {
    std::cout << "{";
    bool first = true;
    for (auto const& [key, val] : tab) {
        if (!first) std::cout << ", ";
        std::cout << key << ": " << val;
        first = false;
    }
    std::cout << "}";
}

int main() {
    // --- 1. INPUT ---
    std::string w;
    std::cout << "\nEnter a string: ";
    std::cin >> w;

    int size;
    std::cout << "Enter array size: ";
    std::cin >> size;

    std::vector<std::string> ar(size);
    std::cout << "Enter " << size << " words for the array:" << std::endl;
    for (int i = 0; i < size; i++) {
        std::cout << "ar[" << i << "]: ";
        std::cin >> ar[i];
    }

    std::map<std::string, std::string> tab;
    std::string k;
    std::cout << "\nEnter a table key: ";
    std::cin >> k;
    std::cout << "Enter value for " << k << ": ";
    std::string val_k;
    std::cin >> val_k;
    
    // Assign age/custom key and fixed key
    tab[k] = val_k;
    tab["fixed"] = "unchangeable";

    // --- 2. DISPLAY LOOPS ---
    std::cout << "\n--- Loop Display ---" << std::endl;
    std::cout << "Pretty Print Array :- ";
    pretty_print_array(ar);
    std::cout << std::endl;

    std::cout << "\nArray (For-In Value Only):" << std::endl;
    for (const auto& val : ar) {
        std::cout << val << std::endl;
    }

    std::cout << "\nArray (For-In Index & Value):" << std::endl;
    for (size_t i = 0; i < ar.size(); ++i) {
        std::cout << "[" << i << "] = " << ar[i] << std::endl;
    }

    std::cout << "\nArray (For-In Value):" << std::endl;
    for (const auto& val : ar) {
        std::cout << "Element = " << val << std::endl;
    }

    std::cout << "\nTable (Key, Value):" << std::endl;
    // Iterate in reverse or specific order to match the Mox/C logic
    // std::map is automatically sorted by key
    for (auto it = tab.rbegin(); it != tab.rend(); ++it) {
        std::cout << "Key: " << it->first << ", Val: " << it->second << std::endl;
    }

    std::cout << "\nTable (Key):" << std::endl;
    for (auto it = tab.rbegin(); it != tab.rend(); ++it) {
        std::cout << "Key: " << it->first << std::endl;
    }

    std::cout << "\nString (For-In):" << std::endl;
    if (!w.empty()) {
        for (char ch : w) {
            std::cout << ch << std::endl;
        }
    }

    // --- 3. COMPLEX ASSIGNMENT ---
    std::cout << "\n--- Complex Assignment ---" << std::endl;

    if (w.length() > 3) {
        std::cout << "4th letter of string: " << w[3] << std::endl;
        if (ar.size() > 3) {
            ar[3] = std::string(1, w[3]);
            std::cout << "Assigned ar[3] = " << ar[3] << std::endl;
        }
    }

    if (ar.size() > 3) {
        tab["fromArray"] = ar[3];
        std::cout << "Assigned tab['fromArray'] = " << ar[3] << std::endl;
    }

    std::cout << "Table : ";
    pretty_print_table(tab);
    std::cout << "\nLength of array : " << ar.size() << std::endl;

    // --- 4. DELETION ---
    std::cout << "\n--- Deletion ---" << std::endl;
    if (ar.size() > 3) {
        std::cout << "Deleting ar[3] (" << ar[3] << ")..." << std::endl;
        ar.erase(ar.begin() + 3);
        std::cout << "Array after delete: ";
        pretty_print_array(ar);
        std::cout << std::endl;
    }

    std::cout << "Length of array : " << ar.size() << std::endl;

    std::cout << "Deleting tab[" << k << "]..." << std::endl;
    tab.erase(k);
    
    std::cout << "Table after delete: ";
    pretty_print_table(tab);
    std::cout << std::endl;

    std::cout << "\n=== TEST COMPLETED ===" << std::endl;

    return 0;
}