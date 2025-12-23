#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// --- TYPEDEF ---
typedef int m_size_t;
typedef char byte;

// --- BITFIELDS ---
// C naturally supports bitfields within structs.
struct BitfieldTest {
    unsigned int first  : 3;
    unsigned int second : 5;
    unsigned int third  : 8;
    unsigned int fourth : 16;
};

// --- PACKED STRUCT ---
// In C (GCC/Clang), we use __attribute__((packed)) to prevent padding.
#pragma pack(push, 1)
struct HardwarePacket {
    char magic;
    int payload;
    char checksum;
};

struct Inner {
    char a;
    char b;
};

struct Outer {
    char header;
    struct Inner data;
    int footer;
};
#pragma pack(pop)

// --- UNION ---
union Converter {
    int raw_bits;
    float value;
};

union Color {
    uint32_t rgba;
    unsigned char components[4];
};

// --- CONSTANTS ---
const char* TEST_PLATFORM = "HOSTED";

// --- INTERRUPT ---
// Note: Standard C does not have a cross-platform 'interrupt' keyword.
// This is usually handled via compiler-specific attributes like __attribute__((interrupt)).
void dummy_interrupt(void* frame) {
    // Feature: Interrupt Calling Convention (Simulator)
}

int main() {
    printf("--- Consolidated C Test Suite (Moksha Port) ---\n");

    // --- SIZEOF & TYPEDEF ---
    m_size_t s1 = (m_size_t)sizeof(struct BitfieldTest);
    m_size_t s2 = (m_size_t)sizeof(int);
    printf("Size of BitfieldTest: %d\n", s1); 
    printf("Size of int alias: %d\n", s2);

    // --- STATIC IF (C equivalent: Preprocessor or Runtime Const) ---
    if (strcmp(TEST_PLATFORM, "HOSTED") == 0) {
        printf("Static If: Successfully executed HOSTED block.\n");
    }

    // --- BITFIELDS (PACKING/UNPACKING) ---
    struct BitfieldTest bf;
    bf.first = 5;      
    bf.second = 21;    
    bf.third = 255;    
    bf.fourth = 1234;

    printf("Bitfield Values:\n");
    printf("  First (3 bits): %u\n", bf.first);
    printf("  Second (5 bits): %u\n", bf.second);
    printf("  Third (8 bits): %u\n", bf.third);

    // --- POINTER CASTING & RAW MEMORY ---
    // This allows us to see how the bitfields are laid out in a single 32-bit int.
    unsigned int* raw = (unsigned int*)&bf;
    printf("Raw Memory Value: %u\n", *raw); 

    // --- PACKED STRUCT OFFSET ---
    struct HardwarePacket packet;
    char* base_addr = (char*)&packet;
    char* payload_addr = (char*)&packet.payload;
    int offset = (int)(payload_addr - base_addr);
    printf("Offset of 'payload' in packed struct: %d\n", offset); 

    // --- UNION REINTERPRETATION ---
    union Converter conv;
    conv.raw_bits = 0x3F800000; 
    printf("Union raw hex as float: %f\n", conv.value); 

    // --- MEMORY MANIPULATION (MALLOC/STRCPY) ---
    char* buffer = (char*)malloc(12);
    if (buffer) {
        strcpy(buffer, "Hello World");
        buffer[6] = 'M'; buffer[7] = 'o'; buffer[8] = 'k'; 
        buffer[9] = 's'; buffer[10] = 'h';
        printf("Modified C-Buffer: %s\n", buffer); 
        free(buffer);
    }

    // --- NESTED PACKED STRUCTS ---
    struct Outer obj;
    char* base = (char*)&obj;
    char* footer_ptr = (char*)&obj.footer;
    int nested_offset = (int)(footer_ptr - base);
    printf("Nested struct footer offset: %d\n", nested_offset);

    // --- POINTER ARITHMETIC ---
    int* data_ptr = (int*)malloc(3 * sizeof(int));
    if (data_ptr) {
        *data_ptr = 100;
        *(data_ptr + 1) = 200;
        *(data_ptr + 2) = 300;
        printf("Pointer index [1]: %d\n", data_ptr[1]);
        printf("Dereferenced offset +2: %d\n", *(data_ptr + 2));
        free(data_ptr);
    }

    // --- LITTLE ENDIAN UNION ACCESS ---
    union Color pixel;
    pixel.rgba = 0x11223344;
    printf("Union Component 0 (LSB): %d\n", (int)pixel.components[0]);
    printf("Union Component 3 (MSB): %d\n", (int)pixel.components[3]);

    return 0;
}