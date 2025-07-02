#include "stc_fs.h"

int main() {
  printf("%s\n", read_file_to_string("test.txt"));

  printf("%s\n", path_filename("D:\\code\\stc2\\str_test.c"));
}