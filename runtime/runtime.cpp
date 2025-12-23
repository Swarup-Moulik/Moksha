#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cstdint>
#include <cstdio>

// --- Type Definitions ---

extern "C"
{
    extern int32_t __exception_flag;
    extern void *__exception_val;
}

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

// Boxed Primitives
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

struct MokshaString
{
    MokshaHeader header;
    char *val;
};

struct MokshaBool
{
    MokshaHeader header;
    int32_t val; // 0 or 1
};

struct MokshaChar
{
    MokshaHeader header;
    int8_t val;
};
struct MokshaShort
{
    MokshaHeader header;
    int16_t val;
};
struct MokshaLong
{
    MokshaHeader header;
    int64_t val;
};
struct MokshaFloat
{
    MokshaHeader header;
    float val;
};

// Structures
struct MokshaArray
{
    MokshaHeader header;
    std::vector<void *> data;
};

// Closure Wrapper
struct MokshaClosure
{
    MokshaHeader header;
    void *funcPtr;                // The raw function pointer
    std::vector<uint8_t> envData; // The captured environment (byte buffer)
};

extern "C" void *moksha_box_string(char *v); // Forward declaration

// --- Hash & Equality for Table ---
struct MokshaKeyHash
{
    std::size_t operator()(void *k) const
    {
        if (!k)
            return 0;
        MokshaHeader *h = (MokshaHeader *)k;
        if (h->type == TYPE_STRING)
        {
            // Hash by string content
            char *s = ((MokshaString *)k)->val;
            unsigned long hash = 5381;
            int c;
            while ((c = *s++))
                hash = ((hash << 5) + hash) + c;
            return hash;
        }
        // Fallback to pointer hash
        return std::hash<void *>{}(k);
    }
};

struct MokshaKeyEqual
{
    bool operator()(void *lhs, void *rhs) const
    {
        if (lhs == rhs)
            return true;
        if (!lhs || !rhs)
            return false;

        MokshaHeader *h1 = (MokshaHeader *)lhs;
        MokshaHeader *h2 = (MokshaHeader *)rhs;

        if (h1->type == TYPE_STRING && h2->type == TYPE_STRING)
        {
            return strcmp(((MokshaString *)lhs)->val, ((MokshaString *)rhs)->val) == 0;
        }
        return false;
    }
};

struct MokshaTable
{
    MokshaHeader header;
    std::unordered_map<void *, void *, MokshaKeyHash, MokshaKeyEqual> data;
};

// --- Helpers ---

void panic(const char *msg)
{
    fprintf(stderr, "\n[Runtime Panic]: %s\n", msg);
    exit(1);
}

// Helper to safely get char* from any void* (assuming it's a MokshaString)
const char *internal_get_str(void *ptr)
{
    if (!ptr)
        return "";
    MokshaHeader *h = (MokshaHeader *)ptr;
    if (h->type == TYPE_STRING)
        return ((MokshaString *)ptr)->val;
    return "";
}

// --- External C ABI (Linked by LLVM) ---

extern "C"
{
    void moksha_throw_exception(void *v)
    {
        __exception_val = v;  // Store the error object
        __exception_flag = 1; // Set the flag
    }

    // --- Memory Release ---
    void moksha_free(void *ptr)
    {
        if (!ptr)
            return;
        MokshaHeader *h = (MokshaHeader *)ptr;

        switch (h->type)
        {
        case TYPE_INT:
            delete (MokshaInt *)ptr;
            break;
        case TYPE_DOUBLE:
            delete (MokshaDouble *)ptr;
            break;
        case TYPE_BOOL:
            delete (MokshaBool *)ptr;
            break;
        case TYPE_STRING:
            delete (MokshaString *)ptr;
            break; // Calls ~MokshaString
        case TYPE_ARRAY:
            delete (MokshaArray *)ptr;
            break; // Calls ~vector
        case TYPE_TABLE:
            delete (MokshaTable *)ptr;
            break; // Calls ~unordered_map
        case TYPE_CLOSURE:
            delete (MokshaClosure *)ptr;
            break;
        default:
            free(ptr);
            break;
        }
    }

    // --- Boxing Functions ---
    void *moksha_box_int(int32_t v)
    {
        MokshaInt *b = new MokshaInt();
        b->header.type = TYPE_INT;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_double(double v)
    {
        MokshaDouble *b = new MokshaDouble();
        b->header.type = TYPE_DOUBLE;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_bool(int32_t v)
    {
        MokshaBool *b = new MokshaBool();
        b->header.type = TYPE_BOOL;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_string(char *v)
    {
        MokshaString *b = new MokshaString();
        b->header.type = TYPE_STRING;
        b->val = strdup(v ? v : "");
        return b;
    }

    void* moksha_box_char(int8_t v) {
        MokshaChar* b = new MokshaChar();
        b->header.type = TYPE_CHAR;
        b->val = v;
        return b;
    }

    void* moksha_box_short(int8_t v) {
        MokshaShort* b = new MokshaShort();
        b->header.type = TYPE_SHORT;
        b->val = v;
        return b;
    }

    void* moksha_box_long(int8_t v) {
        MokshaLong* b = new MokshaLong();
        b->header.type = TYPE_LONG;
        b->val = v;
        return b;
    }

    void* moksha_box_float(int8_t v) {
        MokshaFloat* b = new MokshaFloat();
        b->header.type = TYPE_FLOAT;
        b->val = v;
        return b;
    }

    // --- Unboxing Functions ---
    int32_t moksha_unbox_int(void *ptr)
    {
        if (!ptr)
            return 0;
        MokshaHeader *h = (MokshaHeader *)ptr;

        if (h->type == TYPE_INT)
            return ((MokshaInt *)ptr)->val;
        if (h->type == TYPE_DOUBLE)
            return (int32_t)((MokshaDouble *)ptr)->val;

        // [FIX] Support Parsing and ASCII
        if (h->type == TYPE_STRING)
        {
            char *str = ((MokshaString *)ptr)->val;

            // 1. Try Parsing as Number ("123")
            char *end;
            long num = strtol(str, &end, 10);
            if (end != str && *end == '\0')
            {
                return (int32_t)num;
            }

            // 2. Try ASCII Code ("A" -> 65)
            if (strlen(str) == 1)
            {
                return (int32_t)str[0];
            }

            // 3. Fail
            char buffer[128];
            sprintf(buffer, "FormatException: Cannot cast String '%s' to Int", str);
            moksha_throw_exception(moksha_box_string(buffer));
            return 0;
        }

        char buffer[128];
        sprintf(buffer, "InvalidCastException: Cannot cast type %d to Int", h->type);
        moksha_throw_exception(moksha_box_string(buffer));
        return 0;
    }

    double moksha_unbox_double(void *ptr)
    {
        if (!ptr)
            return 0.0;
        MokshaHeader *h = (MokshaHeader *)ptr;

        if (h->type == TYPE_DOUBLE)
            return ((MokshaDouble *)ptr)->val;

        if (h->type == TYPE_INT)
            return (double)((MokshaInt *)ptr)->val;

        // [FIX] Support Parsing Double ("3.14")
        if (h->type == TYPE_STRING)
        {
            char *str = ((MokshaString *)ptr)->val;
            char *end;
            double val = strtod(str, &end);
            if (end != str && *end == '\0')
            {
                return val;
            }

            char buffer[128];
            sprintf(buffer, "FormatException: Cannot cast String '%s' to Double", str);
            moksha_throw_exception(moksha_box_string(buffer));
            return 0.0;
        }

        return 0.0;
    }

    int32_t moksha_unbox_bool(void *ptr)
    {
        if (!ptr)
            return 0;
        MokshaHeader *h = (MokshaHeader *)ptr;
        if (h->type == TYPE_BOOL)
            return ((MokshaBool *)ptr)->val;
        return 0;
    }

    char *moksha_unbox_string(void *ptr)
    {
        if (!ptr)
            return strdup("");
        MokshaHeader *h = (MokshaHeader *)ptr;
        if (h->type == TYPE_STRING)
            return strdup(((MokshaString *)ptr)->val);
        return strdup("");
    }

    // --- String Ops ---
    void *moksha_any_to_str(void *ptr)
    {
        if (!ptr)
            return strdup("null");

        MokshaHeader *h = (MokshaHeader *)ptr;
        char buffer[128];

        switch (h->type)
        {
        case TYPE_INT:
            sprintf(buffer, "%d", ((MokshaInt *)ptr)->val);
            break;
        case TYPE_DOUBLE:
            sprintf(buffer, "%f", ((MokshaDouble *)ptr)->val);
            break;
        case TYPE_BOOL:
            sprintf(buffer, "%s", ((MokshaBool *)ptr)->val ? "true" : "false");
            break;
        case TYPE_STRING:
            return strdup(((MokshaString *)ptr)->val);

        case TYPE_ARRAY:
        {
            MokshaArray *arr = (MokshaArray *)ptr;
            std::string s = "[";
            for (size_t i = 0; i < arr->data.size(); ++i)
            {
                if (i > 0)
                    s += ", ";
                char *elStr = (char *)moksha_any_to_str(arr->data[i]);
                s += elStr;
                free(elStr);
            }
            s += "]";
            return strdup(s.c_str());
        }
        case TYPE_TABLE:
        {
            MokshaTable *tbl = (MokshaTable *)ptr;
            std::string s = "{";
            bool first = true;
            for (auto const &[key, val] : tbl->data)
            {
                if (!first)
                    s += ", ";
                first = false;
                char *kStr = (char *)moksha_any_to_str(key);
                s += kStr;
                free(kStr);
                s += ": ";
                char *vStr = (char *)moksha_any_to_str(val);
                s += vStr;
                free(vStr);
            }
            s += "}";
            return strdup(s.c_str());
        }
        case TYPE_CLOSURE:
            sprintf(buffer, "[Function]");
            break;
        default:
            sprintf(buffer, "<Object %p>", ptr);
        }
        return strdup(buffer);
    }

    char *moksha_int_to_str(int val)
    {
        char *buffer = (char *)malloc(12);
        sprintf(buffer, "%d", val);
        return buffer;
    }

    char *moksha_int_to_ascii(int32_t code)
    {
        char *buffer = (char *)malloc(2);
        buffer[0] = (char)code;
        buffer[1] = '\0';
        return buffer;
    }

    char *moksha_double_to_str(double val)
    {
        char *buffer = (char *)malloc(64);
        sprintf(buffer, "%g", val);
        return buffer;
    }

    void *moksha_string_concat(void *s1, void *s2)
    {
        const char *c1 = internal_get_str(s1);
        const char *c2 = internal_get_str(s2);
        std::string res = std::string(c1) + std::string(c2);
        return moksha_box_string((char *)res.c_str());
    }

    int32_t moksha_strlen(void *ptr)
    {
        if (!ptr)
            return 0;
        return (int32_t)strlen(internal_get_str(ptr));
    }

    void *moksha_string_get_char(void *ptr, int32_t index)
    {
        if (!ptr)
            return moksha_box_string((char *)"");
        MokshaHeader *h = (MokshaHeader *)ptr;
        if (h->type == TYPE_STRING)
        {
            MokshaString *s = (MokshaString *)ptr;
            size_t len = strlen(s->val);
            if (index < 0 || index >= (int32_t)len)
            {
                char buffer[128];
                sprintf(buffer, "IndexOutOfBoundsException: String index %d out of range (Length %zu)", index, len);
                moksha_throw_exception(moksha_box_string(buffer));
                return moksha_box_string((char *)"");
            }
            if (index >= 0 && index < (int32_t)len)
            {
                char buffer[2] = {s->val[index], '\0'};
                return moksha_box_string(buffer);
            }
        }
        return moksha_box_string((char *)"");
    }

    // --- Array Operations ---
    void *moksha_alloc_array(int32_t cap)
    {
        if (cap < 0)
        {
            moksha_throw_exception(moksha_box_string((char *)"AllocationException: Negative array size"));
            return nullptr;
        }
        MokshaArray *arr = new MokshaArray();
        arr->header.type = TYPE_ARRAY;
        arr->data.reserve(cap > 0 ? cap : 4);
        return (void *)arr;
    }

    void *moksha_array_push_val(void *arrPtr, void *val)
    {
        if (arrPtr)
            ((MokshaArray *)arrPtr)->data.push_back(val);
        return arrPtr;
    }

    void *moksha_array_get(void *arrPtr, int32_t index)
    {
        if (!arrPtr)
        {
            moksha_throw_exception(moksha_box_string((char *)"NullReferenceException: Array is null"));
            return nullptr;
        }
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (index < 0 || index >= (int32_t)arr->data.size())
        {
            char buf[128];
            sprintf(buf, "IndexOutOfBoundsException: Index %d, Size %zu", index, arr->data.size());
            moksha_throw_exception(moksha_box_string(buf));
            return nullptr;
        }
        return arr->data[index];
    }

    void moksha_array_set(void *arrPtr, int32_t index, void *val)
    {
        if (!arrPtr)
            panic("Null array assignment");
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (index < 0 || index >= (int32_t)arr->data.size())
            panic("Index out of bounds");
        arr->data[index] = val;
    }

    void *moksha_alloc_array_fill(int32_t size, void *defaultVal)
    {
        // [FIX] Add Negative Check
        if (size < 0)
        {
            char buffer[128];
            sprintf(buffer, "AllocationException: Negative array size %d", size);
            moksha_throw_exception(moksha_box_string(buffer));
            return nullptr;
        }

        MokshaArray *arr = new MokshaArray();
        arr->header.type = TYPE_ARRAY;
        arr->data.resize(size > 0 ? size : 0, defaultVal);
        return (void *)arr;
    }

    // This is for Arrays/Tables (Boxed Objects)
    int32_t moksha_get_length(void *targetPtr)
    {
        if (!targetPtr)
            return 0;
        MokshaHeader *h = (MokshaHeader *)targetPtr;
        if (h->type == TYPE_ARRAY)
            return (int32_t)((MokshaArray *)targetPtr)->data.size();
        // Fallback for Boxed String if ever passed
        if (h->type == TYPE_STRING)
            return (int32_t)strlen(((MokshaString *)targetPtr)->val);
        return 0;
    }

    void *moksha_array_spread(void *targetPtr, void *sourcePtr)
    {
        if (!targetPtr || !sourcePtr)
            return targetPtr;
        MokshaArray *target = (MokshaArray *)targetPtr;
        MokshaArray *source = (MokshaArray *)sourcePtr;
        target->data.insert(target->data.end(), source->data.begin(), source->data.end());
        return target;
    }

    void moksha_array_delete(void *arrPtr, int32_t index)
    {
        if (!arrPtr)
            return;
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (arr->header.type != TYPE_ARRAY)
            return;

        if (index >= 0 && index < (int32_t)arr->data.size())
        {
            arr->data.erase(arr->data.begin() + index);
        }
    }

    // --- Table Operations ---
    void *moksha_alloc_table(int32_t initial_size)
    {
        MokshaTable *table = new MokshaTable();
        table->header.type = TYPE_TABLE;
        return (void *)table;
    }

    void *moksha_table_keys(void *tablePtr)
    {
        if (!tablePtr)
            return moksha_alloc_array(0);
        MokshaTable *table = (MokshaTable *)tablePtr;

        // Create a new Array to hold the keys
        MokshaArray *arr = new MokshaArray();
        arr->header.type = TYPE_ARRAY;
        arr->data.reserve(table->data.size());

        // Copy every key from the map into the array
        for (auto const &[key, val] : table->data)
        {
            arr->data.push_back(key);
        }
        return (void *)arr;
    }

    void *moksha_table_set(void *tablePtr, void *key, void *val)
    {
        if (!tablePtr)
            panic("Set null table.");
        MokshaTable *table = (MokshaTable *)tablePtr;
        table->data[key] = val;
        return val;
    }

    void *moksha_table_get(void *tablePtr, void *key)
    {
        if (!tablePtr)
            return nullptr;
        MokshaTable *table = (MokshaTable *)tablePtr;
        auto it = table->data.find(key);
        if (it != table->data.end())
            return it->second;
        return nullptr;
    }

    void moksha_table_delete(void *tablePtr, void *key)
    {
        if (!tablePtr)
            return;
        MokshaTable *table = (MokshaTable *)tablePtr;
        auto it = table->data.find(key);
        if (it != table->data.end())
        {
            moksha_free(it->second);
            table->data.erase(it);
        }
    }

    // --- I/O ---
    int32_t moksha_read_int()
    {
        int32_t val;
        if (std::cin >> val)
        {
            std::string dummy;
            std::getline(std::cin, dummy);
            return val;
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return 0;
    }

    double moksha_read_double()
    {
        double val;
        if (std::cin >> val)
        {
            std::string dummy;
            std::getline(std::cin, dummy);
            return val;
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return 0.0;
    }

    int32_t moksha_read_bool()
    {
        std::string line;
        if (!std::getline(std::cin, line))
            return 0;
        if (line.empty())
            return 0;
        char *end = nullptr;
        double d = std::strtod(line.c_str(), &end);
        if ((end != line.c_str()) && (*end == '\0'))
            return (d > 0.0) ? 1 : 0;
        if (line == "false" || line == "FALSE" || line == "False")
            return 0;
        return 1;
    }

    void *moksha_read_string()
    {
        std::string line;
        char c = std::cin.peek();
        if (c == '\n')
            std::cin.ignore();
        if (std::getline(std::cin, line))
        {
            // Remove carriage return if on Windows/mixed environments
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            return moksha_box_string((char *)line.c_str());
        }
        return moksha_box_string((char *)"");
    }

    // --- Closures ---
    void *moksha_create_closure(void *funcPtr, int32_t captureSize, void *envInit)
    {
        MokshaClosure *closure = new MokshaClosure();
        closure->header.type = TYPE_CLOSURE;
        closure->funcPtr = funcPtr;
        if (captureSize > 0)
        {
            closure->envData.resize(captureSize);
            if (envInit)
                std::memcpy(closure->envData.data(), envInit, captureSize);
            else
                std::memset(closure->envData.data(), 0, captureSize);
        }
        return (void *)closure;
    }

    void *moksha_get_closure_env(void *closurePtr)
    {
        if (!closurePtr)
            return nullptr;
        MokshaClosure *closure = (MokshaClosure *)closurePtr;
        if (closure->envData.empty())
            return nullptr;
        return (void *)closure->envData.data();
    }

    void *moksha_get_closure_func(void *closurePtr)
    {
        if (!closurePtr)
            return nullptr;
        MokshaClosure *closure = (MokshaClosure *)closurePtr;
        return closure->funcPtr;
    }

    extern "C" char *moksha_ptr_to_str(void *ptr)
    {
        // Allocate a buffer for the address string (e.g., "0x000000...")
        char *buffer = (char *)malloc(32);
        if (!buffer)
            return NULL;
        // Format as 0x...
        std::sprintf(buffer, "%p", ptr);
        return buffer;
    }
}