#include <stdio.h>
#include "stc_fs.h"
#include "stc_str.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Missing args query filepath\n");
    return 0;
  }
  
  char* query = argv[1];
  char* path  = argv[2];

  String contents = {0};
  bool res = file_read_to_string(&contents, path);
  if (!res) return 0;

  str query_str = str_from_cstr(query);
  str contents_str = String_to_tmp_str(contents);

  StrLines it = str_lines(contents_str);
  String lower = {0};
  while(str_has_line(&it)) {
    str line = str_next_line(&it);
    str_to_lower(&lower, line);
    int i = str_match(String_to_tmp_str(lower), query_str);
    if (i != -1) {
      printf(str_fmt "\n", str_arg(line));
    }
  }

  String_free(&lower);
  String_free(&contents);
}