#define STC_LOG_ERR
#include "stc_build.h"

int main(int argc, char** argv) {
  BIN_REBUILD_ITSELF(argc, argv);

  printf("Shitting my pants?\n");

  char* inputs[] = {
    "fs_test.c",
    "stc_fs.h",
  };

  CstrList entries = get_all_c_sources_in_dir(".", false);
  printf("Entries collected = %lld\n", entries.len);
  listforeach(char*, e, &entries) {
    printf("%s\n", *e);
  }


  int res = binary_rebuild_all(entries.data, entries.len);
  printf("Successes: %d = %lld\n", res, entries.len);

  res = binary_rebuild("fag.exe", inputs, ArrayLen(inputs));
  printf("Rebuild res: %d\n", res);
  if (res == 0) {
    printf("Rebuild succesful!\nRunning binary...\n\n");
  } else if (res == -1) {
    printf("Rebuild not needed!\n");
  } else {
    printf("Rebuild failed!\n");
  }

  // binary_exec("fag.exe");
}