#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_list.h"

typedef struct {
  char *key;
  int val;
} MapEntry;

typedef struct {
  size_t cap, len;
  MapEntry* entries;
} Map;

Map map_new(size_t cap) {
  Map res = {0};
  res.cap = cap;
  res.entries = calloc(res.cap, sizeof(MapEntry));
  return res;
}

void map_free(Map* m) {
  free(m->entries);
  m->cap = 0;
  m->len = 0;
}

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static long hash_key(char* key) {
  const unsigned long FNV_OFFSET = 14695981039346656037UL;
  const unsigned long FNV_PRIME = 1099511628211UL;

  long hash = FNV_OFFSET;
  for (char* c = key; *c; c++) {
    hash ^= (long) (unsigned char) *c;
    hash *= FNV_PRIME;
  }

  return hash;
}

int map_get(Map* map, char* key) {
  size_t hash = hash_key(key);
  size_t i = (hash & (map->cap - 1));

  while (map->entries[i].key != NULL) {
    MapEntry* entry = &map->entries[i]; 
    if (strcmp(key, entry->key) == 0) {
      return entry->val;
    }

    i = (i+1) & (map->cap - 1); 
  }

  return 0;
}

void map_set(Map* map, char* key, int val) {
  // realloc
  if (map->len >= map->cap) {
    Map new_map = map_new(map->cap * 2);
    
    for(int i=0; i<map->cap; ++i) {
      MapEntry* entry = &map->entries[i];
      if (entry->key != NULL) {
        map_set(&new_map, entry->key, entry->val);
      }
    }

    free(map->entries);
    map->len = new_map.len;
    map->cap = new_map.cap;
    map->entries = new_map.entries;
  }

  size_t hash = hash_key(key);
  size_t i = (hash & (map->cap - 1));

  // update entry if present
  while (map->entries[i].key != NULL) {
    MapEntry* entry = &map->entries[i];
    if (strcmp(key, entry->key) == 0) {
      entry->val = val;
      return;
    }

    i = (i+1) & (map->cap - 1); 
  }

  // insert entry if not present
  map->entries[i].key = strdup(key);
  map->entries[i].val = val;
  map->len += 1;
}



#endif