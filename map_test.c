#include <stdio.h>
#include "stc_map.h"
#include "stc_str.h"
#include "stc_list.h"
#include "stc_fs.h"

map_def(int, IntMap, IntMapEntry)

int main() {
  IntMap m = {0};

  printf("%d\n", IntMap_contains(&m, SV("20")));

  String sb = {0};
  rangefor(int, i, 0, 100) {
    IntMap_insert(&m, SBV(int_to_str(&sb, i)), i);
    printf("Inserted %d\n", i);
  }

  printf("Insertion done\n");

  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    printf("\"" str_fmt"\" = %d\n", str_arg(e.key), e.val);
  }

  printf("%d\n", *IntMap_get(&m, SV("20")));
  printf("%d\n", *IntMap_get(&m, SV("45")));
  printf("%d\n", *IntMap_get(&m, SV("69")));

  printf("%d\n", IntMap_remove(&m, SV("20")));
  printf("%d\n", IntMap_remove(&m, SV("43")));
  printf("%d\n", IntMap_remove(&m, SV("44")));
  printf("%d\n", IntMap_remove(&m, SV("45")));
  printf("%d\n", IntMap_remove(&m, SV("46")));
  printf("%d\n", IntMap_remove(&m, SV("47")));
  printf("%d\n", IntMap_remove(&m, SV("69")));
  printf("%d\n", IntMap_remove(&m, SV("fag")));

  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    if (!map_key_is_removed(e.key)) printf("\"" str_fmt"\" = %d\n", str_arg(e.key), e.val);
    else printf("Bucket %d removed\n", i);
  }

  IntMap_insert(&m, SBV(int_to_str(&sb, 45)), 45);
  IntMap_insert(&m, SBV(int_to_str(&sb, 46)), 46);
  IntMap_insert(&m, SBV(int_to_str(&sb, 44)), 44);
  IntMap_insert(&m, SBV(int_to_str(&sb, 43)), 43);
  IntMap_insert(&m, SBV(int_to_str(&sb, 47)), 47);

  printf("\n====\n\n");
  rangefor(int, i, 0, m.cap) {
    IntMapEntry e = m.entries[i];
    if (!map_key_is_removed(e.key)) printf("\"" str_fmt"\" = %d\n", str_arg(e.key), e.val);
    else printf("Bucket %d removed\n", i);
  }

  // printf("Buckets: %lld, Collisions count: %lld - Biggest collision: %lld\n", m.len, m.collisions, m.biggest_collision_chain);

  IntMap_free(&m);

  Set s = {0};
  rangefor(int, i, 0, 100) {
    Set_insert(&s, SBV(int_to_str(&sb, i)));
    printf("Inserted %d\n", i);
  }

  printf("Insertion done\n");

  rangefor(int, i, 0, s.cap) {
    printf("key: "str_fmt"\n", str_arg(s.keys[i]));
  }

  rangefor(int, i, 0, 150) {
    bool contained = Set_get(&s, SBV(int_to_str(&sb, i)));
    printf("%d is contained: %d\n", i, contained);
  }

  Set_free(&s);
}