#define STC_LOG_ERR
#include "stc_build.h"

int main(int argc, char** argv) {
  binary_rebuild_itself(argv, __FILE__);

  printf("Shitting my pants!\n");

  char* inputs[] = {
    "fs_test.c",
    "stc_fs.h",
  };

  StrList entries = get_all_c_sources_in_dir(".", false);
  printf("Entries collected = %lld\n", entries.len);
  listforeach(str, e, &entries) {
    printf("%s\n", e->data);
  }

  binary_rebuild("./fag.exe", inputs, ArrayLen(inputs));
}