#ifndef STC_FS_IMPL
#define STC_FS_IMPL

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "stc_list.h"

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
    return NULL;
  }

  long file_size = ftell(f);
  if (file_size < 0) {
    perror("Could't tell size of file");
    return NULL;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    perror("Couldn't seek start of file");
    return NULL;
  }

  char* buf = malloc(file_size);
  int read = fread(buf, 1, file_size, f);
  if (read != file_size) {
    perror("Couldn't read whole file");
  }

  file_close(f);

  return buf;
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

// TODO: more path functions
// https://doc.rust-lang.org/std/path/struct.Path.html
char* path_filename(char* path) {
  char* unix_path = strrchr(path, '/');
  char* wind_path = strrchr(path, '\\');
  if (unix_path == NULL && wind_path == NULL) return path;
  
  char* res = unix_path > wind_path ? unix_path : wind_path;
  return res + 1;
}

char* path_filename_ext(char* path) {
  return strrchr(path, '.') + 1;
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
    close(fd);
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

  closedir(d);
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
  DWORD attribs = GetFileAttributes(path);
  return attribs != INVALID_FILE_ATTRIBUTES;
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

bool dir_make_if_not_exists(char* path) {
  // TODO: is dir_exists check redundant?
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


// ?? nob_file_metadata(const char *path, bool follow_links);
// bool nob_mkdir_if_not_exists_recursive(const char *path);
// bool nob_rmdir_if_exists_recursive(const char *path);
// bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
// bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children);
// bool nob_write_file(const char *path, const void *data, size_t size);
// bool nob_write_entire_file(const char *path, const void *data, size_t size);

bool dir_delete_if_exists(char* path) {
  // TODO: is dir_exists check redundant?
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

// TODO: creations options and permissions?
bool file_create_if_not_exists(char* path) {
  // https://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html
  // https://learn.microsoft.com/it-it/windows/win32/api/fileapi/nf-fileapi-createfilea

  // Consider doing a system-dependant implementation instead of using fopen;
  // file_exsits() will already query the os (redundant check)
  if (file_exists(path)) return false;
  
  FILE* fd = file_open_write(path);
  return file_close(fd);
}

// TODO: add flag to choose if should overwrite
bool file_copy(char* src, char* dst) {
#ifndef _WIN32
  int src_fd = open(src, O_RDONLY, 0);
  int dst_fd = open(dst O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

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

  return true;
#else
  // https://learn.microsoft.com/it-it/windows/win32/api/winbase/nf-winbase-copyfile
  BOOL res = CopyFile(src, dst, false);
  if (!res) perror_windows("Couldn't copy src file to dst file");
  return res;
#endif
}

bool file_write(char* path, char* data, size_t len) {

}

#endif