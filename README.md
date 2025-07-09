# STC
// TODO: libraries introduction
https://en.wikipedia.org/wiki/Generic_programming

# Modules
## List
Generic [Heap-allocated Dynamic Array](https://en.wikipedia.org/wiki/Dynamic_array).
To use it, use list_def() outside any function to define your generic type.
Its content is accessed with the .data field.
When full, will be reallocated with double the capacity. It never shrinks down, only up.
All of its methods are type safe (no use of void* pointers).
No constructor is provided; always declare a list with the zero initializer  `= {0}`;
```c
list_def(int, IntList)
list_def(char*, CstrList)

int main() {
  IntList l1 = {0};
  CstrList l2 = {0};

  for(int i=0; i < 10; ++i) {
    IntList_push(&l1, i);
  }

  for(int i=0; i < l1.len; ++i) {
    printf("%d\n", l1.data[i]);
  }
  ...
}
```

### Structs
#### `List`
```c
typedef struct {
  size_t len; // how many elements have been pushed
  size_t cap; // how many elements are actually allocated
  T* data;    // pointer to elements array
} List;
```

### Macros
#### `list_def(type, name)`
Generates code for a dynamic array type containing *type* values, named as *name*. The *name* will be appended at the start of all its methods.
Example:
```c
list_def(MyType, MyListName)

int main() {
  MyListName l = {0};
  MyListName_push(&l, 69);
  ...
}
```

#### `rangefor(type, it, start, end)`
Shortand for a ranged loop.
```c
rangefor(int, it, 5, 10) { ... }
// Expands to:
for(int it=5; it<10; ++it) { ... }
```

#### `listfor(type, it, list)`
Shortand for a list indexes loop.
```c
listfor(int, it, &l) { ... }
// Expands to:
for (int it = 0; it < l->len; ++it) { ... }
```
#### `listforrev(type, it, list)`
Shortand for a list indexes loop reversed (starting from the back).
```c
listfor(int, it, &l) { ... }
// Expands to:
for (int it = l->len-1; it >= 0; --it) { ... }
```

#### `listforeach(type, it, list)`
Shortand for a list iterator loop. *it* is a pointer to the current iterated value.
```c
listforeach(float, it, &float_l) { ... }
// Expands to:
for (float* it = float_l->data; it < float_l->data + float_l->len; ++it)
```

### Functions
The prefix *list_* is appended based on the list name you provided.
For example, if you name a list *MyList*, all of its methods will start with *MyList_*, like *MyList_push()*.

#### `void list_push(List* l, T value)`
Pushes *value* to the back of *l*'s **data**, and increases **len** by 1.

#### `T list_pop(List* l)`
Pops out the last value (at the back of **data**), returns it, and decreases **len** by 1.

#### `T* list_first(List l)`
Returns a pointer to the first element in **data**.
If **len** is 0, this function panics.

#### `T* list_last(List l)`
Returns a pointer to the last element in **data**.
If **len** is 0, this function panics.

#### `T* list_swap(List* l, size_t a, size_t b)`
Swaps the values at indexes *a* and *b*. If the indexes are bigger than **len**, this function panics.

#### `void list_remove_swap(List* l, size_t i)`
Removes the value at index *i* by swapping it with the value at the back of **data**, then decreases **len** by 1.
This doesn't shift any other element, unlike the usual 'array remove', at the cost of not mantaining the values order.
If **len** is 0, this function panics.

#### `void list_append(List* this, List other)`
Appends all elements of *other* at the back of *this*, increasing *this*'s **len** accordingly.
This uses [memcpy()](https://en.cppreference.com/w/cpp/string/byte/memcpy.html) for fast copying.

#### `void list_append_array(List* this, T* arr, size_t len)`
Appends elements from pointer *arr** up until *len*, increasing *this*'s **len** accordingly.
This uses memcpy() for fast copying.

#### `List list_from_array(T* arr, size_t len)`
Returns a newly allocated list taking elements from pointer **arr** up until **len**.
This uses memcpy() for fast copying.

#### `List list_clone(List l)`
Returns a newly allocated list by deep copying *l*.
This uses memcpy() for fast copying.

#### `void list_shuffle(List* l)`
Shuffles the values of *l* randomly, using [Knuth's algorithm](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle). The algorithm has O(n) complexity.
It uses libc's [rand()](https://man7.org/linux/man-pages/man3/rand.3.html) as RNG, so it might not have good statistical properties.

#### `int list_find(List* l, T value, bool (\*pred)(const T\* a, const T\* b))`
Searches for *value* in *l*, using the comparison function *pred*. If found, returns its index, otherwise -1.

#### `void list_filter(List* l, bool (\*pred)(const T\* val))`
Filters *l* in place, keeping only the elements accepted by *pred*. **len** is set accordingly.


#### `void list_reserve(List* l, size_t new_cap)`
If *new_cap* is bigger than *l*'s **cap**, reallocates its **data** to contain at least *new_cap* elements. The new capacity **cap** is always guaranteed to be a mutliple of two.
Otherwise, does nothing.

#### `void list_resize(List* l, size_t new_len, T value)`
Sets *l*'s **len** as *new_len*.
If **new_size** is bigger, the added space will be set as *value*. Otherwise, **len** is just shrinked, and elements beyond **len** are lost.

#### `void list_free(List* l)`
Frees the memory allocated for *l*'s **data**, sets **data** to NULL, and sets **len** and **cap** to 0.
> [!WARNING]  
> This function only frees memory for **data**, and not for its elements. If your list contains pointers to elements, you should take care to free each element's pointer yourself, or incurr in a memory leak.

## String

### Structs
```c
typedef struct {
  size_t len;       // how many characters in string
  const char* data; // pointer to string. may be any kind of string, but it is read-only for str
} str;

typedef struct {
  size_t len; // how many characters have been pushed
  size_t cap; // how many characters are actually allocated
  char* data; // pointer to heap-allocated string
} String;
```

### Macros
str_fmt
str_arg(s)
str_dbg(s)
SV(cstr)
SB(cstr)
SBV(sb)
STR_EMPTY
str_iter_done(it)
str_iter(s, next, it)
strforeach(c, s)

### Functions
bool c_is_space(char c)
bool c_is_alpha(char c)
bool c_is_digit(char c)
bool c_is_alphanum(char c)
bool c_is_punct(char c)
bool c_is_lower(char c)
bool c_is_upper(char c)
bool c_is_cntrl(char c)
char c_to_lower(char c)
char c_to_upper(char c)

char* str_to_cstr(str s)
str str_clone(str s) {
str str_from_cstr(const char* s) {
str str_from_cstr_unchecked(const char* s, size_t len) bool str_is_empty(str s) {
bool str_is_empty(str s) {
str str_slice(str s, size_t start, size_t end) {
str cstr_slice(const char* c, size_t start, size_t end) {
bool str_eq(str a, str b) {
bool str_eq_ignorecase(str a, str b) {
int str_cmp(str a, str b) {
int str_find(str s, char c) {
int str_find_rev(str s, char c) {
bool str_contains(str s, char c) {

int str_match(str s, str target) {
IntList str_match_all(str s, str target) {

str str_skip(str s, size_t n) {
str str_skip_rev(str s, size_t n) {

str str_take(str s, size_t n) {
str str_take_rev(str s, size_t n) {

str str_skip_untilc(str s, char c) {
str str_skip_rev_untilc(str s, char c) {
str str_skip_until(str s, str target) {
str str_skip_while(str s, CharPredicate p) {
str str_skip_rev_while(str s, CharPredicate p) {

str str_take_untilc(str s, char c) {
str str_take_rev_untilc(str s, char c) {
str str_take_until(str s, str target) {
str str_take_while(str s, CharPredicate p) {
str str_take_rev_while(str s, CharPredicate p) {

int str_advance_while(str s, CharPredicate p) {
int str_advance_while_not(str s, CharPredicate p) {
int str_advance_rev_while(str s, CharPredicate p) {

bool str_starts_with(str s, str start) {
bool str_ends_with(str s, str end) {

str str_strip_prefix(str s, str prefix) {
str str_strip_postfix(str s, str postfix) {

str str_trim_start(str s) {
str str_trim_end(str s) {
str str_trim(str s) {

StrList str_splitc_collect(str s, char c) {
StrList str_split_collect(str s, str pattern) {
StrList str_split_when_collect(str s, CharPredicate pred) {
StrList str_lines_collect(str s) {
StrList str_words_collect(str s) {

StrMatches str_matches(str s, str target) {
int str_next_match(StrMatches* it) {

StrSplitChar str_splitc(str s, char c) {
str str_next_splitc(StrSplitChar* it) {

StrSplit str_split(str s, str pattern) {
str str_next_split(StrSplit* it) {

StrSplitWhen str_split_when(str s, CharPredicate pred) {
str str_next_split_when(StrSplitWhen* it) {

StrLines str_lines(str s) {
str str_next_line(StrLines* it) {

StrWords str_words(str s) {
str str_next_word(StrWords* it) {


str String_to_str(String sb) {
void String_append_null(String* sb) {
void String_append_cstr(String* sb, const char* s) {
void String_append_str(String* sb, str sv) {
String String_from_cstr(const char* s) {
String String_from_str(str s) {
char* String_to_cstr(String sb) {

char* str_fmt_tmp(const char* fmt, ...) {
String String_fmt(String* sb, const char* fmt, ...) {

int str_parse_int(str s) {
double str_parse_float(str s) {
String int_to_str(String* sb, int n) {
String float_to_str(String* sb, double n) {

String str_concat(String a, String b) {
String str_repeat(String* sb, str sv, size_t n) {
String str_to_upper(String* sb, str sv) {
String str_to_lower(String* sb, str sv) {
String String_to_upper(String* s) {
String str_replace(String* sb, str sv, str from, str to) {
String str_replace_all(String* sb, str sv, str from, str to) {
String str_join(String* sb, str join, StrList strs) {



## Map
Generic [hash table](https://en.wikipedia.org/wiki/Hash_table) with string keys. 
To use it, use map_def() outside any function to define your generic type.
Its content should accessed with the relative generic iterator.
Unlike List, accessing .data field directly is not reccomended.
Uses [djb2](https://theartincode.stanis.me/008-djb2/) algorithm to hash keys. Uses [quadratic probing](https://en.wikipedia.org/wiki/Quadratic_probing) [open addressing](https://en.wikipedia.org/wiki/Open_addressing) to handle collisions.
When full, will be reallocated with double the capacity. It never shrinks down, only up.

A [Bit Set](https://en.wikipedia.org/wiki/Bit_array) with string keys is also avaible.

### Structs
```c
typedef struct {
  str key;
  T val;
} MapEntry;

typedef struct {
  size_t len;
  size_t cap;
  MapEntry* entries;
} Map;

typedef struct {
  const Map* src;
  MapEntry* curr;
  size_t skipped;
} MapIter;
```

```c
typedef struct {
  size_t len;
  size_t cap;
  str* keys;
  char* bits;
} Set;

typedef struct {
  const Set* src;
  str* curr;
  size_t skipped;
} SetIter;
```

### Macros
#### `map_def(type, name)`
#### `map_iter(type, ent, it)`
#### `set_iter(ent, it)`

### Functions
#### `MapEntry* map_search(const Map* m, str key)`
#### `T* map_get(const Map* m, str key)`
#### `bool map_contains(const Map* m, str key)`
#### `bool map_insert(Map* m, str key, T val)`
#### `bool map_remove(Map* m, str key)`
#### `void map_reserve(Map* m, size_t new_cap)`
#### `void map_clear(Map* m)`
#### `void map_free(Map* m)`
#### `bool map_key_is_marker(str key)`

#### `MapIter map_iter(const Map* m)`
#### `MapEntry* map_next(MapIter* it)`

#### `int set_search(const Set* s, str key)`
#### `bool set_contains(const Set* s, str key)`
#### `bool set_insert(Set* s, str key)`
#### `bool set_remove(Set* s, str key)`
#### `int set_reserve(Set* s, size_t new_cap)`
#### `void set_clear(Set* s)`
#### `void set_free(Set* s)`

#### `SetIter set_iter(const Set* s)`
#### `str* set_next(SetIter* it)`

#### `bool set_is_superset(const Set* this, const Set* other)`
#### `bool set_is_subset(const Set* this, const Set* other)`
#### `bool set_is_disjoint(const Set* this, const Set* other)`

## Path
### Functions
#### `bool path_exists(const char* path)`
#### `char* path_last_component(const char* path) `
#### `char* path_filename(const char* path)`
#### `char* path_extension(const char* path)`
#### `str path_filename_no_ext(const char* path)`
#### `str path_parent(const char* path)`
#### `char* path_to_absolute(const char* path)`
#### `str path_prefix(const char* path)`
#### `bool path_is_absolute(const char * path)`
#### `bool path_is_relative(const char* path)`
#### `StrList path_components(const char* path)`
#### `void path_push(String* sb, const char* path)`
#### `bool path_pop(String* sb)`
#### `void path_set_filename(String* sb, const char* filename)`
#### `bool path_set_extension(String* sb, const char* extension)`

## FS (Filesystem)
### Macros
#### `STC_LOG_ERR`
#### `PATH_MAX_LEN`
#### `dir_iter(ent, it)`
### Structs
```c
typedef enum {
  FileType_Other,
  FileType_File,
  FileType_Dir,
  FileType_Link,
} FileType;

typedef struct {
  char* name;
  FileType type;
} DirEntry;

typedef struct {
  // OS dependant data
  bool failed;
  DirEntry curr;
} DirIter;
```
### Functions

#### `char* file_read_to_string(const char* path)`
#### `bool file_write_bytes(const char* path, const char* data, size_t len)`

#### `FileType file_type(const char* path)`
#### `size_t file_size(const char* path)`

#### `DirIter dir_open(const char* dirpath)`
#### `DirEntry* dir_read(DirIter* it)`
#### `bool dir_close(DirIter* it)`

#### `DirEntries dir_entries(const char* dirpath)`
#### `DirEntries dir_entries_sorted(const char* dirpath)`
#### `void DirEntries_drop(DirEntries* entries)`

#### `bool file_exists(const char* path)`
#### `bool file_create(const char* path, bool overwrite)`
#### `bool file_create_recursive(const char* path, bool overwrite)`
#### `bool file_copy(const char* src, const char* dst, bool overwrite)`
#### `bool file_move(const char* src, const char* dst)`
#### `bool file_move_src_if_dst_not_exists(const char* src, const char* dst)`
#### `bool file_delete(const char* src)`

#### `char* dir_current()`
#### `bool dir_set_current(const char* path)`

#### `bool dir_exists(const char* path)`
#### `bool dir_create(const char* path)`
#### `bool dir_create_recursive(const char* path)`
#### `bool dir_copy_recursive(const char* src, const char* dst)`
#### `bool dir_move(const char* src, const char* dst)`
#### `bool dir_delete(const char* path)`
#### `bool dir_delete_recursive(const char* path)`

#### `int fs_err_code()`
#### `char* fs_err_msg()`
#### `void fs_err_print(const char* msg)`