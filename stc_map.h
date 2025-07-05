#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_list.h"

char* const MAP_ENTRY_EMPTY   = (char*) 0;
char* const MAP_ENTRY_REMOVED = (char*) 1;

// TODO: map iterator

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

bool key_is_empty(char* key) {
  return key == MAP_ENTRY_EMPTY;
}
bool key_is_removed(char* key) {
  return key == MAP_ENTRY_REMOVED;
}
bool key_is_marker(char* key) {
  return key_is_empty(key) || key_is_removed(key);
}

long hash_key(char* key) {
  return djb2(key);
}

#define map_def(type, name) \
typedef struct { \
  char *key; \
  type val; \
} name##Entry; \
 \
typedef struct { \
  size_t cap, len; \
  name##Entry* entries; \
} name; \
 \
name##Entry* name##_search(name m, char* key) { \
  size_t hash = hash_key(key); \
  /* \
    invariant: capacity is always multiple of two; \
    "hash % map->cap" can be rewritten as a logical AND, avoiding division \
  */ \
  size_t i = hash & (m.cap-1); \
  name##Entry* e = &m.entries[i]; \
 \
  while (!key_is_empty(e->key)) { \
    if (!key_is_removed(e->key) && strcmp(e->key, key) == 0) return e; \
    i = (i+1) & (m.cap-1); \
    e = &m.entries[i]; \
  } \
 \
  return e; \
} \
 \
name##Entry* name##_search_for_insert(name m, char* key) { \
  size_t hash = hash_key(key); \
  size_t i = hash & (m.cap-1); \
  name##Entry* e = &m.entries[i]; \
 \
  while (!key_is_marker(e->key)) { \
    if (strcmp(e->key, key) == 0) return e; \
    i = (i+1) & (m.cap-1); \
    e = &m.entries[i]; \
  } \
 \
  return e; \
} \
 \
void name##_reserve(name* m, size_t new_cap) { \
  if (new_cap > m->cap) { \
    name new_map = {0}; \
    new_map.cap = m->cap == 0 ? 16 : m->cap; \
    while (new_cap > new_map.cap) new_map.cap *= 2; \
 \
    new_map.entries = malloc(sizeof(name##Entry) * new_map.cap); \
    /* set all buckets to empty */ \
    for(int i=0; i<new_map.cap; ++i) { \
      new_map.entries[i].key = MAP_ENTRY_EMPTY; \
    } \
 \
    /* rehash */ \
    for(int i=0; i<m->cap; ++i) { \
      name##Entry* e = &m->entries[i]; \
      if (!key_is_marker(e->key)) { \
        name##Entry* new_entry = name##_search_for_insert(new_map, e->key); \
        new_entry->key = e->key; \
        new_entry->val = e->val; \
      } \
    } \
 \
    /* drop old map */ \
    free(m->entries); \
    /* len should stay the same */ \
    m->cap = new_map.cap; \
    m->entries = new_map.entries; \
  } \
} \
 \
type* name##_get(name m, char* key) { \
  if (m.len == 0) return NULL; \
   \
  name##Entry* e = name##_search(m, key); \
  if (key_is_marker(e->key)) return NULL; \
  return &e->val; \
} \
 \
bool name##_contains(name m, char* key) { \
  return name##_get(m, key) != NULL; \
} \
 \
bool name##_insert(name* m, char* key, type val) { \
  name##_reserve(m, m->len+1); \
  name##Entry* e = name##_search_for_insert(*m, key); \
  e->val = val; \
 \
  if (key_is_empty(e->key)) { \
    e->key = strdup(key); \
    m->len += 1; \
    return true; \
  } else if (key_is_removed(e->key)) { \
    e->key = strdup(key); \
    return false; \
  } else { \
    return false; \
  } \
} \
 \
bool name##_remove(name* m, char* key) { \
  if (m->len == 0) return NULL; \
  name##Entry* e = name##_search(*m, key); \
  if (key_is_empty(e->key)) { \
    return false; \
  } else { \
    free(e->key); \
    e->key = MAP_ENTRY_REMOVED; \
    return true; \
  } \
} \
 \
void name##_drop(name* m) { \
  /* keys are owned, free them */ \
  for (int i=0; i<m->cap; ++i) { \
    char* key = m->entries[i].key; \
    if (!key_is_marker(key)) free(key); \
  } \
 \
  free(m->entries); \
  m->cap = 0; \
  m->len = 0; \
} \


/*
  typedef struct<T> {
    char *key;
    T val;
  } MapEntry<T>;

  typedef struct<T> {
    size_t cap, len;
    MapEntry<T>* entries;
  } Map<T>;

  MapEntry<T>* map_search(Map<T> m, char* key) {
    size_t hash = hash_key(key);
    // invariant: capacity is always multiple of two;
    // "hash % map->cap" can be rewritten as a logical AND, avoiding division
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

  MapEntry<T>* map_search_for_insert(Map<T> m, char* key) {
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

  void map_reserve(Map<T>* m, size_t new_cap) {
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

  T* map_get(Map<T> m, char* key) {
    if (m.cap == 0) return NULL;
    
    MapEntry* e = map_search(m, key);
    if (e->key == MAP_ENTRY_EMPTY || e->key == MAP_ENTRY_REMOVED) return NULL;
    return &e->val;
  }

  bool map_contains(Map<T> m, char* key) {
    return map_get(m, key) != NULL;
  }

  bool map_insert(Map<T>* m, char* key, int val) {
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

  bool map_remove(Map<T>* m, char* key) {
    if (m->len == 0) return NULL;
    MapEntry* entry = map_search(*m, key);
    if (entry->key == NULL) {
      return false;
    } else {
      free(entry->key);
      entry->key = MAP_ENTRY_REMOVED;
      return true;
    }
  }

  double map_load_factor(Map m) {
    return (double) m.len / (double) m.cap;
  }

  void map_drop(Map<T>* m) {
    // keys are owned, free them
    for (int i=0; i<m->cap; ++i) {
      char* key = m->entries[i].key;
      if (key != MAP_ENTRY_EMPTY && key != MAP_ENTRY_REMOVED) free(key);
    }

    free(m->entries);
    m->cap = 0;
    m->len = 0;
  }
*/

#endif