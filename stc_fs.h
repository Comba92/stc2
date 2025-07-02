#ifndef STC_FS_IMPL
#define STC_FS_IMPL

#include <stdio.h>
#include "stc_str.h"

char* read_file_to_string(char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    perror("Couldn't read file");
    return NULL;
  }

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
  size_t read = fread(buf, 1, file_size, f);
  if (read != file_size) {
    perror("Couldn't read whole file");
  }

  if (fclose(f) != 0) {
    perror("Couldn't close file");
  }

  return buf;
}

FILE* open_file_read(char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    perror("Couldn't read file");
    return NULL;
  }
  return f;
}

FILE* open_file_write(char* path) {
  FILE* f = fopen(path, "a+b");
  if (f == NULL) {
    perror("Couldn't read file");
    return NULL;
  }
  return f;
}

bool close_file(FILE* f) {
  return fclose(f) == 0;
}

char* path_filename(char* path) {
  str path_str = str_from_cstr(path);

  StrList sl = str_split(path_str, str_from_cstr("/"));
  str filename = StrList_last(sl);
  StrList_free(&sl);
  
  sl = str_split(filename, str_from_cstr("\\"));
  filename = StrList_last(sl);
  StrList_free(&sl);

  return filename.data;
}

char* path_filename_ext(char* path) {
  char* filename = path_filename(path);
  
  StrList sl = str_split(filename, str_from_cstr("."))
}

// Nob_Fd nob_fd_open_for_read(const char *path);
// Nob_Fd nob_fd_open_for_write(const char *path);
// void nob_fd_close(Nob_Fd fd);
// static DIR *opendir(const char *dirpath);
// static struct dirent *readdir(DIR *dirp);
// static int closedir(DIR *dirp);
// const char *nob_path_name(const char *path);

// const char *nob_get_current_dir_temp(void)
// int nob_file_exists(const char *file_path);
// bool nob_mkdir_if_not_exists(const char *path);
// bool nob_copy_file(const char *src_path, const char *dst_path);
// bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
// bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children);
// bool nob_write_entire_file(const char *path, const void *data, size_t size);
// Nob_File_Type nob_get_file_type(const char *path);
// bool nob_delete_file(const char *path);

#endif