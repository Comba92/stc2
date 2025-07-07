#include "stc_str.h"
#include "stc_fs.h"
#include "stc_map.h"

map_def(int, IntMap)

int main() {
  char* file = file_read_to_string("./bigfile.txt");
  StrList words = str_words_collect(SV(file));
  printf("File words count = %lld\n", words.len);

  // size_t count = 0;
  // iterfor(it, STR(file)) {
  //   str_iter_word(&it);
  //   count += 1;
  // }
  // printf("File words count = %lld\n", count);

  IntMap map = {0};
  // int count = 0;
  listforeach(str, word, &words) {
    // printf("i = %d, len = %lld\t", count++, map.len); str_dbg(*word);
    int* wcount = IntMap_get(map, *word);
    if (wcount == NULL) IntMap_insert(&map, *word, 1);
    else IntMap_insert(&map, *word, *wcount + 1);
  }

  printf("Map len: %lld\n", map.len);
  printf("Collisions: %lld - Longest collision chain: %lld\n", map.collisions, map.biggest_collision_chain);
  printf("Average collision chain length: %f", (double) map.collision_chain_avg / (double) map.collisions);
}
