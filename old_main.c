#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

size_t upper_power_of_two(size_t n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

#define slice_def(type) slice_def_with_name(type, type##Slice);
#define slice_def_with_name(type, slicename) \
typedef struct slicename { \
  size_t len; \
  type* data; \
} slicename; \
slicename type##_slice(slicename self, size_t start, size_t end) { \
  assert(end >= start); \
  if (start >= self.len) { \
    return (slicename) {0}; \
  } else if (end > self.len) { \
    end = self.len; \
  } \
  return (slicename) { \
    .data = self.data + start, \
    .len = end - start \
  }; \
} \
size_t slicename##_index_of(slicename self, type val) { \
  for(int i=0; i<self.len; ++i) { \
    if (self.data[i] == val) { return i; } \
  } \
  return -1; \
} \
bool slicename##_contains(slicename self, type val) { \
  return slicename##_index_of(self, val) != -1; \
} \

#define list_def(type) list_def_with_name(type, type##List, type##Slice);
#define list_def_with_name(type, listname, slicename) \
slice_def_with_name(type, slicename); \
typedef struct listname { \
  size_t len; \
  size_t capacity; \
  type* data; \
} listname; \
\
void listname##_push(listname* self, type data) { \
  if (self->capacity == 0) { \
    self->capacity = 16; \
    self->data = malloc(sizeof(type) * self->capacity); \
  } else if (self->len >= self->capacity) { \
    self->capacity *= 2; \
    printf("[REALLOC] With cap %d\n", self->capacity); \
    self->data = realloc(self->data, sizeof(type)*self->capacity); \
  } \
  self->data[self->len] = data; \
  self->len += 1; \
} \
type listname##_pop(listname* self) { \
  type ret = self->data[self->len-1]; \
  self->len -= 1; \
  return ret; \
} \
void listname##_append(listname* self, listname* other) { \
  for (int i=0; i<other->len; ++i) { \
    listname##_push(self, other->data[i]); \
  } \
} \
listname listname##_with_cap(size_t cap) { \
  listname res = {0}; \
  res.capacity = upper_power_of_two(cap); \
  res.data = malloc(sizeof(type) * res.capacity); \
  return res; \
} \
listname listname##_from_array(type* arr, size_t len) { \
  listname res = listname##_with_cap(len); \
  for (int i=0; i<len; ++i) { \
    res.data[res.len] = arr[i]; \
    res.len += 1; \
  } \
  return res; \
} \
void listname##_clear(listname* self) { \
  self->len = 0; \
} \
slicename listname##_as_slice(listname* self, size_t start, size_t end) { \
  slicename slice = (slicename) { \
    .data = self->data, .len = self->len \
  }; \
  return type##_slice(slice, start, end); \
} \
size_t listname##_index_of(listname* self, type val) { \
  slicename slice = listname##_as_slice(self, 0, self->len); \
  return slicename##_index_of(slice, val); \
} \
bool listname##_contains(listname* self, type val) { \
  slicename slice = listname##_as_slice(self, 0, self->len); \
  return slicename##_contains(slice, val) != -1; \
} \


list_def(int)
list_def_with_name(char, String, str)

str str_from(char* s) {
  return (str) {
    .data = s, .len = strlen(s)
  };
}

str str_empty() {
  return (str) {
    .data = "", .len = 0
  };
}

str str_slice(char* s, size_t start, size_t end) {
  return char_slice(str_from(s), start, end);
}


int main() {
  
}