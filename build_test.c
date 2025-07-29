#include "stc_build.h"

int main() {
  char* inputs[] = {
    "fs_test.c"
  };

  binary_rebuild("fag.exe", inputs, 1);
}