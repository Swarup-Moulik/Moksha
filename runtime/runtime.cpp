#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <string>

// --- Type Definitions ---

extern "C"
{
    int32_t __exception_flag = 0;
    void *__exception_val = nullptr;
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
    bool isStatic;
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
    size_t cachedHash;
    int32_t length;
    char val[]; // C99 Flexible Array Member
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
    int32_t capacity;
    int32_t length;
    void *data[]; // C99 Flexible Array Member
};

// Closure Wrapper
struct MokshaClosure
{
    MokshaHeader header;
    void *funcPtr;
    size_t envSize;
    uint8_t envData[]; // FAM for environment
};

extern "C" void *moksha_box_string(char *v); // Forward declaration

struct TableEntry
{
    void *key;
    void *value;
};

struct MokshaTable
{
    MokshaHeader header;
    int32_t capacity;
    int32_t count;
    TableEntry *entries; // Flat array
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

static void *globalCharCache[256] = {nullptr};

// --- Table Logic (Linear Probing) ---
// FNV-1a Hash
size_t hash_key(void *key)
{
    if (!key)
        return 0;
    MokshaHeader *h = (MokshaHeader *)key;
    if (h->type == TYPE_STRING)
        return ((MokshaString *)key)->cachedHash;

    // Pointer Hash for objects/arrays
    size_t x = (size_t)key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

bool keys_equal(void *k1, void *k2)
{
    if (k1 == k2)
        return true;
    if (!k1 || !k2)
        return false;
    MokshaHeader *h1 = (MokshaHeader *)k1;
    MokshaHeader *h2 = (MokshaHeader *)k2;

    if (h1->type == TYPE_STRING && h2->type == TYPE_STRING)
    {
        MokshaString *s1 = (MokshaString *)k1;
        MokshaString *s2 = (MokshaString *)k2;
        if (s1->cachedHash != s2->cachedHash)
            return false; // Fast fail
        if (s1->length != s2->length)
            return false;
        return memcmp(s1->val, s2->val, s1->length) == 0;
    }
    return false;
}

TableEntry *table_find_entry(TableEntry *entries, int32_t capacity, void *key)
{
    size_t idx = hash_key(key) & (capacity - 1);
    TableEntry *tombstone = nullptr;

    while (true)
    {
        TableEntry *entry = &entries[idx];
        if (entry->key == nullptr)
        {
            if (entry->value == nullptr)
            { // Empty
                return tombstone != nullptr ? tombstone : entry;
            }
            else
            { // Tombstone (deleted)
                if (tombstone == nullptr)
                    tombstone = entry;
            }
        }
        else if (keys_equal(entry->key, key))
        {
            return entry;
        }
        idx = (idx + 1) & (capacity - 1);
    }
}

// --- External C ABI (Linked by LLVM) ---

extern "C"
{
    void *moksha_any_to_str(void *ptr);

    int32_t moksha_table_capacity(void *tablePtr)
    {
        if (!tablePtr)
            return 0;
        return ((MokshaTable *)tablePtr)->capacity;
    }

    void *moksha_table_get_entry_key(void *tablePtr, int32_t index)
    {
        // Unsafe access for speed. CodeGen ensures index < capacity.
        return ((MokshaTable *)tablePtr)->entries[index].key;
    }

    void *moksha_table_get_entry_val(void *tablePtr, int32_t index)
    {
        return ((MokshaTable *)tablePtr)->entries[index].value;
    }

    void **moksha_get_raw_ptr(void *arrPtr)
    {
        return ((MokshaArray *)arrPtr)->data;
    }

    void moksha_print_val(void *val, int32_t isNewLine)
    {
        char *str = (char *)moksha_any_to_str(val);
        printf("%s%s", str, isNewLine ? "\n" : "");
        free(str); // Fixes the memory leak!
    }

    void *moksha_get_static_string(void **handlePtr, char *content)
    {
        if (*handlePtr != nullptr)
        {
            return *handlePtr;
        }
        void *strObj = moksha_box_string(content);
        ((MokshaHeader *)strObj)->isStatic = true;
        *handlePtr = strObj;
        return strObj;
    }

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

        if (h->isStatic)
            return;

        switch (h->type)
        {
        case TYPE_INT:
            delete (MokshaInt *)ptr;
            break;
        case TYPE_CHAR:
            delete (MokshaChar *)ptr;
            break;
        case TYPE_SHORT:
            delete (MokshaShort *)ptr;
            break;
        case TYPE_LONG:
            delete (MokshaLong *)ptr;
            break;
        case TYPE_FLOAT:
            delete (MokshaFloat *)ptr;
            break;
        case TYPE_DOUBLE:
            delete (MokshaDouble *)ptr;
            break;
        case TYPE_BOOL:
            delete (MokshaBool *)ptr;
            break;
        case TYPE_STRING:
            free(ptr);
            break;
        case TYPE_ARRAY:
        {
            free(ptr);
            break;
        }
        case TYPE_TABLE:
        {
            MokshaTable *t = (MokshaTable *)ptr;
            if (t->entries)
                free(t->entries);
            delete t;
            break;
        }
        case TYPE_CLOSURE:
            free(ptr);
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
        b->header.isStatic = false;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_double(double v)
    {
        MokshaDouble *b = new MokshaDouble();
        b->header.type = TYPE_DOUBLE;
        b->header.isStatic = false;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_bool(int32_t v)
    {
        MokshaBool *b = new MokshaBool();
        b->header.type = TYPE_BOOL;
        b->header.isStatic = false;
        b->val = v;
        return (void *)b;
    }

    void *moksha_box_string(char *v)
    {
        int32_t len = v ? strlen(v) : 0;
        size_t totalSize = sizeof(MokshaString) + len + 1;

        MokshaString *ptr = (MokshaString *)malloc(totalSize);
        ptr->header.type = TYPE_STRING;
        ptr->header.isStatic = false;
        ptr->length = len;

        if (len > 0)
            memcpy(ptr->val, v, len);
        ptr->val[len] = '\0';

        // Hash
        size_t hash = 2166136261u;
        for (int i = 0; i < len; i++)
        {
            hash ^= (uint8_t)ptr->val[i];
            hash *= 16777619;
        }
        ptr->cachedHash = hash;

        return ptr;
    }

    void *moksha_box_char(int8_t v)
    {
        MokshaChar *b = new MokshaChar();
        b->header.type = TYPE_CHAR;
        b->header.isStatic = false;
        b->val = v;
        return b;
    }

    void *moksha_box_short(int16_t v)
    {
        MokshaShort *b = new MokshaShort();
        b->header.type = TYPE_SHORT;
        b->header.isStatic = false;
        b->val = v;
        return b;
    }

    void *moksha_box_long(int64_t v)
    {
        MokshaLong *b = new MokshaLong();
        b->header.type = TYPE_LONG;
        b->header.isStatic = false;
        b->val = v;
        return b;
    }

    void *moksha_box_float(float v)
    {
        MokshaFloat *b = new MokshaFloat();
        b->header.type = TYPE_FLOAT;
        b->header.isStatic = false;
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

        // Support Parsing and ASCII
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

        // Support Parsing Double ("3.14")
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
            return (char *)"";
        MokshaHeader *h = (MokshaHeader *)ptr;
        if (h->type == TYPE_STRING)
            return ((MokshaString *)ptr)->val;
        return (char *)"";
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
            for (int32_t i = 0; i < arr->length; ++i)
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

            // Iterate over the flat 'entries' array instead of 'data' map
            for (int32_t i = 0; i < tbl->capacity; i++)
            {
                if (tbl->entries[i].key != nullptr)
                {
                    if (!first)
                        s += ", ";
                    first = false;

                    char *kStr = (char *)moksha_any_to_str(tbl->entries[i].key);
                    s += kStr;
                    free(kStr);

                    s += ": ";

                    char *vStr = (char *)moksha_any_to_str(tbl->entries[i].value);
                    s += vStr;
                    free(vStr);
                }
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
        MokshaString *ms1 = (MokshaString *)s1;
        MokshaString *ms2 = (MokshaString *)s2;

        // Fast null checks
        int32_t l1 = ms1 ? ms1->length : 0;
        int32_t l2 = ms2 ? ms2->length : 0;
        int32_t newLen = l1 + l2;

        size_t totalSize = sizeof(MokshaString) + newLen + 1;
        MokshaString *res = (MokshaString *)malloc(totalSize);

        res->header.type = TYPE_STRING;
        res->header.isStatic = false;
        res->length = newLen;

        if (l1 > 0)
            memcpy(res->val, ms1->val, l1);
        if (l2 > 0)
            memcpy(res->val + l1, ms2->val, l2);
        res->val[newLen] = '\0';

        // Hash
        size_t hash = 2166136261u;
        for (int i = 0; i < newLen; i++)
        {
            hash ^= (uint8_t)res->val[i];
            hash *= 16777619;
        }
        res->cachedHash = hash;

        return res;
    }

    int32_t moksha_strlen(void *ptr)
    {
        if (!ptr)
            return 0;
        // O(1) Lookup
        return ((MokshaString *)ptr)->length;
    }

    int32_t moksha_get_length(void *targetPtr)
    {
        if (!targetPtr)
            return 0;
        MokshaHeader *h = (MokshaHeader *)targetPtr;
        if (h->type == TYPE_ARRAY)
            return ((MokshaArray *)targetPtr)->length;
        if (h->type == TYPE_STRING)
            return ((MokshaString *)targetPtr)->length;
        return 0;
    }

    void *moksha_string_get_char(void *ptr, int32_t index)
    {
        MokshaString *s = (MokshaString *)ptr;

        // Bounds Check
        if (index < 0 || index >= s->length)
            return moksha_box_string((char *)"");

        uint8_t c = (uint8_t)s->val[index];

        if (globalCharCache[c] != nullptr)
        {
            return globalCharCache[c];
        }

        // Cache Miss: Create, Mark Static, Store
        char b[2] = {(char)c, 0};
        void *newStr = moksha_box_string(b);
        ((MokshaHeader *)newStr)->isStatic = true; // Prevent free()

        globalCharCache[c] = newStr;
        return newStr;
    }

    // --- Array Operations ---
    void *moksha_alloc_array(int32_t cap)
    {
        if (cap < 0)
            cap = 4;
        if (cap == 0)
            cap = 4; // Min capacity

        // Malloc the header + pointers consecutively
        size_t totalSize = sizeof(MokshaArray) + (cap * sizeof(void *));
        MokshaArray *arr = (MokshaArray *)malloc(totalSize);

        arr->header.type = TYPE_ARRAY;
        arr->header.isStatic = false;
        arr->capacity = cap;
        arr->length = 0;

        return (void *)arr;
    }

    void *moksha_array_push_val(void *arrPtr, void *val)
    {
        if (!arrPtr)
            return nullptr;
        MokshaArray *arr = (MokshaArray *)arrPtr;

        if (arr->length >= arr->capacity)
        {
            int32_t newCap = arr->capacity * 2;
            size_t newSize = sizeof(MokshaArray) + (newCap * sizeof(void *));

            MokshaArray *newArr = (MokshaArray *)realloc(arr, newSize);
            if (!newArr)
                panic("Out of memory during array resize");

            newArr->capacity = newCap;
            arr = newArr;
        }

        arr->data[arr->length++] = val;
        return (void *)arr; // Return potentially new pointer
    }

    void *moksha_array_get(void *arrPtr, int32_t index)
    {
        // Safe access with bounds check
        if (!arrPtr)
        {
            moksha_throw_exception(moksha_box_string((char *)"NullRef"));
            return nullptr;
        }
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (index < 0 || index >= arr->length)
        {
            moksha_throw_exception(moksha_box_string((char *)"IndexOutOfBounds"));
            return nullptr;
        }
        return arr->data[index];
    }

    void *moksha_array_get_unsafe(void *arrPtr, int32_t index)
    {
        return ((MokshaArray *)arrPtr)->data[index];
    }

    void moksha_array_set(void *arrPtr, int32_t index, void *val)
    {
        MokshaArray *arr = (MokshaArray *)arrPtr;
        arr->data[index] = val;
    }

    void *moksha_alloc_array_fill(int32_t size, void *defaultVal)
    {
        MokshaArray *arr = (MokshaArray *)moksha_alloc_array(size);
        for (int i = 0; i < size; i++)
            arr->data[i] = defaultVal;
        arr->length = size;
        return (void *)arr;
    }

    void *moksha_array_spread(void *targetPtr, void *sourcePtr)
    {
        if (!targetPtr || !sourcePtr)
            return targetPtr;
        MokshaArray *tgt = (MokshaArray *)targetPtr;
        MokshaArray *src = (MokshaArray *)sourcePtr;

        // Ensure capacity
        int32_t needed = tgt->length + src->length;
        if (needed > tgt->capacity)
        {
            size_t newSize = sizeof(MokshaArray) + (needed * sizeof(void *));
            tgt = (MokshaArray *)realloc(tgt, newSize);
            tgt->capacity = needed;
        }

        // Memcpy (Fast!)
        memcpy(tgt->data + tgt->length, src->data, src->length * sizeof(void *));
        tgt->length = needed;
        return tgt;
    }

    void moksha_array_delete(void *arrPtr, int32_t index)
    {
        if (!arrPtr)
            return;
        MokshaArray *arr = (MokshaArray *)arrPtr;
        if (index < 0 || index >= arr->length)
            return;

        // Memmove to shift left
        int32_t moveCount = arr->length - index - 1;
        if (moveCount > 0)
        {
            memmove(arr->data + index, arr->data + index + 1, moveCount * sizeof(void *));
        }
        arr->length--;
    }

    // --- Table Operations ---
    void table_resize(MokshaTable *t, int32_t newCap)
    {
        TableEntry *newEntries = (TableEntry *)calloc(newCap, sizeof(TableEntry));

        // Rehash
        for (int i = 0; i < t->capacity; i++)
        {
            TableEntry *oldEntry = &t->entries[i];
            if (oldEntry->key == nullptr)
                continue;

            TableEntry *dest = table_find_entry(newEntries, newCap, oldEntry->key);
            dest->key = oldEntry->key;
            dest->value = oldEntry->value;
        }

        free(t->entries);
        t->entries = newEntries;
        t->capacity = newCap;
    }

    void *moksha_alloc_table(int32_t initial_size)
    {
        MokshaTable *t = new MokshaTable();
        t->header = {TYPE_TABLE, false};
        t->count = 0;
        t->capacity = (initial_size < 8) ? 8 : initial_size;
        // Allocate bucket array (calloc zeros it, indicating empty slots)
        t->entries = (TableEntry *)calloc(t->capacity, sizeof(TableEntry));
        return t;
    }

    void *moksha_table_keys(void *tablePtr)
    {
        if (!tablePtr)
            return moksha_alloc_array(0);
        MokshaTable *t = (MokshaTable *)tablePtr;
        MokshaArray *arr = (MokshaArray *)moksha_alloc_array(t->count);
        int idx = 0;
        for (int i = 0; i < t->capacity; i++)
        {
            if (t->entries[i].key != nullptr)
            {
                arr->data[idx++] = t->entries[i].key;
            }
        }
        arr->length = idx;
        return arr;
    }

    void *moksha_table_set(void *tablePtr, void *key, void *val)
    {
        if (!tablePtr)
            return nullptr;
        MokshaTable *t = (MokshaTable *)tablePtr;

        if (t->count + 1 > t->capacity * 0.75)
        {
            table_resize(t, t->capacity * 2);
        }

        TableEntry *entry = table_find_entry(t->entries, t->capacity, key);
        bool isNewKey = (entry->key == nullptr);
        if (isNewKey)
            t->count++;

        entry->key = key;
        entry->value = val;
        return val;
    }

    void *moksha_table_get(void *tablePtr, void *key)
    {
        if (!tablePtr)
            return nullptr;
        MokshaTable *t = (MokshaTable *)tablePtr;
        if (t->count == 0)
            return nullptr;
        TableEntry *entry = table_find_entry(t->entries, t->capacity, key);
        return (entry->key == nullptr) ? nullptr : entry->value;
    }

    void moksha_table_delete(void *tablePtr, void *key)
    {
        if (!tablePtr)
            return;
        MokshaTable *t = (MokshaTable *)tablePtr;
        if (t->count == 0)
            return;
        TableEntry *entry = table_find_entry(t->entries, t->capacity, key);
        if (entry->key == nullptr)
            return;
        // Tombstone
        entry->key = nullptr;
        entry->value = (void *)1; // Mark as tombstone (non-null value, null key)
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
        size_t totalSize = sizeof(MokshaClosure) + captureSize;
        MokshaClosure *closure = (MokshaClosure *)malloc(totalSize);

        closure->header.type = TYPE_CLOSURE;
        closure->header.isStatic = false;
        closure->funcPtr = funcPtr;
        closure->envSize = captureSize;

        if (captureSize > 0)
        {
            if (envInit)
                memcpy(closure->envData, envInit, captureSize);
            else
                memset(closure->envData, 0, captureSize);
        }
        return (void *)closure;
    }

    void *moksha_get_closure_env(void *closurePtr)
    {
        if (!closurePtr)
            return nullptr;
        MokshaClosure *closure = (MokshaClosure *)closurePtr;
        if (closure->envSize == 0)
            return nullptr;
        return (void *)closure->envData;
    }

    void *moksha_get_closure_func(void *closurePtr)
    {
        return closurePtr ? ((MokshaClosure *)closurePtr)->funcPtr : nullptr;
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