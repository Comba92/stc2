#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

typedef struct {
  size_t cap, len;
  long long head, tail;
  int* data;
} Deque;

// https://stackoverflow.com/questions/49072494/how-does-the-vecdeque-ring-buffer-work-internally
// https://github.com/ajain13/Dynamic-Double-Ended-Queue/blob/master/src/Deque.hpp
// https://github.com/Seng3694/Deque/blob/master/src/deque.c

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

void deque_reserve(Deque* d, size_t new_cap) {
  if (new_cap > d->cap) {
    // d->head = d->cap == 0 ? 15 : d->head;
    d->cap = d->cap == 0 ? 16 : d->cap;
    while (new_cap > d->cap) d->cap *= 2;
    d->data = realloc(d->data, sizeof(int) * d->cap);
    assert(d->data != NULL && "deque realloc failed");
  }
}

void deque_grow(Deque* d) {
  if (d->head == d->tail) {
    deque_reserve(d, d->len + 1);

    // data is contiguos, no moving needed
    if (d->tail <= d->head) return;

    long long left_size = d->head;
    long long right_size = d->len - d->tail;

    if (left_size <= right_size) {
      // head grows to the right, and in this case,
      // there is more space to the right of tail
      memmove(
        d->data + d->len,
        d->data,
        d->head
      );
      d->head = d->len + d->head;
      printf("Previous head: %ld, New head: %ld\n", d->head, d->len + d->head);
    } else {
      // tail grows to the left,
      // tail is moved to the end
      memmove(
        d->data + (d->cap - d->tail), 
        d->data + d->tail,
        d->len - d->tail
      );
      printf("Previous tail: %ld, New tail: %ld\n", d->tail, d->cap - d->tail);
      d->tail = d->cap - d->tail;
    }
  }
}

void deque_push_back(Deque* d, int value) {
  deque_grow(d);
  d->data[d->tail] = value;
  d->tail = (d->tail + 1) & (d->cap - 1);
  d->len += 1;
}

int deque_pop_back(Deque* d) {
  assert(d->len > 0 && "popped empty deque");
  if (d->tail == 0) d->tail = d->cap - 1;
  else d->tail = (d->tail - 1) & (d->cap - 1);
  d->len -= 1;
  return d->data[d->tail];
}

void deque_push_front(Deque* d, int value) {
  deque_grow(d);
  if (d->head == 0) d->head = d->cap-1;
  else d->head = (d->head - 1) & (d->cap - 1);
  d->data[d->head] = value;
  d->len += 1;
}

int deque_pop_front(Deque* d) {
  assert(d->len > 0 && "popped empty queue");
  int val = d->data[d->head];
  d->head = (d->head + 1) & (d->cap - 1);
  d->len -= 1;
  return val;
}
