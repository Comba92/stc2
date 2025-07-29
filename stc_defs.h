#ifndef STC_DEFS_IMPL
#define STC_DEFS_IMPL

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

typedef char      byte;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef float     f32;
typedef double    f64;
typedef uintptr_t uptr;
typedef ptrdiff_t isize;
typedef size_t    usize;

#define ArrayLen(arr) (sizeof(arr) / sizeof(arr[0]))

#define IsBetween(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))
#define BitNth(x) (1 << (x))
#define BitIsSet(n, x) ((n) & BitNth(x) != 0)

#define FlagSet(n, f) ((n) |= (f))
#define FlagClear(n, f) ((n) &= -(f))
#define FlagToggle(n, f) ((n) ^= (f))

#define BitSetNth(n, b) FlagSet((n), BitNth((b)))
#define BitClearNth(n, b) FlagClear((n), BitNth((b)))
#define BitToggleNth(n, b) FlagToggle((n), BitNth((b)))

#define UNUSED(v) (void)(value)
#define TODO(msg) { fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, msg); abort(); }
#define UNREACHABLE(msg) { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, msg); abort(); }
#define ASSERT(cond, msg) assert((cond) && (msg));

// TODO: Logging

#endif