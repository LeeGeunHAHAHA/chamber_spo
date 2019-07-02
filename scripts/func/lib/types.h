#ifndef _TYPES_H_
#define _TYPES_H_

#include <limits.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER)								((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#ifndef countof
#define countof(x)              (sizeof(x) / sizeof(x[0]))
#endif

//--------------------------------------------------------------
// Types
//--------------------------------------------------------------
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long        s64;
typedef signed long             slong;
#if 0
typedef signed __int128         s128;
#else
typedef struct { s8 v[16];}     s128;
#endif

typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long      u64;
typedef unsigned long           ulong;
#if 0
typedef unsigned __int128       u128;
#else
typedef struct { u8 v[16];}     u128;
#endif

typedef enum boolean            bool;
enum boolean {
    false   = 0,
    true    = 1,
};

#define S8_MIN                  (SCHAR_MIN)
#define S8_MAX                  (SCHAR_MAX)
#define S16_MIN                 (SHRT_MIN)
#define S16_MAX                 (SHRT_MAX)
#define S32_MIN                 (INT_MIN)
#define S32_MAX                 (INT_MAX)
#define S64_MIN                 (LLONG_MIN)
#define S64_MAX                 (LLONG_MAX)

#define U8_MIN                  (UCHAR_MIN)
#define U8_MAX                  (UCHAR_MAX)
#define U16_MIN                 (USHRT_MIN)
#define U16_MAX                 (USHRT_MAX)
#define U32_MIN                 (UINT_MIN)
#define U32_MAX                 (UINT_MAX)
#define U64_MIN                 (ULLONG_MIN)
#define U64_MAX                 (ULLONG_MAX)

#define PACKED                  __attribute__((packed))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define DIV_ROUND_UP(n, d)      (((n) + (d) - 1) / (d))
#define ALIGN(x, a)             __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))

#endif	// _TYPES_H_

