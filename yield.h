/*

* API (macro, no type check)
CO_BEGIN      : ...
CO_END        : ...
CO_YIELD      : ...
CO_RETURN     : ...
CO_THIS       : ...
CO_STATE(CO)  : ...

CO_DECLARE(NAME, ...)   : declare a coroutine.
CO_DEFINE (NAME)        : define a declared coroutine which not defined.
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

// COGO_DECLARE(NAME, ...): define a coroutine named <NAME>
//
// * Example
//
// COGO_DECLARE(Point, cogo_yield_t co_gen, int x, int y, int z):
//
//  typedef struct {
//      cogo_yield_t co_yield;
//      int x;
//      int y;
//      int z;
//  } Point;
//  void Point_func(Point* CO_THIS)
//
#define COGO_COMMA_static               ,
#define COGO_COMMA_extern               ,
#define COGO_REMOVE_LINKAGE_static
#define COGO_REMOVE_LINKAGE_extern
#define COGO_DECLARE(NAME, ...)         COGO_DECLARE_N(COGO_ARG_COUNT(COGO_COMMA_##NAME), NAME, __VA_ARGS__)
#define COGO_DECLARE_N(...)             COGO_DECLARE_N1(__VA_ARGS__)
#define COGO_DECLARE_N1(N, ...)         COGO_DECLARE_N_##N(__VA_ARGS__)
#define COGO_DECLARE_N_1(NAME, ...)    /* NAME: Type */                     \
    typedef struct NAME NAME;                                               \
    struct NAME {                                                           \
        COGO_MAP(;, COGO_ID, __VA_ARGS__);                                  \
    };                                                                      \
    CO_DEFINE(NAME)
#define COGO_DECLARE_N_2(NAME, ...)    /* NAME: static Type */              \
    typedef struct COGO_REMOVE_LINKAGE_##NAME COGO_REMOVE_LINKAGE_##NAME;   \
    struct COGO_REMOVE_LINKAGE_##NAME {                                     \
        COGO_MAP(;, COGO_ID, __VA_ARGS__);                                  \
    };                                                                      \
    CO_DEFINE(COGO_REMOVE_LINKAGE_##NAME)

#define CO_DECLARE(NAME, ...)                                   \
    COGO_IFNIL(__VA_ARGS__)(                                    \
        COGO_DECLARE(NAME, cogo_yield_t cogo_gen),              \
        COGO_DECLARE(NAME, cogo_yield_t cogo_gen, __VA_ARGS__)  \
    )

#define CO_DEFINE(NAME)     \
    void NAME##_func(NAME* CO_THIS)

#endif // MOXITREL_COGO_YIELD_H_
