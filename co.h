/*

* API
CO_BEGIN                : ...
CO_END                  : ...
CO_YIELD                : ...
CO_RETURN               : ...
CO_THIS                 : ...
CO_STATE  (CO)          : ...
CO_DECLARE(NAME, ...)   : ...
CO_DEFINE (NAME)        : ...

CO_AWAIT(cogo_co_t*)    : call another coroutine.
CO_START(cogo_co_t*)    : run a new coroutine concurrently.

cogo_co_t                               : coroutine type, should be inherited by user.
cogo_sch_t                              : sheduler  type, should be inherited by user.
cogo_co_t* cogo_sch_step(cogo_sch_t*)   : run the current coroutine until yield or finished, return the next coroutine to be run.

* APIs should be implemented by user

// Push a coroutine to the running queue.
COGO_INLINE int cogo_sch_push(cogo_sch_t*, cogo_co_t*);

// Pop a coroutine to be run.
COGO_INLINE cogo_co_t* cogo_sch_pop(cogo_sch_t*);

*/
#ifndef MOXITREL_COGO_CO_H_
#define MOXITREL_COGO_CO_H_

#include "yield.h"

#ifdef __cplusplus
#   define COGO_INLINE inline
#else
#   define COGO_INLINE inline static
#endif

typedef struct cogo_co      cogo_co_t;      // coroutine
typedef struct cogo_sch     cogo_sch_t;     // scheduler

// support call stack, concurrency
struct cogo_co {
    // inherit cogo_yield_t
    cogo_yield_t cogo_yield;

    // the coroutine function
    void (*func)(cogo_co_t*);

    // build call stack, !!!REQUIRE NO LOOP EXISTS!!!
    cogo_co_t* caller;

    // scheduler, updated by cogo_sch_step()
    cogo_sch_t* sch;
};

// cogo_co_t scheduler
struct cogo_sch {
    // the coroutine run by scheduler
    cogo_co_t* stack_top;
};

// push a coroutine into queue
// return !0 will make scheduler switching to the next coroutine immediately
COGO_INLINE int cogo_sch_push(cogo_sch_t*, cogo_co_t*);

// pop the next coroutine to be run
COGO_INLINE cogo_co_t* cogo_sch_pop(cogo_sch_t*);

//
// cogo_co_t
//

// CO_AWAIT(cogo_co_t*): call another coroutine.
#define CO_AWAIT(CALLEE)                                            \
do {                                                                \
    cogo_co_await((cogo_co_t*)(CO_THIS), (cogo_co_t*)(CALLEE));     \
    CO_YIELD;                                                       \
} while (0)
COGO_INLINE void cogo_co_await(cogo_co_t* thiz, cogo_co_t* callee)
{
    COGO_ASSERT(thiz);
    COGO_ASSERT(thiz->sch);
//  COGO_ASSERT(thiz->sch->stack_top == thiz);
    COGO_ASSERT(callee);
//  COGO_ASSERT(!callee->caller);
    COGO_ASSERT(thiz != callee);

    // NOTE: no loop check
    // call stack push
    callee->caller = thiz->sch->stack_top;
    thiz->sch->stack_top = callee;
}

// CO_START(cogo_co_t*): add a new coroutine to the scheduler.
#define CO_START(CO)                                                    \
do {                                                                    \
    if (cogo_co_start((cogo_co_t*)(CO_THIS), (cogo_co_t*)(CO)) != 0) {  \
        CO_YIELD;                                                       \
    }                                                                   \
} while (0)
COGO_INLINE int cogo_co_start(cogo_co_t* thiz, cogo_co_t* co)
{
    COGO_ASSERT(thiz);
    COGO_ASSERT(thiz->sch);
    COGO_ASSERT(co);
    return cogo_sch_push(thiz->sch, co);
}

//
// cogo_sch_t
//

// run the coroutine in stack top until yield or finished, return the next coroutine to be run.
COGO_INLINE cogo_co_t* cogo_sch_step(cogo_sch_t* sch)
{
    COGO_ASSERT(sch);
    while (sch->stack_top) {
        sch->stack_top->sch = sch;
        sch->stack_top->func(sch->stack_top);
        if (!sch->stack_top) {
            // blocked
            break;
        }
        if (CO_STATE(sch->stack_top) > 0) {
            // yield
            cogo_sch_push(sch, sch->stack_top);
            break;
        }
        if (CO_STATE(sch->stack_top) == 0) {
            // await
            continue;
        }
        if (CO_STATE(sch->stack_top) == -1) {
            // return
            sch->stack_top = sch->stack_top->caller;
            continue;
        }
        COGO_ASSERT(!"unexpected case");
        break;  // discard the coroutine
    }
    return sch->stack_top = cogo_sch_pop(sch);
}

#undef CO_DECLARE
#define CO_DECLARE(NAME, ...)                                   \
    COGO_IFNIL(__VA_ARGS__)(                                    \
        COGO_DECLARE(NAME, cogo_co_t cogo_co),                  \
        COGO_DECLARE(NAME, cogo_co_t cogo_co, __VA_ARGS__)      \
    )

#undef CO_MAKE
#define CO_MAKE(NAME, ...)                                      \
    ((NAME){                                                    \
        {.func = (void(*)(cogo_co_t*))(NAME##_func)},           \
        __VA_ARGS__                                             \
    })

#endif // MOXITREL_COGO_CO_H_