/*

* API (macro, no type check)
CO_BEGIN      : ...
CO_END        : ...
CO_YIELD      : ...
CO_RETURN     : ...
CO_STATE(CO)  : ...

CO_THIS                 : the parameter name of coroutine function.
CO_DECLARE(NAME, ...)   : declare a coroutine.
CO_DEFINE (NAME)        : define a declared coroutine which not defined.
CO_MAKE   (NAME, ...)   : coroutine maker.
NAME_func               : coroutine function name, made by CO_DECLARE(NAME), e.g. Nat_func

*/
#ifndef MOXITREL_COGO_YIELD_H_
#define MOXITREL_COGO_YIELD_H_

#if defined(__GNUC__)
#   include "yield_label_value.h"
#else
#   include "yield_case.h"
#endif

#include "utils.h"

// COGO_STRUCT(Type, T1 field1, ...): define a struct named <Type>
//
// * Example
// COGO_STRUCT(Point, int x, int y, int z) will be expanded into
//  typedef struct {
//      int x;
//      int y;
//      int z;
//  } Point
//
#define COGO_COMMA_static               ,
#define COGO_COMMA_extern               ,
#define COGO_REMOVE_LINKAGE_static
#define COGO_REMOVE_LINKAGE_extern
#define COGO_STRUCT(NAME, ...)          COGO_STRUCT1(COGO_ARG_COUNT(COGO_COMMA_##NAME), NAME, __VA_ARGS__)
#define COGO_STRUCT1(...)               COGO_STRUCT2(__VA_ARGS__)
#define COGO_STRUCT2(N, ...)            COGO_STRUCT_##N(__VA_ARGS__)
#define COGO_STRUCT_1(NAME, ...)        /* NAME: Type */                    \
    typedef struct NAME NAME;                                               \
    struct NAME {                                                           \
        COGO_MAP(;, COGO_ID, __VA_ARGS__);                                  \
    }
#define COGO_STRUCT_2(NAME, ...)        /* NAME: static Type */             \
    typedef struct COGO_REMOVE_LINKAGE_##NAME COGO_REMOVE_LINKAGE_##NAME;   \
    struct COGO_REMOVE_LINKAGE_##NAME {                                     \
        COGO_MAP(;, COGO_ID, __VA_ARGS__);                                  \
    }

#define COGO_DECLARE(NAME, BASE, ...)                                       \
    COGO_IFNIL(__VA_ARGS__)(                                                \
        COGO_STRUCT(NAME, BASE),                                            \
        COGO_STRUCT(NAME, BASE, __VA_ARGS__)                                \
    );                                                                      \
    CO_DEFINE(NAME)

#define CO_DEFINE(NAME)                 \
    void NAME##_func(void* CO_THIS)

#define CO_DECLARE(NAME, ...)           \
    COGO_DECLARE(NAME, cogo_yield_t cogo_yield, __VA_ARGS__)

#define CO_MAKE(NAME, ...)              \
    ((NAME){                            \
        .cogo_yield = {.cogo_pc = 0},   \
        __VA_ARGS__                     \
    })

#endif // MOXITREL_COGO_YIELD_H_
