#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_list.h"

const char MAP_ENTRY_EMPTY   = 0;
const char MAP_ENTRY_REMOVED = 1;

typedef struct {
  char *key;
  int val;
} MapEntry;

typedef struct {
  size_t cap, len;
  MapEntry* entries;
} Map;

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
long fnv_1a(char* key) {
  const unsigned long FNV_OFFSET = 14695981039346656037UL;
  const unsigned long FNV_PRIME = 1099511628211UL;

  long hash = FNV_OFFSET;
  for (char* c = key; *c; c++) {
    hash ^= (unsigned long) *c;
    hash *= FNV_PRIME;
  }

  return hash;
}

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

MapEntry* map_search(Map m, char* key) {
  size_t hash = hash_key(key);
  // invariant: capacity is always multiple of two;
  // "hash % map->cap" can be rewritten as a logical AND, avoiding division
  size_t i = (hash & (m.cap - 1));

  MapEntry* entry = &m.entries[i];
  while (entry->key != MAP_ENTRY_EMPTY) {
    if (entry->key != MAP_ENTRY_REMOVED && strcmp(key, entry->key) == 0) {
      return entry;
    }

    i = (i+1) & (m.cap - 1);
    entry = &m.entries[i];
  }

  // entry found immediately
  return &m.entries[i];
}

MapEntry* map_search_for_insert(Map m, char* key) {
  size_t hash = hash_key(key);
  size_t i = (hash & (m.cap - 1));

  MapEntry* entry = &m.entries[i];
  while (entry->key != MAP_ENTRY_EMPTY && entry->key != MAP_ENTRY_REMOVED) {
    printf("\t%s\n", entry->key);
    if (strcmp(key, entry->key) == 0) {
      return entry;
    }

    i = (i+1) & (m.cap - 1);
    entry = &m.entries[i];
  }

  return entry;
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

void map_reserve(Map* m, size_t new_cap) {
  if (new_cap >= m->cap) {
    printf("Reserving...\n");
    Map new_map = {0};
    new_map.cap = m->cap == 0 ? 16 : m->cap;
    while (new_cap > new_map.cap) new_map.cap *= 2;

    printf("Sizeof = %ld\n", sizeof(MapEntry));
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
        new_entry->key = strdup(entry->key);
        new_entry->val = entry->val;
      }
    }

    // drop old map
    map_free(m);
    m->len = new_map.len;
    m->cap = new_map.cap;
    m->entries = new_map.entries;
  }
}

MapEntry map_get_entry(Map m, char* key) {
  // todo
}

int map_get(Map m, char* key) {
  MapEntry* entry = map_search(m, key);
  return entry->val;
}

bool map_contains(Map m, char* key) {
  // TODO
}

void map_insert(Map* m, char* key, int val) {
  map_reserve(m, m->len+1);
  printf("Starting search\n");
  MapEntry* entry = map_search_for_insert(*m, key);
  // if (entry->key == MAP_ENTRY_EMPTY || entry->key == MAP_ENTRY_REMOVED)
  printf("Duplicating string %s at %p\n", key, key);
  entry->key = strdup(key);
  entry->val = val;
  m->len += 1;
}

void map_remove(Map* m, char* key) {
  MapEntry* entry = map_search(*m, key);
  free(entry->key);
  entry->key = entry->key == MAP_ENTRY_EMPTY ? MAP_ENTRY_EMPTY : MAP_ENTRY_REMOVED;
  m->len -= entry->key == MAP_ENTRY_EMPTY ? 0 : 1;
}

double map_load_factor(Map m) {
  return m.len / m.cap;
}

#endif