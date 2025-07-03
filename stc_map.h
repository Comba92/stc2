#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_list.h"

char* const MAP_ENTRY_EMPTY   = (char*) 0;
char* const MAP_ENTRY_REMOVED = (char*) 1;

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
// long fnv_1a(char* key) {
//   const unsigned long FNV_OFFSET = 14695981039346656037UL;
//   const unsigned long FNV_PRIME = 1099511628211UL;

//   long hash = FNV_OFFSET;
//   for (char* c = key; *c; c++) {
//     hash ^= (unsigned long) *c;
//     hash *= FNV_PRIME;
//   }

//   return hash;
// }

// https://theartincode.stanis.me/008-djb2/
long djb2(char *key)
{
  const unsigned long MAGIC = 5381;

  long hash = MAGIC;
  for (char* c = key; *c; c++) {
    hash = ((hash << 5) + hash) + (unsigned long) *c;
    /* hash = hash * 33 + c */
  }

  return hash;
}


long hash_key(char* key) {
  return djb2(key);
}

typedef struct {
  char *key;
  int val;
} MapEntry;

typedef struct {
  size_t cap, len;
  MapEntry* entries;
} Map;

MapEntry* map_search(Map m, char* key) {
  size_t hash = hash_key(key);
  /* \
    invariant: capacity is always multiple of two; \
    "hash % map->cap" can be rewritten as a logical AND, avoiding division \
  */
  size_t i = hash & (m.cap-1);
  MapEntry* e = &m.entries[i];

  while (e->key != MAP_ENTRY_EMPTY) {
    // found correct bucket
    if (e->key != MAP_ENTRY_REMOVED && strcmp(e->key, key) == 0) return e;
    i = (i+1) & (m.cap-1);
    e = &m.entries[i];
  }

  // found empty bucket
  return e;
}

MapEntry* map_search_for_insert(Map m, char* key) {
  size_t hash = hash_key(key);
  size_t i = hash & (m.cap-1);
  MapEntry* e = &m.entries[i];

  while (e->key != MAP_ENTRY_EMPTY && e->key != MAP_ENTRY_REMOVED) {
    // found correct bucket, overwrite
    if (strcmp(e->key, key) == 0) return e;
    i = (i+1) & (m.cap-1);
    e = &m.entries[i];
  }

  // found empty/deleted bucket, we can insert
  return e;
}

void map_reserve(Map* m, size_t new_cap) {
  if (new_cap > m->cap) {
    Map new_map = {0};
    new_map.cap = m->cap == 0 ? 16 : m->cap;
    while (new_cap > new_map.cap) new_map.cap *= 2;

    new_map.entries = malloc(sizeof(MapEntry) * new_map.cap);
    // set all buckets to empty
    for(int i=0; i<new_map.cap; ++i) {
      new_map.entries[i].key = MAP_ENTRY_EMPTY;
    }

    // rehash
    for(int i=0; i<m->cap; ++i) {
      MapEntry* entry = &m->entries[i];
      if (entry->key != MAP_ENTRY_EMPTY && entry->key != MAP_ENTRY_REMOVED) {
        MapEntry* new_entry = map_search_for_insert(new_map, entry->key);
        new_entry->key = entry->key;
        new_entry->val = entry->val;
      }
    }

    // drop old map
    free(m->entries);
    // len should stay the same
    m->cap = new_map.cap;
    m->entries = new_map.entries;
  }
}

int* map_get(Map m, char* key) {
  if (m.cap == 0) return NULL;
  
  MapEntry* e = map_search(m, key);
  if (e->key == MAP_ENTRY_EMPTY || e->key == MAP_ENTRY_REMOVED) return NULL;
  return &e->val;
}

bool map_contains(Map m, char* key) {
  return map_get(m, key) != NULL;
}

bool map_insert(Map* m, char* key, int val) {
  map_reserve(m, m->len+1);
  MapEntry* entry = map_search_for_insert(*m, key);
  entry->val = val;

  if (entry->key == MAP_ENTRY_EMPTY) {
    entry->key = strdup(key);
    m->len += 1;
    return true;
  } else if (entry->key == MAP_ENTRY_REMOVED) {
    entry->key = strdup(key);
    return false;
  } else {
    return false;
  }
}

bool map_remove(Map* m, char* key) {
  MapEntry* entry = map_search(*m, key);
  if (entry->key == NULL) {
    return false;
  } else {
    free(entry->key);
    entry->key = MAP_ENTRY_REMOVED;
    m->len -= 1;
    return true;
  }
}

double map_load_factor(Map m) {
  return m.len / m.cap;
}

void map_free(Map* m) {
  // keys are owned, free them
  for (int i=0; i<m->cap; ++i) {
    char* key = m->entries[i].key;
    if (key != MAP_ENTRY_EMPTY && key != MAP_ENTRY_REMOVED) free(key);
  }

  free(m->entries);
  m->cap = 0;
  m->len = 0;
}

#endif