#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>

// --- TYPEDEF ---
// Moksha: typedef int m_size_t; [cite: 22]
using m_size_t = int;
using byte = char;

// --- BITFIELDS ---
// Tested in Moksha to verify packing logic [cite: 2, 5, 36]
struct BitfieldTest {
    uint32_t first  : 3;  // Bits 0-2 (Max 7) [cite: 2]
    uint32_t second : 5;  // Bits 3-7 (Max 31) [cite: 3]
    uint32_t third  : 8;  // Bits 8-15 [cite: 4]
    uint32_t fourth : 16; // Bits 16-31 [cite: 5]
};

// --- PACKED STRUCTS ---
// Moksha uses 'packed struct' to eliminate padding [cite: 25, 26]
#pragma pack(push, 1)
struct HardwarePacket {
    char magic;
    int32_t payload;
    char checksum;
};

struct Inner {
    char a;
    char b;
};

struct Outer {
    char header;
    Inner data;
    int32_t footer;
};
#pragma pack(pop)

// --- UNIONS ---
// Used for type reinterpretation [cite: 27, 28, 43]
union Converter {
    int32_t raw_bits;
    float value;
};

union Color {
    uint32_t rgba;
    unsigned char components[4];
};

// --- CONSTANTS ---
const std::string TEST_PLATFORM = "HOSTED"; // [cite: 6]

// --- INTERRUPT EMULATION ---
// Moksha uses specific calling conventions for interrupts [cite: 30]
void dummy_interrupt(void* frame) {
    // Feature: Interrupt Calling Convention simulation
}

int main() {
    std::cout << "--- Consolidated C++ Test Suite (Moksha Port) ---\n";

    // --- SIZEOF & TYPEDEF ---
    m_size_t s1 = static_cast<m_size_t>(sizeof(BitfieldTest));
    m_size_t s2 = static_cast<m_size_t>(sizeof(int));
    std::cout << "Size of BitfieldTest: " << s1 << "\n"; // Should be 4 [cite: 9]
    std::cout << "Size of int alias: " << s2 << "\n";

    // --- STATIC IF EQUIVALENT ---
    // Moksha 'static if' evaluates at compile time [cite: 19, 33]
    if constexpr (true) { // In C++, if constexpr is used for static if
        if (TEST_PLATFORM == "HOSTED") {
            std::cout << "Static If: Successfully compiled HOSTED block.\n";
        }
    }

    // --- BITFIELDS (PACKING/UNPACKING) ---
    BitfieldTest bf;
    bf.first = 5;      
    bf.second = 21;
    bf.third = 255;    
    bf.fourth = 1234;

    std::cout << "Bitfield Values:\n";
    std::cout << "  First (3 bits): " << bf.first << "\n";  // 5 [cite: 13, 37]
    std::cout << "  Second (5 bits): " << bf.second << "\n"; // 21 [cite: 14, 37]
    std::cout << "  Third (8 bits): " << bf.third << "\n";   // 255 [cite: 15, 37]

    // --- POINTER CASTING & RAW MEMORY ---
    // Viewing bitfields as a raw 32-bit integer [cite: 16, 39]
    uint32_t* raw = reinterpret_cast<uint32_t*>(&bf);
    std::cout << "Raw Memory Value: " << *raw << "\n"; // [cite: 18, 39]

    // --- PACKED STRUCT OFFSET ---
    HardwarePacket packet;
    char* base_addr = reinterpret_cast<char*>(&packet);
    int32_t* payload_addr = &packet.payload;
    // Difference between field address and base address [cite: 41]
    int offset = static_cast<int>(reinterpret_cast<uintptr_t>(payload_addr) - reinterpret_cast<uintptr_t>(base_addr));
    std::cout << "Offset of 'payload' in packed struct: " << offset << "\n"; // Expected: 1 [cite: 42]

    // --- UNION REINTERPRETATION ---
    Converter conv;
    conv.raw_bits = 0x3F800000;
    std::cout << "Union raw hex as float: " << std::fixed << conv.value << "\n"; // 1.0 [cite: 43]

    // --- MEMORY INTEROP (MALLOC/STRCPY) ---
    // Direct memory manipulation similar to Moksha unsafe blocks [cite: 44, 45]
    char* buffer = static_cast<char*>(std::malloc(12));
    if (buffer) {
        std::strcpy(buffer, "Hello World");
        buffer[6] = 'M'; buffer[7] = 'o'; buffer[8] = 'k'; 
        buffer[9] = 's'; buffer[10] = 'h';
        buffer[11] = '\0';
        std::cout << "Modified Buffer: " << buffer << "\n"; // "Hello Moksh" [cite: 45]
        std::free(buffer);
    }

    // --- NESTED PACKED STRUCTS ---
    Outer obj;
    char* obj_base = reinterpret_cast<char*>(&obj);
    int32_t* footer_ptr = &obj.footer;
    int nested_offset = static_cast<int>(reinterpret_cast<uintptr_t>(footer_ptr) - reinterpret_cast<uintptr_t>(obj_base));
    std::cout << "Nested struct footer offset: " << nested_offset << "\n"; // [cite: 47, 48]

    // --- POINTER ARITHMETIC ---
    // Testing manual pointer offsets [cite: 49, 50]
    int32_t* data_ptr = static_cast<int32_t*>(std::malloc(12));
    if (data_ptr) {
        *data_ptr = 100;
        *(data_ptr + 1) = 200;
        *(data_ptr + 2) = 300;
        std::cout << "Pointer index [1]: " << data_ptr[1] << "\n";       // 200 [cite: 50]
        std::cout << "Dereferenced offset +2: " << *(data_ptr + 2) << "\n"; // 300 [cite: 50]
        std::free(data_ptr);
    }

    // --- LITTLE ENDIAN UNION ACCESS ---
    // Verifying byte-level access order [cite: 51, 52]
    Color pixel;
    pixel.rgba = 0x11223344;
    std::cout << "Union Component 0 (LSB): " << static_cast<int>(pixel.components[0]) << "\n"; // 0x44 (68) [cite: 52]
    std::cout << "Union Component 3 (MSB): " << static_cast<int>(pixel.components[3]) << "\n"; // 0x11 (17) [cite: 52]

    return 0;
}