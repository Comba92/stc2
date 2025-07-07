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
  #include <windows.h>

  #define PATH_MAX_LEN MAX_PATH
#endif

// TODO: should if_exists/not_exists function check for existance?

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

void fs_err_print(char* msg) {
  if (msg == NULL || msg == "") {
    fprintf(stderr, "%s\n", fs_err_msg());
  } else {
    fprintf(stderr, "%s: %s\n", msg, fs_err_msg());
  }
}

static FILE* file_open(char* path, const char* opts) {
  FILE* f = fopen(path, opts);
  #ifdef STC_LOG_ERR
  if (f == NULL) fs_err_print(path);
  #endif
  return f;
}

FILE* file_open_read(char* path) {
  return file_open(path, "rb");
}

FILE* file_open_write(char* path) {
  return file_open(path, "wb");
}

FILE* file_open_append(char* path) {
  return file_open(path, "ab");
}

bool file_close(FILE* f) {
  bool res = fclose(f) != 0;
  #ifdef STC_LOG_ERR
    if (!res) fs_err_print(str_fmt_tmp("File descriptor %d", fileno(f)));
  #endif
  return res;
}

// TODO: consider memory mapped file reading
// https://stackoverflow.com/questions/10836609/fastest-technique-to-read-a-file-into-memory/10836820#10836820
// https://stackoverflow.com/questions/3002122/fastest-file-reading-in-c
char* file_read_to_string(char* path) {
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

bool file_write_bytes(char* path, char* data, size_t len) {
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

bool file_append_bytes(FILE* fd, char* data, size_t len) {
  size_t wrote = fwrite(data, 1, len, fd);

  if (wrote != len) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("File descriptor %d", fileno(fd)));
    #endif
    return false;
  }

  return true;
}

bool file_delete(char* src) {
  int res = remove(src) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(src);
  #endif
  return res;
}

bool file_move(char* src, char* dst) {
  int res = rename(src, dst) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(src);
  #endif
  return res;
}


// #ifdef _WIN32
// static void perror_windows(const char* msg) {
//   DWORD err = GetLastError();
//   static char buf[512];

//   // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
//   DWORD len = FormatMessage(
//     FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//     NULL,
//     err,
//     LANG_SYSTEM_DEFAULT,
//     buf, 512,
//     NULL
//   );

//   if (len == 0) {
//     fprintf(stderr, "%s: couldn't get error message from Windows for error ID %ld\n", msg, err);
//   } else {
//     while (len > 0 && isspace(buf[len-1])) buf[len--] = '\0';
//     fprintf(stderr, "%s: %s\n", msg, buf);
//   }
// }
// #endif

/////////////////////

// TODO: consider making path a sperate type
// https://doc.rust-lang.org/std/path/struct.Path.html

char* path_last_component(char* path) {
  char* unix_path = strrchr(path, '/');
  char* wind_path = strrchr(path, '\\');
  char* res = unix_path > wind_path ? unix_path : wind_path;
  return res;
}

char* path_filename(char* path) {
  char* res = path_last_component(path);
  if (res == NULL) return path;
  return res + 1;
}

char* path_extension(char* path) {
  char* res = strrchr(path, '.');
  if (res == NULL) return "";
  return res + 1;
}

str path_filename_no_ext(char* path) {
  str s = str_from_cstr(path_filename(path));
  return str_skip_rev_untilc(s, '.');
}

str path_parent(char* path) {
  char* parent_start = path_last_component(path);
  // there are no parents
  if (parent_start == NULL || parent_start == path) return STR_EMPTY;
  return str_from_cstr_unchecked(path, parent_start - path);
}

// TODO: Should this return NULL on error?
char* path_to_absolute(char* path) {
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

str path_prefix(char* path) {
  if (path[0] == '/') return str_from_cstr_unchecked("/", 1);
  else if (isalpha(path[0]) && path[1] == ':' && path[2] == '\\') {
    return str_take(str_from_cstr(path), 3);
  }
  // TODO: windows other weird prefixes aren't supported

  return STR_EMPTY;
}

bool path_is_absolute(char * path) {
  bool is_unix_abs = path[0] == '/';
  bool is_wind_abs = 
  (isalpha(path[0]) && path[1] == ':' && path[2] == '\\')
  || (path[0] == '\\' && path[1] == '\\');

  return is_unix_abs || is_wind_abs;
}

bool path_is_relative(char* path) {
  return !path_is_absolute(path);
}

// TODO: might be an iterator
int isseparator(int c) { return c == '/' || c == '\\'; }
StrList path_components(char* path) {
  return str_split_when_collect(str_from_cstr(path), isseparator);
}

// https://doc.rust-lang.org/std/path/struct.PathBuf.html

void path_push(String* sb, char* path) {
  if (path_is_absolute(path)) sb->len = 0;
  else if (String_last(*sb) != '/') String_push(sb, '/');
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

void path_set_filename(String* sb, char* filename) {
  path_pop(sb);
  path_push(sb, filename);
}

bool path_set_extension(String* sb, char* extension) {
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
  FileType_Link,
} FileType;

typedef struct {
  char* parent;
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
  DirEntry last_entry;
} DirRead;

DirRead dir_open(char* dirpath) {
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

  return (DirRead) { dir_fd, dir };
#else
  char buf[PATH_MAX_LEN];
  snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

  // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(buf, &data);

  #ifdef STC_LOG_ERR
  if (h == INVALID_HANDLE_VALUE) {
    fs_err_print(dirpath);
  }
  #endif

  return (DirRead) { data, h };
#endif
}

bool dir_close(DirRead* it) {
#ifndef _WIN32
  bool res = closedir(it->dir) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(str_fmt_tmp("Dir descriptor %d", it->dir_fd));
  #endif
#else
  bool res = FindClose(it->h) == 0;
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(str_fmt_tmp("Dir handle %p", it->h));
  #endif
#endif
  return res;
}

DirEntry dir_read(DirRead* it) {
  DirEntry entry = {0};
#ifndef _WIN32
  int prev_err = errno;
  
  struct dirent* dp;
  char* filename;
  do {
    dp = readdir(it->dir);

    if (dp == NULL) {
      #ifdef STC_LOG_ERR
      if (prev_err != errno) fs_err_print(str_fmt_tmp("Dir descriptor %d", it->dir_fd));
      #endif

      dir_close(it);
      return entry;
    }

    filename = dp->d_name;
  } while (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0);

  struct stat statbuf;
  int fd = openat(it->dir_fd, filename, O_RDONLY);
  if (fstat(fd, &statbuf) != 0) {
    #ifdef STC_LOG_ERR
      fs_err_print(str_fmt_tmp("Dir descriptor %d", it->dir_fd));
    #endif
    // printf("Bad filename %s\n", filename);
    // perror("Couldn't read file during directory scan");

    return entry;
  }

  entry.name = dp->d_name;
  entry.parent = path_to_absolute(entry.name);
  switch (statbuf.st_mode & S_IFMT) {
    case S_IFREG: entry.type = FileType_File; break;
    case S_IFDIR: entry.type = FileType_Dir; break;
    case S_IFLNK: entry.type = FileType_Link; break;
    default: entry.type = FileType_Other; break;
  }

  if (close(fd) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(entry.name);
    #endif
  };
#else
  char* filename;
  do {
    bool res = FindNextFile(it->h, &it->data) != 0;
    if (!res) {
      #ifdef STC_LOG_ERR
      if (GetLastError() != ERROR_NO_MORE_FILES) {
        fs_err_print(str_fmt_tmp("Dir handle %p", it->h));
      }
      #endif

      dir_close(it);
      return entry;
    }

    filename = it->data.cFileName;
  } while (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0);

  entry.name = strdup(filename);
  entry.parent = path_to_absolute(entry.name);
  switch (it->data.dwFileAttributes) {
    case FILE_ATTRIBUTE_DIRECTORY: entry.type = FileType_Dir; break;
    default: entry.type = FileType_File; break;
  }
#endif
  return entry;
}


list_def(DirEntry, DirEntries)
void DirEntries_drop(DirEntries* entries) {
  listforeach(DirEntry, e, entries) free(e->name);
  free(entries->data);
}

// TODO: enable entries sorting
// TODO: rewrite in terms of dir_open() and dir_read()
DirEntries dir_read_collect(char* dirpath) {
  DirEntries entries = {0};
  DirEntry entry = {0};

#ifndef _WIN32
  int dir_fd = open(dirpath, O_RDONLY, O_DIRECTORY);
  DIR* d = fdopendir(dir_fd);

  if (d == NULL) {
    #ifdef STC_LOG_ERR
    fs_err_print(dirpath);
    #endif
    return entries;
  }
  
  struct dirent* dp;
  struct stat statbuf;

  int prev_err = errno;
  // https://man7.org/linux/man-pages/man3/fdopendir.3p.html
  // https://man7.org/linux/man-pages/man3/readdir.3p.html
  while ((dp = readdir(d)) != NULL) {
    char* filename = dp->d_name;
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

    int fd = openat(dir_fd, filename, O_RDONLY);
    if (fstat(fd, &statbuf) != 0) {
      #ifdef STC_LOG_ERR
        fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
      #endif
      continue;
    }

    entry.name = dp->d_name;
    entry.parent = path_to_absolute(entry.name);
    switch (statbuf.st_mode & S_IFMT) {
      case S_IFREG: entry.type = FileType_File; break;
      case S_IFDIR: entry.type = FileType_Dir; break;
      case S_IFLNK: entry.type = FileType_Link; break;
      default: entry.type = FileType_Other; break;
    }

    DirEntries_push(&entries, entry);
    if (close(fd) != 0) {
      #ifdef STC_LOG_ERR
      fs_err_print(entry.name);
      #endif
      continue;
    };
  }

    #ifdef STC_LOG_ERR
    if (prev_err != errno) fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
    #endif

  // struct dirent **entries;
  // int n = scandir(".", &entries, 0, alphasort);
  // if (n < 0) {
    // perror("scandir");
  // } else {
    // for (int i = 0; i < n; i++) {
      //   printf("%s\n", entries[i]->d_name);
      // }
  // }

  if (closedir(d) != 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("Dir descriptor %d", dir_fd));
    #endif
  };
#else
  char buf[PATH_MAX_LEN];
  snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

  // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(buf, &data);
  if (h == INVALID_HANDLE_VALUE) {
    #ifdef STC_LOG_ERR
    fs_err_print(dirpath);
    #endif
    return entries;
  }

  do {
    char* filename = data.cFileName;
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

    entry.name = strdup(filename);
    entry.parent = path_to_absolute(entry.name);
    switch (data.dwFileAttributes) {
      case FILE_ATTRIBUTE_DIRECTORY: entry.type = FileType_Dir; break;
      default: entry.type = FileType_File; break;
    }

    DirEntries_push(&entries, entry);
  } while(FindNextFile(h, &data) != 0);
  
  if (GetLastError() != ERROR_NO_MORE_FILES) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("Dir handle %p", h));
    #endif
  }

  if (FindClose(h) == 0) {
    #ifdef STC_LOG_ERR
    fs_err_print(str_fmt_tmp("Dir handle %p", h));
    #endif
  }
#endif

  return entries;
}

/////////////////////

bool file_exists(char* path) {
#ifndef _WIN32
  struct stat buf;
  return stat(path, &buf) == 0 && S_ISREG(buf.st_mode);
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesa?redirectedfrom=MSDN
  return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
#endif
}


bool dir_exists(char* path) {
#ifndef _WIN32
  struct stat buf;
  return stat(path, &buf) == 0 && S_ISDIR(buf.st_mode);
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesa?redirectedfrom=MSDN
  DWORD attribs = GetFileAttributes(path);
  return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}

FileType file_type(char* path) {
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

bool dir_set_current(char* path) {
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

bool dir_create_if_not_exists(char* path) {
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

bool dir_create_recursive(char* path) {
  fs_tmp_sb.len = 0;
  StrList components = path_components(path);

  String_append_str(&fs_tmp_sb, path_prefix(path));

  bool had_error = false;
  listforeach(str, component, &components) {
    String_append_str(&fs_tmp_sb, *component);
    String_push(&fs_tmp_sb, '/');
    String_append_null(&fs_tmp_sb);
    
    if (!dir_create_if_not_exists(fs_tmp_sb.data)) {
      had_error = true;
      break;
    }
  }

  StrList_free(&components);
  return !had_error;
}

// TODO
void dir_walk(char* dirpath) {
  DirRead it = dir_open(dirpath);
  DirEntry e = dir_read(&it);
  
  String sb = {0};
  String_append_cstr(&sb, dirpath);
  str_dbg(sb);

  while (e.name != NULL) {
    switch (e.type) {
      case FileType_Dir: {
        printf("Reading %s\n", e.name);
        String_append_null(&sb);
        str_dbg(sb);
        String_fmt(&sb, "hello %s %s %s", sb.data, e.name, sb.data);
        str_dbg(sb);
        printf("%s\n", e.name);
        // dir_walk(sb.data); 
      } break;
      default: printf("File: %s/%s\n", sb.data, e.name); break;
    }

    e = dir_read(&it);
  }

  dir_close(&it);
}

// TODO:
// ?? nob_file_metadata(const char *path, bool follow_links);
// bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
// bool nob_rmdir_if_exists_recursive(const char *path);
// bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children);

bool dir_delete_if_exists(char* path) {
  if (!dir_exists(path)) return true;
  
  #ifndef _WIN32
    bool res = rmdir(path) == 0;
  #else
    bool res = RemoveDirectory(path) != 0;
  #endif

  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(path);
  #endif

  return res;
}

bool dir_delete_contents(char* path) {
  // TODO
  // https://stackoverflow.com/questions/5467725/how-to-delete-a-directory-and-its-contents-in-posix-c
}

bool dir_delete_recursive(char* path) {
  fs_tmp_sb.len = 0;
  String_append_str(&fs_tmp_sb, str_from_cstr(path));
  String_append_null(&fs_tmp_sb);

  bool had_error = false;
  while (fs_tmp_sb.len > 0) {
    char* dir = path_last_component(fs_tmp_sb.data);
    
    if (!dir_delete_contents(dir)) {
      had_error = true;
      break;
    }

    path_pop(&fs_tmp_sb);
    String_append_null(&fs_tmp_sb);
  }

  return !had_error;
}

// TODO: creations options and permissions?
bool file_create_if_not_exists(char* path) {
  if (file_exists(path)) return false;

#ifndef _WIN32
  // https://man7.org/linux/man-pages/man2/open.2.html
  int fd = creat(path, S_IRWXU | S_IRWXG | S_IRWXO);
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
  HANDLE h = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
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

bool file_create_recursive(char* path) {
  str parent = path_parent(path);
  fs_tmp_sb.len = 0;
  String_append_str(&fs_tmp_sb, parent);
  String_append_null(&fs_tmp_sb);

  return dir_create_recursive(fs_tmp_sb.data) 
    && file_create_if_not_exists(path);
}

// TODO: add flag to choose if should overwrite
bool file_copy(char* src, char* dst) {
#ifndef _WIN32
  int src_fd = open(src, O_RDONLY, 0);

  if (src_fd == -1) {
    #ifdef STC_LOG_ERR
    fs_err_print(src);
    #endif
    return false;
  }

  // bool overwrite = false;
  // int flags = 0;
  // if (overwrite) {
  //   flags = O_TRUNCATE:
  // }

  int dst_fd = open(dst, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

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
  BOOL res = CopyFile(src, dst, false);
  #ifdef STC_LOG_ERR
  if (!res) fs_err_print(dst);
  #endif
  return res;
#endif
}

bool dir_copy_recursive(char* src, char* dst) {
  // TODO
}

#endif