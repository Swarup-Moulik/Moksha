// ============================================================================
// MOKSHA KERNEL RUNTIME (FINAL FIX)
// Targeted for: x86_64 Bare Metal (Freestanding)
// ============================================================================

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// ----------------------------------------------------------------------------
// 1. HARDWARE I/O (PORT OPERATIONS)
// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#define MOKSHA_ASM __asm
// MSVC (Windows) Implementation
extern "C" void outb(unsigned short port, unsigned char val)
{
    __asm {
            mov dx, port
            mov al, val
            out dx, al
    }
}
extern "C" unsigned char inb(unsigned short port)
{
    unsigned char retVal;
    __asm {
            mov dx, port
            in al, dx
            mov retVal, al
    }
    return retVal;
}
static void halt_cpu()
{
    __asm hlt
}
#else
// GCC / Clang (Linux/OSDev) Implementation
extern "C" void outb(unsigned short port, unsigned char val)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
extern "C" unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static void halt_cpu(){
    __asm__ __volatile__ ("hlt");
}
#endif

// ----------------------------------------------------------------------------
// 2. MEMORY MANAGEMENT GLOBALS
// ----------------------------------------------------------------------------

#define HEAP_SIZE 1024 * 1024 * 4 // 4 MB Kernel Heap
static uint8_t kernel_heap[HEAP_SIZE];
static size_t heap_top = 0;

// Forward Declarations to satisfy C++ compiler
extern "C"
{
    void vga_print_str(const char *str);
    char *moksha_unbox_string(void *ptr);
    void start_kernel(); // Defined in your Moksha code
}

// ----------------------------------------------------------------------------
// 3. STANDARD LIBRARY REPLACEMENTS (Required for Kernel)
// ----------------------------------------------------------------------------

extern "C"
{

    void *memcpy(void *dest, const void *src, size_t n)
    {
        char *d = (char *)dest;
        const char *s = (const char *)src;
        while (n--)
            *d++ = *s++;
        return dest;
    }

    void *memmove(void *dest, const void *src, size_t n)
    {
        unsigned char *d = (unsigned char *)dest;
        const unsigned char *s = (const unsigned char *)src;
        if (d < s)
        {
            while (n--)
                *d++ = *s++;
        }
        else
        {
            d += n;
            s += n;
            while (n--)
                *--d = *--s;
        }
        return dest;
    }

    void *memset(void *dest, int val, size_t n)
    {
        unsigned char *d = (unsigned char *)dest;
        while (n--)
            *d++ = (unsigned char)val;
        return dest;
    }

    int memcmp(const void *s1, const void *s2, size_t n)
    {
        const unsigned char *p1 = (const unsigned char *)s1;
        const unsigned char *p2 = (const unsigned char *)s2;
        for (size_t i = 0; i < n; i++)
        {
            if (p1[i] != p2[i])
                return p1[i] - p2[i];
        }
        return 0;
    }

    size_t strlen(const char *str)
    {
        size_t len = 0;
        while (str[len])
            len++;
        return len;
    }

    int strcmp(const char *s1, const char *s2)
    {
        while (*s1 && (*s1 == *s2))
        {
            s1++;
            s2++;
        }
        return *(const unsigned char *)s1 - *(const unsigned char *)s2;
    }

    char *strcpy(char *dest, const char *src)
    {
        char *d = dest;
        while ((*d++ = *src++))
            ;
        return dest;
    }

    char *strcat(char *dest, const char *src)
    {
        char *ptr = dest + strlen(dest);
        while ((*ptr++ = *src++))
            ;
        return dest;
    }

    // --- KERNEL ALLOCATOR (Bump Allocator) ---
    void *malloc(size_t size)
    {
        if (heap_top + size >= HEAP_SIZE)
            return nullptr;
        void *ptr = &kernel_heap[heap_top];
        heap_top += size;
        // Align to 8 bytes
        if (heap_top % 8 != 0)
            heap_top += (8 - (heap_top % 8));
        return ptr;
    }

    void *calloc(size_t num, size_t size)
    {
        void *ptr = malloc(num * size);
        if (ptr)
            memset(ptr, 0, num * size);
        return ptr;
    }

    void free(void *ptr)
    {
        // No-op in a simple bump allocator
    }

    void moksha_free(void *ptr)
    {
        // No-op
    }
}

// ----------------------------------------------------------------------------
// 4. VGA DRIVER
// ----------------------------------------------------------------------------

static volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;
static int vga_col = 0;
static int vga_row = 0;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;

// Note: These functions are C++, but we export them via extern "C" wrappers below if needed.
// Or we just use them internally.

void vga_scroll()
{
    memmove((void *)vga_buffer, (void *)(vga_buffer + VGA_WIDTH), (VGA_HEIGHT - 1) * VGA_WIDTH * 2);
    for (int i = 0; i < VGA_WIDTH; i++)
    {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = (0x0F << 8) | ' ';
    }
}

void vga_newline()
{
    vga_col = 0;
    vga_row++;
    if (vga_row >= VGA_HEIGHT)
    {
        vga_row = VGA_HEIGHT - 1;
        vga_scroll();
    }
}

void vga_print_char_internal(char c)
{
    if (c == '\n')
    {
        vga_newline();
        return;
    }
    const int index = vga_row * VGA_WIDTH + vga_col;
    vga_buffer[index] = (uint16_t)c | (uint16_t)(0x0F << 8); // White text on Black
    vga_col++;
    if (vga_col >= VGA_WIDTH)
        vga_newline();
}

extern "C"
{
    void vga_print_char(char c)
    {
        vga_print_char_internal(c);
    }

    void vga_print_str(const char *str)
    {
        while (*str)
            vga_print_char_internal(*str++);
    }

    void vga_print_int(int val)
    {
        if (val == 0)
        {
            vga_print_str("0");
            return;
        }
        if (val < 0)
        {
            vga_print_char('-');
            val = -val;
        }
        char buffer[12];
        int i = 10;
        buffer[11] = 0;
        while (val > 0)
        {
            buffer[i--] = (val % 10) + '0';
            val /= 10;
        }
        vga_print_str(&buffer[i + 1]);
    }

    void vga_print_hex(size_t val)
    {
        vga_print_str("0x");
        char buffer[20];
        int i = 18;
        buffer[19] = 0;
        if (val == 0)
        {
            vga_print_str("00");
            return;
        }
        while (val > 0)
        {
            int digit = val % 16;
            buffer[i--] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
            val /= 16;
        }
        vga_print_str(&buffer[i + 1]);
    }

    int printf(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        while (*fmt)
        {
            if (*fmt == '%')
            {
                fmt++;
                if (*fmt == 's')
                    vga_print_str(va_arg(args, const char *));
                else if (*fmt == 'd')
                    vga_print_int(va_arg(args, int));
                else if (*fmt == 'p')
                    vga_print_hex(va_arg(args, size_t));
                else
                {
                    vga_print_char('%');
                    vga_print_char(*fmt);
                }
            }
            else
            {
                vga_print_char(*fmt);
            }
            fmt++;
        }
        va_end(args);
        return 0;
    }
}

// ----------------------------------------------------------------------------
// 5. MOKSHA TYPE SYSTEM DEFINITIONS
// ----------------------------------------------------------------------------

enum MokshaType
{
    TYPE_UNKNOWN = 0,
    TYPE_ARRAY = 1,
    TYPE_TABLE = 2,
    TYPE_STRING = 3,
    TYPE_CLOSURE = 4,
    TYPE_INT = 5,
    TYPE_DOUBLE = 6,
    TYPE_BOOL = 7,
    TYPE_CHAR = 8,
    TYPE_SHORT = 9,
    TYPE_LONG = 10,
    TYPE_FLOAT = 11
};

struct MokshaHeader
{
    MokshaType type;
};
struct MokshaInt
{
    MokshaHeader header;
    int32_t val;
};
struct MokshaDouble
{
    MokshaHeader header;
    double val;
};
struct MokshaBool
{
    MokshaHeader header;
    int32_t val;
};
struct MokshaFloat
{
    MokshaHeader header;
    int32_t val;
};
struct MokshaString
{
    MokshaHeader header;
    char *val;
};
struct MokshaChar
{
    MokshaHeader header;
    int8_t val;
};

struct MokshaArray
{
    MokshaHeader header;
    void **data;
    int32_t size;
    int32_t capacity;
};

struct TableEntry
{
    void *key;
    void *value;
    TableEntry *next;
};

struct MokshaTable
{
    MokshaHeader header;
    TableEntry *head;
};

struct MokshaClosure
{
    MokshaHeader header;
    void *funcPtr;
    uint8_t *envData;
    int32_t envSize;
};

// ----------------------------------------------------------------------------
// 6. MOKSHA API IMPLEMENTATION
// ----------------------------------------------------------------------------

extern "C"
{

    // --- BOXING ---
    void *moksha_box_int(int32_t v)
    {
        MokshaInt *b = (MokshaInt *)malloc(sizeof(MokshaInt));
        b->header.type = TYPE_INT;
        b->val = v;
        return b;
    }
    void *moksha_box_bool(int32_t v)
    {
        MokshaBool *b = (MokshaBool *)malloc(sizeof(MokshaBool));
        b->header.type = TYPE_BOOL;
        b->val = v;
        return b;
    }
    void *moksha_box_double(double v)
    {
        MokshaDouble *b = (MokshaDouble *)malloc(sizeof(MokshaDouble));
        b->header.type = TYPE_DOUBLE;
        b->val = v;
        return b;
    }
    void *moksha_box_string(char *v)
    {
        MokshaString *b = (MokshaString *)malloc(sizeof(MokshaString));
        b->header.type = TYPE_STRING;
        size_t len = strlen(v);
        b->val = (char *)malloc(len + 1);
        strcpy(b->val, v);
        return b;
    }
    void *moksha_box_char(int8_t v)
    {
        MokshaChar *b = (MokshaChar *)malloc(sizeof(MokshaChar));
        b->header.type = TYPE_CHAR;
        b->val = v;
        return b;
    }
    void* moksha_box_float(float v) { 
        MokshaFloat* b = new MokshaFloat();
        b->header.type = TYPE_FLOAT;
        b->val = v;
        return b;
    }

    // --- UNBOXING ---
    int32_t moksha_unbox_int(void *ptr) { return ptr ? ((MokshaInt *)ptr)->val : 0; }
    char *moksha_unbox_string(void *ptr) { return ptr ? ((MokshaString *)ptr)->val : (char *)""; }
    double moksha_unbox_double(void *ptr) { return ptr ? ((MokshaDouble *)ptr)->val : 0.0; }
    int32_t moksha_unbox_bool(void *ptr) { return ptr ? ((MokshaBool *)ptr)->val : 0; }

    // --- ARRAYS ---
    void *moksha_alloc_array(int32_t cap)
    {
        MokshaArray *arr = (MokshaArray *)malloc(sizeof(MokshaArray));
        arr->header.type = TYPE_ARRAY;
        arr->size = 0;
        arr->capacity = (cap > 0) ? cap : 4;
        arr->data = (void **)malloc(arr->capacity * sizeof(void *));
        return arr;
    }

    void *moksha_array_push_val(void *arrPtr, void *val)
    {
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (arr->size >= arr->capacity)
        {
            int32_t newCap = arr->capacity * 2;
            void **newData = (void **)malloc(newCap * sizeof(void *));
            memcpy(newData, arr->data, arr->size * sizeof(void *));
            arr->data = newData;
            arr->capacity = newCap;
        }
        arr->data[arr->size++] = val;
        return arrPtr;
    }

    void *moksha_array_get(void *arrPtr, int32_t index)
    {
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (!arr || index < 0 || index >= arr->size)
            return nullptr;
        return arr->data[index];
    }

    int32_t moksha_get_length(void *ptr)
    {
        if (!ptr)
            return 0;
        MokshaHeader *h = (MokshaHeader *)ptr;
        if (h->type == TYPE_ARRAY)
            return ((MokshaArray *)ptr)->size;
        if (h->type == TYPE_STRING)
            return (int32_t)strlen(((MokshaString *)ptr)->val);
        return 0;
    }

    // --- TABLES (MAPS) ---
    void *moksha_alloc_table(int32_t capacity)
    {
        MokshaTable *t = (MokshaTable *)malloc(sizeof(MokshaTable));
        t->header.type = TYPE_TABLE;
        t->head = nullptr;
        return t;
    }

    void moksha_table_set(void *tablePtr, void *key, void *val)
    {
        MokshaTable *t = (MokshaTable *)tablePtr;
        TableEntry *curr = t->head;
        while (curr)
        {
            const char *k1 = moksha_unbox_string(key);
            const char *k2 = moksha_unbox_string(curr->key);
            if (strcmp(k1, k2) == 0)
            {
                curr->value = val;
                return;
            }
            curr = curr->next;
        }
        TableEntry *entry = (TableEntry *)malloc(sizeof(TableEntry));
        entry->key = key;
        entry->value = val;
        entry->next = t->head;
        t->head = entry;
    }

    void *moksha_table_get(void *tablePtr, void *key)
    {
        MokshaTable *t = (MokshaTable *)tablePtr;
        TableEntry *curr = t->head;
        while (curr)
        {
            const char *k1 = moksha_unbox_string(key);
            const char *k2 = moksha_unbox_string(curr->key);
            if (strcmp(k1, k2) == 0)
                return curr->value;
            curr = curr->next;
        }
        return nullptr;
    }

    void *moksha_table_keys(void *tablePtr)
    {
        MokshaTable *t = (MokshaTable *)tablePtr;
        int count = 0;
        TableEntry *curr = t->head;
        while (curr)
        {
            count++;
            curr = curr->next;
        }

        void *arr = moksha_alloc_array(count);
        curr = t->head;
        while (curr)
        {
            moksha_array_push_val(arr, curr->key);
            curr = curr->next;
        }
        return arr;
    }

    // --- STRINGS ---
    void *moksha_string_concat(void *s1, void *s2)
    {
        const char *c1 = moksha_unbox_string(s1);
        const char *c2 = moksha_unbox_string(s2);
        size_t len1 = strlen(c1);
        size_t len2 = strlen(c2);
        char *newStr = (char *)malloc(len1 + len2 + 1);
        strcpy(newStr, c1);
        strcat(newStr, c2);
        return moksha_box_string(newStr);
    }

    void *moksha_string_get_char(void *strPtr, int32_t index)
    {
        char *cStr = moksha_unbox_string(strPtr);
        int len = strlen(cStr);
        if (index < 0 || index >= len)
            return moksha_box_string((char *)"");
        char buffer[2] = {cStr[index], 0};
        return moksha_box_string(buffer);
    }

    // --- CLOSURES ---
    void *moksha_create_closure(void *funcPtr, int32_t captureSize, void *envInit)
    {
        MokshaClosure *closure = (MokshaClosure *)malloc(sizeof(MokshaClosure));
        closure->header.type = TYPE_CLOSURE;
        closure->funcPtr = funcPtr;
        closure->envSize = captureSize;
        if (captureSize > 0)
        {
            closure->envData = (uint8_t *)malloc(captureSize);
            if (envInit)
                memcpy(closure->envData, envInit, captureSize);
            else
                memset(closure->envData, 0, captureSize);
        }
        else
        {
            closure->envData = nullptr;
        }
        return (void *)closure;
    }

    void *moksha_get_closure_env(void *closurePtr)
    {
        if (!closurePtr)
            return nullptr;
        return ((MokshaClosure *)closurePtr)->envData;
    }

    void *moksha_get_closure_func(void *closurePtr)
    {
        if (!closurePtr)
            return nullptr;
        return ((MokshaClosure *)closurePtr)->funcPtr;
    }

    // --- CONVERSIONS & UTILS ---
    void *moksha_any_to_str(void *ptr) { return moksha_box_string((char *)"<Object>"); }

    char *moksha_int_to_str(int val)
    {
        char *buf = (char *)malloc(12);
        int i = 0;
        if (val == 0)
        {
            buf[i++] = '0';
        }
        else
        {
            if (val < 0)
            {
                buf[i++] = '-';
                val = -val;
            }
            int temp = val;
            int len = 0;
            while (temp > 0)
            {
                temp /= 10;
                len++;
            }
            for (int k = 0; k < len; k++)
            {
                buf[i + len - 1 - k] = (val % 10) + '0';
                val /= 10;
            }
            i += len;
        }
        buf[i] = 0;
        return buf;
    }

    char *moksha_ptr_to_str(void *ptr) { return (char *)"<Ptr>"; }
    char *moksha_double_to_str(double val) { return (char *)"float"; }

    void panic(const char *msg)
    {
        vga_print_str("\n[PANIC] ");
        vga_print_str(msg);
        while (1)
        {
            halt_cpu();
        }
    }

    void moksha_throw_exception(void *v)
    {
        panic("Unhandled Exception");
    }

    void _start()
    {
        start_kernel();
        while (1)
        {
            halt_cpu();
        }
    }
}