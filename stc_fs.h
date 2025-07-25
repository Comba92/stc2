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
  
  #include <sys/stat.h>
  #define fileno _fileno
  #define stat _stati64
  #define fstat _fstati64
  #define ftell _ftelli64
  #define fseek _fseeki64
#endif

#ifdef STC_LOG_ERR
#define LOG_ERR(cond, msg) if ((cond)) fs_err_print((msg));
#else
#define LOG_ERR(...)
#endif

// TODO: maybe returning bools is not a great idea, returning 0 on success might be better
// TODO: functions versions which take descriptor/handle instead of path

#ifndef _WIN32
typedef int FileDescriptor;
#define INVALID_FD -1 
#else
typedef HANDLE FileDescriptor;
#define INVALID_FD INVALID_HANDLE_VALUE
#endif

static __thread String fs_tmp_sb1 = {0};
static __thread String fs_tmp_sb2 = {0};

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
  static __thread char buf[512];
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
  LOG_ERR(f == NULL, path)
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
  bool res = fclose(f) == 0;
  LOG_ERR(!res, str_fmt_tmp("File descriptor %d", fileno(f)))
  return res;
}

// TODO: consider memory mapped file reading?
// https://stackoverflow.com/questions/10836609/fastest-technique-to-read-a-file-into-memory/10836820#10836820
// https://stackoverflow.com/questions/3002122/fastest-file-reading-in-c

bool file_read_to_string(String* sb, const char* path) {
  FILE* f = file_open_read(path);
  if (f == NULL) { return false; }

  // ftell is broken on linux for directories, also we are iterating the file back and forth
  // if (fseek(f, 0, SEEK_END) != 0) {
  //   file_close(f);
  //   return false;
  // }
  // isize file_size = ftell(f);
  // // for some reason, this has to be done on linux when opening folders
  // if (file_size < 0 || (int) file_size == -1) {
  //   file_close(f);
  //   return false;
  // }
  // if (fseek(f, 0, SEEK_SET) != 0) {
  //   file_close(f);
  //   return false;
  // }

  struct stat buf;
  if (fstat(fileno(f), &buf) != 0) {
    LOG_ERR(true, path)
    file_close(f);
    return false;
  }

  usize size = buf.st_size;
  String_reserve(sb, size);
  isize read = fread(sb->data, 1, size, f);
  sb->len = read;

  LOG_ERR(read != size, path)

  file_close(f);
  return read == size;
}

bool file_write_bytes(const char* path, const char* data, isize len) {
  FILE* fd = file_open_write(path);
  isize wrote = fwrite(data, 1, len, fd);
  bool res = wrote != len;

  LOG_ERR(!res, path)

  return file_close(fd) && res;
}

/////////////////////

// TODO: maybe path should be its own type
// passing char* will lose length information,
// and will do unnecessary str_from_cstr() to scan for null

// https://doc.rust-lang.org/std/path/struct.Path.html

// this is doing unnecessary str_from_cstr()
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

// this only works if path is a correct relative path; no check are done to see if absolute path is valid
char* path_to_absolute(String* sb, const char* path) {
  sb->len = 0;
#ifndef _WIN32
  String_reserve(sb, PATH_MAX_LEN);

  // https://man7.org/linux/man-pages/man3/realpath.3.html
  char* res = realpath(path, sb->data);
  if (res == NULL) {
    LOG_ERR(true, str_fmt_tmp("Failed to get absolute path for relative path %s", path))
    return sb->data;
  }

  sb->len = strlen(sb->data);
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
  DWORD path_len = GetFullPathName(path, 0, NULL, NULL);
  if (path_len == 0) {
    LOG_ERR(true, str_fmt_tmp("Failed to get absolute path for relative path %s", path))
    return sb->data;
  }

  String_reserve(sb, path_len);
  DWORD res = GetFullPathName(path, path_len, sb->data, NULL);
  if (res == 0) {
    LOG_ERR(true, str_fmt_tmp("Failed to get absolute path for relative path %s", path))
    return sb->data;
  }

  /* If the lpBuffer buffer is too small to contain the path, the return value is the size, in TCHARs, of the buffer that is required to hold the path and the terminating null character. */
  sb->len = path_len-1;
#endif
  return sb->data;
}

str path_prefix(const char* path) {
  if (path[0] == '/') return str_from_cstr_unchecked("/", 1);
  else if (c_is_alpha(path[0]) && path[1] == ':' && path[2] == '\\') {
    return str_from_cstr_unchecked(path, 3);
  }
  // TODO: windows' other weird prefixes aren't supported

  return STR_EMPTY;
}

bool path_is_absolute(const char * path) {
  isize len = strlen(path);

  if (len < 1) return false;
  bool is_unix_abs = path[0] == '/';
  if (len < 3) return is_unix_abs;
  bool is_wind_abs = 
  (c_is_alpha(path[0]) && path[1] == ':' && path[2] == '\\')
  || (path[0] == '\\' && path[1] == '\\');

  return is_unix_abs || is_wind_abs;
}

bool path_is_relative(const char* path) {
  return !path_is_absolute(path);
}

bool c_is_separator(char c) { return c == '/' || c == '\\'; }
StrList path_components(const char* path) {
  return str_split_when_collect(str_from_cstr(path), c_is_separator);
}

typedef StrSplitWhen PathComponents ;
PathComponents path_components_iter(const char* path) {
  return str_split_when(str_from_cstr(path), c_is_separator);
}
bool path_has_component(const PathComponents* it) {
  return str_has_split_when(it);
}
str path_next_component(PathComponents* it) {
  return str_next_split_when(it);
}

// https://doc.rust-lang.org/std/path/struct.PathBuf.html

void path_push(String* sb, const char* path) {
  if (path_is_absolute(path)) sb->len = 0;
  else if ((sb->len > 0 && *String_last(*sb) != '/') && path[0] != '/') String_push(sb, '/');
  String_append_cstr(sb, path);
}

bool path_pop(String* sb) {
  // we append null so we can use path_last_component
  String_append_null(sb);
  char* last_component_start = path_last_component(sb->data);

  if (last_component_start == NULL) return false;
  sb->len = last_component_start - sb->data;
  // append null again, to truncate the string
  String_append_null(sb);

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


char* dir_current(String* sb) {
#ifndef _WIN32
  sb->len = 0;
  // PATH_MAX_LEN is usally smaller in Linux, 4096
  String_reserve(sb, PATH_MAX_LEN+1);

  // https://man7.org/linux/man-pages/man3/getcwd.3.html
  char* res = getcwd(sb->data, PATH_MAX_LEN);
  if (res == NULL) { 
    LOG_ERR(true, str_fmt_tmp("Failed to get working directory"))
    String_append_cstr(sb, "");
    return sb->data;
  }

  sb->len = strlen(sb->data);
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
  isize path_len = GetCurrentDirectory(0, NULL);

  if (path_len == 0) {
    LOG_ERR(true, str_fmt_tmp("Failed to get working directory"))
    return sb->data;
  }
  
  sb->len = 0;
  String_reserve(sb, path_len);
  if (GetCurrentDirectory(path_len, sb->data) == 0) {
    LOG_ERR(true, str_fmt_tmp("Failed to get working directory"))
    String_append_cstr(sb, "");
    return sb->data;
  }

  /* 
    If the buffer that is pointed to by lpBuffer is not large enough, the return value specifies the required size of the buffer, in characters, including the null-terminating character.
  */
  sb->len = path_len-1;
#endif
  return sb->data;
}

bool dir_set_current(const char* path) {
#ifndef _WIN32
  bool res = chdir(path) == 0;
#else
  bool res = SetCurrentDirectory(path) != 0;
#endif
  LOG_ERR(!res, str_fmt_tmp("Failed to set working directory"))
  return res;
}

/////////////////////

typedef enum {
  FileType_Other,
  FileType_File,
  FileType_Dir,
  // TODO: symlinks are not yet supported anywhere
  FileType_Link,
} FileType;

// TODO: created, accessed, modified? https://doc.rust-lang.org/std/fs/struct.Metadata.html
typedef struct {
  char* name;
  isize size;
  FileType type;
  bool readonly;
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
  isize err_count;
  DirEntry curr;
} DirIter;

bool dir_scanning(const DirIter* it) {
  return !it->finished;
}

DirIter dir_open(const char* dirpath) {
#ifndef _WIN32
  int dir_fd = open(dirpath, O_RDONLY, O_DIRECTORY);
  LOG_ERR(dir_fd == -1, dirpath)

  // https://man7.org/linux/man-pages/man3/fdopendir.3p.html
  DIR* dir = fdopendir(dir_fd);
  LOG_ERR(dir == NULL, dirpath)

  bool had_error = dir_fd == -1 || dir == NULL;
  DirIter it = { dir_fd, dir, had_error, had_error ? 1 : 0 };
#else
  static char buf[PATH_MAX_LEN];
  snprintf(buf, PATH_MAX_LEN, "%s\\*", dirpath);

  // https://learn.microsoft.com/it-it/windows/win32/fileio/listing-the-files-in-a-directory
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilea
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfileexa
  WIN32_FIND_DATA data;
  // HANDLE h = FindFirstFile(buf, &data);
  // this should be faster than FindFirstFile()
  HANDLE h = FindFirstFileEx(buf, FindExInfoBasic, &data, FindExSearchNameMatch, NULL, 0);
  LOG_ERR(h == INVALID_HANDLE_VALUE, dirpath)

  bool had_error = h == INVALID_HANDLE_VALUE;
  DirIter it = { data, h, had_error, had_error ? 1 : 0 };
#endif
  return it;
}

bool dir_close(DirIter* it) {
#ifndef _WIN32
  bool res = closedir(it->dir) == 0;
  LOG_ERR(!res, str_fmt_tmp("Closing dir descriptor %d", it->dir_fd))
#else
  bool res = FindClose(it->h) != 0;
  LOG_ERR(!res, str_fmt_tmp("Closing dir handle %p", it->h))
#endif
  it->finished = true;
  return res;
}


DirEntry* dir_read(DirIter* it) {
#ifndef _WIN32
  #ifdef STC_LOG_ERR
  int prev_err = errno;
  #endif
  struct dirent* dp;
  char* filename;

  // readdir until a valid file is returned
  bool is_special_entry = false;
  do {
    // https://man7.org/linux/man-pages/man3/readdir.3.html
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
    LOG_ERR(true, str_fmt_tmp(filename))

    it->err_count += 1;
    it->curr.name = "";
    it->curr.size = -1;
    it->curr.type = FileType_Other;
    return &it->curr;
  }

  it->curr.name = dp->d_name;
  it->curr.size = statbuf.st_size;
  bool can_read = statbuf.st_mode & S_IRUSR ||
    statbuf.st_mode & S_IRGRP ||
    statbuf.st_mode & S_IROTH;
  bool can_write = statbuf.st_mode & S_IWUSR ||
    statbuf.st_mode & S_IWUGRP ||
    statbuf.st_mode & S_IWOTH;
  it->curr.readonly = can_read && !can_write;

  switch (statbuf.st_mode & S_IFMT) {
    case S_IFREG: it->curr.type = FileType_File; break;
    case S_IFDIR: it->curr.type = FileType_Dir; break;
    case S_IFLNK: it->curr.type = FileType_Link; break;
    default: it->curr.type = FileType_Other; break;
  }

  close(fd);
#else
  char* filename;
  bool is_special_entry = false;

  // FindNextFile until a valid file is returned
  do {
    bool res = FindNextFile(it->h, &it->data) != 0;

    // no more files left
    if (!res) {
      LOG_ERR(GetLastError() != ERROR_NO_MORE_FILES, str_fmt_tmp("Dir handle %p", it->h))

      dir_close(it);
      return NULL;
    }

    filename = it->data.cFileName;
    is_special_entry = strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0;
  } while (is_special_entry);

  // get file data
  it->curr.name = filename;
  it->curr.size = (((isize) it->data.nFileSizeHigh) << 32) | it->data.nFileSizeLow;
  it->curr.readonly = it->data.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
  
  if (it->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    it->curr.type = FileType_Dir;
  } else {
    it->curr.type = FileType_File
  }
#endif
  return &it->curr;
}

#define dir_iter(ent, it) for(DirEntry* ent; (ent = dir_read((it))) != NULL;)

list_def(DirEntry, DirEntries)
void DirEntries_drop(DirEntries* entries) {
  if (entries->data == NULL) return;
  listforeach(DirEntry, e, entries) free(e->name);
  free(entries->data);
}

DirEntries dir_entries(const char* dirpath) {
  DirEntries entries = {0};
  DirIter it = dir_open(dirpath);

  for (DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    DirEntry to_push = *entry;
    to_push.name = strdup(entry->name);
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

// TODO: file_metadata()

FileType file_type(const char* path) {
#ifndef _WIN32
  struct stat buf;
  if (stat(path, &buf) != 0) {
    LOG_ERR(true, path)
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
    LOG_ERR(true, path)
    return FileType_Other;
  }

  if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
    return FileType_Dir;
  } else {
    return FileType_File;
  }
#endif
}

isize file_size(const char* path) {
  struct stat buf;
  if (stat(path, &buf) != 0) {
    LOG_ERR(true, path)
    return -1;
  }

  return buf.st_size;
}

bool dir_create(const char* path) {
#ifndef _WIN32
  // https://man7.org/linux/man-pages/man2/mkdir.2.html
  bool res = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya?redirectedfrom=MSDN
  bool res = CreateDirectory(path, NULL) != 0;
#endif
  LOG_ERR(!res, path)
  return res;
}

bool dir_create_recursive(const char* path) {
  StrList components = path_components(path);
  listforeach(str, component, &components) {
    printf("\tComponent: " str_fmt "\n", str_arg(*component));
  }

  fs_tmp_sb1.len = 0;
  String_append_str(&fs_tmp_sb1, path_prefix(path));

  bool had_error = false;
  listforeach(str, component, &components) {
    had_error = false;
    String_append_str(&fs_tmp_sb1, *component);
    String_push(&fs_tmp_sb1, '/');
    String_append_null(&fs_tmp_sb1);
    
    printf("\tCreating %s\n", fs_tmp_sb1.data);
    if (!dir_create(fs_tmp_sb1.data)) had_error = true;
  }

  StrList_free(&components);
  return !had_error;
}

bool file_create(const char* path, bool overwrite) {
#ifndef _WIN32
  int flags = overwrite ? O_TRUNC : 0;
  // https://man7.org/linux/man-pages/man2/open.2.html
  int fd = open(path, O_WRONLY | O_CREAT | flags, S_IRWXU | S_IRWXG | S_IRWXO);
  // int fd = creat(path, S_IRWXU | S_IRWXG | S_IRWXO);

  if (fd == -1 || close(fd) != 0) {
    LOG_ERR(true, path)
    return false;
  }
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/fileapi/nf-fileapi-createfilea
  DWORD flags = overwrite ? CREATE_ALWAYS : CREATE_NEW;
  HANDLE h = CreateFile(path, GENERIC_WRITE, 0, NULL, flags, FILE_ATTRIBUTE_NORMAL, NULL);

  if (h == INVALID_HANDLE_VALUE || CloseHandle(h) == 0) {
    LOG_ERR(true, path)
    return false;
  }
#endif

  return true;
}

bool file_create_recursive(const char* path, bool overwrite) {
  str parent = path_parent(path);
  // use fs_tmp_sb2 here, as dir_create_recursive() uses fs_tmp_sb1
  fs_tmp_sb2.len = 0;
  String_append_str(&fs_tmp_sb2, parent);
  String_append_null(&fs_tmp_sb2);

  return dir_create_recursive(fs_tmp_sb2.data)
    && file_create(path, overwrite);
}

bool file_copy(const char* src, const char* dst, bool overwrite) {
#ifndef _WIN32
  int src_fd = open(src, O_RDONLY, 0);

  struct stat statbuf;
  if (fstat(src_fd, &statbuf) != 0) {
    LOG_ERR(true, src)
    return false;
  }
  isize src_size = statbuf.st_size;

  int flags = overwrite ? O_TRUNC : 0;
  int dst_fd = open(dst, O_WRONLY | O_CREAT | flags, S_IRWXU | S_IRWXG | S_IRWXO);

  // TODO: both copy_file_range and sendfile are not always avaible on unix
  // might want to fallback to fread and fwrite
  // macOS has flconefileat and fcopyfile

  // https://man7.org/linux/man-pages/man2/copy_file_range.2.html
  // https://man7.org/linux/man-pages/man2/sendfile.2.html
  if (sendfile(dst_fd, src_fd, NULL, src_size) == -1) {
  // if (copy_file_range(src_fd, NULL, dst_fd, NULL, src_size, 0) == -1) {
    LOG_ERR(true, dst)
    return false;
  }

  if (close(src_fd) != 0) {
    LOG_ERR(true, src)
    return false;
  }

  if (close(dst_fd) != 0) {
    LOG_ERR(true, dst)
    return false;
  }

  return true;
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/winbase/nf-winbase-copyfile
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfileexa
  BOOL res = CopyFile(src, dst, !overwrite);
  LOG_ERR(!res, dst)
  return res;
#endif
}

bool file_move(const char* src, const char* dst, bool overwrite) {
#ifndef _WIN32
  if (!overwrite && path_exists(dst)) return false;

  int res = rename(src, dst) == 0;
  LOG_ERR(!res, src)
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefile
  // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexa
  DWORD flags = overwrite ? MOVEFILE_REPLACE_EXISTING : 0;
  bool res = MoveFileEx(src, dst, MOVEFILE_COPY_ALLOWED | flags) != 0;
  LOG_ERR(!res, src)
#endif

  return res;
}

bool file_delete(const char* src) {
  int res = remove(src) == 0;
  LOG_ERR(!res, src)
  return res;
}

static bool dir_copy_recursive_internal(String* src_sb, String* dst_sb, bool overwrite) {
  DirIter it = dir_open(src_sb->data);
  if (it.finished) return false;

  /*
    https://man7.org/linux/man-pages/man3/readdir.3.html
    The data returned by readdir() may be overwritten by subsequent
    calls to readdir() for the same directory stream.

    This means we don't need to collect the entries!
  */

  bool had_error = false;
  for(DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    path_push(src_sb, entry->name);
    path_push(dst_sb, entry->name);

    switch (entry->type) {
      case FileType_File: {
        had_error |= !file_copy(src_sb->data, dst_sb->data, overwrite);
      } break;

      case FileType_Dir: {
        had_error |= !dir_create(dst_sb->data);
        had_error |= !dir_copy_recursive_internal(src_sb, dst_sb, overwrite);
      } break;

      default: break;
    }

    path_pop(src_sb);
    path_pop(dst_sb);
  }

  return !had_error;
}

bool dir_copy_recursive(const char* src, const char* dst, bool overwrite) {
  // be sure dst exists
  dir_create_recursive(dst);

  fs_tmp_sb1.len = 0;
  String_append_cstr(&fs_tmp_sb1, src);
  fs_tmp_sb2.len = 0;
  String_append_cstr(&fs_tmp_sb2, dst);

  return dir_copy_recursive_internal(&fs_tmp_sb1, &fs_tmp_sb2, overwrite);
}

bool dir_move(const char* src, const char* dst, bool overwrite) {
  return file_move(src, dst, overwrite);
}

bool dir_delete(const char* path) {
  if (!dir_exists(path)) return true;
  
#ifndef _WIN32
  bool res = rmdir(path) == 0;
#else
  // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-removedirectorya
  bool res = RemoveDirectory(path) != 0;
#endif
  LOG_ERR(!res, path)
  return res;
}

static bool dir_delete_recursive_internal(String* path_sb) { 
  DirIter it = dir_open(path_sb->data);
  if (it.finished) return false;

  bool had_error = false;
  for(DirEntry* entry; (entry = dir_read(&it)) != NULL;) {
    path_push(path_sb, entry->name);

    switch (entry->type) {
      case FileType_File: {
        had_error |= !file_delete(path_sb->data);
      } break;
      case FileType_Dir: {
        had_error |= !dir_delete_recursive_internal(path_sb);
        had_error |= !dir_delete(path_sb->data);
      } break;

      default: break;
    }

    path_pop(path_sb);
  }

  had_error |= !dir_delete(path_sb->data);
  return !had_error;
}

bool dir_delete_recursive(const char* path) {
  if (path_is_absolute(path)) {
    fprintf(stderr, "I AM NOT SURE YOU WANT TO DELETE AN ABSOLUTE DIRECTORY, ABORTING\n");
    return false;
  }

  fs_tmp_sb1.len = 0;
  String_append_cstr(&fs_tmp_sb1, path);
  return dir_delete_recursive_internal(&fs_tmp_sb1);
}

bool fs_copy(const char* src, const char* dst, bool overwrite) {
  FileType type = file_type(src);
  switch(type) {
    case FileType_File: return file_copy(src, dst, overwrite);
    case FileType_Dir: return dir_copy_recursive(src, dst, overwrite);
    default: return false;
  }
}

bool fs_move(const char* src, const char* dst, bool overwrite) {
  return file_move(src, dst, overwrite);
}

bool fs_delete(const char* path) {
  FileType type = file_type(path);
  switch (type) {
    case FileType_File: return file_delete(path);
    case FileType_Dir: return dir_delete_recursive(path);
    default: return false;
  }
}

#endif