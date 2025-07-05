#ifndef STC_STR_IMPL
#define STC_STR_IMPL

#include "stc_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
  size_t len;
  char* data;
} str;

char* str_to_cstr(str s) {
  char* res = malloc(s.len + 1);
  memcpy(res, s.data, s.len);
  res[s.len + 1] = '\0';
  return res;
}

str str_from_cstr(char* s) {
  return (str) { strlen(s), s };
}

str str_from_cstr_unchecked(char* s, size_t len) {
  return (str) { len, s };
}

const str STR_EMPTY = {0, ""};

bool str_is_empty(str s) {
  return s.data == NULL || s.len == 0;
}

// TODO: handle negative indexes (back of string)
str str_slice(str s, size_t start, size_t end) {
  if (start >= end) return STR_EMPTY;
  // assert(start <= end && "str_slice(): start must be smaller than end");

  #define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
  start = MIN(start, s.len);
  end   = MIN(end,   s.len);
  #undef MIN

  return (str) {
    .len = end - start,
    .data = s.data + start,
  };
}

str cstr_slice(char* c, size_t start, size_t end) {
  return str_slice(str_from_cstr(c), start, end);
}

bool str_eq(str a, str b) {
  if (a.len != b.len) return false;
  else return memcmp(a.data, b.data, a.len) == 0;
}

bool str_eq_ignorecase(str a, str b) {
  if (a.len != b.len) return false;

  listfor(int, i, &a) {
    if (tolower(a.data[i]) != tolower(b.data[i])) return false;
  }

  return true;
}

int str_cmp(str a, str b) {
  if (a.len != b.len) return a.len - b.len;
  else return memcmp(a.data, b.data, a.len);
}

int str_find(str s, char c) {
  char* res = memchr(s.data, c, s.len);
  return res != NULL ? res - s.data : -1;
}

int str_find_rev(str s, char c) {
  // for (int i=s.len-1; i >= 0; --i) {
  //   if (s.data[i] == c) return i;
  // }
  // return -1;
  char* res = strrchr(s.data, c);
  return res != NULL ? res - s.data : -1;
}

bool str_contains(str s, char c) {
  return str_find(s, c) != -1;
}

int str_match(str s, str target) {
  if (target.len > s.len) return -1;
  str window = str_slice(s, 0, target.len);

  for(int i=0; i <= s.len - target.len; ++i) {
    if (str_eq(window, target)) return i;
    window.data += 1;
  }

  return -1;
}

IntList str_match_all(str s, str target) {
  IntList matches = {0};
  str window = str_slice(s, 0, target.len);

  for (int i=0; i <= s.len - target.len; ++i) {
    if (str_eq(window, target)) {
      IntList_push(&matches, i);
    }
    window.data += 1;
  }

  return matches;
}


str str_skip(str s, size_t n) {
  // redundant check; str_slice already clamps n
  // if (n > s.len) return STR_EMPTY;
  return str_slice(s, n, s.len);
}
str str_take(str s, size_t n) {
  // redundant check; str_slice already clamps n
  // if (n > s.len) return s;
  return str_slice(s, 0, n);
}


str str_skip_rev(str s, size_t n) {
  // check neccessary; overflow otherwise
  if (n > s.len) return STR_EMPTY;
  return str_slice(s, 0, s.len - n);
}
str str_skip_until_char(str s, char c) {
  int idx = str_find(s, c);
  if (idx == -1) return STR_EMPTY;
  return str_skip(s, idx);
}

str str_take_rev(str s, size_t n) {
  // check neccessary; overflow otherwise
  if (n > s.len) return s;
  return str_slice(s, s.len - n, s.len);
}
str str_take_until_char(str s, char c) {
  int idx = str_find(s, c);
  if (idx == -1) return s;
  return str_take(s, idx);
}

str str_skip_until_match(str s, str target) {
  int idx = str_match(s, target);
  return str_skip(s, idx);
}
str str_take_until_match(str s, str target) {
  int idx = str_match(s, target);
  return str_take(s, idx);
}

typedef int (*CharPredicate)(int val);
size_t str_match_while(str s, CharPredicate p) {
  listfor(size_t, i, &s) {
    if (!p(s.data[i])) return i;
  }
  return s.len-1;
}
size_t str_match_while_rev(str s, CharPredicate p) {
  listforrev(size_t, i, &s) {
    if (!p(s.data[i])) return s.len-1 - i;
  }
  return s.len-1;
}

str str_skip_while(str s, CharPredicate p) {
  int match = str_match_while(s, p);
  return str_skip(s, match);
}
str str_skip_while_rev(str s, CharPredicate p) {
  int match = str_match_while_rev(s, p);
  return str_skip_rev(s, match);
}

str str_take_while(str s, CharPredicate p) {
  int match = str_match_while(s, p);
  return str_take(s, match);
}
str str_take_while_rev(str s, CharPredicate p) {
  int match = str_match_while_rev(s, p);
  return str_take_rev(s, match);
}


bool str_starts_with(str s, str start) {
  str prefix = str_take(s, start.len);
  return str_eq(prefix, start);
}
bool str_ends_with(str s, str end) {
  str postfix = str_skip(s, s.len - end.len);
  return str_eq(postfix, end);
}

str str_trim_start(str s) {
  return str_skip_while(s, isblank);
}
str str_trim_end(str s) {
  return str_skip_while_rev(s, isblank);
}
str str_trim(str s) {
  return str_trim_end(str_trim_start(s));
}

#define PARSE_BUF_LEN 1024
static char parse_buf[PARSE_BUF_LEN];

int str_parse_int(str s) {
  assert(s.len+1 < PARSE_BUF_LEN && "str_parse_int(): number string exceeds temporary buffer size");
  memcpy(parse_buf, s.data, s.len);
  parse_buf[s.len + 1] = '\0';
  int n = atoi(parse_buf);
  return n;
}

double str_parse_float(str s) {
  assert(s.len < PARSE_BUF_LEN && "str_parse_float(): number string exceeds temporary buffer size");
  memcpy(parse_buf, s.data, s.len);
  parse_buf[s.len + 1] = '\0';
  int n = atof(parse_buf);
  return n;
}

list_def(str, StrList)
StrList str_split_char(str s, char c) {
  StrList ss = {0};

  while (s.len > 0) {
    int i = str_find(s, c);
    if (i == -1) {
      StrList_push(&ss, s);
      break;
    }
    StrList_push(&ss, str_take(s, i));
    s = str_skip(s, i+1);
  }

  return ss;
}

StrList str_split(str s, str pattern) {
  StrList ss = {0};

  while (s.len > 0) {
    int i = str_match(s, pattern);
    if (i == -1) {
      StrList_push(&ss, s);
      break;
    }
    StrList_push(&ss, str_take(s, i));
    s = str_skip(s, i+pattern.len);
  }

  return ss;
}

StrList str_lines(str s) {
  return str_split_char(s, '\n');
}

typedef struct {
  size_t curr;
  str s;
} StrIter;

StrIter str_iter(str s) {
  return (StrIter) {0, s};
}

bool str_iter_at_end(StrIter it) {
  return it.s.len == 0;
}

int str_iter_match(StrIter* it, str target) {
  int match = str_match(it->s, target);
  if (match == -1) {
    it->curr += it->s.len;
    it->s = STR_EMPTY;
    return match;
  } else {
    int curr = it->curr;
    it->curr += match+1;
    it->s = str_skip(it->s, match+1);
    return curr + match;
  }
}

str str_iter_split_char(StrIter* it, char c) {
  int i = str_find(it->s, c);
  if (i == -1) {
    str res = it->s;
    it->s = STR_EMPTY;
    return res;
  } else {
    str res = str_take(it->s, i);
    it->s = str_skip(it->s, i+1);
    return res;
  }
}

str str_iter_split_match(StrIter* it, str pattern) {
  int i = str_match(it->s, pattern);
  if (i == -1) {
    str res = it->s;
    it->s = STR_EMPTY;
    return res;
  } else {
    str res = str_take(it->s, i);
    it->s = str_skip(it->s, i+pattern.len);
    return res;
  }
}

str str_iter_line(StrIter* it) {
  return str_iter_split_char(it, '\n');
}


//////////////////////

list_def(char, String)

str String_to_str(String sb) {
  return (str) {
    .len = sb.len,
    .data = sb.data,
  };
}

// str String_to_owned_str(String* sb) {
//   char* s = malloc(sb->len);
//   memcpy(s, sb->data, sb->len);
//   return (str) {
//     .len = sb->len,
//     .data = s,
//   };
// }

void String_append_null(String* sb) {
  // String_push(sb, '\0');
  // whenever we append a cstr, we also push the null,
  // without incrementing the size; this lets us use the
  // string builder as a cstr. 
  // Any other append will overwrite the null,
  // so it has to be appended again
  sb->data[sb->len] = '\0';
}

void String_append_cstr(String* sb, char* s) {
  String_append_array(sb, s, strlen(s));
  String_append_null(sb);
}

void String_append_str(String* sb, str sv) {
  String_append_array(sb, sv.data, sv.len);
}

String String_from_cstr(char* s) {
  String sb = {0};
  String_append_cstr(&sb, s);
  return sb;
}

String String_from_str(str s) {
  return String_from_array(s.data, s.len);
}

char* String_to_cstr(String sb) {
  return str_to_cstr(String_to_str(sb));
}

#define FMT_BUF_LEN 2048
static char fmt_buf[FMT_BUF_LEN];
String String_format(String* sb, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int real_size = vsnprintf(fmt_buf, FMT_BUF_LEN, fmt, args);
  va_end(args);

  if (real_size > FMT_BUF_LEN) {
    fprintf(stderr, 
      "String_format(): fmt_buf size is not big enough: size is %d, should have written %d",
      FMT_BUF_LEN,
      real_size
    );
  }

  sb->len = 0;
  String_append_cstr(sb, fmt_buf);
  return *sb;
}

String int_to_str(String* sb, int n) {
  return String_format(sb, "%d", n);
}

String float_to_str(String* sb, double n) {
  return String_format(sb, "%f", n);
}

// TODO: this is possibly dangerous, as the returned string has to be freed. 
// think about what to do with this
String str_concat(String a, String b) {
  // use String_append() instead if you want to allocate your own String
  String sb = {0};
  String_reserve(&sb, a.len + b.len);
  String_append(&sb, a);
  String_append(&sb, b);
  return sb;
}

String str_repeat(String* sb, str sv, size_t n) {
  sb->len = 0;
  String_reserve(sb, sv.len * n);
  while (n-- > 0) String_append_str(sb, sv);

  return *sb;
}

String str_to_upper(String* sb, str sv) {
  sb->len = 0;
  listforeach(char, c, &sv) {
    String_push(sb, toupper(*c));
  }
  return *sb;
}

String str_to_lower(String* sb, str sv) {
  sb->len = 0;
  listforeach(char, c, &sv) {
    String_push(sb, tolower(*c));
  }
  return *sb;
}

void String_to_upper(String* s) {
  listforeach(char, c, s) *c = toupper(*c);
}

void String_to_lower(String* s) {
  listforeach(char, c, s) *c = tolower(*c);
}

String str_replace(String* sb, str sv, str from, str to) {
  int match = str_match(sv, from);
  if (match == -1) return String_from_str(sv);

  sb->len = 0;
  String_append_str(sb, str_take(sv, match));
  String_append_str(sb, to);
  String_append_str(sb, str_skip(sv, match + from.len));
  return *sb;
}

String str_replace_all(String* sb, str sv, str from, str to) {
  IntList matches = str_match_all(sv, from);
  
  sb->len = 0;
  int last = 0;
  listforeach(int, match, &matches) {
    String_append_str(sb, str_slice(sv, last, *match));
    String_append_str(sb, to);
    last = *match + from.len;
  }
  String_append_str(sb, str_skip(sv, last));
  IntList_drop(&matches);

  return *sb;
}

String str_join(String* sb, str join, StrList strs) {
  sb->len = 0;
  if (strs.len == 0) { return *sb; }

  size_t size = 0;
  listforeach(str, s, &strs) size += s->len;
  size += join.len * (strs.len-1);
  String_reserve(sb, size);

  for(size_t i=0; i<strs.len-1; ++i) {
    String_append_str(sb, strs.data[i]);
    String_append_str(sb, join);
  }
  String_append_str(sb, StrList_last(strs));

  return *sb;
}

String str_join_two(String* sb, str join, str a, str b) {
  sb->len = 0;
  String_reserve(sb, a.len + b.len + join.len);
  String_append_str(sb, a);
  String_append_str(sb, join);
  String_append_str(sb, b);
  return *sb;
}

#define str_fmt "%.*s"
#define str_arg(s) (int) (s).len, (s).data

#endif