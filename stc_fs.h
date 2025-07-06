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

// TODO: better error handling
// IDEA: do not print any error; introduce win_errno for Windows-specific errors
// TODO: if the user wants errors to be printed, it should #define a symbol for it
// TODO: should if_exists/not_exists function check for existance?

static FILE* file_open(char* path, const char* opts) {
  FILE* f = fopen(path, opts);
  if (f == NULL) {
    perror("Couldn't open file");
    return NULL;
  }
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
  if (fclose(f) != 0) {
    perror("Couldn't close file");
    return false;
  }
  return true;
}

// TODO: consider memory mapped file reading
// https://stackoverflow.com/questions/10836609/fastest-technique-to-read-a-file-into-memory/10836820#10836820
// https://stackoverflow.com/questions/3002122/fastest-file-reading-in-c
char* file_read_to_string(char* path) {
  FILE* f = file_open_read(path);

  if (fseek(f, 0, SEEK_END) != 0) {
    perror("Couldn't seek end of file");
    file_close(f);
    return NULL;
  }

  size_t file_size = ftell(f);
  if (file_size < 0) {
    perror("Could't tell size of file");
    file_close(f);
    return NULL;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    perror("Couldn't seek start of file");
    file_close(f);
    return NULL;
  }

  char* buf = malloc(file_size);
  size_t read = fread(buf, 1, file_size, f);
  if (read != file_size) {
    perror("Couldn't read whole file");
  }

  file_close(f);

  return buf;
}

bool file_write_bytes(char* path, char* data, size_t len) {
  FILE* fd = file_open_write(path);
  size_t wrote = fwrite(data, 1, len, fd);
  bool res = wrote != len;
  if (!res) {
    perror("Couldn't write whole file");
  }

  return file_close(fd) && res;
}

bool file_append_bytes(FILE* fd, char* data, size_t len) {
  size_t wrote = fwrite(data, 1, len, fd);
  if (wrote != len) {
    perror("Couldn't append all data");
    return false;
  }

  return true;
}

bool file_delete(char* src) {
  int res = remove(src) == 0;
  if (!res) perror("Couldn't remove file");
  return res;
}

bool file_move(char* src, char* dst) {
  int res = rename(src, dst) == 0;
  if (!res) perror("Couldn't rename file");
  return res;
}

#ifdef _WIN32
static void perror_windows(const char* msg) {
  DWORD err = GetLastError();
  static char buf[512];

  DWORD len = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    err,
    LANG_SYSTEM_DEFAULT,
    buf, 512,
    NULL
  );

  if (len == 0) {
    fprintf(stderr, "%s: couldn't get error message from Windows for error ID %ld\n", msg, err);
  } else {
    while (len > 0 && isspace(buf[len-1])) buf[len--] = '\0';
    fprintf(stderr, "%s: %s\n", msg, buf);
  }
}
#endif

/////////////////////

// TODO: consider making path a sperate type
// TODO: should we check for null?
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
  if (res == NULL) return res;
  return res + 1;
}

str path_filename_no_ext(char* path) {
  str s = str_from_cstr(path_filename(path));
  return str_skip_rev_until_char(s, '.');
}

str path_parent(char* path) {
  char* parent_start = path_last_component(path);
  // there are no parents
  if (parent_start == NULL || parent_start == path) return STR_EMPTY;
  return str_from_cstr_unchecked(path, parent_start - path);
}

char* path_absolute(char* path) {
#ifndef _WIN32
  // https://man7.org/linux/man-pages/man3/realpath.3.html
  // TODO: should path be prefixed with "./" for realpath to work?
  char* res = realpath(path, NULL);
  if (res == NULL) perror("Couldn't get absolute path");
  return res;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
  DWORD path_size = GetFullPathName(path, 0, NULL, NULL);
  if (path_size == 0) {
    perror_windows("Couldn't calculate absolute path length");
    return NULL;
  }
  char* buf = malloc(path_size);
  DWORD res = GetFullPathName(path, path_size, buf, NULL);
  if (res == 0) {
    perror_windows("Couldn't get absolute path");
    return NULL;
  }
  return buf;
#endif
}

str path_prefix(char* path) {
  if (path[0] == '/') return str_from_cstr("/");
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
  return str_split_when(str_from_cstr(path), isseparator);
}

// https://doc.rust-lang.org/std/path/struct.PathBuf.html

void path_push(String* sb, char* path) {
  if (path_is_absolute(path)) sb->len = 0;
  else if (String_last(*sb) != '/') String_push(sb, '/');
  String_append_cstr(sb, path);
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

list_def(DirEntry, DirEntries)

void DirEntries_free(DirEntries* entries) {
  listforeach(DirEntry, e, entries) {
    free(e->name);
  }
  free(entries->data);
}

FileType file_type(char* path) {
#ifndef _WIN32
  struct stat buf;
  if (stat(path, &buf) != 0) {
    perror("Couldn't open file");
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
    perror_windows("Couldn't open file");
    return FileType_Other;
  }

  switch (attribs) {
    case FILE_ATTRIBUTE_DIRECTORY: return FileType_Dir;
    default: return FileType_File;
  }
#endif
}

// TODO: consider making this an iterator instead, together with a dir_open and a dir_close.
DirEntries dir_read(char* dirpath) {
  DirEntries entries = {0};
  DirEntry entry = {0};
  entry.parent = dirpath;

  // TODO: fetch full path as parent

#ifndef _WIN32
  int dir_fd = open(dirpath, O_RDONLY, O_DIRECTORY);
  DIR* d = fdopendir(dir_fd);

  if (d == NULL) {
    perror("Couldn't open directory for reading entries");
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
      perror("Couldn't read file during directory scan");
      continue;
    }

    entry.name = dp->d_name;
    switch (statbuf.st_mode & S_IFMT) {
      case S_IFREG: entry.type = FileType_File; break;
      case S_IFDIR: entry.type = FileType_Dir; break;
      case S_IFLNK: entry.type = FileType_Link; break;
      default: entry.type = FileType_Other; break;
    }

    DirEntries_push(&entries, entry);
    if (close(fd) != 0) {
      perror("Coudln't close file during directory scan");
      continue;
    };
  }

  if (prev_err != errno) {
    perror("Couldn't read all directory entries");
  }

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
    perror("Couldn't close directory after completed scan");
  };
#else
  char buf[PATH_MAX_LEN];
  snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

  // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(buf, &data);
  if (h == INVALID_HANDLE_VALUE) {
    perror_windows("Couldn't open directory for reading entries");
    return entries;
  }

  do {
    char* filename = data.cFileName;
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

    entry.name = strdup(filename);
    switch (data.dwFileAttributes) {
      case FILE_ATTRIBUTE_DIRECTORY: entry.type = FileType_Dir; break;
      default: entry.type = FileType_File; break;
    }

    DirEntries_push(&entries, entry);
  } while(FindNextFile(h, &data) != 0);
  
  if (GetLastError() != ERROR_NO_MORE_FILES) {
    perror_windows("Couldn't read all directory entries");
  }

  FindClose(h);
#endif

  return entries;
}

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

static char cwd[PATH_MAX_LEN];
char* dir_current() {
#ifndef _WIN32
  char* res = getcwd(cwd, PATH_MAX_LEN);
  if (res == NULL) perror("Couldn't get working directory");
  return res;
#else
  if (GetCurrentDirectory(PATH_MAX_LEN, cwd) == 0) perror_windows("Couldn't get working directory");
  return cwd;
#endif
}

bool dir_create_if_not_exists(char* path) {
  if (dir_exists(path)) return true;
  
#ifndef _WIN32
  bool res = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
  if (!res) perror("Couldn't create directory");
  return res;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya?redirectedfrom=MSDN
  bool res = CreateDirectory(path, NULL) != 0;
  if (!res) perror_windows("Couldn't create directory");
  return res;
#endif
}


static String tmp_sb = {0};
bool dir_create_recursive(char* path) {
  tmp_sb.len = 0;
  StrList components = path_components(path);

  String_append_str(&tmp_sb, path_prefix(path));

  bool had_error = false;
  listforeach(str, component, &components) {
    String_append_str(&tmp_sb, *component);
    String_push(&tmp_sb, '/');
    String_append_null(&tmp_sb);
    
    if (!dir_create_if_not_exists(tmp_sb.data)) {
      had_error = true;
      break;
    }
  }

  StrList_drop(&components);
  return !had_error;
}

// ?? nob_file_metadata(const char *path, bool follow_links);
// bool nob_mkdir_if_not_exists_recursive(const char *path);
// bool nob_rmdir_if_exists_recursive(const char *path);
// bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
// bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children);

bool dir_delete_if_exists(char* path) {
  if (!dir_exists(path)) return true;
  
  #ifndef _WIN32
    bool res = rmdir(path) == 0;
    if (!res) perror("Couldn't remove directory");
    return res;
  #else
    bool res = RemoveDirectory(path) != 0;
    if (!res) perror_windows("Couldn't remove directory");
    return res;
  #endif
}

bool dir_delete_contents(char* path) {
  // TODO
  // https://stackoverflow.com/questions/5467725/how-to-delete-a-directory-and-its-contents-in-posix-c
}

bool dir_delete_recursive(char* path) {
  tmp_sb.len = 0;
  String_append_str(&tmp_sb, str_from_cstr(path));
  String_append_null(&tmp_sb);

  bool had_error = false;
  while (tmp_sb.len > 0) {
    char* dir = path_last_component(tmp_sb.data);
    
    if (!dir_delete_contents(dir)) {
      had_error = true;
      break;
    }

    path_pop(&tmp_sb);
    String_append_null(&tmp_sb);
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
    perror("Couldn't create file");
    return false;
  }
  if (close(fd) != 0) {
    perror("Couldn't close created file");
    return false;
  }
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/fileapi/nf-fileapi-createfilea
  printf("Creating file: %s\n", path);
  HANDLE h = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    perror_windows("Couldn't create file");
    return false;
  }
  if (CloseHandle(h) == 0) {
    perror_windows("Couldn't close created file");
    return false;
  }
#endif

  return true;
}

bool file_create_recursive(char* path) {
  str parent = path_parent(path);
  char* parent_cstr = str_to_cstr(parent);
  bool dir_res = dir_create_recursive(parent_cstr);
  free(parent_cstr);
  return dir_res && file_create_if_not_exists(path);
}

// TODO: add flag to choose if should overwrite
bool file_copy(char* src, char* dst) {
#ifndef _WIN32
  int src_fd = open(src, O_RDONLY, 0);

  // bool overwrite = false;
  // int flags = 0;
  // if (overwrite) {
  //   flags = O_TRUNCATE:
  // }

  int dst_fd = open(dst, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

  if (src_fd == -1 || dst_fd == -1) {
    perror("Couldn't open either src or dst");
    return false;
  }

  struct stat statbuf;
  if (fstat(src_fd, &statbuf) != 0) {
    perror("Couldn't get src file size");
    return false;
  }
  size_t src_size = statbuf.st_size;

  // https://man7.org/linux/man-pages/man2/copy_file_range.2.html
  // https://man7.org/linux/man-pages/man2/sendfile.2.html
  // if (sendfile(dst_fd, src_fd, NULL, src_size) == -1) {
  if (copy_file_range(src_fd, NULL, dst_fd, NULL, src_size, 0) == -1) {
    perror("Couldn't copy src file to dst file");
    return false;
  }

  if (close(src_fd) != 0) {
    perror("Coudlnt close src file");
    return false;
  }

  if (close(dst_fd) != 0) {
    perror("Coudlnt close dst file");
    return false;
  }

  return true;
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/winbase/nf-winbase-copyfile
  BOOL res = CopyFile(src, dst, false);
  if (!res) perror_windows("Couldn't copy src file to dst file");
  return res;
#endif
}

#endif