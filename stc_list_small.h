#ifndef STC_LIST_SMALL_IMPL
#define STC_LIST_SMALL_IMPL
#include <stdlib.h>
#include "stc_defs.h"

#define ListDef(type, name) \
typedef struct { \
  isize len, cap; \
  type* data; \
} name; \
 \

#define ListReserve(l, new_cap) \
{ \
  if ((new_cap) > (l).cap) { \
    (l).cap = (l).cap == 0 ? 16 : (l).cap; \
    (l).cap = NextPowerOfTwo(new_cap); \
    (l).data = realloc((l).data, sizeof((l).data[0]) * (l).cap); \
  } \
} \

#define ListPush(l, val) \
{ \
  ListReserve((l), (l).len+1); \
  (l).data[(l).len++] = (val); \
} \

#define ListPop(l) ((l).data[--(l).len])
#define ListRemoveSwap(l, i) ((l).data[i] = ListPop(l))

#define ListFirst(l) ((l).data[0])
#define ListLast(l) ((l).data[(l).len-1])

#define ListClear(l) ((l).len = 0)

#define ListFree(l) \
{ \
  free((l).data); \
  (l).len = (l).cap = 0; \
} \

#endif