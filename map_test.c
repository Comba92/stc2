#include <stdio.h>
#include "stc_map.h"
#include "stc_str.h"
#include "stc_list.h"

int main() {
  Map m = {0};

  printf("%d\n", map_contains(m, "20"));


  rangefor(int, i, 0, 100) {
    map_insert(&m, int_to_str(i).data, i);
    printf("Inserted %d\n", i);
  }

  printf("Insertion done\n");

  rangefor(int, i, 0, m.cap) {
    MapEntry e = m.entries[i];
    printf("%s = %d\n", e.key, e.val);
  }

  printf("%d\n", *map_get(m, "20"));
  printf("%d\n", *map_get(m, "45"));
  printf("%d\n", *map_get(m, "69"));

  printf("%d\n", map_remove(&m, "20"));
  printf("%d\n", map_remove(&m, "43"));
  printf("%d\n", map_remove(&m, "44"));
  printf("%d\n", map_remove(&m, "45"));
  printf("%d\n", map_remove(&m, "46"));
  printf("%d\n", map_remove(&m, "47"));
  printf("%d\n", map_remove(&m, "69"));
  printf("%d\n", map_remove(&m, "fag"));

  rangefor(int, i, 0, m.cap) {
    MapEntry e = m.entries[i];
    if (e.key != 1) printf("%s = %d\n", e.key, e.val);
    else printf("Bucket %d removed\n", i);
  }

  map_insert(&m, int_to_str(45).data, 45);
  map_insert(&m, int_to_str(46).data, 46);
  map_insert(&m, int_to_str(44).data, 44);
  map_insert(&m, int_to_str(43).data, 43);
  map_insert(&m, int_to_str(47).data, 47);

  printf("\n====\n\n");
  rangefor(int, i, 0, m.cap) {
    MapEntry e = m.entries[i];
    if (e.key != 1) printf("%s = %d\n", e.key, e.val);
    else printf("Bucket %d removed\n", i);
  }

  map_free(&m);
}