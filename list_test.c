#include <stdlib.h>
#include <stdio.h>
#include "stc_list.h"

struct Dummy {
  int a, b;
};

struct Dummy dummy_new() {
  return (struct Dummy) { rand() % 69, rand() % 420 };
}

list_def(struct Dummy, DummyList)
list_def(char*, StringList)

bool int_is_even(int val) {
  return val % 2 == 0;
}

bool int_cmp(int a, int b) {
  return a == b;
}

int main() {
  IntList list = {0};
  IntList other = {0};

  rangefor(int, i, 0, 40) {
    IntList_push(&list, i);
    IntList_push(&other, i*3);
    printf("%d\n", i);
  }

  printf("len = %d, cap = %d\n", list.len, list.cap);
  listfor(int, i, &list) {
    printf("%d = %d\n", i, list.data[i]);
  }

  rangefor(int, i, 0, 10) {
    printf("Popped: %d\n", IntList_pop(&list));
  }
  printf("len = %d, cap = %d\n", list.len, list.cap);

  printf("Value 30 at index: %d\n", IntList_find(&list, 30, *int_cmp));
  printf("Value 5 at index: %d\n", IntList_find(&list, 5, *int_cmp));

  IntList_filter(&list, *int_is_even);
  listfor(int, i, &list) {
    printf("%d = %d\n", i, list.data[i]);
  }
  printf("len = %d, cap = %d\n", list.len, list.cap);

  IntList_append(&list, other);
  listfor(int, i, &list) {
    printf("%d = %d\n", i, list.data[i]);
  }

  IntList_remove_swap(&list, 2);
  IntList_remove_swap(&list, 1);
  IntList_remove_swap(&list, 0);

  listforeach(int, n, &list) {
    printf("%d\n", *n);
  }

  DummyList dummies = {0};
  rangefor(int, i, 0, 50) {
    DummyList_push(&dummies, dummy_new());
    printf("%d = %d %d\n", i, dummies.data[i].a, dummies.data[i].b);
  }

  StringList s = {0};
  StringList_push(&s, "Hello!");
  StringList_push(&s, "World!");
  StringList_push(&s, "Bye!");

  listforeach(char*, str, &s) {
    printf("%s\n", *str);
  }
}