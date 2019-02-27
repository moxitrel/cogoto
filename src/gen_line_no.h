/*

* Drawbacks
- Must specify the correct line numbers (__LINE__) of co_yield().
- Avoid using local variables. (Local variables become invalid after co_yield())

* API  (!!! not type safe !!!)
- co_begin (gen_t *, ...) :: mark coroutine begin. List with line numbers of co_yield() and co_return().
- co_end   (gen_t *)      :: mark coroutine end.
- co_yield (gen_t *)      :: yield from coroutine.
- co_return(gen_t *)      :: return with ending coroutine.

- co_state(gen_t *) -> int  :: get the current running state.

* Usage

// 1. include header
#include "gen_line_no.h"

// 2. inherit gen_t (put in first)
typedef struct {
    gen_t gen_t;

    //
    // declare vars for coroutine function
    //
    int i;
    ...
} co_fun_t;

// 3. define a function with type "void (gen_t *)"
void co_fun(co_fun_t *co)
{
    //
    // before co_begin(), codes run every time when function called
    //
    
    // e.g. alias
    int *i = &co->i;

    // 4. mark coroutine begin
    co_begin(co,44,46);     // list line numbers (__LINE__) of co_yield()


    // 5. user codes (don't use local variables)
    for (*i = 0; ; (*i) ++) {
        co_yield(co);       // yield
    }
    co_yield(co);           // yield


    // 4. mark coroutine end
    co_end(co);

    //
    // after co_end(), codes run every time before return
    //
}


// 6. define constructor if needed
#define CO_FUN(...)  ((co_fun_t){...})

void example(void)
{
    co_fun_t co = CO_FUN(...);
    
    co_fun(&co);    // co.i = 0
    co_fun(&co);    // co.i = 1
    co_fun(&co);    // co.i = 2
}


* Internal

    switch (pc) {
    case  0: break;         // coroutine begin
    case 11: goto yield_11; // restore
    ...                     // restore
    case  N: goto yield_N;  // restore
    default: return;        // coroutine end
    }

    ...
    pc = 11;    //
    return;     // yield
yield_11:       //

    ...
    pc = N;     //
    return;     // yield
yield_N:        //

** Source
void f(gen_t *co)
{
    co_begin(co, 11);   // coroutine begin

    for (co->i = 0; ; co->i++) {
        co_yield(co);   // yield
    }

    co_end(co);         // coroutine end
}

** Expand Macro
void f(gen_t *co)
{
    switch (co->pc) {           //
    case  0: break;             //
    case 11: goto CO_YIELD_11;  // co_begin(co, 11);
    default: return;            //
    }                           //

    for (co->i = 0; ; co->i++) {

        co->pc = 11;            //
        return;                 // co_yield(co);
CO_YIELD_11:;                   //

    }

    pc = -1;                    // co_end(co);
}


* See Also
- Coroutines in C               (https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html)
- P99, advanced macro tricks    (http://p99.gforge.inria.fr/p99-html/index.html)

*/
#ifndef COGO_GEN_H
#define COGO_GEN_H

#ifndef assert
#   define assert(...)  /* nop */
#endif

// gen_t: generator context, must be inherited (as first field) by user-defined struct.
//
// e.g. typedef struct {
//          gen_t co;
//          ...
//      } user_defined_t;
typedef struct {
    // start point where coroutine function continue to run after yield.
    //   0: inited
    //  >0: running
    //  <0: stopped (-1: success)
    int pc;
} gen_t;

// gen_t.pc
#define GEN_PC(GEN)     (((gen_t *)(GEN))->pc)

// get the current running state
#define co_state(GEN)   GEN_PC(GEN)

// co_begin(gen_t *, ...): mark coroutine begin.
#define co_begin(GEN, ...)                              \
do {                                                    \
    switch (GEN_PC(GEN)) {                              \
    case  0:                /* coroutine begin  */      \
        break;                                          \
    case -1:                /* coroutine end    */      \
        goto CO_END;                                    \
 /* case  N:              *//* restore          */      \
 /*     goto CO_YIELD_N;  */                            \
    MAP(CASE_GOTO, __VA_ARGS__);                        \
    default:                /* invalid  pc      */      \
        assert(((void)"pc isn't valid.", 0));           \
        goto CO_END;                                    \
    }                                                   \
} while (0)

// co_yield(gen_t *): yield from the coroutine.
#define co_yield(GEN, ...)                                                              \
do {                                                                                    \
    GEN_PC(GEN) = __LINE__;     /* 1. save the restore point, at label CO_YIELD_N */    \
    goto CO_END;                /* 2. return */                                         \
CO_LABEL(__LINE__):;            /* 3. put label after each *return* as restore point */ \
} while (0)

// co_return(): end coroutine and return.
#define co_return(...)                                                                  \
do {                                                                                    \
    goto CO_RETURN;             /* return */                                            \
CO_LABEL(__LINE__):;            /* redundant label for co_begin() */                    \
} while (0)

// co_end(gen_t *): mark coroutine end.
#define co_end(GEN)                         \
do {                                        \
CO_RETURN:                                  \
    GEN_PC(GEN) = -1;       /* finish */    \
CO_END:;                                    \
} while (0)


//
// Helper Macros
//

// Generate switch case clause. (case N: goto CO_YIELD_N)
// e.g. CO_LABEL(13)        -> CO_YIELD_13
//      CASE_GOTO(__LINE__) -> case 118: goto CO_YIELD_118
#define CO_LABEL(N)     CO_LABEL_(N)
#define CO_LABEL_(N)    CO_YIELD_##N
#define CASE_GOTO(N)    case N: goto CO_LABEL(N)

// Count the number of arguments. (BUG)
// e.g. LEN(1)      -> 1
//      LEN(1,2)    -> 2
//      LEN(1,2,3)  -> 3
// BUG: LEN1()      -> 1, expect 0
#define LEN1(...)       ARG_PAT(__VA_ARGS__, LEN_PADDING)

#define ARG_PAT(...)    ARG_PAT_(__VA_ARGS__)
#define ARG_PAT_(                                           \
     _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10,       \
    _11, _12, _13, _14, _15, _16, _17, _18, _19,   N, ...)  N
#define LEN_PADDING                                         \
     19,  18,  17,  16,  15,  14,  13,  12,  11,  10,       \
      9,   8,   7,   6,   5,   4,   3,   2,   1,   0

// Count the number of arguments. (Solved the LEN1(...)'s BUG)
//
// e.g. LEN()       -> LEN_(0,1,1) -> LEN_01(1) -> 0
//      LEN(1)      -> LEN_(0,0,1) -> LEN_00(1) -> 1
//      LEN(1,2)    -> LEN_(1,1,2) -> LEN_11(2) -> 2
//      LEN(1,2,3)  -> LEN_(1,1,3) -> LEN_11(3) -> 3
//      LEN(1,2,...)-> LEN_(1,1,N) -> LEN_11(N) -> N
//
// See: https://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
//      http://p99.gforge.inria.fr/p99-html/p99__args_8h_source.html
//
#define LEN(...)                            \
LEN_(                                       \
    HAS_COMMA(__VA_ARGS__),                 \
    HAS_COMMA(GET_COMMA __VA_ARGS__ ()),    \
    LEN1(__VA_ARGS__)                       \
)
#define LEN_(D1, D2, N)     LEN_01N(D1, D2, N)
#define LEN_01N(D1, D2, N)  LEN_##D1##D2(N)
#define LEN_01(N)           0
#define LEN_00(N)           1
#define LEN_11(N)           N

#define HAS_COMMA_PADDING                                   \
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,       \
      1,   1,   1,   1,   1,   1,   1,   1,   0,   0
#define HAS_COMMA(...)      ARG_PAT(__VA_ARGS__, HAS_COMMA_PADDING)
#define GET_COMMA(...)      ,

//e.g. MAP(CASE_GOTO, 10, 20, 30)
//     -> MAP_3(CASE_GOTO, 10, 20, 30)
//     -> CASE_GOTO(10);
//        CASE_GOTO(20);
//        CASE_GOTO(30)
//     -> case 10: goto CO_LABEL(10);
//        case 20: goto CO_LABEL(20);
//        case 30: goto CO_LABEL(30)
#define MAP(F, ...)       MAP_N_(LEN(__VA_ARGS__), F, __VA_ARGS__)
#define MAP_N_(...)       MAP_N(__VA_ARGS__)
#define MAP_N(N, F, ...)  MAP_##N(F, __VA_ARGS__)
#define MAP_0( F, ...)
#define MAP_1( F, X, ...) F(X)
#define MAP_2( F, X, ...) F(X); MAP_1( F, __VA_ARGS__)
#define MAP_3( F, X, ...) F(X); MAP_2( F, __VA_ARGS__)
#define MAP_4( F, X, ...) F(X); MAP_3( F, __VA_ARGS__)
#define MAP_5( F, X, ...) F(X); MAP_4( F, __VA_ARGS__)
#define MAP_6( F, X, ...) F(X); MAP_5( F, __VA_ARGS__)
#define MAP_7( F, X, ...) F(X); MAP_6( F, __VA_ARGS__)
#define MAP_8( F, X, ...) F(X); MAP_7( F, __VA_ARGS__)
#define MAP_9( F, X, ...) F(X); MAP_8( F, __VA_ARGS__)
#define MAP_10(F, X, ...) F(X); MAP_9( F, __VA_ARGS__)
#define MAP_11(F, X, ...) F(X); MAP_10(F, __VA_ARGS__)
#define MAP_12(F, X, ...) F(X); MAP_11(F, __VA_ARGS__)
#define MAP_13(F, X, ...) F(X); MAP_12(F, __VA_ARGS__)
#define MAP_14(F, X, ...) F(X); MAP_13(F, __VA_ARGS__)
#define MAP_15(F, X, ...) F(X); MAP_14(F, __VA_ARGS__)
#define MAP_16(F, X, ...) F(X); MAP_15(F, __VA_ARGS__)
#define MAP_17(F, X, ...) F(X); MAP_16(F, __VA_ARGS__)
#define MAP_18(F, X, ...) F(X); MAP_17(F, __VA_ARGS__)
#define MAP_19(F, X, ...) F(X); MAP_18(F, __VA_ARGS__)

#endif // COGO_GEN_H
