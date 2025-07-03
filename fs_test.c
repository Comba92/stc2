#include "stc_fs.h"

int main() {
  printf("%s\n", read_file_to_string("test.txt"));

  printf("%s\n", path_filename("D:\\code\\stc2\\str_test.c"));
  printf("%s\n", path_filename_ext("D:\\code\\stc2\\str_test.c"));

  read_dir(".");

  read_dir("/home/comba");

  printf("%d\n", file_exists("./test.txt"));
  printf("%d\n", file_exists("./faggot_killer.txt"));
  printf("%d\n", dir_exists(".."));
  printf("%d\n", dir_exists("../stc"));
  printf("%d\n", dir_exists("../urmom"));

  make_dir_if_not_exists("./test1");
  make_dir_if_not_exists("./test2");
}