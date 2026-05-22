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
  MokshaAny key;   // <-- Fat Pointer
  MokshaAny value; // <-- Fat Pointer
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

// ============================================================================
// Internal Helper: Unwrap compiler-boxed IndexExpr pointers
// ============================================================================

// Bare-metal string comparison
static int internal_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static uint32_t hash_any(MokshaAny *key) {
  if (!key || !key->data || !key->vtable)
    return 0;
  uint32_t type_id = key->vtable->type_id;
  uint32_t hash = 2166136261u;

  if (type_id == MOKSHA_TYPE_STRING) {
    const char *str = (const char *)key->data;
    while (*str) {
      hash ^= (uint8_t)(*str++);
      hash *= 16777619;
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
  if (a->data == b->data && a->vtable == b->vtable)
    return true;
  if (!a->data || !b->data || !a->vtable || !b->vtable)
    return false;
  if (a->vtable->type_id != b->vtable->type_id)
    return false;

  if (a->vtable->type_id == MOKSHA_TYPE_STRING) {
    return internal_strcmp((const char *)a->data, (const char *)b->data) == 0;
  }
  if (a->vtable->type_id == MOKSHA_TYPE_I32 ||
      a->vtable->type_id == MOKSHA_TYPE_U32) {
    return *(uint32_t *)a->data == *(uint32_t *)b->data;
  }
  if (a->vtable->type_id == MOKSHA_TYPE_I64 ||
      a->vtable->type_id == MOKSHA_TYPE_U64) {
    return *(uint64_t *)a->data == *(uint64_t *)b->data;
  }
  return false;
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

// ============================================================================
// Moksha Map Builtins
// ============================================================================

// Helper to check key equality securely across types
static bool map_keys_equal(MokshaAny *k1, MokshaAny *k2) {
  if (k1->vtable->type_id != k2->vtable->type_id)
    return false;

  if (k1->data == k2->data)
    return true; // Matches Ints/Bools/Refs

  char *s1 = (char *)k1->data;
  char *s2 = (char *)k2->data;
  if (s1 && s2) {
    return internal_strcmp(s1, s2) == 0; // Safely matches dynamic strings
  }
  return false;
}

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
