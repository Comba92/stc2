#ifndef STC_BUILD_IMPL
#define STC_BUILD_IMPL
#include "stc_str.h"

bool binary_needs_rebuild(const char* binpath, char** sources, isize sources_count) {
  // TODO
  return true;
}

char* cmd_render(String* cmd, StrList components) {
  // sanitize
  listforeach(str, s, &components) {
    printf(str_fmt"\n", str_arg(*s));
    
    if (str_contains(*s, ' ')) {
      String_push(cmd, '"');
      String_append_str(cmd, *s);
      String_push(cmd, '"');
    } else {
      String_append_str(cmd, *s);
    }

    String_push(cmd, ' ');
  }

  printf(str_fmt"\n", str_arg(*cmd));


  // remove last space
  String_pop(cmd);
  String_append_null(cmd);

  printf(str_fmt"\n", str_arg(*cmd));

  return cmd->data;
}

int binary_rebuild(const char* binpath, char** sources, isize sources_count) {
  // check for windows .exe extension

  if (!binary_needs_rebuild(binpath, sources, sources_count)) return -1;
  StrList cmd_cmp = {0};

  StrList_push(&cmd_cmp, SV("gcc"));
  StrList_push(&cmd_cmp, SV("-Wall"));
  StrList_push(&cmd_cmp, SV("-Wextra"));
  StrList_push(&cmd_cmp, SV("-o"));
  StrList_push(&cmd_cmp, SVC(binpath));

  rangefor(isize, i, 0, sources_count) {
    printf("%s\n", sources[i]);
    StrList_push(&cmd_cmp, SVC(sources[i]));
  }

  String cmd = {0};
  const char* build_cmd = cmd_render(&cmd, cmd_cmp);
  printf("Executing cmd: %s\n", build_cmd);
  
  // Temporary command caller
  return system(build_cmd);
}

#endif