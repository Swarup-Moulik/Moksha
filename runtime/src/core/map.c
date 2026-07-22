#include "../../include/moksha_rt.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

extern void *moksha_mem_alloc(size_t size);
extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);
extern void moksha_rt_retain(void *ptr);
extern void moksha_rt_release(void *ptr);
extern void moksha_mem_free(void *ptr);

#define MAP_INITIAL_CAPACITY 16

typedef struct MapEntry {
  MokshaAny key;
  MokshaAny value;
  struct MapEntry *next;
  struct MapEntry *order_next;
} MapEntry;

typedef struct {
  MapEntry **buckets;
  uint32_t capacity;
  uint32_t size;
  MapEntry *head;
  MapEntry *tail;
} MokshaMap;

/** @brief Internal Helper: Unwrap compiler-boxed IndexExpr pointers */

// Bare-metal string comparison
static int internal_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Safely extract type_id even if vtable is NULL
static uint32_t get_any_type(MokshaAny *key) {
  if (!key || !key->data)
    return 0;

  if (key->vtable)
    return key->vtable->type_id;

  MokshaHeader *hdr =
      (MokshaHeader *)((uint8_t *)key->data - sizeof(MokshaHeader));
  return hdr->type_id;
}

static uint32_t hash_any(MokshaAny *key) {
  if (!key || !key->data)
    return 0;

  uint32_t type_id = get_any_type(key);
  uint32_t hash = 2166136261u;

  if (type_id == MOKSHA_TYPE_STRING) {
    const char *str = (const char *)key->data;
    while (*str) {
      hash ^= (uint8_t)(*str);
      hash *= 16777619;
      str++;
    }
    return hash;
  }

  if (type_id == MOKSHA_TYPE_I32 || type_id == MOKSHA_TYPE_U32 ||
      type_id == MOKSHA_TYPE_F32) {
    uint32_t val = *(uint32_t *)key->data;
    hash ^= val;
    hash *= 16777619;
    return hash;
  }

  if (type_id == MOKSHA_TYPE_I64 || type_id == MOKSHA_TYPE_U64 ||
      type_id == MOKSHA_TYPE_ISIZE || type_id == MOKSHA_TYPE_USIZE ||
      type_id == MOKSHA_TYPE_F64) {
    uint64_t val = *(uint64_t *)key->data;
    hash ^= (uint32_t)(val & 0xFFFFFFFF);
    hash *= 16777619;
    hash ^= (uint32_t)(val >> 32);
    hash *= 16777619;
    return hash;
  }

  // Fallback: Pointer Hashing for Objects
  uint64_t addr = (uint64_t)(uintptr_t)key->data;
  hash ^= (uint32_t)(addr & 0xFFFFFFFF);
  hash *= 16777619;
  hash ^= (uint32_t)(addr >> 32);
  hash *= 16777619;
  return hash;
}

static bool cmp_any(MokshaAny *a, MokshaAny *b) {
  if (!a || !b)
    return false;
  if (a->data == b->data)
    return true; // Fast path for identical pointers
  if (!a->data || !b->data)
    return false;

  uint32_t type_a = get_any_type(a);
  uint32_t type_b = get_any_type(b);

  if (type_a != type_b)
    return false;

  if (type_a == MOKSHA_TYPE_STRING) {
    const char *str_a = (const char *)a->data;
    const char *str_b = (const char *)b->data;
    return internal_strcmp(str_a, str_b) == 0;
  }

  if (type_a == MOKSHA_TYPE_I32 || type_a == MOKSHA_TYPE_U32 ||
      type_a == MOKSHA_TYPE_F32) {
    return *(uint32_t *)a->data == *(uint32_t *)b->data;
  }

  if (type_a == MOKSHA_TYPE_I64 || type_a == MOKSHA_TYPE_U64 ||
      type_a == MOKSHA_TYPE_ISIZE || type_a == MOKSHA_TYPE_USIZE ||
      type_a == MOKSHA_TYPE_F64) {
    return *(uint64_t *)a->data == *(uint64_t *)b->data;
  }

  return false;
}

// Forward to our unified logic
static bool map_keys_equal(MokshaAny *k1, MokshaAny *k2) {
  return cmp_any(k1, k2);
}

void *moksha_rt_map_new(void) {
  MokshaMap *map =
      (MokshaMap *)moksha_rt_alloc(sizeof(MokshaMap), MOKSHA_TYPE_TABLE);
  if (!map)
    moksha_rt_panic("OOM: Failed to allocate Map");

  map->capacity = MAP_INITIAL_CAPACITY;
  map->size = 0;
  map->head = NULL;
  map->tail = NULL;
  map->buckets =
      (MapEntry **)moksha_mem_alloc(sizeof(MapEntry *) * MAP_INITIAL_CAPACITY);

  for (uint32_t i = 0; i < MAP_INITIAL_CAPACITY; i++) {
    map->buckets[i] = NULL;
  }
  return map;
}

void moksha_rt_map_insert(void *map_ptr, MokshaAny *key, MokshaAny *value) {
  if (!map_ptr || !key || !key->data)
    return;
  MokshaMap *map = (MokshaMap *)map_ptr;

  uint32_t hash = hash_any(key);
  uint32_t index = hash % map->capacity;

  MapEntry *entry = map->buckets[index];
  while (entry) {
    if (cmp_any(&entry->key, key)) {
      moksha_rt_release(entry->value.data);
      moksha_rt_retain(value->data);
      entry->value = *value;
      return;
    }
    entry = entry->next;
  }

  moksha_rt_retain(key->data);
  moksha_rt_retain(value->data);

  MapEntry *new_entry = (MapEntry *)moksha_mem_alloc(sizeof(MapEntry));
  new_entry->key = *key;
  new_entry->value = *value;
  new_entry->next = map->buckets[index];
  new_entry->order_next = NULL;

  map->buckets[index] = new_entry;
  map->size++;

  if (map->tail)
    map->tail->order_next = new_entry;
  else
    map->head = new_entry;
  map->tail = new_entry;
}

MokshaAny *moksha_rt_map_get(void *map_ptr, MokshaAny *key) {
  if (!map_ptr || !key || !key->data)
    return NULL;

  MokshaMap *map = (MokshaMap *)map_ptr;
  uint32_t hash = hash_any(key);
  uint32_t index = hash % map->capacity;

  MapEntry *entry = map->buckets[index];
  while (entry) {
    if (cmp_any(&entry->key, key))
      return &entry->value;
    entry = entry->next;
  }
  return NULL;
}

/** @brief Dynamic 'Any' Indexing Dispatcher */

MokshaAny *moksha_rt_any_get(MokshaAny *container, MokshaAny *key) {
  if (!container || !key || !container->data)
    return NULL;

  uint32_t type_id = get_any_type(container);

  // 1. Route Map Lookups (e.g., row["name"])
  if (type_id == MOKSHA_TYPE_TABLE) {
    return moksha_rt_map_get(container->data, key);
  }
  // 2. Route Array/Slice Lookups (e.g., data_in[0])
  else if (type_id == MOKSHA_TYPE_ARRAY) {
    uint32_t key_type = get_any_type(key);
    int64_t index = 0;

    // Safely extract the index regardless of integer size
    if (key_type == MOKSHA_TYPE_I32 || key_type == MOKSHA_TYPE_U32) {
      index = *(int32_t *)key->data;
    } else if (key_type == MOKSHA_TYPE_I64 || key_type == MOKSHA_TYPE_U64 ||
               key_type == MOKSHA_TYPE_ISIZE || key_type == MOKSHA_TYPE_USIZE) {
      index = *(int64_t *)key->data;
    } else {
      moksha_rt_panic("Type Error: Array index must be an integer.");
    }

    MokshaSlice *slice = (MokshaSlice *)container->data;

    // Bounds checking
    if (index < 0 || (uint64_t)index >= slice->length) {
      moksha_rt_panic_out_of_bounds(index, slice->length);
    }

    // Because the container is an 'any', its elements are boxed as 'MokshaAny'
    // structs
    MokshaAny *arr = (MokshaAny *)slice->data;
    return &arr[index];
  }

  // 3. Fallback for invalid types
  moksha_rt_panic("Type Error: Cannot index into a non-collection 'any' type.");
  return NULL;
}

static MapEntry *get_entry_at(MokshaMap *map, int32_t index) {
  int32_t count = 0;
  MapEntry *curr = map->head;
  while (curr) {
    if (count == index)
      return curr;
    count++;
    curr = curr->order_next;
  }
  return NULL;
}

MokshaAny *moksha_rt_map_get_key_at(void *map_ptr, int32_t index) {
  if (!map_ptr)
    return NULL;
  MapEntry *entry = get_entry_at((MokshaMap *)map_ptr, index);
  return entry ? &entry->key : NULL;
}

MokshaAny *moksha_rt_map_get_val_at(void *map_ptr, int32_t index) {
  if (!map_ptr)
    return NULL;
  MapEntry *entry = get_entry_at((MokshaMap *)map_ptr, index);
  return entry ? &entry->value : NULL;
}

void moksha_rt_map_free_internal(void *map_ptr) {
  if (!map_ptr)
    return;
  MokshaMap *map = (MokshaMap *)map_ptr;

  for (uint32_t i = 0; i < map->capacity; i++) {
    MapEntry *entry = map->buckets[i];
    while (entry) {
      MapEntry *next = entry->next;

      // Release the keys and values so they can drop to 0 safely
      moksha_rt_release(entry->key.data);
      moksha_rt_release(entry->value.data);

      // Free the linked list node itself
      moksha_mem_free(entry);
      entry = next;
    }
  }

  // Free the bucket array
  if (map->buckets) {
    moksha_mem_free(map->buckets);
  }
}

int32_t moksha_rt_map_len(void *map_ptr) {
  if (!map_ptr)
    return 0;

  MokshaMap *map = (MokshaMap *)map_ptr;
  return (int32_t)map->size;
}

/** @brief Moksha Map Builtins */

extern MokshaAny *moksha_rt_map_get(void *map_ptr, MokshaAny *key);

bool moksha_rt_map_has(void *map_ptr, MokshaAny *key) {
  if (!map_ptr || !key)
    return false;
  return moksha_rt_map_get(map_ptr, key) != NULL;
}

int32_t moksha_rt_map_length(void *map_ptr) {
  if (!map_ptr)
    return 0;
  return ((MokshaMap *)map_ptr)->size;
}

void moksha_rt_map_clear(void *map_ptr) {
  if (!map_ptr)
    return;
  MokshaMap *map = (MokshaMap *)map_ptr;
  MapEntry *curr = map->head;
  while (curr) {
    MapEntry *next = curr->order_next;
    moksha_mem_free(curr);
    curr = next;
  }
  for (uint32_t i = 0; i < map->capacity; i++) {
    map->buckets[i] = NULL;
  }
  map->head = NULL;
  map->tail = NULL;
  map->size = 0;
}

void moksha_rt_map_remove(void *map_ptr, MokshaAny *key) {
  if (!map_ptr || !key)
    return;
  MokshaMap *map = (MokshaMap *)map_ptr;

  MapEntry *curr = map->head;
  MapEntry *prev = NULL;
  while (curr) {
    if (map_keys_equal(&curr->key, key)) {
      // 1. Unlink from insertion-ordered list
      if (prev)
        prev->order_next = curr->order_next;
      else
        map->head = curr->order_next;
      if (map->tail == curr)
        map->tail = prev;

      // 2. Unlink from hash buckets
      for (uint32_t i = 0; i < map->capacity; i++) {
        MapEntry *b_curr = map->buckets[i];
        MapEntry *b_prev = NULL;
        while (b_curr) {
          if (b_curr == curr) {
            if (b_prev)
              b_prev->next = b_curr->next;
            else
              map->buckets[i] = b_curr->next;
            break;
          }
          b_prev = b_curr;
          b_curr = b_curr->next;
        }
      }

      map->size--;
      moksha_mem_free(curr);
      return;
    }
    prev = curr;
    curr = curr->order_next;
  }
}
