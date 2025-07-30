#ifndef STC_BUILD_IMPL
#define STC_BUILD_IMPL
#include "stc_str.h"
#include "stc_fs.h"

typedef struct {
  char* binpath;
  StrList flags;
  StrList sources;
} BuildCmd;

StrList get_all_c_sources_in_dir(char* dirpath, bool recursive) {
  DirEntries entries = dir_entries(dirpath, recursive);
  
  isize curr = 0;
  listforeach(DirEntry, e, &entries) {
    str name = SVC(e->name);
    if (str_ends_with(name, SV(".c")) || str_ends_with(name, SV(".h"))) {
      entries.data[curr++] = *e;
    } else {
      DirEntry_free(e);
    }
  }
  entries.len = curr;

  StrList filenames = {0};
  listforeach(DirEntry, e, &entries) {
    String filename = {0};

    if (e->parent != NULL) {
      String_append_cstr(&filename, e->parent);
      String_push(&filename, '/');
    }

    String_append_cstr(&filename, e->name);
    String_append_null(&filename);

    // memory leak but we dont care
    StrList_push(&filenames, SBV(filename));
  }

  DirEntries_drop(&entries);
  return filenames;
}

bool binary_needs_rebuild(char* binpath, char** sources, isize sources_count) {
#ifndef _WIN32
  struct stat statbuf;
  if (stat(binpath, &statbuf) != 0) {
    // binary not found, needs rebuild
    if (errno == ENOENT) return true;
    TODO("fatal error");
  }

  int binary_time = statbuf.st_mtime;

  rangefor(isize, i, 0, sources_count) {
    if (stat(sources[i], &statbuf) != 0) {
      // source not found, can't build
      TODO("fatal error");
    }

    int source_time = statbuf.st_mtime;
    // if a single source is fresher than binary, needs rebuild
    if (source_time > binary_time) return true;
  }
#else
  HANDLE binh = CreateFile(binpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (binh == INVALID_HANDLE_VALUE) {
    // binary not found, needs rebuild
    if (GetLastError() == ERROR_FILE_NOT_FOUND) return true;
    TODO("fatal error");
  }

  FILETIME binary_time;
  BOOL res = GetFileTime(binh, NULL, NULL, &binary_time);
  CloseHandle(binh);

  if (!res) {
    TODO("fatal error");
  }

  rangefor(isize, i, 0, sources_count) {
    HANDLE sourceh = CreateFile(sources[i], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (sourceh == INVALID_HANDLE_VALUE) {
      // source not found, can't build
      TODO("fatal error");
    }

      FILETIME source_time;
      res = GetFileTime(binh, NULL, NULL, &source_time);
      CloseHandle(sourceh);
      if (!res) {
        TODO("fatal error");
      }

      // if a single source is fresher than binary, needs rebuild
      if (CompareFileTime(&binary_time, &source_time) == -1) return true;
  }
#endif

  return false;
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

int binary_rebuild(char* binpath, char** sources, isize sources_count) {
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
  char* build_cmd = cmd_render(&cmd, cmd_cmp);
  printf("Executing cmd: %s\n", build_cmd);
  
  // Temporary command caller
  return system(build_cmd);
}

int binary_exec(char* binpath) {
  return system(binpath);
}

int binary_rebuild_and_exec(char* binpath, char** sources, isize sources_count) {
  int res = binary_rebuild(binpath, sources, sources_count);
  if (res != 0) return res;
  
  printf("\nBuilding completed!\n\n");
  return binary_exec(binpath); 
}

int binary_rebuild_itself(char** argv, char* source) {
  // name of binary
  char* binpath = argv[0];
  char* sources[] = {
    source,
    "stc_build.h"
  };

  if (!binary_needs_rebuild(binpath, sources, ArrayLen(sources))) return -1;

  printf("Rebuilding itself!!!\n");
  
  char* oldbinpath = str_fmt_tmp("%s.old", binpath);
  if (!file_move(binpath, oldbinpath, true)) TODO("fatal error");

  if (binary_rebuild(binpath, sources, ArrayLen(sources)) != 0) {
    file_move(oldbinpath, binpath, true);
    TODO("fatal error");
  }

  file_delete(oldbinpath);
  binary_exec(binpath);
  exit(0);
}

#endif