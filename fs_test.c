#define STC_LOG_ERR
#include "stc_fs.h"

int main() {
  file_read_to_string("fag.txt");
  file_read_to_string("test.txt");

  printf("%s\n", path_filename("D:\\code\\stc2\\str_test.c"));
  printf("%s\n", path_extension("D:\\code\\stc2\\str_test.c"));

  DirEntries entries = {0};
  entries = dir_entries_sorted(".");

  printf("Entries collected = %ld\n", entries.len);
  listforeach(DirEntry, e, &entries) {
    printf("%s %d\n", e->name, e->type);
  }
  
  printf("\nDir iterator\n");

  DirIter it = dir_open(".");
  dir_iter(entry, &it) {
    printf("%s\n", entry->name);
  }
  
  dir_entries("/home/comba/");
  entries = dir_entries_sorted("..");
  printf("Entries collected = %ld\n", entries.len);
  listforeach(DirEntry, e, &entries) {
    printf("%s %d\n", e->name, e->type);
  }

  // printf("\nDir Walk:\n");
  // dir_walk(".");

  printf("%d\n", file_exists("./test.txt"));
  printf("%d\n", file_exists("./faggot_killer.txt"));
  printf("%d\n", dir_exists(".."));
  printf("%d\n", dir_exists("../stc"));
  printf("%d\n", dir_exists("../urmom"));

  dir_create("./test1");
  dir_create_recursive("./test2/daaaaa/dsadasad/adasdas");
  dir_delete("./test1");

  file_create_recursive("./test3/kysfag/hahahah/test", false);
  printf("test3 dir created\n");
  dir_delete_recursive("./test3/");
  printf("test3 dir deleted\n");

  printf("Working dir: %s\n", dir_current());
  printf("Copy: %d\n", file_copy("./test.txt", "./test2.txt", false));
  printf("Delete: %d\n", file_delete("./test2.txt"));
  printf("Rename: %d\n", file_move("./test.txt", "./kys.txt"));
  printf("Create: %d\n", file_create("test3.txt", true));

  printf("Copy FAIL: %d\n", file_copy("./test4.txt", "./test2.txt", false));
  printf("Delete FAIL: %d\n", file_delete("./test4.txt"));
  printf("Rename FAIL: %d\n", file_move("./test4.txt", "./kys.txt"));
  printf("Create FAIL: %d\n", file_create("kys.txt", false));

  printf("Absolute path: %s\n", path_to_absolute("kys.txt"));
  str_dbg(path_parent("./dada/dasdad/.weqrwq.rwq/rqwr.q/kys.txt"));

  dir_copy_recursive("./test_copy", "./test_copy_del");
  printf("Delete had errors: %d\n", dir_delete_recursive("./test_copy_del"));
}