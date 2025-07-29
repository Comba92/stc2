#ifndef STC_STR_IMPL
#define STC_STR_IMPL

#include "stc_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

// https://nrk.neocities.org/articles/isdigit-multi-implementation#info-ref-0
bool c_is_space(char c) {
  switch (c) {
    case ' ': case '\f': case '\n': case '\r': case '\t': case '\v': return true;
    default: return false;
  }
}
bool c_is_alpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool c_is_digit(char c) {
  return c >= '0' && c <= '9';
}
bool c_is_alphanum(char c) {
  return c_is_alpha(c) || c_is_digit(c);
}
bool c_is_punct(char c) {
  // const char puntcs[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

  return (c >= '!' && c <= '/') 
  || (c >= ':' && c <= '@') 
  || (c >= '[' && c <= '`')
  || (c >= '{' && c <= '~');
}
bool c_is_lower(char c) {
  return c >= 'a' && c <= 'z';
}
bool c_is_upper(char c) {
  return c >= 'A' && c <= 'Z';
}
bool c_is_cntrl(char c) {
  return (c >= 0 || c <= 31) || c == 127;
}

char c_to_lower(char c) {
  return c_is_upper(c) ? c - 'A' + 'a' : c;
}
char c_to_upper(char c) {
  return c_is_lower(c) ? c - 'a' + 'A' : c;
}

typedef struct {
  isize len;
  const char* data;
} str;

// we do not keep the null char
#define SV(str) str_from_cstr_unchecked((str), sizeof((str))-1)
#define SVC(cstr) str_from_cstr((cstr))

#define str_fmt "%.*s"
#define str_arg(s) (int) (s).len, (s).data
#define str_dbg(s) printf("\"%.*s\"\n", (int) (s).len, (s).data);


// should be freed
char* str_to_cstr(str s) {
  char* res = malloc(s.len + 1);
  memcpy(res, s.data, s.len);
  res[s.len + 1] = '\0';
  return res;
}

str str_clone(str s) {
  char* cloned = malloc(s.len);
  memcpy(cloned, s.data, s.len);
  return (str) { s.len, cloned };
}

str str_from_cstr(const char* s) {
  return (str) { strlen(s), s };
}

str str_from_cstr_unchecked(const char* s, isize len) {
  return (str) { len, s };
}

static const str STR_EMPTY = {0, ""};

bool str_is_empty(str s) {
  return s.data == NULL || s.data[0] == '\0' || s.len == 0;
}

str str_slice(str s, isize start, isize end) {
  // assert(start <= end && "str_slice(): start must be smaller than end");
  if (start >= end) return STR_EMPTY;

  #define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
  start = MIN(start, s.len);
  end   = MIN(end,   s.len);
  #undef MIN

  return (str) {
    .len = end - start,
    .data = s.data + start,
  };
}

str cstr_slice(const char* c, isize start, isize end) {
  return str_slice(str_from_cstr(c), start, end);
}

bool str_eq(str a, str b) {
  if (a.len != b.len) return false;
  else return memcmp(a.data, b.data, a.len) == 0;
}

bool str_eq_ignorecase(str a, str b) {
  if (a.len != b.len) return false;

  listfor(int, i, &a) {
    if (c_to_lower(a.data[i]) != c_to_lower(b.data[i])) return false;
  }

  return true;
}

isize str_cmp(str a, str b) {
  if (a.len != b.len) return a.len - b.len;
  else return memcmp(a.data, b.data, a.len);
}

isize str_find(str s, char c) {
  char* res = memchr(s.data, c, s.len);
  return res != NULL ? res - s.data : -1;
}

isize str_find_rev(str s, char c) {
  char* res = strrchr(s.data, c);
  return res != NULL ? res - s.data : -1;
}

bool str_contains(str s, char c) {
  return str_find(s, c) != -1;
}

isize str_match(str s, str target) {
  if (target.len > s.len) return -1;
  str window = str_slice(s, 0, target.len);

  for(isize i=0; i <= s.len - target.len; ++i) {
    if (str_eq(window, target)) return i;
    window.data += 1;
  }

  return -1;
}

IntList str_match_all(str s, str target) {
  IntList matches = {0};
  str window = str_slice(s, 0, target.len);

  for (isize i=0; i <= s.len - target.len; ++i) {
    if (str_eq(window, target)) {
      IntList_push(&matches, i);
    }
    window.data += 1;
  }

  return matches;
}


str str_skip(str s, isize n) {
  // redundant check; str_slice already clamps n
  // if (n > s.len) return STR_EMPTY;
  return str_slice(s, n, s.len);
}
str str_take(str s, isize n) {
  // redundant check; str_slice already clamps n
  // if (n > s.len) return s;
  return str_slice(s, 0, n);
}


str str_skip_rev(str s, isize n) {
  // check neccessary; overflow otherwise
  if (n > s.len) return STR_EMPTY;
  return str_slice(s, 0, s.len - n);
}
str str_skip_untilc(str s, char c) {
  int idx = str_find(s, c);
  if (idx == -1) return STR_EMPTY;
  return str_skip(s, idx);
}
str str_skip_rev_untilc(str s, char c) {
  int idx = str_find_rev(s, c);
  if (idx == -1) return STR_EMPTY;
  return str_take(s, idx);
}

str str_take_rev(str s, isize n) {
  // check neccessary; overflow otherwise
  if (n > s.len) return s;
  return str_slice(s, s.len - n, s.len);
}
str str_take_untilc(str s, char c) {
  int idx = str_find(s, c);
  if (idx == -1) return s;
  return str_take(s, idx);
}
str str_take_rev_untilc(str s, char c) {
  int idx = str_find_rev(s, c);
  if (idx == -1) return s;
  return str_skip(s, idx);
}

str str_skip_until(str s, str target) {
  int idx = str_match(s, target);
  return str_skip(s, idx);
}
str str_take_until(str s, str target) {
  int idx = str_match(s, target);
  return str_take(s, idx);
}

typedef bool (*CharPredicate)(char val);

isize str_advance_while(str s, CharPredicate p) {
  listfor(isize, i, &s) {
    if (!p(s.data[i])) return i;
  }
  return s.len;
}
isize str_advance_while_not(str s, CharPredicate p) {
  listfor(isize, i, &s) {
    if (p(s.data[i])) return i;
  }
  return s.len;
}
isize str_advance_rev_while(str s, CharPredicate p) {
  listforrev(isize, i, &s) {
    if (!p(s.data[i])) return s.len-1 - i;
  }
  return s.len;
}

str str_skip_while(str s, CharPredicate p) {
  isize match = str_advance_while(s, p);
  return str_skip(s, match);
}
str str_skip_rev_while(str s, CharPredicate p) {
  isize match = str_advance_rev_while(s, p);
  return str_skip_rev(s, match);
}

str str_take_while(str s, CharPredicate p) {
  isize match = str_advance_while(s, p);
  return str_take(s, match);
}
str str_take_rev_while(str s, CharPredicate p) {
  isize match = str_advance_rev_while(s, p);
  return str_take_rev(s, match);
}


bool str_starts_with(str s, str start) {
  str prefix = str_take(s, start.len);
  return str_eq(prefix, start);
}
bool str_ends_with(str s, str end) {
  str suffix = str_skip(s, s.len - end.len);
  return str_eq(suffix, end);
}

str str_strip_prefix(str s, str prefix) {
  if (str_starts_with(s, prefix)) return str_take(s, prefix.len);
  else return s;
}

str str_strip_suffix(str s, str suffix) {
  if (str_ends_with(s, suffix)) return str_skip(s, suffix.len);
  else return s;
}


str str_trim_start(str s) {
  return str_skip_while(s, c_is_space);
}
str str_trim_end(str s) {
  return str_skip_rev_while(s, c_is_space);
}
str str_trim(str s) {
  return str_trim_end(str_trim_start(s));
}

list_def(str, StrList)
StrList str_splitc_collect(str s, char c) {
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

StrList str_split_collect(str s, str pattern) {
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

StrList str_split_when_collect(str s, CharPredicate pred) {
  StrList ss = {0};

  while (s.len > 0) {
    int i = str_advance_while_not(s, pred);
    StrList_push(&ss, str_take(s, i));
    s = str_skip(s, i);
    i = str_advance_while(s, pred);
    s = str_skip(s, i);
  }

  return ss;
}

// TODO: does this work on windows? (line separator is "\r\n")
StrList str_lines_collect(str s) {
  return str_splitc_collect(s, '\n');
}

StrList str_words_collect(str s) {
  return str_split_when_collect(s, c_is_space);
}

//////////////////////

// End condition for all str iterators: src.len == 0

typedef struct {
  str src;
  const str target;
  isize skipped;
  isize last_match;
} StrMatches;

StrMatches str_matches(str s, str target) {
  isize match = str_match(s, target);
  if (match == -1) {
    return (StrMatches) { STR_EMPTY, target, s.len, match };
  } else {
    return (StrMatches) { str_skip(s, match+1), target, 0, match };
  }
}

bool str_has_match(const StrMatches* it) {
  return it->last_match != -1;
}

isize str_next_match(StrMatches* it) {
  if (it->last_match == -1) return -1;

  isize last = it->last_match;
  isize idx = it->skipped;
  
  isize match = str_match(it->src, it->target);
  if (match == -1) {
    it->skipped += it->src.len;
    it->src = STR_EMPTY;  
  } else {
    it->skipped += last+1;
    it->src = str_skip(it->src, match+1);
  }
  it->last_match = match;

  return idx + last;
}

// TODO: this is clutter, consider removing it
typedef struct {
  str src;
  const char c;
} StrSplitChar;

StrSplitChar str_splitc(str s, char c) {
  return (StrSplitChar) { s, c };
}

bool str_has_splitc(const StrSplitChar* it) {
  return it->src.len > 0;
}

str str_next_splitc(StrSplitChar* it) {
  isize i = str_find(it->src, it->c);
  if (i == -1) {
    str res = it->src;
    it->src = STR_EMPTY;
    return res;
  } else {
    str res = str_take(it->src, i);
    it->src = str_skip(it->src, i+1);
    return res;
  }
}

typedef struct {
  str src;
  const str pattern;
} StrSplit;

StrSplit str_split(str s, str pattern) {
  return (StrSplit) { s, pattern };
}

bool str_has_split(const StrSplit* it) {
  return it->src.len > 0;
}

str str_next_split(StrSplit* it) {
  int i = str_match(it->src, it->pattern);
  if (i == -1) {
    str res = it->src;
    it->src = STR_EMPTY;
    return res;
  } else {
    str res = str_take(it->src, i);
    it->src = str_skip(it->src, i+it->pattern.len);
    return res;
  }
}

typedef struct {
  str src;
  const CharPredicate pred;
} StrSplitWhen;

StrSplitWhen str_split_when(str s, CharPredicate pred) {
  return (StrSplitWhen) { s, pred };
}

bool str_has_split_when(const StrSplitWhen* it) {
  return it->src.len > 0;
}

str str_next_split_when(StrSplitWhen* it) {
  int i = str_advance_while_not(it->src, it->pred);
  str res = str_take(it->src, i);
  it->src = str_skip(it->src, i);
  i = str_advance_while(it->src, it->pred);
  it->src = str_skip(it->src, i);
  return res;
}

// TODO: does this work on windows? (line separator is "\r\n")
typedef StrSplitChar StrLines;
StrLines str_lines(str s) {
  return str_splitc(s, '\n');
}
bool str_has_line(const StrLines* it) {
  return str_has_splitc(it);
}
str str_next_line(StrLines* it) {
  return str_next_splitc(it);
}

typedef StrSplitWhen StrWords;
StrWords str_words(str s) {
  return str_split_when(s, c_is_space);
}
bool str_has_word(const StrWords* it) {
  return str_has_split_when(it);
}
str str_next_word(StrWords* it) {
  return str_next_split_when(it);
}

//////////////////////

list_def(char, String)
list_def(String, StringList)
#define SB(str) String_from_str(SV(str))
#define SBC(cstr) String_from_cstr(cstr)
#define SBV(sb) String_to_tmp_str(sb)

// THIS SHOULD'T BE DONE!!!
// String can reallocate, so if a reference is taken earlier, it will be invalidated.
str String_to_tmp_str(String sb) {
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
  
  String_reserve(sb, sb->len+1);
  sb->data[sb->len] = '\0';
}

void String_append_cstr(String* sb, const char* s) {
  String_append_array(sb, s, strlen(s));
  String_append_null(sb);
}

void String_append_str(String* sb, str sv) {
  String_append_array(sb, sv.data, sv.len);
}

String String_from_cstr(const char* s) {
  String sb = {0};
  String_append_cstr(&sb, s);
  return sb;
}

String cstr_heap_to_String(char** s) {
  return array_heap_to_String(s, strlen(*s));
}

String String_from_str(str s) {
  return String_from_array(s.data, s.len);
}

char* String_to_cstr(String sb) {
  return str_to_cstr(String_to_tmp_str(sb));
}

static __thread String tmp_sb = {0};

// TODO: read this
// https://nullprogram.com/blog/2023/02/13/
char* str_fmt_tmp(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int real_size = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  // real_size excludes null
  real_size += 1;
  String_reserve(&tmp_sb, real_size);
  
  va_start(args, fmt);
  // should write_real_size + null
  // be sure to set tmp_sb.len!
  tmp_sb.len = vsnprintf(tmp_sb.data, real_size, fmt, args);
  va_end(args);

  return tmp_sb.data;
}

// returns itself
String String_readline_stdin(String* sb) {
  char* res = fgets(sb->data, sb->cap, stdin);
  if (res == NULL || ferror(stdin) != 0) sb->len = 0;
  else sb->len = strlen(sb->data);
  return *sb;
}

// returns itself
String String_append_fmt(String* sb, const char* fmt, ...) {
  // we can't reuse str_fmt_tmp for this; you can't pass varargs to another function
  // https://stackoverflow.com/questions/3530771/passing-variable-arguments-to-another-function-that-accepts-a-variable-argument

  va_list args;
  va_start(args, fmt);
  int real_size = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  // real_size excludes null
  real_size += 1;
  String_reserve(&tmp_sb, real_size);
  
  va_start(args, fmt);
  // should write_real_size + null
  // be sure to set tmp_sb.len!
  tmp_sb.len = vsnprintf(tmp_sb.data, real_size, fmt, args);
  va_end(args);

  String_append(sb, tmp_sb);
  return *sb;
}

int str_parse_int(str s) {
  String_reserve(&tmp_sb, s.len+1);
  memcpy(tmp_sb.data, s.data, s.len);
  tmp_sb.len = s.len+1;
  String_append_null(&tmp_sb);

  int n = atoi(tmp_sb.data);
  return n;
}

double str_parse_float(str s) {
  String_reserve(&tmp_sb, s.len+1);
  memcpy(tmp_sb.data, s.data, s.len);
  tmp_sb.len = s.len+1;
  String_append_null(&tmp_sb);

  double n = atof(tmp_sb.data);
  return n;
}

// returns itself
String int_to_str(String* sb, int n) {
  sb->len = 0;
  return String_append_fmt(sb, "%d", n);
}

// returns itself
String float_to_str(String* sb, double n) {
  sb->len = 0;
  return String_append_fmt(sb, "%f", n);
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

// returns itself
String str_repeat(String* sb, str sv, isize n) {
  sb->len = 0;
  String_reserve(sb, sv.len * n);
  while (n-- > 0) String_append_str(sb, sv);

  return *sb;
}

#define strforeach(c, s) listforeach(const char, c, s)

// returns itself
String str_to_upper(String* sb, str sv) {
  sb->len = 0;
  strforeach(c, &sv) {
    String_push(sb, c_to_upper(*c));
  }
  return *sb;
}

// returns itself
String str_to_lower(String* sb, str sv) {
  sb->len = 0;
  strforeach(c, &sv) {
    String_push(sb, c_to_lower(*c));
  }
  return *sb;
}

// returns itself
String String_to_upper(String* s) {
  listforeach(char, c, s) *c = c_to_upper(*c);
  return *s;
}

// returns itself
String String_to_lower(String* s) {
  listforeach(char, c, s) *c = c_to_lower(*c);
  return *s;
}

// returns itself
String str_replace(String* sb, str sv, str from, str to) {
  int match = str_match(sv, from);
  if (match == -1) return String_from_str(sv);

  sb->len = 0;
  String_append_str(sb, str_take(sv, match));
  String_append_str(sb, to);
  String_append_str(sb, str_skip(sv, match + from.len));
  return *sb;
}

// String str_replace_all(String* sb, str sv, str from, str to) {
//   IntList matches = str_match_all(sv, from);
  
//   sb->len = 0;
//   int last = 0;
//   listforeach(int, match, &matches) {
//     String_append_str(sb, str_slice(sv, last, *match));
//     String_append_str(sb, to);
//     last = *match + from.len;
//   }
//   String_append_str(sb, str_skip(sv, last));
//   IntList_free(&matches);

//   return *sb;
// }

// returns itself
String str_replace_all(String* sb, str sv, str from, str to) {
  StrMatches it = str_matches(sv, from);
  
  sb->len = 0;
  int last = 0;
  while (str_has_match(&it)) {
    int match = str_next_match(&it);
    String_append_str(sb, str_slice(sv, last, match));
    String_append_str(sb, to);
    last = match + from.len;
  }
  String_append_str(sb, str_skip(sv, last));

  return *sb;
}

// returns itself
String str_join(String* sb, str join, StrList strs) {
  sb->len = 0;
  if (strs.len == 0) { return *sb; }

  isize size = 0;
  listforeach(str, s, &strs) size += s->len;
  size += join.len * (strs.len-1);
  String_reserve(sb, size);

  for(isize i=0; i<strs.len-1; ++i) {
    String_append_str(sb, strs.data[i]);
    String_append_str(sb, join);
  }
  String_append_str(sb, *StrList_last(strs));

  return *sb;
}

#endif