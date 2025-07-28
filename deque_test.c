#include "stc_deque.h"
#include "stc_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  Deque d = {0};

  // rangefor(int, i, 0, 100) {
  //   deque_push_back(&d, i);
  // }

  // rangefor(int, i, 0, 100) {
  //   deque_pop_back(&d);
  // }

  // rangefor(int, i, 0, 50) {
  //   deque_push_back(&d, i);
  // }

  // rangefor(int, i, 100, 200) {
  //   deque_push_front(&d, i);
  // }

  rangefor(int, i, 0, 100) {
    deque_push_front(&d, i);
  }

  rangefor(int, i, 0, 300) {
    deque_push_back(&d, -i);
  }

  rangefor(int, i, 0, 50) {
    deque_push_front(&d, i+100);
  }

  rangefor(int, i, 0, 100) {
    printf("Popping: %d\n", deque_pop_front(&d));
  }

  rangefor(int, i, 0, d.cap) {
    printf("i = %d\t%d\n", i, d.data[i]);
  }

  printf("Cap = %d, Len = %d, Head = %d, Tail = %d\n", d.cap, d.len, d.front, d.back);

  srand(time(NULL));
  BinaryHeap b = {0};
  rangefor(int, i, 0, 50) {
    bheap_push(&b, rand() % 100);
    listfor(int, i, &b) {
      printf("%d\n", b.data[i]);
    }

    printf("Max: %d\n\n", b.data[0]);
  }

  rangefor(int, i, 0, 20) {
    bheap_pop(&b);
    listfor(int, i, &b) {
      printf("%d\n", b.data[i]);
    }

    printf("Max: %d\n\n", b.data[0]);
  }
}