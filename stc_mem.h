#ifndef STC_MEM_IMPL
#define STC_MEM_IMPL

// TODO: bump allocator (fixed size)
// TODO: arena allocator (chained fixed size)
// https://nullprogram.com/blog/2023/09/27/

static const int REGION_DEFAULT_LEN = 4096;

struct Region {
  struct Region* next;
  size_t cap, len;
  char* data;
}

struct Region region_new(size_t cap) {

}

typedef struct {
  struct Region* head;
} Arena;

void* arena_alloc(Arena* a, size_t size_bytes) {

}

void arena_free(Arena* a) {

}

#endif