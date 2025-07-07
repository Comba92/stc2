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

typedef struct {
  str key;
  int val;
} MapEntry;

typedef struct {
  size_t cap, len;
  MapEntry* entries;

  size_t collisions;
  size_t last_insert_collisions;
  size_t biggest_collision_chain;
  size_t collision_chain_avg;
} Map;

MapEntry* Map_search(Map m, str key) {
  size_t i = map_hash_key(key, m.cap);
  MapEntry* e = &m.entries[i];

  size_t retries = 0;
  while (!map_key_is_empty(e->key) && retries < m.cap) {
    if (str_cmp(e->key, key) == 0) return e;
    i = map_next_hash(i, retries+1, m.cap);
    e = &m.entries[i];
    retries += 1;
  }

  return NULL;
}

MapEntry* Map_search_for_insert(Map* m, str key) {
  /* invariant: this can't fail to find an entry, as we always increase cap first */
  size_t i = map_hash_key(key, m->cap);
  MapEntry* e = &m->entries[i];

  size_t retries = 0;
  while (!map_key_is_empty(e->key)) {
    if (str_cmp(e->key, key) == 0) return e;
    i = map_next_hash(i, retries+1, m->cap);
    e = &m->entries[i];
    retries += 1;
  }
  m->last_insert_collisions = retries;
  m->collision_chain_avg += retries;

  return e;
}

void Map_reserve(Map* m, size_t new_cap) {
  if (new_cap > m->cap) {
    Map new_map = {0};
    new_map.cap = m->cap == 0 ? 16 : m->cap;
    while (new_cap > new_map.cap) new_map.cap *= 2;

    new_map.entries = malloc(sizeof(MapEntry) * new_map.cap);
    assert(new_map.entries != NULL && "map realloc failed");
    
    /* set all buckets to empty */
    for(int i=0; i<new_map.cap; ++i) {
      new_map.entries[i].key.data = MAP_ENTRY_EMPTY;
      new_map.entries[i].key.len = 0;
    }

    /* rehash */
    for(int i=0; i<m->cap; ++i) {
      MapEntry* e = &m->entries[i];
      if (!map_key_is_marker(e->key)) {
        MapEntry* new_entry = Map_search_for_insert(&new_map, e->key);
        new_entry->key = e->key;
        new_entry->val = e->val;
      }
    }

    /* drop old map */
    free(m->entries);
    /* len should stay the same */
    m->cap = new_map.cap;
    m->entries = new_map.entries;
  }
}

int* Map_get(Map m, str key) {
  if (m.len == 0) return NULL;
  
  MapEntry* e = Map_search(m, key);
  if (e == NULL) return NULL;
  return &e->val;
}

bool Map_contains(Map m, str key) {
  return Map_get(m, key) != NULL;
}

bool Map_insert(Map* m, str key, int val) {
  Map_reserve(m, m->len+1);
  MapEntry* e = Map_search_for_insert(m, key);
  e->val = val;

  if (map_key_is_empty(e->key)) {
    e->key = str_clone(key);
    m->len += 1;
    m->collisions += m->last_insert_collisions > 0 ? 1 : 0;
    m->biggest_collision_chain = m->last_insert_collisions > m->biggest_collision_chain ? m->last_insert_collisions : m->biggest_collision_chain;
    return true;
  } else if (map_key_is_removed(e->key)) {
    e->key = str_clone(key);
    m->collisions += m->last_insert_collisions > 0 ? 1 : 0;
    m->biggest_collision_chain = m->last_insert_collisions > m->biggest_collision_chain ? m->last_insert_collisions : m->biggest_collision_chain;
    return false;
  } else {
    return false;
  }
}

bool Map_remove(Map* m, str key) {
  if (m->len == 0) return false;
  MapEntry* e = Map_search(*m, key);
  if (e == NULL) {
    return false;
  } else {
    free(e->key.data);
    e->key.data = MAP_ENTRY_REMOVED;
    e->key.len = 0;
    return true;
  }
}

void Map_free(Map* m) {
  /* keys are owned, free them */
  for (int i=0; i<m->cap; ++i) {
    str key = m->entries[i].key;
    if (!map_key_is_marker(key)) free(key.data);
  }

  free(m->entries);
  m->cap = 0;
  m->len = 0;
  m->entries = NULL;
}

typedef struct {
  str* keys;
  char* bits;
} Set;