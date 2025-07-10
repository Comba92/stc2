#ifndef STC_FS_IMPL
#define STC_FS_IMPL

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "stc_list.h"
#include "stc_str.h"

#ifndef _WIN32
  #include <dirent.h>
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <sys/sendfile.h>
  #include <unistd.h>

  #define PATH_MAX_LEN PATH_MAX
#else
  // this trims windows.h with minimal functionality
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>

  #define PATH_MAX_LEN MAX_PATH
#endif

// TODO: fs_copy, fs_move, fs_delete ?? 
// TODO: create, copy and move check/versions if exists both for file and dir

// TODO: this is not thread safe retard
static String fs_tmp_sb = {0};

int fs_err_code() {
#ifndef _WIN32
  return errno;
#else
  return GetLastError();
#endif
}

char* fs_err_msg() {
  int err = fs_err_code();
#ifndef _WIN32
  return strerror(err);
#else 
  // TODO: is this thread safe?
  static char buf[512];
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
  DWORD len = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    err,
    LANG_SYSTEM_DEFAULT,
    buf, 512,
    NULL
  );

  if (len == 0) {
    snprintf(buf, len <= 512 ? len : 512, "Failed to get error message for error code %d", err);
  } else {
    while (len > 0 && isspace(buf[len-1])) buf[len--] = '\0';
  }

  return buf;
#endif
}

void fs_err_print(const char* msg) {
  int err = fs_err_code();
  if (msg == NULL || msg[0] == '\0') {
    fprintf(stderr, "[%d] %s\n", err, fs_err_msg());
  } else {
    fprintf(stderr, "%s: [%d] %s\n", msg, err, fs_err_msg());
  }
}

static FILE* file_open(const char* path, const char* opts) {
  FILE* f = fopen(path, opts);
  #ifdef STC_LOG_ERR
  if (f == NULL) fs_err_print(path);
  #endif
  return f;
}

FILE* file_open_read(const char* path) {
  // opening files without binary mode is utterly retarded
  return file_open(path, "rb");
}

FILE* file_open_write(const char* path) {
  return file_open(path, "wb");
}

bool file_close(FILE* f) {
  bool res = fclose(f) != 0;
  #ifdef STC_LOG_ERR
    if (!res) fs_err_print(str_fmt_tmp("File descriptor %d", fileno(f)));
  #endif
  return res;
}

// TODO: consider memory mapped file reading?
// https://stackoverflow.com/questions/10836609/fastest-technique-to-read-a-file-into-memory/10836820#10836820
// https://stackoverflow.com/questions/3002122/fastest-file-reading-in-c
char* file_read_to_string(const char* path) {
  FILE* f = file_open_read(path);
  if (f == NULL) {
    return NULL;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    file_close(f);
    return NULL;
  }

  size_t file_size = ftell(f);
  if (file_size < 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    file_close(f);
    return NULL;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    file_close(f);
    return NULL;
  }

  char* buf = malloc(file_size);
  size_t read = fread(buf, 1, file_size, f);

  #ifdef STC_LOG_ERR
  if (read != file_size) {
    fs_err_print(path);
  }
  #endif

  file_close(f);
  return buf;
}

bool file_write_bytes(const char* path, const char* data, size_t len) {
  FILE* fd = file_open_write(path);
  size_t wrote = fwrite(data, 1, len, fd);
  bool res = wrote != len;

  #ifdef STC_LOG_ERR
  if (!res) {
    fs_err_print(path);
  }
  #endif

  return file_close(fd) && res;
}

/////////////////////

// TODO: consider making path a sperate type
// https://doc.rust-lang.org/std/path/struct.Path.html

char* path_last_component(const char* path) {
  char* unix_path = strrchr(path, '/');
  char* wind_path = strrchr(path, '\\');
  char* res = unix_path > wind_path ? unix_path : wind_path;
  return res;
}

char* path_filename(const char* path) {
  char* res = path_last_component(path);
  if (res == NULL) return (char*) path;
  return res + 1;
}

char* path_extension(const char* path) {
  char* res = strrchr(path, '.');
  if (res == NULL) return "";
  return res + 1;
}

str path_filename_no_ext(const char* path) {
  str s = str_from_cstr(path_filename(path));
  return str_skip_rev_untilc(s, '.');
}

str path_parent(const char* path) {
  char* parent_start = path_last_component(path);
  // there are no parents
  if (parent_start == NULL || parent_start == path) return STR_EMPTY;
  return str_from_cstr_unchecked(path, parent_start - path);
}

// TODO: Should this return NULL on error?
// TODO: consider using dir_current() and concat the relative path
// TODO: this only works if path is a correct relative path; no check are done to see if absolute path is valid
char* path_to_absolute(const char* path) {
#ifndef _WIN32
  // https://man7.org/linux/man-pages/man3/realpath.3.html
  char* res = realpath(path, NULL);
  #ifdef STC_LOG_ERR
  if (res == NULL) fs_err_print(str_fmt_tmp("Failed to get absolute path for relative path %s", path));
  #endif
  return res;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
  DWORD path_size = GetFullPathName(path, 0, NULL, NULL);
  if (path_size == 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp( "Failed to get absolute path length for relative path %s", path));
    #endif
    return NULL;
  }
  char* buf = malloc(path_size);
  DWORD res = GetFullPathName(path, path_size, buf, NULL);
  if (res == 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("Failed to get absolute path for relative path %s", path));
    #endif
    return NULL;
  }
  return buf;
#endif
}

str path_prefix(const char* path) {
  if (path[0] == '/') return str_from_cstr_unchecked("/", 1);
  else if (c_is_alpha(path[0]) && path[1] == ':' && path[2] == '\\') {
    return str_from_cstr_unchecked(path, 3);
  }
  // TODO: windows other weird prefixes aren't supported

  return STR_EMPTY;
}

bool path_is_absolute(const char * path) {
  bool is_unix_abs = path[0] == '/';
  bool is_wind_abs = 
  (c_is_alpha(path[0]) && path[1] == ':' && path[2] == '\\')
  || (path[0] == '\\' && path[1] == '\\');

  return is_unix_abs || is_wind_abs;
}

bool path_is_relative(const char* path) {
  return !path_is_absolute(path);
}

// TODO: might be an iterator
bool c_is_separator(char c) { return c == '/' || c == '\\'; }
StrList path_components(const char* path) {
  return str_split_when_collect(str_from_cstr(path), c_is_separator);
}

// https://doc.rust-lang.org/std/path/struct.PathBuf.html

void path_push(String* sb, const char* path) {
  if (path_is_absolute(path)) sb->len = 0;
  else if (*String_last(*sb) != '/' && path[0] != '/') String_push(sb, '/');
  String_append_cstr(sb, path);
  String_append_null(sb);
}

bool path_pop(String* sb) {
  // we append null so we can use path_last_component
  String_append_null(sb);
  char* last_component_start = path_last_component(sb->data);
  if (last_component_start == NULL) return false;
  sb->len = last_component_start - sb->data;
  return true;
}

void path_set_filename(String* sb, const char* filename) {
  path_pop(sb);
  path_push(sb, filename);
}

bool path_set_extension(String* sb, const char* extension) {
  if (sb->len == 0) return false;

  String_append_null(sb);
  bool has_dot = extension[0] == '.';
  char* ext_start = strrchr(sb->data, '.');

  if (ext_start != NULL) {
    sb->len = (ext_start - sb->data) - 1;
  }

  if (!has_dot) String_push(sb, '.');
  String_append_cstr(sb, extension);
  return true;
}

/////////////////////

typedef enum {
  FileType_Other,
  FileType_File,
  FileType_Dir,
  // TODO: symlinks are not yet supported anywhere
  FileType_Link,
} FileType;

typedef struct {
  char* name;
  FileType type;
} DirEntry;

typedef struct {
#ifndef _WIN32
  int dir_fd;
  DIR* dir;
#else
  WIN32_FIND_DATA data;
  HANDLE h;
#endif
  bool finished;
  DirEntry curr;
} DirIter;

// TODO: should precompute at least first element?
DirIter dir_open(const char* dirpath) {
#ifndef _WIN32
  int dir_fd = open(dirpath, O_RDONLY, O_DIRECTORY);

  #ifdef STC_LOG_ERR
  if (dir_fd == -1) {
    fs_err_print(dirpath);
  }
  #endif

  // https://man7.org/linux/man-pages/man3/fdopendir.3p.html
  DIR* dir = fdopendir(dir_fd);

  #ifdef STC_LOG_ERR
  if (dir == NULL) {
    fs_err_print(dirpath);
  }
  #endif

  bool had_error = dir_fd == -1 || dir == NULL;
  DirIter it = { dir_fd, dir, had_error };
#else
  char buf[PATH_MAX_LEN];
  snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

  // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilea
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfileexa
  WIN32_FIND_DATA data;
  // HANDLE h = FindFirstFile(buf, &data);
  // this should be faster than FindFirstFile()
  HANDLE h = FindFirstFileEx(buf, FindExInfoBasic, &data, FindExSearchNameMatch, NULL, 0);

  #ifdef STC_LOG_ERR
  if (h == INVALID_HANDLE_VALUE) {
    fs_err_print(dirpath);
  }
  #endif

  bool had_error = h == INVALID_HANDLE_VALUE;
  DirIter it = { data, h, had_error };
#endif
  return it;
}

bool dir_close(DirIter* it) {
#ifndef _WIN32
  bool res = closedir(it->dir) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(str_fmt_tmp("Closing dir descriptor %d", it->dir_fd));
  #endif
#else
  bool res = FindClose(it->h) != 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(str_fmt_tmp("Closing dir handle %p", it->h));
  #endif
#endif
  it->finished = true;
  return res;
}

bool dir_scanning(const DirIter* it) {
  return !it->finished;
}

// TODO: there are currently no way to get an error bool out of this
// when an error occurs, the iterator simply stops
// TODO: on error/end, should return NULL or empty struct?
DirEntry* dir_read(DirIter* it) {
#ifndef _WIN32
  int prev_err = errno;
  struct dirent* dp;
  char* filename;

  // readdir until a valid file is returned
  bool is_special_entry = false;
  do {
    dp = readdir(it->dir);

    // no more files left (or some other error)
    if (dp == NULL) {
      #ifdef STC_LOG_ERR
      if (prev_err != errno) fs_err_print(str_fmt_tmp("Dir descriptor %d", it->dir_fd));
      #endif

      dir_close(it);
      return NULL;
    }

    filename = dp->d_name;
    is_special_entry = strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0;
  } while (is_special_entry);

  // get file data
  struct stat statbuf;
  int fd = openat(it->dir_fd, filename, O_RDONLY);
  if (fstat(fd, &statbuf) != 0) {
    #ifdef STC_LOG_ERR
      fs_err_print(str_fmt_tmp(filename));
    #endif

    it->curr.name = "";
    it->curr.type = FileType_Other;
    return &it->curr;
  }

  it->curr.name = dp->d_name;
  switch (statbuf.st_mode & S_IFMT) {
    case S_IFREG: it->curr.type = FileType_File; break;
    case S_IFDIR: it->curr.type = FileType_Dir; break;
    case S_IFLNK: it->curr.type = FileType_Link; break;
    default: it->curr.type = FileType_Other; break;
  }

  if (close(fd) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(it->curr.name);
    #endif
  };
#else
  char* filename;
  bool is_special_entry = false;

  // FindNextFile until a valid file is returned
  do {
    bool res = FindNextFile(it->h, &it->data) != 0;

    // no more files left
    if (!res) {
      #ifdef STC_LOG_ERR
      if (GetLastError() != ERROR_NO_MORE_FILES) {
        fs_err_print(str_fmt_tmp("Dir handle %p", it->h));
      }
      #endif

      dir_close(it);
      return NULL;
    }

    filename = it->data.cFileName;
    is_special_entry = strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0;
  } while (is_special_entry);

  // get file data
  it->curr.name = filename;
  switch (it->data.dwFileAttributes) {
    case FILE_ATTRIBUTE_DIRECTORY: it->curr.type = FileType_Dir; break;
    default: it->curr.type = FileType_File; break;
  }
#endif
  return &it->curr;
}

#define dir_iter(ent, it) for(DirEntry* ent; (ent = dir_read(it)) != NULL;)

list_def(DirEntry, DirEntries)
void DirEntries_drop(DirEntries* entries) {
  if (entries->data == NULL) return;
  listforeach(DirEntry, e, entries) free(e->name);
  free(entries->data);
}

// TODO: what if any function fails? Consider passing DirEntries as pointer and returning success as bool
DirEntries dir_entries(const char* dirpath) {
  DirEntries entries = {0};
  DirIter it = dir_open(dirpath);

  for (DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    DirEntry to_push = { strdup(entry->name), entry->type };
    DirEntries_push(&entries, to_push);
  }

  return entries;
}

int dir_entries_cmp(const void* a, const void* b) {
  char* a_str = ((DirEntry*)a)->name;
  char* b_str = ((DirEntry*)b)->name;
  return strcmp(a_str, b_str);
}
DirEntries dir_entries_sorted(const char* dirpath) {
  DirEntries entries = dir_entries(dirpath);
  qsort(entries.data, entries.len, sizeof(DirEntry), dir_entries_cmp);
  return entries;
}

// DirEntries dir_read_collect(const char* dirpath) {
//   DirEntries entries = {0};
//   DirEntry entry = {0};

// #ifndef _WIN32
//   int dir_fd = open(dirpath, O_RDONLY, O_DIRECTORY);
//   DIR* d = fdopendir(dir_fd);

//   if (d == NULL) {
//     #ifdef STC_LOG_ERR
//     fs_err_print(dirpath);
//     #endif
//     return entries;
//   }
  
//   struct dirent* dp;
//   struct stat statbuf;

//   int prev_err = errno;
//   // https://man7.org/linux/man-pages/man3/fdopendir.3p.html
//   // https://man7.org/linux/man-pages/man3/readdir.3p.html
//   while ((dp = readdir(d)) != NULL) {
//     char* filename = dp->d_name;
//     if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

//     int fd = openat(dir_fd, filename, O_RDONLY);
//     if (fstat(fd, &statbuf) != 0) {
//       #ifdef STC_LOG_ERR
//         fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
//       #endif
//       continue;
//     }

//     entry.name = dp->d_name;
//     switch (statbuf.st_mode & S_IFMT) {
//       case S_IFREG: entry.type = FileType_File; break;
//       case S_IFDIR: entry.type = FileType_Dir; break;
//       case S_IFLNK: entry.type = FileType_Link; break;
//       default: entry.type = FileType_Other; break;
//     }

//     DirEntries_push(&entries, entry);
//     if (close(fd) != 0) {
//       #ifdef STC_LOG_ERR
//       fs_err_print(entry.name);
//       #endif
//       continue;
//     };
//   }

//     #ifdef STC_LOG_ERR
//     if (prev_err != errno) fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
//     #endif

//     if (closedir(d) != 0) {
//       #ifdef STC_LOG_ERR
//       fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
//       #endif
//     };
// #else
//   char buf[PATH_MAX_LEN];
//   snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

//   // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
//   WIN32_FIND_DATA data;
//   HANDLE h = FindFirstFile(buf, &data);
//   if (h == INVALID_HANDLE_VALUE) {
//     #ifdef STC_LOG_ERR
//     fs_err_print(dirpath);
//     #endif
//     return entries;
//   }

//   do {
//     char* filename = data.cFileName;
//     if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

//     entry.name = strdup(filename);
//     switch (data.dwFileAttributes) {
//       case FILE_ATTRIBUTE_DIRECTORY: entry.type = FileType_Dir; break;
//       default: entry.type = FileType_File; break;
//     }

//     DirEntries_push(&entries, entry);
//   } while(FindNextFile(h, &data) != 0);
  
//   if (GetLastError() != ERROR_NO_MORE_FILES) {
//     #ifdef STC_LOG_ERR
//     fs_err_print(str_fmt_tmp("Dir handle %p", h));
//     #endif
//   }

//   if (FindClose(h) == 0) {
//     #ifdef STC_LOG_ERR
//     fs_err_print(str_fmt_tmp("Dir handle %p", h));
//     #endif
//   }
// #endif

//   return entries;
// }

/////////////////////

bool path_exists(const char* path) {
#ifndef _WIN32
  struct stat buf;
  return stat(path, &buf) == 0;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesa?redirectedfrom=MSDN
  return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
#endif
}

bool file_exists(const char* path) {
#ifndef _WIN32
  struct stat buf;
  return stat(path, &buf) == 0 && S_ISREG(buf.st_mode);
#else
  return path_exists(path);
#endif
}

bool dir_exists(const char* path) {
#ifndef _WIN32
  struct stat buf;
  return stat(path, &buf) == 0 && S_ISDIR(buf.st_mode);
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesa?redirectedfrom=MSDN
  DWORD attribs = GetFileAttributes(path);
  return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}

FileType file_type(const char* path) {
#ifndef _WIN32
  struct stat buf;
  if (stat(path, &buf) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return FileType_Other;
  }
  
  switch (buf.st_mode & S_IFMT) {
    case S_IFREG: return FileType_File;
    case S_IFDIR: return FileType_Dir;
    case S_IFLNK: return FileType_Link;
    default: return FileType_Other;
  }
#else
  DWORD attribs = GetFileAttributes(path);
  if (attribs == INVALID_FILE_ATTRIBUTES) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return FileType_Other;
  }

  switch (attribs) {
    case FILE_ATTRIBUTE_DIRECTORY: return FileType_Dir;
    default: return FileType_File;
  }
#endif
}

size_t file_size(const char* path) {
  FILE* f = file_open_read(path);
  if (f == NULL) return 0;

#ifndef _WIN32
  if (fseek(f, 0, SEEK_END) != 0) return 0;
  long long res = ftell(f);
#else 
  if (_fseeki64(f, 0, SEEK_END) != 0) return 0;
  long long res = _ftelli64(f);
#endif

  if (res < 0) return 0;
  else return res;
}

// TODO: this is not thread safe retard
static char cwd[PATH_MAX_LEN];
char* dir_current() {
#ifndef _WIN32
  char* res = getcwd(cwd, PATH_MAX_LEN);
  #ifdef STC_LOG_ERR
  if (res == NULL) fs_err_print(str_fmt_tmp("Failed to get working directory"));
  #endif
  return res;
#else
  if (GetCurrentDirectory(PATH_MAX_LEN, cwd) == 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("Failed to get working directory"));
    #endif
  }
  return cwd;
#endif
}

bool dir_set_current(const char* path) {
#ifndef _WIN32
  bool res = chdir(path) == 0;
#else
  bool res = SetCurrentDirectory(path) != 0;
#endif

  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(str_fmt_tmp("Failed to set working directory"));
  #endif
  return res;
}

bool file_delete(const char* src) {
  int res = remove(src) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(src);
  #endif
  return res;
}


bool file_move(const char* src, const char* dst) {
#ifndef _WIN32
  int res = rename(src, dst) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(src);
  #endif
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefile
  bool res = MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) != 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(src);
  #endif
  return res;
#endif
}

bool dir_create(const char* path) {
  if (dir_exists(path)) return true;
  
#ifndef _WIN32
  bool res = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya?redirectedfrom=MSDN
  bool res = CreateDirectory(path, NULL) != 0;
#endif

  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(path);
  #endif
  return res;
}

bool dir_create_recursive(const char* path) {
  fs_tmp_sb.len = 0;
  StrList components = path_components(path);

  String_append_str(&fs_tmp_sb, path_prefix(path));

  bool had_error = false;
  listforeach(str, component, &components) {
    String_append_str(&fs_tmp_sb, *component);
    String_push(&fs_tmp_sb, '/');
    String_append_null(&fs_tmp_sb);
    
    if (!dir_create(fs_tmp_sb.data)) {
      had_error = true;
      break;
    }
  }

  StrList_free(&components);
  return !had_error;
}

bool dir_move(const char* src, const char* dst) {
  return file_move(src, dst);
}

bool dir_delete(const char* path) {
  if (!dir_exists(path)) return true;
  
#ifndef _WIN32
  bool res = rmdir(path) == 0;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-removedirectorya
  bool res = RemoveDirectory(path) != 0;
#endif

  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(path);
  #endif

  return res;
}

bool file_create(const char* path, bool overwrite) {
#ifndef _WIN32
  int flags = overwrite ? O_TRUNC : 0;
  // https://man7.org/linux/man-pages/man2/open.2.html
  int fd = open(path, O_WRONLY | O_CREAT | flags, S_IRWXU | S_IRWXG | S_IRWXO);
  // int fd = creat(path, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd == -1) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return false;
  }
  if (close(fd) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return false;
  }
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/fileapi/nf-fileapi-createfilea
  DWORD flags = overwrite ? CREATE_ALWAYS : CREATE_NEW;
  HANDLE h = CreateFile(path, GENERIC_WRITE, 0, NULL, flags, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return false;
  }
  if (CloseHandle(h) == 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(path);
    #endif
    return false;
  }
#endif

  return true;
}

bool file_create_recursive(const char* path, bool overwrite) {
  str parent = path_parent(path);
  fs_tmp_sb.len = 0;
  String_append_str(&fs_tmp_sb, parent);
  String_append_null(&fs_tmp_sb);

  return dir_create_recursive(fs_tmp_sb.data) 
    && file_create(path, overwrite);
}


bool file_copy(const char* src, const char* dst, bool overwrite) {
#ifndef _WIN32
  int src_fd = open(src, O_RDONLY, 0);

  if (src_fd == -1) {
    #ifdef STC_LOG_ERR
    fs_err_print(src);
    #endif
    return false;
  }

  int flags = overwrite ? O_TRUNC : 0;
  int dst_fd = open(dst, O_WRONLY | O_CREAT | flags, S_IRWXU | S_IRWXG | S_IRWXO);

  if (dst_fd == -1) {
    #ifdef STC_LOG_ERR
    fs_err_print(dst);
    #endif
    return false;
  }

  struct stat statbuf;
  if (fstat(src_fd, &statbuf) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(src);
    #endif
    return false;
  }
  size_t src_size = statbuf.st_size;

  // both copy_file_range abd sendfile are not always avaible on unix
  // might want to fallback to fread and fwrite
  // TODO: macos has flconefileat and fcopyfile

  // https://man7.org/linux/man-pages/man2/copy_file_range.2.html
  // https://man7.org/linux/man-pages/man2/sendfile.2.html
  if (sendfile(dst_fd, src_fd, NULL, src_size) == -1) {
  // if (copy_file_range(src_fd, NULL, dst_fd, NULL, src_size, 0) == -1) {
    #ifdef STC_LOG_ERR
    fs_err_print(dst);
    #endif
    return false;
  }

  if (close(src_fd) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(src);
    #endif
    return false;
  }

  if (close(dst_fd) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(dst);
    #endif
    return false;
  }

  return true;
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/winbase/nf-winbase-copyfile
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfileexa
  BOOL res = CopyFile(src, dst, !overwrite);
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(dst);
  #endif
  return res;
#endif
}

bool dir_copy_recursive(const char* src, const char* dst) {
  if (!dir_create_recursive(dst)) {
    return false;
  }

  DirIter it = dir_open(src);
  if (it.finished) return false;

  /*
    https://man7.org/linux/man-pages/man3/readdir.3.html
    The data returned by readdir() may be overwritten by subsequent
    calls to readdir() for the same directory stream.

    This means we don't need to collect the entries!
  */

  String sb_src = String_from_cstr(src);
  String sb_dst = String_from_cstr(dst);

  bool had_error = false;
  for(DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    path_push(&sb_src, entry->name);
    path_push(&sb_dst, entry->name);

    switch (entry->type) {
      case FileType_File: {
        had_error |= !file_copy(sb_src.data, sb_dst.data, true);
      } break;
      case FileType_Dir: {
        had_error |= !dir_create(sb_dst.data);
        had_error |= !dir_copy_recursive(sb_src.data, sb_dst.data);
      } break;

      default: break;
    }

    path_pop(&sb_src);
    path_pop(&sb_dst);
  }

  return !had_error;
}


bool dir_delete_recursive(const char* path) { 
  if (path_is_absolute(path)) {
    fprintf(stderr, "I AM NOT SURE YOU WANT TO DELETE AN ABSOLUTE DIRECTORY, ABORTING\n");
    return false;
  }

  DirIter it = dir_open(path);
  if (it.finished) return false;

  String sb = String_from_cstr(path);

  bool had_error = false;
  for(DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    path_push(&sb, entry->name);

    switch (entry->type) {
      case FileType_File: {
        had_error |= !file_delete(sb.data);
      } break;
      case FileType_Dir: {
        had_error |= !dir_delete_recursive(sb.data);
        had_error |= !dir_delete(sb.data);
      } break;

      default: break;
    }

    path_pop(&sb);
  }

  had_error |= !dir_delete(path);
  return !had_error;
}

#endif