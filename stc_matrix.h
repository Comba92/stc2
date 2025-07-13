// https://nullprogram.com/blog/2016/10/27/
#include "stc_defs.h"

typedef struct {
  isize width, height;
  byte* data;
} Matrix;

Matrix mat_new(isize width, isize height) {
  byte* data = malloc(width * height * sizeof(byte));
  assert(data != NULL && "matrix realloc failed");
  return { width, height, data };
}

isize mat_index(const Matrix* m, isize row, isize col) {
  return row * m->width + col;
}