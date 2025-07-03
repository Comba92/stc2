#include <stdio.h>
#include "stc_map.h"
#include "stc_str.h"
#include "stc_list.h"

int main() {
  Map m = {0};
  printf("Sizeof = %ld\n", sizeof(MapEntry));
  map_insert(&m, int_to_str(1).data, 1);

  rangefor(int, i, 0, m.cap) {
    printf("key = %s value = %d\n", m.entries[i].key, m.entries[i].val);
  }

  rangefor(int, i, 0, 100) {
    map_insert(&m, int_to_str(i).data, i);
    printf("Inserted %d\n", i);
  }

  printf("Insertion done\n");

  rangefor(int, i, 0, m.cap) {
    MapEntry e = m.entries[i];
    printf("%s = %d\n", e.key, e.val);
  }

  // printf("%d\n", map_get(&m, "20"));
  // printf("%d\n", map_get(&m, "45"));
  // printf("%d\n", map_get(&m, "69"));
}