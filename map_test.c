#include <stdio.h>
#include "stc_map.h"
#include "stc_str.h"
#include "stc_list.h"

map_def(int, IntMap)

int main() {
  IntMap m = {0};

  printf("%d\n", IntMap_contains(m, "20"));

  rangefor(int, i, 0, 100) {
    IntMap_insert(&m, int_to_str(i).data, i);
    printf("Inserted %d\n", i);
  }

  printf("Insertion done\n");

  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    printf("%s = %d\n", e.key, e.val);
  }

  printf("%d\n", *IntMap_get(m, "20"));
  printf("%d\n", *IntMap_get(m, "45"));
  printf("%d\n", *IntMap_get(m, "69"));

  printf("%d\n", IntMap_remove(&m, "20"));
  printf("%d\n", IntMap_remove(&m, "43"));
  printf("%d\n", IntMap_remove(&m, "44"));
  printf("%d\n", IntMap_remove(&m, "45"));
  printf("%d\n", IntMap_remove(&m, "46"));
  printf("%d\n", IntMap_remove(&m, "47"));
  printf("%d\n", IntMap_remove(&m, "69"));
  printf("%d\n", IntMap_remove(&m, "fag"));

  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    if (!key_is_removed(e.key)) printf("%s = %d\n", e.key, e.val);
    else printf("Bucket %d removed\n", i);
  }

  IntMap_insert(&m, int_to_str(45).data, 45);
  IntMap_insert(&m, int_to_str(46).data, 46);
  IntMap_insert(&m, int_to_str(44).data, 44);
  IntMap_insert(&m, int_to_str(43).data, 43);
  IntMap_insert(&m, int_to_str(47).data, 47);

  printf("\n====\n\n");
  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    if (!key_is_removed(e.key)) printf("%s = %d\n", e.key, e.val);
    else printf("Bucket %d removed\n", i);
  }

  IntMap_free(&m);
}