#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stc_defs.h"

typedef struct {
  isize cap, len;
  isize front, back;
  int* data;
} Deque;

// https://stackoverflow.com/questions/49072494/how-does-the-vecdeque-ring-buffer-work-internally
// https://doc.rust-lang.org/std/collections/struct.VecDeque.html

// TODO: deque iterator
// TODO: negative modulo?
// TODO: should out of bounds access panic or return bool?
// TODO: generalize with macro

void deque_reserve(Deque* d, isize new_cap) {
  if (new_cap > d->cap) {
    d->cap = d->cap == 0 ? 16 : d->cap;

    isize cap = d->cap;
    while (new_cap > d->cap) d->cap *= 2;
    d->data = realloc(d->data, sizeof(int) * d->cap);
    assert(d->data != NULL && "deque realloc failed");

    if (d->front <= d->back) return;

    isize back_len  = d->back;
    isize front_len = d->len - d->front;

    if (back_len < front_len) {
      memcpy(
        d->data + d->len,
        d->data,
        back_len * sizeof(int)
      );
      d->back = d->len + back_len;
    } else {
      /* might overlap? */
      memmove(
        d->data + (d->cap - front_len),
        d->data + d->front,
        front_len * sizeof(int)
      );
      d->front = d->cap - front_len;
    }
  }
}

void deque_push_back(Deque* d, int value) {
  // we always have to be sure to realloc before computing the new back
  d->len += 1;
  deque_reserve(d, d->len+1);

  d->data[d->back] = value;
  d->back = (d->back + 1) & (d->cap - 1);
}

void deque_push_ring(Deque* d, int value) {
  /* this will overwrite data if full */
  d->len += 1;
  d->data[d->back] = value;
  d->back = (d->back + 1) & (d->cap - 1);
}

int deque_pop_back(Deque* d) {
  assert(d->len > 0 && "popped empty deque");

  if (d->back == 0) d->back = d->cap - 1;
  else d->back = (d->back - 1) & (d->cap - 1);
  d->len -= 1;
  
  return d->data[d->back];
}

void deque_push_front(Deque* d, int value) {
  d->len += 1;
  deque_reserve(d, d->len+1);
  
  if (d->front == 0) d->front = d->cap-1;
  else d->front = (d->front - 1) & (d->cap - 1);

  d->data[d->front] = value;
}

int deque_pop_front(Deque* d) {
  assert(d->len > 0 && "popped empty queue");

  int value = d->data[d->front];
  d->front = (d->front + 1) & (d->cap - 1);
  d->len += 1;
  return value;
}

int deque_real_idx(Deque* d, isize i) {
  assert(i < d->len && "deque access out of bounds");
  return (d->front + i) & (d->cap - 1);
}

int* deque_get(Deque* d, isize i) {
  return &d->data[deque_real_idx(d, i)];
}

void deque_set(Deque* d, isize i, int value) {
  d->data[deque_real_idx(d, i)] = value;
}

int deque_back(Deque* d) {
  assert(d->len > 0 && "accessed empty queue");
  isize back;
  if (d->back == 0) back = d->cap - 1;
  else back = (d->back - 1) & (d->cap - 1);
  return d->data[back];
}

int deque_front(Deque* d) {
  assert(d->len > 0 && "accessed empty queue");
  return d->data[d->front];
}

void deque_swap(Deque* d, isize a, isize b) {
  int left = deque_get(d, a);
  int right = deque_get(d, b);
  int tmp = left;
  deque_set(d, a, right);
  deque_set(d, b, tmp);
}

// TODO: this is confusing
// void deque_append_front(Deque* d, int* data, isize len) {
//   isize old_cap;
//   deque_reserve(d, d->len + len);
//   int remaining = len;
//   if (d->front >= d->back) {
//     // right part
//     memcpy(
//       d->data + d->front - len, 
//       d->data + d->front,
//       (old_cap - d->front) * sizeof(int)
//     );
//     remaining -= (old_cap - d->front);
//   }
//   if (remaining > 0) {
//     // left part
//     memcpy(d->data);
//   }

//   d->len += len;
//   d->front -= len;
// }

void deque_append_back(Deque* d, int* data, isize len) {
  deque_reserve(d, d->len + len);
  memcpy(d->data + d->back, data, len);
  d->back += len;
  d->len += len;
}

Deque deque_from_array(int* data, isize len) {
  Deque res = {0};
  deque_append_back(&res, data, len);
  return res;
}

// TODO: this is confusing
// void deque_resize(Deque* d, isize new_len, type value) {
//   if (new_len <= d->len) {
//     d->len = new_len;
//     d->back = (new_len > d->back) ?  : new_len;
//   } else {
//     deque_reserve(d, new_len);
//     isize range = (new_len - d->len);
//     for (int i=d->back; i<range; ++i) {
//       d->data[(d->back + i) & (d->cap - 1)] = value;
//     }
//     d->len = new_len;
//     d->back = (d->back + range) & (d->cap - 1);
//   }
// }

void deque_free(Deque* d) {
  free(d->data);
  d->len = 0;
  d->cap = 0;
  d->data = NULL;
}