#include "../../include/moksha_rt.h"
#include <stdint.h>
#include <string.h>

extern void *moksha_mem_alloc(size_t size);
extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_panic(const char *message);

#define MAP_INITIAL_CAPACITY 16

typedef struct MapEntry {
  void *key;
  void *value;
  struct MapEntry *next;
} MapEntry;

typedef struct {
  MapEntry **buckets;
  uint32_t capacity;
  uint32_t size;
} MokshaMap;

// Dynamic Hashing based on ARC Type
static uint32_t hash_any(void *key) {
  if (!key)
    return 0;

  MokshaHeader *header = ((MokshaHeader *)key) - 1;
  uint32_t hash = 2166136261u;

  if (header->type_id == MOKSHA_TYPE_STRING) {
    // Dereference the payload to get the char*
    const char *str = *(const char **)key;
    if (!str)
      return 0;
    while (*str) {
      hash ^= (uint8_t)(*str++);
      hash *= 16777619;
    }
    return hash;
  }

  if (header->type_id == MOKSHA_TYPE_I32 ||
      header->type_id == MOKSHA_TYPE_U32) {
    uint32_t val = *(uint32_t *)key;
    hash ^= val;
    hash *= 16777619;
    return hash;
  }

  // Fallback hash for ptr addresses
  uint64_t addr = (uint64_t)key;
  hash ^= (addr & 0xFFFFFFFF);
  hash *= 16777619;
  return hash;
}

// Dynamic Comparison based on ARC Type
static bool cmp_any(void *a, void *b) {
  if (a == b)
    return true;
  if (!a || !b)
    return false;

  MokshaHeader *hA = ((MokshaHeader *)a) - 1;
  MokshaHeader *hB = ((MokshaHeader *)b) - 1;

  if (hA->type_id != hB->type_id)
    return false;

  if (hA->type_id == MOKSHA_TYPE_STRING) {
    const char *strA = *(const char **)a;
    const char *strB = *(const char **)b;
    if (!strA || !strB)
      return strA == strB;
    return strcmp(strA, strB) == 0;
  }

  if (hA->type_id == MOKSHA_TYPE_I32 || hA->type_id == MOKSHA_TYPE_U32) {
    return *(uint32_t *)a == *(uint32_t *)b;
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
  map->buckets =
      (MapEntry **)moksha_mem_alloc(sizeof(MapEntry *) * MAP_INITIAL_CAPACITY);

  for (uint32_t i = 0; i < MAP_INITIAL_CAPACITY; i++) {
    map->buckets[i] = NULL;
  }
  return map;
}

void moksha_rt_map_insert(void *map_ptr, void *key, void *value) {
  if (!map_ptr || !key)
    return;
  MokshaMap *map = (MokshaMap *)map_ptr;

  uint32_t hash = hash_any(key);
  uint32_t index = hash % map->capacity;

  MapEntry *entry = map->buckets[index];
  while (entry) {
    if (cmp_any(entry->key, key)) {
      moksha_rt_release(entry->value);
      moksha_rt_retain(value);
      entry->value = value;
      return;
    }
    entry = entry->next;
  }

  moksha_rt_retain(key);
  moksha_rt_retain(value);

  MapEntry *new_entry = (MapEntry *)moksha_mem_alloc(sizeof(MapEntry));
  new_entry->key = key;
  new_entry->value = value;
  new_entry->next = map->buckets[index];

  map->buckets[index] = new_entry;
  map->size++;
}

void *moksha_rt_map_get(void *map_ptr, void *key) {
  if (!map_ptr || !key)
    return NULL;
  MokshaMap *map = (MokshaMap *)map_ptr;

  uint32_t hash = hash_any(key);
  uint32_t index = hash % map->capacity;

  MapEntry *entry = map->buckets[index];
  while (entry) {
    if (cmp_any(entry->key, key)) {
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}
