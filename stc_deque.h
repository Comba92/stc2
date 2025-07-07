#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

typedef struct {
  size_t cap, len;
  size_t head, tail;
  int* data;
} Deque;

// https://stackoverflow.com/questions/49072494/how-does-the-vecdeque-ring-buffer-work-internally

void deque_reserve(Deque* d, size_t new_cap) {
  if (new_cap > d->cap) {
    d->cap = d->cap == 0 ? 16 : d->cap;
    // d->head = d->cap == 0 ? 15 : d->head;

    size_t cap = d->cap;
    while (new_cap > d->cap) d->cap *= 2;
    printf("Reallocating with cap %d\n", d->cap);
    d->data = realloc(d->data, sizeof(int) * d->cap);
    assert(d->data != NULL && "deque realloc failed");

    if (d->head <= d->tail) return;

    // size_t head_len = cap - d->head;
    // size_t tail_len = d->tail - head_len;
    // if (head_len > tail_len && d->cap - cap >= tail_len) {
    //   memcpy(
    //     d->data,
    //     d->data + cap,
    //     tail_len * sizeof(int)
    //   );
    // } else {
    //   memmove(
    //     d->data + d->head,
    //     d->data + (d->cap - head_len), 
    //     head_len * sizeof(int)
    //   );
    //   d->head = d->cap - head_len;
    // }

    size_t tail_len  = d->tail;
    size_t head_len = d->len - d->head;

    if (tail_len < head_len) {
      memmove(
        d->data + d->len,
        d->data,
        tail_len * sizeof(int)
      );
      d->tail = d->len + tail_len;
    } else {
      memmove(
        d->data + (d->cap - head_len),
        d->data + d->head,
        head_len * sizeof(int)
      );
      d->head = d->cap - head_len;
    }

    printf("After realoc: head %d tail %d\n", d->head, d->tail);
  }
}

void deque_push_back(Deque* d, int value) {
  // we always have to be sure to realloc before computing the new tail
  d->len += 1;
  deque_reserve(d, d->len+1);

  printf("Pushing %d into tail %d\n", value, d->tail);
  d->data[d->tail] = value;

  d->tail = (d->tail + 1) & (d->cap - 1);
}

int deque_pop_back(Deque* d) {
  assert(d->len > 0 && "popped empty deque");

  if (d->tail == 0) d->tail = d->cap - 1;
  else d->tail = (d->tail - 1) & (d->cap - 1);
  d->len -= 1;
  
  return d->data[d->tail];
}

void deque_push_front(Deque* d, int value) {
  d->len += 1;
  deque_reserve(d, d->len+1);
  
  if (d->head == 0) d->head = d->cap-1;
  else d->head = (d->head - 1) & (d->cap - 1);

  d->data[d->head] = value;
}

int deque_pop_front(Deque* d) {
  assert(d->len > 0 && "popped empty queue");

  int value = d->data[d->head];
  d->head = (d->head + 1) & (d->cap - 1);
  d->len += 1;
  return value;
}
