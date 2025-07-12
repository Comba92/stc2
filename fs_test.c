#define STC_LOG_ERR
#include "stc_fs.h"

int main() {
  String sb = {0};
  // file_read_to_string(&sb, "fag.txt");
  // file_read_to_string(&sb, "test.txt");

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

  printf("%d\n", file_exists("./fs_test_dir/test.txt"));
  printf("%d\n", file_exists("./faggot_killer.txt"));
  printf("%d\n", dir_exists("./fs_test_dir"));
  printf("%d\n", dir_exists(".."));
  printf("%d\n", dir_exists("../stc"));
  printf("%d\n", dir_exists("../urmom"));

  dir_create_recursive("./fs_test_dir/test1/");

  printf("Delete ok: %d\n", dir_delete("./fs_test_dir/test1"));
  printf("Delete fail: %d\n", dir_delete("./test1"));

  String_append_null(&sb);
  String_append_cstr(&sb, "OOOOOOOOOH??");
  printf(str_fmt "\n", str_arg(sb));

  printf("Working dir: %s\n", dir_current(&sb));
  file_create("./fs_test_dir/test.txt", true);
  printf("Copy base: %d\n", file_copy("./fs_test_dir/test.txt", "./fs_test_dir/test2.txt", false));
  printf("Copy overwrite: %d\n", file_copy("./fs_test_dir/test.txt", "./fs_test_dir/test2.txt", true));
  printf("Copy no overwrite: %d\n", file_copy("./fs_test_dir/test.txt", "./fs_test_dir/test2.txt", false));
  printf("Delete: %d\n", file_delete("./fs_test_dir/test.txt"));
  file_create("./fs_test_dir/test.txt", false);
  printf("Move base: %d\n", file_move("./fs_test_dir/test.txt", "./fs_test_dir/test3.txt", false));
  file_create("./fs_test_dir/test.txt", false);
  printf("Move overwrite: %d\n", file_move("./fs_test_dir/test.txt", "./fs_test_dir/test3.txt", true));
  file_create("./fs_test_dir/test.txt", false);
  printf("Move no overwrite: %d\n", file_move("./fs_test_dir/test.txt", "./fs_test_dir/test3.txt", false));
  printf("Create overwrite: %d\n", file_create("./fs_test_dir/test3.txt", true));
  printf("Create no overwrite: %d\n", file_create("./fs_test_dir/test3.txt", false));

  printf("Copy FAIL: %d\n", file_copy("./test4.txt", "./test2.txt", false));
  printf("Delete FAIL: %d\n", file_delete("./test4.txt"));
  printf("Rename FAIL: %d\n", file_move("./test4.txt", "./kys.txt", false));

  printf("Absolute path: %s\n", path_to_absolute(&sb, "kys.txt"));
  printf("Absolute path: %s\n", path_to_absolute(&sb, "fag.txt"));
  printf("Absolute path: %s\n", path_to_absolute(&sb, "not_here.txt"));
  printf("Absolute path: %s\n", path_to_absolute(&sb, "test2\\daaaaa\\dsadasad\\adasdas"));
  printf("Absolute path: %s\n", path_to_absolute(&sb, ".\\daaaaa\\dsadasad\\adasdas"));
  printf("Absolute path: %s\n", path_to_absolute(&sb, "..\\daaaaa\\dsadasad\\adasdas"));

  path_push(&sb, "fag");
  path_push(&sb, "fag");
  path_push(&sb, "fag");
  path_push(&sb, "fag");
  path_push(&sb, "kill");
  path_pop(&sb);
  printf("Path push: %s\n", sb.data);

  str_dbg(path_parent("./dada/dasdad/.weqrwq.rwq/rqwr.q/kys.txt"));

  dir_create("./fs_test_dir/test3/dasdas/reiorew/fdsjporetjw");
  dir_create_recursive("./fs_test_dir/test3/dasdas/reiorew/fdsjporetjw");
  printf("test3 dir created\n");
  dir_delete_recursive("./fs_test_dir/test3/");
  dir_delete_recursive("./test3/");
  printf("test3 dir deleted\n");

  printf("test5 dir created\n");
  dir_create_recursive("./fs_test_dir/test5/daaaaa/dsadasad/adasdas");
  file_create_recursive("./fs_test_dir/test6/kysfag/hahahah/test", false);

  printf("\nRecursive copies and dels\n");
  dir_copy_recursive("./fs_test_dir/test5", "./test_copy", false);
  dir_copy_recursive("./fs_test_dir/test6", "./test_copy_del", false);

  printf("test6 dir copied\n");
  printf("Delete rec: %d\n", dir_delete_recursive("./test_copy"));
  dir_copy_recursive("./test_copy_del/", "./fs_test_dir/test_copy_del", false);
  printf("Delete rec: %d\n", dir_delete_recursive("./test_copy_del"));
}