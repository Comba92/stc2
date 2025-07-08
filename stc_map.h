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
// TODO: can search be merged into one?
// TODO: replace operation map_replace(m, old_key, new_key, value) ?

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
  str key; \
  type val; \
} name##Entry; \
 \
typedef struct { \
  size_t cap, len; \
  name##Entry* entries; \
 \
} name; \
 \
name##Entry* name##_search(name* m, str key) { \
  size_t i = map_hash_key(key, m->cap); \
  name##Entry* e = &m->entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_empty(e->key) && retries < m->cap) { \
    if (!map_key_is_removed(e->key) && str_cmp(e->key, key) == 0) return e; \
    i = map_next_hash(i, retries+1, m->cap); \
    e = &m->entries[i]; \
    retries += 1; \
  } \
 \
  return NULL; \
} \
 \
name##Entry* name##_search_for_insert(name* m, str key) { \
  /* invariant: this can't fail to find an entry, as we always increase cap first */ \
  size_t i = map_hash_key(key, m->cap); \
  name##Entry* e = &m->entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_empty(e->key)) { \
    if (map_key_is_removed(e->key) || str_cmp(e->key, key) == 0) return e; \
    i = map_next_hash(i, retries+1, m->cap); \
    e = &m->entries[i]; \
    retries += 1; \
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
    assert(new_map.entries != NULL && "map realloc failed"); \
     \
    /* set all buckets to empty */ \
    for(int i=0; i<new_map.cap; ++i) { \
      new_map.entries[i].key.data = MAP_ENTRY_EMPTY; \
      new_map.entries[i].key.len = 0; \
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
int* name##_get(name* m, str key) { \
  if (m->len == 0) return NULL; \
   \
  name##Entry* e = name##_search(m, key); \
  if (e == NULL) return NULL; \
  return &e->val; \
} \
 \
bool name##_contains(name* m, str key) { \
  return name##_get(m, key) != NULL; \
} \
 \
bool name##_insert(name* m, str key, int val) { \
  name##_reserve(m, m->len+1); \
  name##Entry* e = name##_search_for_insert(m, key); \
  e->val = val; \
 \
  if (map_key_is_empty(e->key)) { \
    e->key = str_clone(key); \
    m->len += 1; \
    return true; \
  } else if (map_key_is_removed(e->key)) { \
    e->key = str_clone(key); \
    return false; \
  } else { \
    return false; \
  } \
} \
 \
bool name##_remove(name* m, str key) { \
  if (m->len == 0) return false; \
  name##Entry* e = name##_search(m, key); \
  if (e == NULL) { \
    return false; \
  } else { \
    free(e->key.data); \
    e->key.data = MAP_ENTRY_REMOVED; \
    e->key.len = 0; \
    m->len -= 1; \
    return true; \
  } \
} \
 \
void name##_clear(name* m) { \
  m->len = 0; \
  if (m->entries == NULL) return; \
  /* keys are owned, free them */ \
  for (int i=0; i<m->cap; ++i) { \
    str key = m->entries[i].key; \
    if (!map_key_is_marker(key)) free(key.data); \
  } \
} \
 \
void name##_free(name* m) { \
  name##_clear(m); \
  free(m->entries); \
  m->cap = 0; \
  m->entries = NULL; \
} \
typedef struct {
  name* src;
  size_t curr;
  size_t skipped;
} name##Iter;

name##Iter name##_iter(name* m) {
  name##Iter it = { m, 0, 0 };
  // TODO: fetch first value
} 

type* name##_next(name##Iter* it) {
  // TODO: fetch next value
}

typedef struct {
  size_t cap, len;
  str* keys;
  char* bits;
} Set;

typedef struct {
  size_t byte_idx;
  char bit_idx;
} SetIndex;

SetIndex Set_real_idx(size_t i) {
  size_t byte_idx = i / 8;
  size_t bit_idx = i % 8;
  return (SetIndex) { byte_idx, bit_idx };
}

int Set_search(Set* s, str key) {
  size_t i = map_hash_key(key, s->cap);
  str fkey = s->keys[i];

  int retries = 0;
  while (!map_key_is_empty(fkey) && retries < s->cap) {
    if (!map_key_is_removed(fkey) && str_cmp(key, fkey) == 0) return i;

    i = map_next_hash(i, retries+1, s->cap);
    fkey = s->keys[i];
    retries += 1;
  }

  return -1;
}

int Set_search_for_insert(Set* s, str key) {
  size_t i = map_hash_key(key, s->cap);
  str fkey = s->keys[i];

  int retries = 0;
  while (!map_key_is_empty(fkey)) {
    if (map_key_is_removed(fkey)) break;
    if (str_cmp(key, fkey) == 0) return -1;

    i = map_next_hash(i, retries+1, s->cap);
    fkey = s->keys[i];
    retries += 1;
  }

  return i;
}

bool Set_insert(Set*, str);
void Set_reserve(Set* s, size_t new_cap) {
  if (new_cap > s->cap) {
    Set new_set = {0};
    new_set.cap = s->cap == 0 ? 16 : s->cap;
    while (new_cap > new_set.cap) new_set.cap *= 2;

    new_set.keys = malloc(new_set.cap * sizeof(str));
    new_set.bits = calloc(new_set.cap / 8, 1);
    assert(new_set.keys != NULL && new_set.bits != NULL && "set realloc failed");

    /* empty all buckets */
    for(int i=0; i<new_set.cap; ++i) {
      new_set.keys[i].data = MAP_ENTRY_EMPTY;
      new_set.keys[i].len = 0;
    }

    /* rehash */
    for(int i=0; i<s->cap; ++i) {
      str* key = &s->keys[i];
      if (!map_key_is_empty(*key)) Set_insert(&new_set, *key);
    }

    /* drop old map */
    free(s->keys);
    free(s->bits);
    /* len should stay the same */
    s->cap = new_set.cap;
    s->keys = new_set.keys;
    s->bits = new_set.bits;
  }
}

bool Set_contains(Set* s, str key) {
  if (s->len == 0) return false;
  int i = Set_search(s, key);
  if (i == -1) return false;

  SetIndex idx = Set_real_idx(i);

  char* byte = &s->bits[idx.byte_idx];
  return (((*byte) >> idx.bit_idx) & 1) != 0;
}

bool Set_insert(Set* s, str key) {
  Set_reserve(s, s->len+1);

  int i = Set_search_for_insert(s, key);
  // already inserted
  if (i == -1) return false;

  SetIndex idx = Set_real_idx(i);

  s->keys[i] = str_clone(key);
  s->len += 1;
  char* byte = &s->bits[idx.byte_idx];
  *byte |= (1 << idx.bit_idx);

  return true;
}

bool Set_remove(Set* s, str key) {
  if (s->len == 0) return false;
  int i = Set_search(s, key);
  if (i == -1) return false;

  SetIndex idx = Set_real_idx(i);
  str* fkey = &s->keys[i];
  free(fkey->data);
  fkey->data = MAP_ENTRY_REMOVED;
  fkey->len = 0;
  char* byte = &s->bits[idx.byte_idx];
  *byte &= ~(1 << idx.bit_idx);
  s->len -= 1;
  return true;
}

void Set_clear(Set* s) { \
  /* keys are owned, free them */
  s->len = 0;
  if (s->keys == NULL) return;

  for (int i=0; i<s->cap; ++i) {
    str key = s->keys[i];
    if (!map_key_is_marker(key)) free(key.data);
  }
}

void Set_free(Set* s) {
  Set_clear(s);
  free(s->keys);
  free(s->bits);
  s->cap = 0;
  s->keys = NULL;
  s->bits = NULL;
}

bool Set_is_superset(Set* this, Set* other) {

}

bool Set_is_subset(Set* this, Set* other) {
  
}

bool Set_is_disjoint(Set* this, Set* other) {
  // Returns true if this has no elements in common with other
  // same as checking for empty intersection
}

// https://doc.rust-lang.org/std/collections/struct.HashSet.html
// TODO: union, intersection, difference, symmetric_difference

#endif