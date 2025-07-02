#ifndef STC_STRING_IMPL
#define STC_STRING_IMPL

#include "stc_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// TODO: temporary allocator for Strings

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

str str_empty() {
  return (str) { 0, "" };
}

// TODO: handle negative indexes (back of string)
str str_slice(str s, size_t start, size_t end) {
  assert(start <= end && "str_slice(): start must be smaller than end");

  start = MIN(start, s.len);
  end   = MIN(end, s.len);
  return (str) {
    .len = end - start,
    .data = s.data + start,
  };
}

bool str_eq(str a, str b) {
  if (a.len != b.len) return false;

  // listfor(int, i, &a) {
  //   if (a.data[i] != b.data[i]) return false;
  // }

  // return true;
  return memcmp(a.data, b.data, a.len) == 0;
}

bool str_eq_ignorecase(str a, str b) {
  if (a.len != b.len) return false;

  listfor(int, i, &a) {
    if (tolower(a.data[i]) != tolower(b.data[i])) return false;
  }

  return true;
}

int str_cmp(str a, str b) {
  if (a.len == b.len) {
    return memcmp(a.data, b.data, a.len);
  }

  // for(int i=0; i<a.len && i<b.len; ++i) {
  //   if (a.data[i] != b.data[i]) {
  //     return a.data[i] - b.data[i];
  //   }
  // }

  return a.len - b.len;
}

int str_find(str s, char c) {
  // listfor(int, i, &s) {
  //   if (s.data[i] == c) return i;
  // }
  // return -1;

  char* res = memchr(s.data, c, s.len);
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
  return str_slice(s, n, s.len);
}
str str_skip_rev(str s, size_t n) {
  if (n > s.len) return str_empty();
  return str_slice(s, 0, s.len - n);
}
str str_skip_until_char(str s, char c) {
  int idx = str_find(s, c);
  if (idx == -1) return str_empty();
  return str_skip(s, idx);
}

str str_take(str s, size_t n) {
  return str_slice(s, 0, n);
}
str str_take_rev(str s, size_t n) {
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
  for(int i=0; i<s.len; ++i) {
    if (!p(s.data[i])) return i;
  }
  return s.len-1;
}
size_t str_match_while_rev(str s, CharPredicate p) {
  for(int i=s.len-1; i>=0; --i) {
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
  str prefix = str_slice(s, 0, start.len);
  return str_eq(prefix, start);
}
bool str_ends_with(str s, str end) {
  str postfix = str_slice(s, s.len - end.len, s.len);
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

int str_parse_int(str s) {
  char* buf = str_to_cstr(s);
  int n = atoi(buf);
  free(buf);
  return n;
}

double str_parse_float(str s) {
  char* buf = str_to_cstr(s);
  int n = atof(buf);
  free(buf);
  return n;
}

list_def(str, StrList)
// TODO: fix this
StrList str_split(str s, char c) {
  StrList ss = {0};

  for (int i=0; s.len > 0 && i<s.len; ++i) {
    if (s.data[i] == c) {
      StrList_push(&ss, str_slice(s, 0, i));
      s = str_slice(s, i+1, s.len);
    }
  }

  StrList_push(&ss, str_slice(s, 0, s.len));
  return ss;
}

StrList str_split_match(str s, str pattern) {
  StrList ss = {0};

  while (s.len > 0) {
    int i = str_match(s, pattern);
    if (i == -1) {
      StrList_push(&ss, str_slice(s, 0, s.len));
      break;
    }
    StrList_push(&ss, str_slice(s, 0, i));
    s = str_slice(s, i+pattern.len, s.len);
  }

  return ss;
}

StrList str_lines(str s) {
  return str_split(s, '\n');
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

void String_append_cstr(String* sb, char* s) {
  String_append_array(sb, s, strlen(s));
}

String String_from_cstr(char* s) {
  String sb = {0};
  String_append_cstr(&sb, s);
  return sb;
}

String String_from_str(str s) {
  return String_from_cstr(str_to_cstr(s));
}

char* String_to_cstr(String* sb) {
  return str_to_cstr(String_to_str(*sb));
}

void String_append_str(String* sb, str sv) {
  String_append_array(sb, sv.data, sv.len);
}

void String_append_null(String* sb) {
  String_push(sb, '\0');
}

String String_format(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const size_t buf_len = 1024;
  char* buf = malloc(buf_len);
  vsnprintf(buf, buf_len, fmt, args);

  String res = String_from_cstr(buf);
  free(buf);
  va_end(args);

  return res;
}

String int_to_str(int n) {
  return String_format("%d", n);
}

String float_to_str(double n) {
  return String_format("%f", n);
}

String str_concat(String a, String b) {
  String res = {0};
  String_append(&res, a);
  String_append(&res, b);
  return res;
}

String str_repeat(str s, size_t n) {
  String res = {0};
  String_reserve(&res, s.len * n);
  while (n > 0) {
    String_append_str(&res, s);
    n--;
  }

  return res;
}

String str_to_upper(str s) {
  String res = {0};
  listforeach(char, c, &s) {
    String_push(&res, toupper(*c));
  }
  return res;
}

String str_to_lower(str s) {
  String res = {0};
  listforeach(char, c, &s) {
    String_push(&res, tolower(*c));
  }
  return res;
}

String str_replace(str s, str from, str to) {
  int match = str_match(s, from);
  if (match == -1) return String_from_str(s);

  String res = {0};
  String_append_str(&res, str_slice(s, 0, match));
  String_append_str(&res, to);
  String_append_str(&res, str_slice(s, match + from.len, s.len));
  return res;
}

String str_replace_all(str s, str from, str to) {
  IntList matches = str_match_all(s, from);

  String res = {0};
  int last = 0;
  listforeach(int, match, &matches) {
    String_append_str(&res, str_slice(s, last, *match));
    String_append_str(&res, to);
    last = *match + from.len;
  }
  String_append_str(&res, str_slice(s, last, s.len));
  return res;
}

String str_join(StrList strs, str join) {
  String ss = {0};
  if (strs.len == 0) { return ss; }

  for(int i=0; i<strs.len-1; ++i) {
    String_append_str(&ss, strs.data[i]);
    String_append_str(&ss, join);
  }
  String_append_str(&ss, StrList_last(strs));

  return ss;
}

#define str_fmt "%.*s"
#define str_arg(s) (int) (s).len, (s).data

#endif