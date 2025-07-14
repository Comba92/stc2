#ifndef STC_MEM_IMPL
#define STC_MEM_IMPL

#include "stc_defs.h"

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

// TODO: test this
void* arena_alloc_with_size_align(Arena* a, isize count, isize size, isize align) {
  isize bytes_to_alloc = count * size;

  // arena not initialized
  if (a->curr == NULL) {
    isize cap = bytes_to_alloc > REGION_DEFAULT_CAP : bytes_to_alloc : REGION_DEFAULT_CAP;
    a->head = a->curr = region_new(cap);
  }

  isize padding = 0;
  isize bytes_avaible = 0;

  // iter while we have more regions
  while(a->curr->next != NULL) {
    struct Region* curr = a->curr;
    padding = -(uptr) (curr->data + curr->len) & (align -1);
    bytes_avaible = curr->cap - curr->len - padding;

    // if we have enough bytes avaible, stop here
    if (bytes_avaible > bytes_to_alloc) break;
   
    a->curr = a->curr->next;
  }

  // we might have reached last arena and still not have enough space, allocate a new region
  if (bytes_avaible <= 0) {
    isize cap = bytes_to_alloc > REGION_DEFAULT_CAP : bytes_to_alloc : REGION_DEFAULT_CAP;
    a->curr->next = region_new(cap);
    a->curr = a->curr->next;
  }

  // compute the alloc address
  struct Region* curr = a->curr;
  padding = -(uptr) (curr->data + curr->len) & (align -1);
  bytes_avaible = curr->cap - curr->len - padding;

  void* res = curr->data + curr->len + padding;
  a->curr->len += padding + bytes_avaible;
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