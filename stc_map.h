#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_str.h"

char* const MAP_ENTRY_EMPTY   = (char*) 0;
char* const MAP_ENTRY_REMOVED = (char*) 1;

// TODO: map iterator
// TODO: clean up this code for god's sake, also might have bugs


// https://theartincode.stanis.me/008-djb2/
long djb2(char *s, size_t len)
{
  const unsigned long MAGIC = 5381;

  long hash = MAGIC;
  for (char* c = s; len > 0; c++, len--) {
    hash = ((hash << 5) + hash) + (unsigned long) *c;
    /* hash = hash * 33 + c */
  }

  return hash;
}

size_t map_hash_key(str key, size_t cap) {
  /*
    invariant: capacity is always multiple of two;
    "hash % map->cap" can be rewritten as a logical AND, avoiding division
  */
  size_t hash = djb2(key.data, key.len);
  return hash & (cap - 1);
}

size_t map_next_hash(size_t h, size_t i, size_t cap) {
  // size_t hash = (h + 1); // linear probing
  size_t hash = (h + i*i); // quadratic probing
  return hash & (cap - 1);
}

bool map_key_is_empty(str key) {
  return key.data == MAP_ENTRY_EMPTY;
}
bool map_key_is_removed(str key) {
  return key.data == MAP_ENTRY_REMOVED;
}
bool map_key_is_marker(str key) {
  return map_key_is_empty(key) || map_key_is_removed(key);
}

#define map_def(type, name) \
typedef struct { \
  str  key; \
  type val; \
} name##Entry; \
 \
typedef struct { \
  size_t cap, len; \
  name##Entry* entries; \
  size_t collisions; \
  size_t last_insert_collisions; \
  size_t biggest_collision_chain; \
  size_t collision_chain_avg; \
} name; \
 \
name##Entry* name##_search(name m, str key) { \
  size_t i = map_hash_key(key, m.cap); \
  name##Entry* e = &m.entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_marker(e->key)) { \
    if (retries >= m.len) return NULL; \
    if (str_cmp(e->key, key) == 0) return e; \
    i = map_next_hash(i, retries+1, m.cap); \
    e = &m.entries[i]; \
    retries += 1; \
  } \
 \
  return e; \
} \
 \
name##Entry* name##_search_for_insert(name* m, str key) { \
  size_t i = map_hash_key(key, m->cap); \
  name##Entry* e = &m->entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_empty(e->key)) { \
    if (str_cmp(e->key, key) == 0) return e; \
    i = map_next_hash(i, retries+1, m->cap); \
    e = &m->entries[i]; \
    retries += 1; \
  } \
  m->last_insert_collisions = retries; \
  m->collision_chain_avg += retries; \
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
      new_map.entries[i].key.data = MAP_ENTRY_EMPTY; \
      new_map.entries[i].key.len  = 0; \
    } \
 \
    /* rehash */ \
    for(int i=0; i<m->cap; ++i) { \
      name##Entry* e = &m->entries[i]; \
      if (!map_key_is_marker(e->key)) { \
        name##Entry* new_entry = name##_search_for_insert(&new_map, e->key); \
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
type* name##_get(name m, str key) { \
  if (m.len == 0) return NULL; \
   \
  name##Entry* e = name##_search(m, key); \
  if (e == NULL || map_key_is_marker(e->key)) return NULL; \
  return &e->val; \
} \
 \
bool name##_contains(name m, str key) { \
  return name##_get(m, key) != NULL; \
} \
 \
bool name##_insert(name* m, str key, type val) { \
  name##_reserve(m, m->len+1); \
  name##Entry* e = name##_search_for_insert(m, key); \
  e->val = val; \
 \
  if (map_key_is_empty(e->key)) { \
    e->key = str_clone(key); \
    m->len += 1; \
    m->collisions += m->last_insert_collisions > 0 ? 1 : 0; \
    m->biggest_collision_chain = m->last_insert_collisions > m->biggest_collision_chain ? m->last_insert_collisions : m->biggest_collision_chain; \
    return true; \
  } else if (map_key_is_removed(e->key)) { \
    e->key = str_clone(key); \
    m->collisions += m->last_insert_collisions > 0 ? 1 : 0; \
    m->biggest_collision_chain = m->last_insert_collisions > m->biggest_collision_chain ? m->last_insert_collisions : m->biggest_collision_chain; \
    return false; \
  } else { \
    return false; \
  } \
} \
 \
bool name##_remove(name* m, str key) { \
  if (m->len == 0) return NULL; \
  name##Entry* e = name##_search(*m, key); \
  if (map_key_is_empty(e->key)) { \
    return false; \
  } else { \
    free(e->key.data); \
    e->key.data = MAP_ENTRY_REMOVED; \
    e->key.len = 0; \
    return true; \
  } \
} \
 \
void name##_free(name* m) { \
  /* keys are owned, free them */ \
  for (int i=0; i<m->cap; ++i) { \
    str key = m->entries[i].key; \
    if (!map_key_is_marker(key)) free(key.data); \
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
    size_t hash = map_hash_key(key);
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
    size_t hash = map_hash_key(key);
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

  void map_free(Map<T>* m) {
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