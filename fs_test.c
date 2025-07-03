#include "stc_fs.h"

int main() {
  printf("%s\n", read_file_to_string("test.txt"));

  printf("%s\n", path_filename("D:\\code\\stc2\\str_test.c"));
  printf("%s\n", path_filename_ext("D:\\code\\stc2\\str_test.c"));

  open_dir(".");

  open_dir("/home/comba");
}