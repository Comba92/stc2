#ifndef STC_BUILD_IMPL
#define STC_BUILD_IMPL
#include "stc_defs.h"
#include "stc_str.h"
#include "stc_fs.h"

#ifndef _WIN32
#define BIN_EXTENSION ""
#define BIN_RELATIVE "./"
#else 
#define BIN_EXTENSION ".exe"
#define BIN_RELATIVE ""
#endif

// TODO: FIX TODOs
// TODO: better logging
// TODO: async command running (create new process and wait for it)

CstrList get_all_c_sources_in_dir(char* dirpath, bool recursive) {
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

  CstrList filenames = {0};
  listforeach(DirEntry, e, &entries) {
    String filename = {0};

    if (e->parent != NULL) {
      String_append_cstr(&filename, e->parent);
      String_push(&filename, '/');
    }

    String_append_cstr(&filename, e->name);
    String_append_null(&filename);

    CstrList_push(&filenames, filename.data);
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
  res &= CloseHandle(binh);

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
      res &= CloseHandle(sourceh);
      if (!res) {
        TODO("fatal error");
      }

      // if a single source is fresher than binary, needs rebuild
      if (CompareFileTime(&binary_time, &source_time) == -1) return true;
  }
#endif

  return false;
}

char* cmd_render(String* cmd, CstrList components) {
  // sanitize
  listforeach(char*, s, &components) {
    if (strchr(*s, ' ')) {
      String_push(cmd, '"');
      String_append_cstr(cmd, *s);
      String_push(cmd, '"');
    } else {
      String_append_cstr(cmd, *s);
    }
    String_push(cmd, ' ');
  }

  // remove last space
  String_pop(cmd);
  String_append_null(cmd);

  return cmd->data;
}

int binary_rebuild_flags(char* binpath, char** flags, isize flags_count, char** sources, isize sources_count) {
  // TODO: check for windows .exe extension

  if (!binary_needs_rebuild(binpath, sources, sources_count)) return -1;

  CstrList cmd_cmp = {0};
  // push compiler name
  CstrList_push(&cmd_cmp, "gcc");

  // push flags
  rangefor(isize, i, 0, flags_count) {
    CstrList_push(&cmd_cmp, flags[i]);
  }

  // push inputs and sanitize
  rangefor(isize, i, 0, sources_count) {
    if (strchr(sources[i], ' ')) {
      String sb = {0};
      String_push(&sb, '"');
      String_append_cstr(&sb, sources[i]);
      String_push(&sb, '"');
      String_append_null(&sb);

      // memory leak here, we dont care
      CstrList_push(&cmd_cmp, sb.data);
    } else {
      CstrList_push(&cmd_cmp, sources[i]);
    }
  }

  // push output
  CstrList_push(&cmd_cmp, "-o");
  CstrList_push(&cmd_cmp, binpath);

  // this is needed for execvpe()
  // CstrList_push(&cmd_cmp, NULL);

  String cmd = {0};
  char* build_cmd = cmd_render(&cmd, cmd_cmp);
  printf("Executing cmd: %s\n", build_cmd);
  
  CstrList_free(&cmd_cmp);

  // Temporary command caller
  int res = system(build_cmd);
  free(build_cmd);
  return res;
}

// return 0 -> ok
// return -1 -> no rebuild needed
// return >0 -> rebuild error
int binary_rebuild(char* binpath, char** sources, isize sources_count) {
  static char* flags[] = {
    "-Wall",
    "-Wextra",
  };

  return binary_rebuild_flags(binpath, flags, ArrayLen(flags), sources, sources_count);
}

int binary_exec(char* binpath) {
  printf("Executing %s\n", binpath);
#ifndef _WIN32
  if (str_starts_with(SVC(binpath), SV("./"))) {
    return system(binpath);
  } else {
    return system(str_fmt_tmp("./%s", binpath));
  }
#else
  return system(binpath);
#endif
}

#define BIN_REBUILD_ITSELF(argc, argv) binary_rebuild_itself((argc), (argv), __FILE__)
void binary_rebuild_itself(int argc, char** argv, char* source) {
  UNUSED(argc);

  // name of binary
  char* binpath = argv[0];
  char* sources[] = {
    source,
    "stc_build.h",
  };

  char* tmpbinpath = str_fmt_tmp("%s.tmp", binpath);
  // delte tmp file if any
  file_delete(tmpbinpath);

  if (!binary_needs_rebuild(binpath, sources, ArrayLen(sources))) return;

  printf("Rebuilding itself!\n");
  // move binary to tmp file
  if (!file_move(binpath, tmpbinpath, true)) TODO("fatal error");

  // rebuild binary
  if (binary_rebuild(binpath, sources, 1) != 0) {
    // rebuild failed, restore old binary
    file_move(tmpbinpath, binpath, true);
    TODO("fatal error");
  }

  binary_exec(binpath);
  exit(0);
}

int binary_rebuild_all(char** sources, isize sources_count) {
  int success = 0;
  
  rangefor(isize, i, 0, sources_count) {
    str filename = SVC(sources[i]);
    if (!str_ends_with(filename, SV(".c"))) { continue; }

    String binpath = String_from_str(filename);
    path_set_extension(&binpath, BIN_EXTENSION);
    success += binary_rebuild(binpath.data, sources + i, 1) == 0 ? 1 : 0;
  }

  return success;
}

#endif