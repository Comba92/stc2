#include "stc_str.h"
#include <stdio.h>

#define res_dbg(s) printf("%d > " #s "\n", (int) (s));

int main() {
  str s = str_from_cstr("Hello World!");
  printf("String of len %lld says %s\n", s.len, str_to_cstr(s));
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
  res_dbg(str_advance_while(s, isupper));
  res_dbg(str_advance_rev_while(s, isupper));
  
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

  str_dbg(str_skip_while(s, isalpha));
  str_dbg(str_skip_rev_while(s, isalpha));
  str_dbg(str_skip_while(s, isdigit));
  str_dbg(str_skip_rev_while(s, isdigit));

  str_dbg(str_skip_until_char(s, 'r'));
  str_dbg(str_skip_until_match(s, str_from_cstr("World")));

  printf("\n");

  str_dbg(str_take(s, 1));
  str_dbg(str_take(s, s.len-1));
  str_dbg(str_take(s, s.len));

  str_dbg(str_take_rev(s, 1));
  str_dbg(str_take_rev(s, s.len-1));
  str_dbg(str_take_rev(s, s.len));

  str_dbg(str_take_while(s, isalpha));
  str_dbg(str_take_rev_while(s, isalpha));
  str_dbg(str_take_while(s, isdigit));
  str_dbg(str_take_rev_while(s, isdigit));

  str_dbg(str_take_until_char(s, 'W'));
  str_dbg(str_take_until_match(s, str_from_cstr("World")));

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
  StrList ss1 = str_split_char(str_from_cstr("abc,,bcd,cde,defg"), ',');
  listforeach(str, s, &ss1) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss2 = str_lines(str_from_cstr("abc\nbcd\ncde\ndefg"));
  listforeach(str, s, &ss2) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss3 = str_split(str_from_cstr("abcbbccbcd"), str_from_cstr("bc"));
  listforeach(str, s, &ss3) {
    str_dbg(*s);
  }
  printf("\n");

  StrList ss4 = str_words(str_from_cstr("This is the 100th Etext file presented by Project Gutenberg, and\n"
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
  String_append_str(&ls, str_from_cstr(" kys faggot"));
  str_dbg(ls);

  str_dbg(str_replace(&ls, s, str_from_cstr("Hello"), str_from_cstr("Bye")));
  str_dbg(str_replace(&ls, s, str_from_cstr("Hello"), str_from_cstr("ByeBye")));

  str_dbg(str_replace_all(&ls, s, str_from_cstr("l"), str_from_cstr("Bye")));
  str_dbg(str_replace_all(&ls, s, str_from_cstr("l"), str_from_cstr("ByeBye")));

  str_dbg(str_to_upper(&ls, String_to_str(ls)));
  str_dbg(str_to_lower(&ls, String_to_str(ls)));

  str_dbg(str_concat(String_from_cstr("hello "), String_from_cstr("world...?")));
  str_dbg(str_repeat(&ls, str_from_cstr("hello "), 5));

  StrList sl = {0};
  StrList_push(&sl, str_from_cstr("ciao1"));
  StrList_push(&sl, str_from_cstr("ciao2"));
  StrList_push(&sl, str_from_cstr("ciao3"));

  str_dbg(str_join(&ls, str_from_cstr(", "), sl));

  str_dbg(String_format(&ls, "Kys %d times stupid faggot %s", 100, "killer"));
  str_dbg(int_to_str(&ls, 20));
  str_dbg(float_to_str(&ls, 20.23));

  iterfor(it, str_from_cstr("abc\nbcd\ncde\ndefg")) {
    str line = str_iter_line(&it);
    str_dbg(line);
  }

  iterfor(it, str_from_cstr("abc,,bcd,cde,defg")) {
    str split = str_iter_split(&it, str_from_cstr(","));
    str_dbg(split);
  }

  printf("Done\n");
}