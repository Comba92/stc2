#ifndef STC_LIST_IMPL
#define STC_LIST_IMPL

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "stc_defs.h"

#define rangefor(type, it, start, end) for (type it = (start); it < (end); ++it)
#define listfor(type, it, list) for (type it = 0; it < (list)->len; ++it)
#define listforrev(type, it, list) for (type it = (list)->len-1; it >= 0; --it)
#define listforeach(type, it, list) for (type* it = (list)->data; it < (list)->data + (list)->len; ++it)

static const isize LIST_DEFAULT_CAP = 16; 

// TODO: not sure if i want insert and remove
// TODO: array_heap_to_list() is extremely dangerous
// TODO: small size opt: https://nullprogram.com/blog/2016/10/07/

// TODO: two different list_defs: one for minimal functionality and one for full
// TODO: list_first and first_last, should they be macros?

#define list_first(list) (list.data[0])
#define list_last(list) (list.data[list.len-1])

#define list_def_all(type, name) \
list_def(type, name) \
list_def_alg(type, name) \
\

#define list_def(type, name) \
typedef struct { \
  isize len, cap; \
  type* data; \
} name; \
 \
name name##_with_cap(isize cap) { \
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
void name##_reserve(name* l, isize new_cap) { \
  if (new_cap > l->cap) { \
    l->cap = l->cap == 0 ? LIST_DEFAULT_CAP : l->cap; \
    while (new_cap > l->cap) l->cap *= 2; \
 \
    l->data = realloc(l->data, sizeof(type) * l->cap); \
    assert(l->data != NULL && "list realloc failed"); \
  } \
} \
void name##_resize(name* l, isize new_len, type value) { \
  if (new_len <= l->len) { \
    l->len = new_len; \
  } else { \
    name##_reserve(l, l->len + new_len); \
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
void name##_assert(name l, isize i) { \
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
void name##_swap(name* l, isize a, isize b) { \
  name##_assert(*l, a); \
  name##_assert(*l, b); \
  type tmp = l->data[a]; \
  l->data[a] = l->data[b]; \
  l->data[b] = tmp; \
} \
 \
type name##_remove_swap(name* l, isize i) { \
  name##_assert(*l, i); \
  l->len--; \
  type res = l->data[i]; \
  l->data[i] = l->data[l->len]; \
  return res; \
} \
 \
void name##_append_array(name* l, const type* arr, isize arr_len) { \
  name##_reserve(l, l->len + arr_len); \
  memcpy(l->data + l->len, arr, arr_len * sizeof(type)); \
  l->len += arr_len; \
} \
 \
name name##_from_array(const type* arr, isize arr_len) { \
  name res = {0}; \
  name##_append_array(&res, arr, arr_len); \
  return res; \
} \
 \
name array_heap_to_##name(type* *const arr, isize arr_len) { \
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

#define list_def_alg_fn(type, name, cmpfn) \
name name##_shuffle(name* l) { \
  /* https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle */ \
  srand(time(NULL)); \
  for(isize i=l->len-1; i>0; --i) { \
    isize r = rand() % (i+1); \
    name##_swap(l, i, r); \
  } \
  return *l; \
} \
 \
name name##_reverse(name* l) { \
  for(isize left=0, right=l->len-1; left<right; ++left, --right) { \
    name##_swap(l, left, right); \
  } \
  return *l; \
} \
 \
typedef isize (*name##CmpFn)(const type* a, const type* b); \
isize name##_find_idx(const name* l, type value) { \
  listfor(isize, i, l) { \
    if (cmpfn(&l->data[i], &value) == 0) return i; \
  } \
 \
  return -1; \
} \
 \
bool name##_contains(const name* l, type value) { \
  return name##_find_idx(l, value) != -1; \
} \
 \
bool name##_is_sorted(const name* l) { \
  for(isize i=0; i<l->len-1; ++i) { \
    if (cmpfn(&l->data[i], &l->data[i+1]) >= 0) return false; \
  } \
  return true; \
} \
 \
name name##_sort(name* l) { \
  /* we cast the function pointer because we are crazy and we can do that */ \
  qsort(l->data, l->len, sizeof(type), (int (*)(const void*, const void*)) cmpfn); \
  return *l; \
} \
 \
typedef bool (*name##EqFn)(const type* val); \
isize name##_find(const name* l, name##EqFn pred) { \
  listfor(isize, i, l) { \
    if (pred(&l->data[i])) return i; \
  } \
 \
  return -1; \
} \
bool name##_all(const name* l, name##EqFn pred) { \
  listfor(isize, i, l) { \
    if (!pred(&l->data[i])) return false; \
  } \
  return true; \
} \
 \
bool name##_any(const name* l, name##EqFn pred) { \
  listfor(isize, i, l) { \
    if (pred(&l->data[i])) return true; \
  } \
  return false; \
} \
 \
isize name##_count(const name* l, name##EqFn pred) { \
  isize count = 0; \
  listfor(isize, i, l) { \
    count += pred(&l->data[i]); \
  } \
  return count; \
} \
 \
name name##_retain(name* l, name##EqFn pred) { \
  isize curr = 0; \
  listfor(isize, i, l) { \
    if (pred(&l->data[i])) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
name name##_filter(const name* l, name##EqFn pred) { \
  name res = name##_clone(*l); \
  return name##_retain(&res, pred); \
} \
 \
name name##_dedup(name* l) { \
  if (!name##_is_sorted(l)) name##_sort(l); \
  isize curr = 0; \
  for (isize i=0; i<l->len-1; ++i) { \
    type* a = &l->data[i]; \
    type* b = &l->data[i+1]; \
    if (cmpfn(a, b) < 0) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
isize name##_bsearch(const name* l, type val) { \
  type* res = bsearch( \
    &val, \
    l->data, \
    sizeof(type), \
    l->len, \
    (int (*)(const void*, const void*)) cmpfn \
  ); \
  return res - l->data; \
} \
 \
name name##_next_perm(name* l) { \
  isize i; \
 \
  /* find pivot */ \
  for(i=l->len-2; i >= 0 && cmpfn(&l->data[i], &l->data[i+1]) >= 0; --i); \
 \
  /* no pivot, reverse list (this is the last perm) */ \
  if (i == -1) return name##_reverse(l); \
 \
  type pivot = l->data[i]; \
  /* find smallest number right to pivot */ \
  isize j; \
  for(j=l->len-1; j > i && cmpfn(&l->data[j], &pivot) <= 0; --j); \
  name##_swap(l, i, j); \
 \
  for(isize left=i+1, right=l->len-1; left<right; ++left, --right) { \
    name##_swap(l, left, right); \
  } \
 \
  return *l; \
} \

#define list_def_alg(type, name) \
typedef isize (*name##CmpFn)(const type* a, const type* b); \
isize name##_find(const name* l, type value, name##CmpFn pred) { \
  const type* b = (const type*) &value; \
  listfor(isize, i, l) { \
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
bool name##_is_sorted(const name* l, name##CmpFn pred) { \
  for(isize i=0; i<l->len-1; ++i) { \
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
bool name##_all(const name* l, name##EqFn pred) { \
  listfor(isize, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (!pred(it)) return false; \
  } \
  return true; \
} \
 \
bool name##_any(const name* l, name##EqFn pred) { \
  listfor(isize, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (pred(it)) return true; \
  } \
  return false; \
} \
 \
isize name##_count(const name* l, name##EqFn pred) { \
  isize count = 0; \
  listfor(isize, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    count += pred(it); \
  } \
  return count; \
} \
 \
name name##_filter(name* l, name##EqFn pred) { \
  isize curr = 0; \
  listfor(isize, i, l) { \
    const type* it = (const type*) &l->data[i]; \
    if (pred(it)) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
 \
name name##_dedup(name* l, name##CmpFn pred) { \
  if (!name##_is_sorted(l, pred)) name##_sort(l, pred); \
  isize curr = 0; \
  for (isize i=0; i<l->len-1; ++i) { \
    const type* a = (const type*) &l->data[i]; \
    const type* b = (const type*) &l->data[i+1]; \
    if (pred(a, b) < 0) l->data[curr++] = l->data[i]; \
  } \
  l->len = curr; \
  return *l; \
} \
isize name##_bsearch(const name* l, type val, name##CmpFn pred) { \
  type* res = bsearch( \
    (const type*) &val, \
    (const type*) l->data, \
    sizeof(type), \
    l->len, \
    (int (*)(const void*, const void*)) pred \
  ); \
  return res - l->data; \
} \
 \
 
list_def(int, IntList)


// TODO: what's this doing here
typedef struct {
  isize start, len;
} IntSlice;

typedef struct {
  const IntList* src;
  const isize size;
  isize curr;
} ChunksIter;

ChunksIter list_chunks(const IntList* l, isize size) {
  return (ChunksIter) { l, size, 0 };
}
bool list_has_chunk(const ChunksIter* it) {
  return it->curr < it->src->len;
}
IntSlice list_next_chunk(ChunksIter* it) {
  isize start = it->curr;
  isize len = start + it->size;
  it->curr += it->size;
  return (IntSlice) { start, len };
}

typedef struct {
  const IntList* src;
  const isize size;
  isize curr;
} WindowsIter;

WindowsIter list_windows(const IntList* l, isize size) {
  return (WindowsIter) { l, size, 0 };
}
bool list_has_window(const WindowsIter* it) {
  return it->curr + it->size < it->src->len;
}
IntSlice list_next_window(WindowsIter* it) {
  isize start = it->curr;
  isize len = start + it->size;
  it->curr += 1;
  return (IntSlice) { start, len };
}

#endif
