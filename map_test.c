#include <stdio.h>
#include "stc_map.h"
#include "stc_str.h"
#include "stc_list.h"

int main() {
  Map m = map_new(16);

  rangefor(int, i, 0, 100) {
    map_set(&m, int_to_str(i).data, i);
  }

  rangefor(int, i, 0, m.cap) {
    MapEntry e = m.entries[i];
    printf("%s = %d\n", e.key, e.val);
  }

  printf("%d\n", map_get(&m, "20"));
  printf("%d\n", map_get(&m, "45"));
  printf("%d\n", map_get(&m, "69"));
}