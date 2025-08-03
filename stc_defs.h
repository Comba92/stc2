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

#define Swap(type, a, b) { type tmp = (a); (a) = (b), (b) = tmp; }

#define b2(x)   (   (x) | (   (x) >> 1) )
#define b4(x)   ( b2(x) | ( b2(x) >> 2) )
#define b8(x)   ( b4(x) | ( b4(x) >> 4) )
#define b16(x)  ( b8(x) | ( b8(x) >> 8) )  
#define b32(x)  (b16(x) | (b16(x) >>16) )

#define NextPowerOfTwo(x) (b32((x)-1) + 1)
#define IsPowerOfTwo(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

#define IsBetween(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))
#define BitNth(x) (1 << (x))
#define BitIsSet(n, x) ((n) & BitNth(x) != 0)

#define FlagSet(n, f) ((n) |= (f))
#define FlagClear(n, f) ((n) &= -(f))
#define FlagToggle(n, f) ((n) ^= (f))

#define BitSetNth(n, b) FlagSet((n), BitNth((b)))
#define BitClearNth(n, b) FlagClear((n), BitNth((b)))
#define BitToggleNth(n, b) FlagToggle((n), BitNth((b)))

// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html
// https://gcc.gnu.org/onlinedocs/gcc/Byte-Swapping-Builtins.html
// https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html

// TODO: these only work on GGC

#define LeadingZeros(x) __builtin_stdc_leading_zeros(x)
#define TrailingZeros(x) __builtin_stdc_trailing_zeros(x)
#define LeadingOnes(x) __builtin_stdc_leading_ones(x)
#define TrailingOnes(x) __builtin_stdc_trailing_ones(x)

#define FirstLeadingZero(x) __builtin_stdc_first_leading_zero(x)
#define FirstTrailingZero(x) __builtin_stdc_first_trailing_zero(x)
#define FirstLeadingOne(x) __builtin_stdc_first_leading_one(x)
#define FirstTrailingOne(x) __builtin_stdc_first_trailing_one(x)

#define CountZeros(x) __builtin_stdc_count_zeros(x)
#define CountOnes(x) __builtin_stdc_count_ones(x)
#define HasOnes(x) __builting_stdc_has_single_bit(x)

#define SwapBytes16(x) __builtin_swap16(x)
#define SwapBytes32(x) __builtin_swap32(x)
#define SwapBytes64(x) __builtin_swap64(x)

#define UNUSED(val) (void)(val)
#define TODO(msg) { fprintf(stderr, "%s:%d:%s() TODO: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, msg); abort(); }
#define UNREACHABLE(msg) { fprintf(stderr, "%s:%d:%s() UNREACHABLE: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, msg); abort(); }
#define ASSERT(cond, msg) { if(!(cond)) { fprintf(stderr, "%s:%d:%s() ASSERT: (" #cond ") %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, msg); abort(); }}

#define Thousands(v) ((v) * 1000LL)
#define Millions(v) (Thoudands(v) * 1000LL)
#define Billions(v) (Millions(v) * 1000LL)

#define Kilobytes(v) ((v) * 1024LL)
#define Megabytes(v) (Kilobytes(v) * 1024LL)
#define Gigabytes(v) (Megabytes(v) * 1024LL)

#define ToKilobytes(v) ((v) / 1024LL)
#define ToMegabytes(v) (ToKilobytes(v) / 1024LL)
#define ToGigabytes(v) (ToMegabytes(v) / 1024LL)

// TODO: Logging

#endif