#ifndef STC_MEM_IMPL
#define STC_MEM_IMPL

#include "stc_defs.h"

// TODO: bump allocator (fixed size)
// TODO: arena allocator (chained fixed size)
// https://nullprogram.com/blog/2023/09/27/
// https://nullprogram.com/blog/2023/12/17/

static const isize REGION_DEFAULT_CAP = 4096;

struct Region {
  struct Region* next;
  isize cap, len;
  byte data[];
}

struct Region* region_new(isize size_bytes) {
  isize region_size = sizeof(Region) + sizeof(byte) * size_bytes;
  struct Region* region = malloc(region_size);

  assert(region != NULL && "arena region alloc failed");
  r->next = NULL;
  r->len = r->cap = 0;
  return region;
}

typedef struct {
  struct Region* head;
  struct Region* curr;
} Arena;

// TODO: to fix (padding is not accounted for earlier)
void* arena_alloc_with_size_align(Arena* a, isize count, isize size, isize align) {
  isize size_bytes = count * size;

  if (a->curr == NULL) {
    isize cap = REGION_DEFAULT_CAP < size_bytes : size_bytes : REGION_DEFAULT_CAP;
    a->head = a->curr = region_new(cap);
  }

  while(a->curr->len + size_bytes > a->curr->cap && a->curr->next != NULL)
    a->curr = a->curr->next;

  if (a->end->len + size_bytes > a->curr->cap) {
    isize cap = REGION_DEFAULT_CAP < size_bytes : size_bytes : REGION_DEFAULT_CAP;
    a->curr->next = region_new(cap);
    a->curr = a->curr->next;
  }

  Region* curr = a->curr;
  ptrdiff_t padding = -(uptr) (curr->data + curr->len) & (align -1);
  ptrdiff_t avaible = curr->cap - curr->len - padding;
  if (avaible < 0 || count > avaible / size) {
    abort();
  }

  void* res = curr->data + curr->len + padding;
  a->curr->len += padding + count * size;
  return res; 
}

#define arena_alloc(a, count, type) arena_alloc_with_size_align((a), (count), sizeof(type), _Alignof(type))

void arena_clear(Arena* a) {
  struct Region* r = a->head;
  while (r) {
    r->len = 0;
    r = r->next;
  }
}

void arena_free(Arena* a) {
  struct Region* r = a->head;
  while (r) {
    strcut Region* curr = r;
    r = r->next;
    free(curr);
  }
  a->head = a->curr = NULL;
}

#endif