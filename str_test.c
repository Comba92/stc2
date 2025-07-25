#include "stc_str.h"
#include <stdio.h>

#define res_dbg(s) printf("%d > " #s "\n", (int) (s));

static const char const_cstr[] = "Hello Worldie!";
static char static_cstr[] = "Hello Worldie!";


int main() {
  char buf[256] = "Hello Worldie!";
  char* cstr = "Hello Worldie!";
  const char puntcs[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  printf("Sizeof puntcs: %d\n", sizeof(puntcs));
  printf("Static const cstr: %d\tStatic cstr: %d\tStatic array: %d\tCstr: %d\n", sizeof(const_cstr), sizeof(static_cstr), sizeof(buf), sizeof(cstr));

  printf("Types sizes in bytes:\nchar = %lld\nshort = %lld\nint = %lld\nlong = %lld\nlong int %lld\nlong long %lld\nlong long int %lld\nsize_t %lld\nfloat %lld\ndouble %lld\n",
    sizeof(char),
    sizeof(short),
    sizeof(int),
    sizeof(long),
    sizeof(long int),
    sizeof(long long),
    sizeof(long long int),
    sizeof(size_t),
    sizeof(float),
    sizeof(double)
  );

  str s = str_from_cstr("Hello World!");
  printf("String of len %ld says %s\n", s.len, str_to_cstr(s));
  str_dbg(s);
  str_dbg(str_slice(s, 0, 5));
  str_dbg(str_slice(s, 0, 100));
  str_dbg(str_slice(s, 50, 100));

  // str_dbg(str_slice(s, 100, 5));
  res_dbg(str_eq(str_from_cstr("ass"), str_from_cstr("ass")));
  res_dbg(str_eq(str_from_cstr("ass"), str_from_cstr("as")));
  res_dbg(str_eq_ignorecase(str_from_cstr("ASS"), str_from_cstr("ass")));
  res_dbg(str_cmp(str_from_cstr("ass"), str_from_cstr("bass")));
  res_dbg(str_cmp(str_from_cstr("bass"), str_from_cstr("ass")));
  res_dbg(str_cmp(str_from_cstr("ass"), str_from_cstr("asshole")));
  res_dbg(str_cmp(str_from_cstr("ass"), str_from_cstr("ass")));


  res_dbg(str_find(s, 'W'));
  res_dbg(str_contains(s, 'W'));
  res_dbg(str_advance_while(s, c_is_upper));
  res_dbg(str_advance_rev_while(s, c_is_upper));
  
  printf("\n");

  res_dbg(str_match(s, str_from_cstr("World")));
  res_dbg(str_match(s, str_from_cstr("!")));
  res_dbg(str_match(s, str_from_cstr("Fuck")));
  res_dbg(str_match_all(s, str_from_cstr("l")).len);
  res_dbg(str_match_all(s, str_from_cstr("ll")).len);

  str_dbg(str_skip(s, 1));
  str_dbg(str_skip(s, 6));
  str_dbg(str_skip(s, s.len-1));
  str_dbg(str_skip(s, s.len));
  str_dbg(str_skip(s, s.len+1));

  str_dbg(str_skip_rev(s, 1));
  str_dbg(str_skip_rev(s, 6));
  str_dbg(str_skip_rev(s, s.len-1));
  str_dbg(str_skip_rev(s, s.len));

  str_dbg(str_skip_while(s, c_is_alpha));
  str_dbg(str_skip_rev_while(s, c_is_alpha));
  str_dbg(str_skip_while(s, c_is_digit));
  str_dbg(str_skip_rev_while(s, c_is_digit));

  str_dbg(str_skip_untilc(s, 'r'));
  str_dbg(str_skip_until(s, str_from_cstr("World")));

  printf("\n");

  str_dbg(str_take(s, 1));
  str_dbg(str_take(s, s.len-1));
  str_dbg(str_take(s, s.len));

  str_dbg(str_take_rev(s, 1));
  str_dbg(str_take_rev(s, s.len-1));
  str_dbg(str_take_rev(s, s.len));

  str_dbg(str_take_while(s, c_is_alpha));
  str_dbg(str_take_rev_while(s, c_is_alpha));
  str_dbg(str_take_while(s, c_is_digit));
  str_dbg(str_take_rev_while(s, c_is_digit));

  str_dbg(str_take_untilc(s, 'W'));
  str_dbg(str_take_until(s, str_from_cstr("World")));

  printf("\n");

  res_dbg(str_starts_with(s, str_from_cstr("FAG")));
  res_dbg(str_starts_with(s, str_from_cstr("Hello")));

  res_dbg(str_ends_with(s, str_from_cstr("FAG")));
  res_dbg(str_ends_with(s, str_from_cstr("World!")));
  res_dbg(str_ends_with(s, str_from_cstr("!")));

  str_dbg(str_trim(str_from_cstr("  d dsad das  ")));
  res_dbg(str_parse_int(str_from_cstr("1234")));
  res_dbg(str_parse_int(str_from_cstr("a1234")));
  res_dbg(str_parse_int(str_from_cstr("1234a")));

  // StrList ss1 = str_split(str_from_cstr("abc,,bcd,cde,defg"), str_from_cstr(","));
  StrList ss1 = str_splitc_collect(str_from_cstr("abc,,bcd,cde,defg"), ',');
  listforeach(str, s, &ss1) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss2 = str_lines_collect(str_from_cstr("abc\nbcd\ncde\ndefg"));
  listforeach(str, s, &ss2) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss3 = str_split_collect(str_from_cstr("abcbbccbcd"), str_from_cstr("bc"));
  listforeach(str, s, &ss3) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss4 = str_words_collect(str_from_cstr("This is the 100th Etext file presented by Project Gutenberg, and\n"
"is presented in cooperation with World Library, Inc., from their\n"
"Library  of  the Future and Shakespeare CDROMS.  Project Gutenberg\n"
"often releases Etexts that are NOT placed in the Public Domain!!     "));
  listforeach(str, s, &ss4) {
    str_dbg(*s);
  }
  printf("\n");

  String ls = {0};
  String_append_cstr(&ls, " kys faggot");
  str_dbg(ls);
  // ls.len = 0;
  String_append_str(&ls, str_from_cstr(", kys faggot"));
  str_dbg(ls);
  str_dbg(s);
  str_dbg(str_replace(&ls, s, SV("Hello"), SV("Bye")));
  str_dbg(str_replace(&ls, s, SV("Hello"), SV("ByeBye")));

  ls = SB("Hello World!");
  str_dbg(str_replace_all(&ls, s, SV("l"), SV(".")));
  ls = SB("Hello World!");
  str_dbg(str_replace_all(&ls, s, SV("ll"), SV(".")));

  ls = SB("Hello World!");
  str_dbg(str_to_upper(&ls, SBV(ls)));
  str_dbg(str_to_lower(&ls, SBV(ls)));

  str_dbg(str_concat(SB("hello "), SB("world...?")));
  str_dbg(str_repeat(&ls, SV("hello "), 5));

  StrList sl = {0};
  StrList_push(&sl, SV("ciao1"));
  StrList_push(&sl, SV("ciao2"));
  StrList_push(&sl, SV("ciao3"));

  str_dbg(str_join(&ls, SV(", "), sl));

  ls.len = 0;
  String_append_fmt(&ls, "Kys %d times stupid faggot %s", 100, "killer");
  str_dbg(ls);
  str_dbg(int_to_str(&ls, 20));
  str_dbg(float_to_str(&ls, 20.23));

  StrLines it1 = str_lines(SV("abc\nbcd\ncde\ndefg"));
  while (str_has_line(&it1)) {
    str line = str_next_line(&it1);
    str_dbg(line);
  }

  StrSplitChar it2 = str_splitc(SV("abc,,bcd,cde,defg"), ',');
  while (str_has_splitc(&it2)) {
    str split = str_next_splitc(&it2);
    str_dbg(split);
  }

  StrSplitWhen it3 = str_split_when(SV("abc,,bcd,cde,defg"), c_is_punct);
  while (str_has_split_when(&it3)) {
    str split = str_next_split_when(&it3);
    str_dbg(split);
  }

  String fmt = SB("Welcome back my friend to the show that never ends!");
  printf("Fmt contents: %s\n", fmt.data);
  printf("Test: %s\n", &fmt.data);
  String_append_fmt(&fmt, " Overwriting the text with fmt... %s", fmt.data);
  str_dbg(fmt);

  printf("Done\n");
}