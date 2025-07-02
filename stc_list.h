#ifndef STC_LIST_IMPL
#define STC_LIST_IMPL

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define rangefor(type, it, start, end) for (type it = (start); it < end; ++it)
#define listfor(type, it, list) for (type it = 0; it < (list)->len; ++it)
#define listforeach(type, it, list) for (type* it = (list)->data; it < (list)->data + (list)->len; ++it)

#define list_def(type, list_name) \
typedef struct { \
  size_t len, cap; \
  type* data; \
} list_name; \
 \
void list_name##_reserve(list_name* l, size_t new_cap) { \
  if (new_cap > l->cap) { \
    l->cap = l->cap == 0 ? 16 : l->cap; \
    while (new_cap > l->cap) l->cap *= 2; \
 \
    l->data = realloc(l->data, sizeof(l->data[0]) * l->cap); \
    assert(l->data != NULL && "list realloc failed"); \
  } \
} \
void list_name##_resize(list_name* l, size_t new_len, type value) { \
  if (new_len <= l->len) { \
    l->len = new_len; \
  } else { \
    list_name##_reserve(l, l->len + new_len); \
    for (int i=0; i<new_len; ++i) { \
      l->data[l->len + i] = value; \
    } \
  } \
} \
 \
void list_name##_push(list_name* l, type value) { \
  list_name##_reserve(l, l->len + 1); \
  l->data[l->len++] = value; \
} \
 \
type list_name##_first(list_name l) { \
  return l.data[0]; \
} \
type list_name##_last(list_name l) { \
  return l.data[l.len-1]; \
} \
 \
type list_name##_pop(list_name* l) { \
  return l->data[--l->len]; \
} \
 \
void list_name##_assert(list_name l, size_t i) { \
  assert(i < l.len && "list access out of bounds"); \
} \
 \
typedef bool (*list_name##CmpFn)(type a, type b); \
int list_name##_find(list_name* l, type value, list_name##CmpFn pred) { \
  for(size_t i=0; i<l->len; ++i) { \
    if (pred(l->data[i], value)) return i; \
  } \
 \
  return -1; \
} \
 \
typedef bool (*list_name##FilterFn)(type val); \
void list_name##_filter(list_name* l, list_name##FilterFn pred) { \
  size_t curr = 0; \
  for(size_t i=0; i < l->len; ++i) { \
    if (pred(l->data[i])) { \
      l->data[curr++] = l->data[i]; \
    } \
  } \
  l->len = curr; \
} \
 \
void list_name##_swap(list_name* l, size_t a, size_t b) { \
  type tmp = l->data[a]; \
  l->data[a] = l->data[b]; \
  l->data[b] = tmp; \
} \
 \
type list_name##_remove_swap(list_name* l, size_t i) { \
  list_name##_assert(*l, i); \
  l->len--; \
  type res = l->data[i]; \
  l->data[i] = l->data[l->len]; \
  return res; \
} \
 \
void list_name##_append(list_name* this, list_name other) { \
  list_name##_reserve(this, this->len + other.len); \
  /* \
  size_t len = this->len; \
  for(size_t i=0; i<other.len; ++i) { \
    this->data[len + i] = other.data[i]; \
  } \
  this->len += other.len; \
  */ \
  memcpy(this->data + this->len, other.data, other.len * sizeof(this->data[0])); \
  this->len += other.len; \
} \
 \
list_name list_name##_from_array(type* arr, size_t arr_len) { \
  list_name res = {0}; \
  list_name##_reserve(&res, arr_len); \
  /* \
  for(size_t i=0; i<arr_len; ++i) { \
    res.data[res.len++] = arr[i]; \
  } \
  */ \
  memcpy(res.data, arr, arr_len * sizeof(res.data[0])); \
  res.len += arr_len; \
  return res; \
} \
\
void list_name##_append_array(list_name* l, type* arr, size_t arr_len) { \
  list_name##_reserve(l, l->len + arr_len); \
  /* \
  size_t len = l->len; \
  for(size_t i=0; i<arr_len; ++i) { \
    l->data[len + i] = arr[i]; \
  } \
  */ \
  memcpy(l->data + l->len, arr, arr_len * sizeof(l->data[0])); \
  l->len += arr_len; \
} \
 \
void list_name##_free(list_name* l) { \
  free(l->data); \
  l->cap = 0; \
  l->len = 0; \
  l->data = NULL; \
} \


list_def(int, IntList)

#endif

/*

typedef struct<T> {
  size_t len, cap;
  T* data;
} List<T>;

void list_reserve(List<T>* l, size_t new_cap) {
  if (new_cap > l->cap) {
    l->cap = l->cap == 0 ? 16 : l->cap;
    while (new_cap > l->cap) l->cap *= 2;

    l->data = realloc(l->data, sizeof(l->data[0]) * l->cap);
    assert(l->data != NULL && "list realloc failed");
  }
}

void list_push(List<T>* l, T value) {
  list_reserve(l, l->len + 1);
  l->data[l->len++] = value;
}

T list_first(List<T>* l) {
  return l->data[0];
}
T list_last(List<T>* l) {
  return l->data[l->len-1];
}

T list_pop(List<T>* l) {
  return l->data[--l->len];
}

void list_assert(List<T>* l, size_t i) {
  assert(i < l->len && "list access out of bounds");
}

typedef bool (*listCmpFn)(T a, T b);
int list_find(List<T>* l, T value, listCmpFn pred) {
  for(size_t i=0; i<l->len; ++i) {
    if (pred(l->data[i], value)) return i;
  }

  return -1;
}

typedef bool (*listFilterFn)(T val);
void list_filter(List<T>* l, listFilterFn pred) {
  size_t curr = 0;
  for(size_t i=0; i < l->len; ++i) {
    if (pred(l->data[i])) {
      l->data[curr++] = l->data[i];
    }
  }
  l->len = curr;
}

void list_swap(List<T>* l, size_t a, size_t b) {
  T tmp = l->data[a];
  l->data[a] = l->data[b];
  l->data[b] = tmp;
}

T list_remove_swap(List<T>* l, size_t i) {
  list_assert(l, i);
  l->len--;
  T res = l->data[i];
  l->data[i] = l->data[l->len];
  return res;
}

void list_append(List<T>* this, List<T> other) {
  list_reserve(this, this->len + other.len);
  // size_t len = this->len;
  // for(size_t i=0; i<other.len; ++i) {
  //   this->data[len + i] = other.data[i];
  // }
  // this->len += other.len;
  memcpy(this->data + this->len, other.data, other.len);
  this->len += other.len;
}

list_name list_from_array(T* arr, size_t arr_len) {
  list_name res = {0};
  list_reserve(&res, arr_len);
  // for(size_t i=0; i<arr_len; ++i) {
  //   res.data[res.len++] = arr[i];
  // }
  memcpy(res.data, arr, arr_len);
  res.len = arr_len;
  return res;
}
\
void list_append_array(List<T>* l, T* arr, size_t arr_len) {
  list_reserve(l, l->len + len);
  // size_t len = l->len;
  // for(size_t i=0; i<arr_len; ++i) {
  //   l->data[len + i] = arr[i];
  // }
  memcpy(l->data + l->len, arr, arr_len);
  l->len += arr_len;
}

void list_free(List<T>* l) {
  free(l->data);
  l->cap = 0;
  l->len = 0;
  l->data = NULL;
}

*/