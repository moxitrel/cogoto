/* Labels as Values (GCC Extension)

* Example

#include "yield_label_value.h"

// nature number generator
typedef struct {
    cogo_yield_t    cogo_yield;
    int             i;
} nat_t;

void nat_func(nat_t* CO_THIS)
{
CO_BEGIN:

    for (CO_THIS->i = 0; ; CO_THIS->i++) {
        CO_YIELD;
    }

CO_END:;
}

* Internal
void nat_func(nat_t* CO_THIS)
{
    if (CO_THIS->cogo_pc) {     //
        goto *CO_THIS->cogo_pc; // CO_BEGIN:
    }                           //

    for (CO_THIS->i = 0; ; CO_THIS->i++) {

        cogo_pc = &&yield_11;   //
        return;                 // CO_YIELD;
    yield_11:;                  //

    }

    cogo_pc = &&yield_end;      // CO_END:
yield_end:;                     //
}

* Drawbacks
- Use GCC extension.

*/
#ifndef MOXITREL_COGO_YIELD_IMPL_H_
#define MOXITREL_COGO_YIELD_IMPL_H_

#ifdef assert
#   define COGO_ASSERT(...) assert(__VA_ARGS__)
#else
#   define COGO_ASSERT(...) /*nop*/
#endif

// yield context
typedef struct {
    // start point where coroutine function continue to run after yield.
    const void* cogo_pc;

    //  0: inited
    // >0: running
    // -1: finish successfully
    int cogo_state;
} cogo_yield_t;

// cogo_yield_t.cogo_pc
#define COGO_PC(CO)         (((cogo_yield_t*)(CO))->cogo_pc)

// cogo_yield_t.cogo_state
#define COGO_STATE(CO)      (((cogo_yield_t*)(CO))->cogo_state)

// get the current running state
#define CO_STATE(CO)        COGO_STATE(CO)


#define CO_BEGIN                                \
    if (COGO_STATE(CO_THIS) == 0) {             \
        COGO_PC(CO_THIS) = &&cogo_enter;        \
        COGO_STATE(CO_THIS) = __LINE__;         \
    }                                           \
    goto *COGO_PC(CO_THIS);                     \
cogo_enter


#define CO_YIELD                                                            \
    do {                                                                    \
        COGO_PC(CO_THIS) = &&COGO_LABEL;    /* 1. save restore point */     \
        goto cogo_exit;                     /* 2. return */                 \
    COGO_LABEL:;                            /* 3. restore point */          \
    } while (0)


#define CO_RETURN                                   \
    goto cogo_return            /* end coroutine */ \


#define CO_END                                      \
    cogo_return:                                    \
        COGO_PC(CO_THIS) = &&cogo_exit;             \
        COGO_STATE(CO_THIS) = -1;   /* finish */    \
    cogo_exit

//
// Helper
//

// Make goto label.
// e.g. COGO_LABEL(13)       -> cogo_yield_13
//      COGO_LABEL(__LINE__) -> cogo_yield_118
#define COGO_LABEL          COGO_LABEL_N(__LINE__)
#define COGO_LABEL_N(...)   COGO_LABEL_N_(__VA_ARGS__)
#define COGO_LABEL_N_(N)    cogo_yield_##N

#endif // MOXITREL_COGO_YIELD_IMPL_H_
