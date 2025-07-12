#ifndef STC_LIST_IMPL
#define STC_LIST_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define rangefor(type, it, start, end) for (type it = (start); it < (end); ++it)
#define listfor(type, it, list) for (type it = 0; it < (list)->len; ++it)
#define listforrev(type, it, list) for (type it = (list)->len-1; it >= 0; --it)
#define listforeach(type, it, list) for (type* it = (list)->data; it < (list)->data + (list)->len; ++it)

static const int LIST_DEFAULT_CAP = 16; 

// TODO: deal with indexes sizes, just fucking use stdint.h
// TODO: bitflags?
// TODO: bitfields?
// TODO: not sure if i want insert and remove
// TODO: array_heap_to_list() is extremely dangerous
// TODO: small size opt: https://nullprogram.com/blog/2016/10/07/
// TODO: list_repeat() and list_fill()

#define list_def(type, name) \
typedef struct { \
  size_t len, cap; \
  type* data; \
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
  type* data = malloc(cap * sizeof(type)); \
  assert(data != NULL && "list realloc failed"); \
  return (name) { 0, cap, data }; \
} \
 \
void name##_reserve(name* l, size_t new_cap) { \
  if (new_cap > l->cap) { \
    l->cap = l->cap == 0 ? LIST_DEFAULT_CAP : l->cap; \
    while (new_cap > l->cap) l->cap *= 2; \
 \
    l->data = realloc(l->data, sizeof(type) * l->cap); \
    assert(l->data != NULL && "list realloc failed"); \
  } \
} \
void name##_resize(name* l, size_t new_len, type value) { \
  if (new_len <= l->len) { \
    l->len = new_len; \
  } else { \
    name##_reserve(l, l->len + new_len); \
    /*size_t range = (new_len - l->len); \
    for (int i=0; i<range; ++i) { \
      l->data[l->len + i] = value; \
    } */ \
    rangefor(int, i, l->len, new_len) { \
      l->data[i] = value; \
    } \
    l->len = new_len; \
  } \
} \
 \
void name##_push(name* l, type value) { \
  name##_reserve(l, l->len + 1); \
  l->data[l->len++] = value; \
} \
 \
void name##_assert(name l, size_t i) { \
  assert(i < l.len && "list access out of bounds"); \
} \
 \
type* name##_first(name l) { \
  name##_assert(l, 0); \
  return &l.data[0]; \
} \
type* name##_last(name l) { \
  name##_assert(l, 0); \
  return &l.data[l.len-1]; \
} \
 \
type name##_pop(name* l) { \
  assert(l->len > 0 && "popped empty list"); \
  return l->data[--l->len]; \
} \
 \
void name##_swap(name* l, size_t a, size_t b) { \
  name##_assert(*l, a); \
  name##_assert(*l, b); \
  type tmp = l->data[a]; \
  l->data[a] = l->data[b]; \
  l->data[b] = tmp; \
} \
 \
name name##_shuffle(name* l) { \
  /* https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle */ \
  srand(time(NULL)); \
  for(int i=l->len-1; i>0; --i) { \
    int r = rand() % (i+1); \
    name##_swap(l, i, r); \
  } \
  return *l; \
} \
type name##_remove_swap(name* l, size_t i) { \
  name##_assert(*l, i); \
  l->len--; \
  type res = l->data[i]; \
  l->data[i] = l->data[l->len]; \
  return res; \
} \
 \
typedef int (*name##CmpFn)(const type* a, const type* b); \
int name##_find(const name* l, type value, name##CmpFn pred) { \
  const type* b = (const type*) &value; \
  listfor(int, i, l) { \
    const type* a = (const type*) &l->data[i]; \
    if (pred(a, b) == 0) return i; \
  } \
 \
  return -1; \
} \
 \
bool name##_contains(const name* l, type value, name##CmpFn pred) { \
  return name##_find(l, value, pred) != -1; \
} \
 \
name name##_reverse(name* l) { \
  for(int left=0, right=l->len-1; left<right; ++left, --right) { \
    name##_swap(l, left, right); \
  } \
  return *l; \
} \
 \
void name##_append_array(name* l, const type* arr, size_t arr_len) { \
  name##_reserve(l, l->len + arr_len); \
  memcpy(l->data + l->len, arr, arr_len * sizeof(type)); \
  l->len += arr_len; \
} \
 \
name name##_from_array(const type* arr, size_t arr_len) { \
  name res = {0}; \
  name##_append_array(&res, arr, arr_len); \
  return res; \
} \
 \
name array_heap_to_##name(type* *const arr, size_t arr_len) { \
  name res = {0}; \
  res.len = arr_len; \
  /* trick: if we set cap to 0, next time we push, will be reallocated accordingly to len */ \
  /* this way, we don't need to realloc here! */ \
  res.cap = 0; \
  res.data = *arr; \
  *arr = NULL; \
  return res; \
} \
 \
void name##_append(name* this, name other) { \
  name##_append_array(this, (const type*) other.data, other.len); \
} \
 \
name name##_clone(name l) { \
  return name##_from_array((const type*) l.data, l.len); \
} \
 \
void name##_free(name* l) { \
  free(l->data); \
  l->cap = 0; \
  l->len = 0; \
  l->data = NULL; \
} \
 \
bool name##_is_sorted(const name* l, name##CmpFn pred) { \
  for(int i=0; i<l->len-1; ++i) { \
    const type* a = (const type*) &l->data[i]; \
    const type* b = (const type*) &l->data[i+1]; \
    if (pred(a, b) >= 0) return false; \
  } \
  return true; \
} \
 \
name name##_sort(name* l, name##CmpFn pred) { \
  /* we cast the function pointer because we are crazy and we can do that */ \
  qsort(l->data, l->len, sizeof(type), (int (*)(const void*, const void*)) pred); \
  return *l; \
} \
 \
typedef bool (*name##EqFn)(const type* val); \
bool name##_all(name* l, name##EqFn pred) { \
  listfor(int, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (!pred(it)) return false; \
  } \
  return true; \
} \
 \
bool name##_any(name* l, name##EqFn pred) { \
  listfor(int, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (pred(it)) return true; \
  } \
  return false; \
} \
 \
size_t name##_count(name* l, name##EqFn pred) { \
  size_t count = 0; \
  listfor(int, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    count += pred(it); \
  } \
  return count; \
} \
 \
name name##_filter(name* l, name##EqFn pred) { \
  size_t curr = 0; \
  listfor(int, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (pred(it)) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
 \
name name##_dedup(name* l, name##CmpFn pred) { \
  if (! name##_is_sorted(l, pred)) name##_sort(l, pred); \
  size_t curr = 0; \
  for (int i=0; i<l->len-1; ++i) { \
    const type* a = (const type*) &l->data[i]; \
    const type* b = (const type*) &l->data[i+1]; \
    if (pred(a, b) < 0) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
size_t name##_bsearch(name* l, type val, name##CmpFn pred) { \
  type* res = bsearch( \
    (const type*) &val, \
    (const type*) l->data, \
    sizeof(type), \
    l->len, \
    (int (*)(const void*, const void*)) pred \
  ); \
  return res - l->data; \
} \

list_def(int, IntList)

IntList IntList_next_perm(IntList* l) {
  int i;

  // find pivot
  for(i=l->len-2; i >= 0 && l->data[i] >= l->data[i+1]; --i);

  // no pivot, reverse list (this is the last perm)
  if (i == -1) return IntList_reverse(l);

  int pivot = l->data[i];
  // find smallest number right to pivot
  int j;
  for(j=l->len-1; j > i && l->data[j] <= pivot; --j);
  IntList_swap(l, i, j);

  for(int left=i+1, right=l->len-1; left<right; ++left, --right) {
    IntList_swap(l, left, right);
  }

  return *l;
}

typedef struct {
  size_t start, len;
} IntSlice;

typedef struct {
  const IntList* src;
  const size_t size;
  size_t curr;
} ChunksIter;

ChunksIter list_chunks(const IntList* l, size_t size) {
  return (ChunksIter) { l, size, 0 };
}
bool list_has_chunk(const ChunksIter* it) {
  return it->curr < it->src->len;
}
IntSlice list_next_chunk(ChunksIter* it) {
  size_t start = it->curr;
  size_t len = start + it->size;
  it->curr += it->size;
  return (IntSlice) { start, len };
}

typedef struct {
  const IntList* src;
  const size_t size;
  size_t curr;
} WindowsIter;

WindowsIter list_windows(const IntList* l, size_t size) {
  return (WindowsIter) { l, size, 0 };
}
bool list_has_window(const WindowsIter* it) {
  return it->curr + it->size < it->src->len;
}
IntSlice list_next_window(WindowsIter* it) {
  size_t start = it->curr;
  size_t len = start + it->size;
  it->curr += 1;
  return (IntSlice) { start, len };
}

#endif
