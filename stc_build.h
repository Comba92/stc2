#ifndef STC_BUILD_IMPL
#define STC_BUILD_IMPL
#include "stc_str.h"
#include "stc_fs.h"

typedef struct {
  const char* binpath;
  StrList flags;
  StrList sources;
} BuildCmd;

StringList get_all_c_sources_in_dir(const char* dirpath, bool recursive) {
  DirEntries entries = dir_entries(dirpath, recursive);
  
  isize curr = 0;
  listforeach(DirEntry, e, &entries) {
    if (str_ends_with(SVC(e->name), SV(".c"))) {
      entries.data[curr++] = *e;
    } else {
      DirEntry_free(e);
    }
  }
  entries.len = curr;

  StringList filenames = {0};
  listforeach(DirEntry, e, &entries) {
    String filename = {0};

    if (e->parent != NULL) {
      String_append_cstr(&filename, e->parent);
      String_push(&filename, '/');
    }

    String_append_cstr(&filename, e->name);
    String_append_null(&filename);

    StringList_push(&filenames, filename);
  }

  DirEntries_drop(&entries);
  return filenames;
}

bool binary_needs_rebuild(const char* binpath, char** sources, isize sources_count) {
#ifndef _WIN32
  struct stat statbuf;
  // TODO: use stat error codes
  if (stat(binpath, &statbuf) != 0) return true;

  int binary_time = statbuf.st_mtime;

  rangefor(isize, i, 0, sources_count) {
    if (stat(sources[i], &statbuf) != 0) {
      exit(1);
    }

    int source_time = statbuf.st_mtime;
    if (source_time > binary_time) return true;
  }

  return false;
#else
  TODO("windows needs_rebuild()")
#endif
}

char* cmd_render(String* cmd, StrList components) {
  // sanitize
  listforeach(str, s, &components) {
    if (str_contains(*s, ' ')) {
      String_push(cmd, '"');
      String_append_str(cmd, *s);
      String_push(cmd, '"');
    } else {
      String_append_str(cmd, *s);
    }
    String_push(cmd, ' ');
  }

  // remove last space
  String_pop(cmd);
  String_append_null(cmd);

  return cmd->data;
}

int binary_rebuild(const char* binpath, char** sources, isize sources_count) {
  // TODO: check for windows .exe extension
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

int binary_exec(const char* binpath) {
  return system(binpath);
}

int binary_rebuild_and_exec(const char* binpath, char** sources, isize sources_count) {
  int res = binary_rebuild(binpath, sources, sources_count);
  if (res != 0) return res;
  
  printf("\nBuilding completed!\n\n");
  return binary_exec(binpath); 
}

// TODO: this doesnt take header files into account
int binary_rebuild_itself(char** argv, const char* source) {
  // name of binary
  const char* binpath = argv[0];
  const char* sources[] = {
    source
  };

  if (!binary_needs_rebuild(binpath, sources, 1)) return -1;

  printf("Rebuilding itself...\n");
  const char* oldbinpath = str_fmt_tmp("%s.old", binpath);
  if (!file_move(binpath, oldbinpath, true)) exit(1);

  if (binary_rebuild(binpath, sources, 1) != 0) {
    file_move(oldbinpath, binpath, true);
    exit(1);
  }

  file_delete(oldbinpath);
  binary_exec(binpath);
  exit(0);
}

#endif