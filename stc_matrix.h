// https://nullprogram.com/blog/2016/10/27/
#include "stc_defs.h"
#include "stc_list.h"

#define gridrowsfor(type, row, grid) for(type row = 0; row < (grid)->height; ++row) 
#define gridcolsfor(type, col, grid) for(type col = 0; col < (grid)->width; ++col)

typedef struct {
  const isize width, height;
  byte* data;
} Grid;

Grid grid_new(isize width, isize height) {
  byte* data = malloc(width * height * sizeof(byte));
  assert(data != NULL && "grid alloc failed");
  return { width, height, data };
}

isize grid_idx(const Grid* m, isize row, isize col) {
  return row * m->width + col;
}

void grid_fill(Grid* m, int value) {
  listfor(isize, i, *m) m->data[i] = value;
}

Grid grid_from_array(const byte* arr, isize arr_len, isize cols) {
  isize rows = cols / arr_len;
  Grid res = grid_new(cols, rows);
  memcpy(res.data, arr, arr_len * sizeof(byte));
  return res;
}