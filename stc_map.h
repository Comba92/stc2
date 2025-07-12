#ifndef STC_MAP_IMPL
#define STC_MAP_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "stc_str.h"

// TODO: rework iterators
// TODO: map_get() should return pointer?
// TODO: arena keys

static const int MAP_DEFAULT_CAP = 16; 

// https://nullprogram.com/blog/2018/07/31/
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

// https://theartincode.stanis.me/008-djb2/
long djb2(const char *s, size_t len)
{
  const unsigned long MAGIC = 5381;

  long hash = MAGIC;
  for (const char* c = s; len > 0; c++, len--) {
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

/*
  Little trick; even though str is just a slice (thus doesn't own its memory),
  we can still break the rules. Only inside the map, str OWNS its memory.
  This breaks the const char* qualifier only when we need to free its memory
  (as free() doesn't expect const char* but only char*).
  Plus, supposing the user is not accessing the map entries directly, we can use the
  str pointer as a 'marker' for empty and deleted entries, 
  by assigning it low integers as it's pointer.
  This is hacky, but saves us the need for a marker field!
*/

char* const MAP_ENTRY_EMPTY   = (char*) 0;
char* const MAP_ENTRY_REMOVED = (char*) 1;

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
  size_t len, cap; \
  name##Entry* entries; \
 \
} name; \
 \
name name##_with_cap(size_t cap) { \
  /* https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2 */ \
  if ((cap & (cap - 1)) != 0) { \
    cap--; \
    cap |= cap >> 1; \
    cap |= cap >> 2; \
    cap |= cap >> 4; \
    cap |= cap >> 8; \
    cap |= cap >> 16; \
    cap++; \
  } \
 \
  name##Entry* data = malloc(cap * sizeof(name##Entry)); \
  assert(data != NULL && "list realloc failed"); \
  return (name) { 0, cap, data }; \
} \
name##Entry* name##_search(const name* m, str key) { \
  size_t i = map_hash_key(key, m->cap); \
  name##Entry* e = &m->entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_empty(e->key) && retries < m->cap) { \
    if (!map_key_is_removed(e->key) && str_cmp(e->key, key) == 0) return e; \
    retries += 1; \
    i = map_next_hash(i, retries, m->cap); \
    e = &m->entries[i]; \
  } \
 \
  return NULL; \
} \
 \
bool name##_insert(name*, str, type); \
void name##_reserve(name* m, size_t new_cap) { \
  if (new_cap > m->cap) { \
    name new_map = {0}; \
    new_map.cap = m->cap == 0 ? MAP_DEFAULT_CAP : m->cap; \
    while (new_cap > new_map.cap) new_map.cap *= 2; \
 \
    new_map.entries = calloc(new_map.cap, sizeof(name##Entry)); \
    assert(new_map.entries != NULL && "map realloc failed"); \
 \
    /* rehash */ \
    for(int i=0; i<m->cap; ++i) { \
      name##Entry* e = &m->entries[i]; \
      if (!map_key_is_marker(e->key)) { \
        name##_insert(&new_map, e->key, e->val); \
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
int* name##_get(const name* m, str key) { \
  if (m->len == 0) return NULL; \
   \
  name##Entry* e = name##_search(m, key); \
  if (e == NULL) return NULL; \
  return &e->val; \
} \
 \
bool name##_contains(const name* m, str key) { \
  return name##_get(m, key) != NULL; \
} \
 \
bool name##_insert(name* m, str key, int val) { \
  name##_reserve(m, m->len+1); \
  /* invariant: this can't fail to find an entry, as we always increase cap first */ \
  size_t i = map_hash_key(key, m->cap); \
  name##Entry* e = &m->entries[i]; \
 \
  size_t retries = 0; \
  while (!map_key_is_empty(e->key)) { \
    if (map_key_is_removed(e->key) || str_cmp(e->key, key) == 0) break; \
    retries += 1; \
    i = map_next_hash(i, retries, m->cap); \
    e = &m->entries[i]; \
  } \
 \
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
    free((char*) e->key.data); \
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
    if (!map_key_is_marker(key)) free((char*) key.data); \
  } \
} \
 \
void name##_free(name* m) { \
  name##_clear(m); \
  free(m->entries); \
  m->cap = 0; \
  m->entries = NULL; \
} \
typedef struct { \
  const name* src; \
  name##Entry* curr; \
  size_t skipped; \
} name##Iter; \
 \
 \
name##Iter name##_iter(const name* m) { \
  name##Iter it = { m, NULL, 0 }; \
 \
  if (m->cap == 0) return it; \
  int i; \
  for(i=0; i<m->cap && map_key_is_marker(m->entries[i].key); ++i); \
  it.skipped = i+1; \
  it.curr = i < m->cap ? &m->entries[i] : NULL; \
 \
  return it; \
}  \
 \
bool name##_has(name##Iter* it) { \
  return it->curr != NULL; \
} \
 \
name##Entry* name##_next(name##Iter* it) { \
  if (it->curr == NULL) return NULL; \
 \
  int i; \
  for(i=it->skipped; i<it->src->cap && map_key_is_marker(it->src->entries[i].key); ++i); \
  it->skipped = i+1; \
  name##Entry* e = it->curr; \
  it->curr = i < it->src->cap ? &it->src->entries[i] : NULL; \
   \
  return e; \
} \

#define map_iter(type, ent, it) for(type##Entry* ent; (ent = type##_next(it)) != NULL;)
#define set_iter(ent, it) for(str* ent; (ent = Set_next(it)) != NULL)

// TODO: wouldn't it be more effficient to store u32 or u64 for bits?

typedef struct {
  size_t cap, len;
  str* keys;
  char* bits;
} Set;

struct SetBitIdx {
  size_t byte_idx;
  char bit_idx;
};

struct SetBitIdx Set_bit_idx(size_t i) {
  size_t byte_idx = i / 8;
  size_t bit_idx = i % 8;
  return (struct SetBitIdx) { byte_idx, bit_idx };
}

int Set_search(const Set* s, str key) {
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

bool Set_insert(Set*, str);
void Set_reserve(Set* s, size_t new_cap) {
  if (new_cap > s->cap) {
    Set new_set = {0};
    new_set.cap = s->cap == 0 ? 16 : s->cap;
    while (new_cap > new_set.cap) new_set.cap *= 2;

    new_set.keys = calloc(new_set.cap, sizeof(str));
    new_set.bits = calloc(new_set.cap / 8, sizeof(char));
    assert(new_set.keys != NULL && new_set.bits != NULL && "set realloc failed");

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

bool Set_contains(const Set* s, str key) {
  if (s->len == 0) return false;
  int i = Set_search(s, key);
  if (i == -1) return false;

  struct SetBitIdx idx = Set_bit_idx(i);

  char* byte = &s->bits[idx.byte_idx];
  return (((*byte) >> idx.bit_idx) & 1) != 0;
}

bool Set_insert(Set* s, str key) {
  Set_reserve(s, s->len+1);

  size_t i = map_hash_key(key, s->cap);
  str fkey = s->keys[i];
  int retries = 0;
  while (!map_key_is_empty(fkey)) {
    if (map_key_is_removed(fkey)) break;
    if (str_cmp(key, fkey) == 0) i = -1; break;
    retries += 1;
    i = map_next_hash(i, retries, s->cap);
    fkey = s->keys[i];
  }

  // already inserted
  if (i == -1) return false;

  s->keys[i] = str_clone(key);
  s->len += 1;
  struct SetBitIdx idx = Set_bit_idx(i);
  char* byte = &s->bits[idx.byte_idx];
  *byte |= (1 << idx.bit_idx);

  return true;
}

bool Set_remove(Set* s, str key) {
  if (s->len == 0) return false;
  int i = Set_search(s, key);
  if (i == -1) return false;

  str* fkey = &s->keys[i];
  free((char*) fkey->data);
  fkey->data = MAP_ENTRY_REMOVED;
  fkey->len = 0;
  s->len -= 1;

  struct SetBitIdx idx = Set_bit_idx(i);
  char* byte = &s->bits[idx.byte_idx];
  *byte &= ~(1 << idx.bit_idx);

  return true;
}

void Set_clear(Set* s) { \
  /* keys are owned, free them */
  s->len = 0;
  if (s->keys == NULL) return;

  for (int i=0; i<s->cap; ++i) {
    str key = s->keys[i];
    if (!map_key_is_marker(key)) free((char*) key.data);
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

typedef struct {
  const Set* src;
  str* curr;
  size_t skipped;
} SetIter;

SetIter Set_iter(const Set* s) {
  SetIter it = { s, 0, 0 };
  int i;
  for(i=0; i<s->cap && map_key_is_marker(s->keys[i]); ++i);
  it.skipped = i+1;
  it.curr = i < s->cap ? &s->keys[i] : NULL;

  return it;
}

bool Set_has(const SetIter* s) {
  return s->curr != NULL;
}

str* Set_next(SetIter* it) {
  if (it->curr == NULL) return NULL;

  int i;
  for(i=it->skipped; i<it->src->cap && map_key_is_marker(it->src->keys[i]); ++i);
  it->skipped = i+1;
  str* e = it->curr;
  it->curr = i < it->src->cap ? &it->src->keys[i] : NULL;
  return e;
}

bool Set_is_superset(const Set* this, const Set* other) {
  if (this->len < other->len) return false;

  SetIter other_it  = Set_iter(other);

  // this should have at least ALL elements of other
  for(str* other_e; (other_e = Set_next(&other_it)) != NULL;) {
    // if this doesn't contain an element of other, this it is not a superset
    if (!Set_contains(this, *other_e)) return false;
  }

  return true;
}

bool Set_is_subset(const Set* this, const Set* other) {
  if (this->len > other->len) return false;

  SetIter this_it  = Set_iter(this);

  // ALL this elements should ALWAYS be in other
  for(str* this_e; (this_e = Set_next(&this_it)) != NULL;) {
    // if other doesn't contain an element of this, this it is not a subset
    if (!Set_contains(other, *this_e)) return false;
  }

  return true;
}

bool Set_is_disjoint(const Set* this, const Set* other) {
  // Returns true if this has no elements in common with other
  // same as checking for empty intersection

  SetIter this_it  = Set_iter(this);

  // all elements should be different
  for(str* this_e; (this_e = Set_next(&this_it)) != NULL;) {
    // if an element of this is contained in other, they are not disjoint
    if (Set_contains(other, *this_e)) return false;
  }

  return true;
}

// https://doc.rust-lang.org/std/collections/struct.HashSet.html
// TODO: union, intersection, difference, symmetric_difference iterators

#endif