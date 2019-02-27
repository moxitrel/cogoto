/* Labels as Values (GCC Extension)

* Drawbacks
- Use GCC extension.
- Slow.
- Avoid using local variables. (Local variables become invalid after co_yield())

* Internal
** Overview
    if (pc) {           //
        goto *pc;       // coroutine begin
    }                   //

    ...
    pc = &&yield_11;    //
    return;             // yield
yield_11:               //

    ...
    pc = &&yield_N;     //
    return;             // yield
yield_N:                //

** Source
void f(gen_t *co)
{
    co_begin(co);       // coroutine begin

    for (co->i = 0; ; co->i++) {
        co_yield(co);   // yield
    }

    co_end(co);         // coroutine end
}

** Expand Macro
void f(gen_t *co)
{
    if (co->pc) {           //
        goto *co->pc;       // co_begin(co);
    }                       //

    for (co->i = 0; ; co->i++) {

        pc = &&yield_11;    //
        return;             // co_yield(co);
    yield_11:;              //

    }

    pc = &&yield_end;       // co_end(co);
yield_end:;                 //
}

*/
#ifndef COGO_GEN_H
#define COGO_GEN_H

#ifndef assert
#   define assert(...)  /* nop */
#endif

// gen_t: generator context.
typedef struct {
    // start point where coroutine function continue to run after yield.
    const void *pc;

    //  0: inited
    // >0: running
    // <0: stopped (-1: success)
    int state;
} gen_t;

// gen_t.pc
#define GEN_PC(GEN)     (((gen_t *)(GEN))->pc)
// gen_t.state
#define GEN_STATE(GEN)  (((gen_t *)(GEN))->state)

// get the current running state
#define co_state(GEN)   GEN_STATE(GEN)


// co_begin(gen_t *);
#define co_begin(GEN, ...)                          \
do {                                                \
    const void *_pc = GEN_PC(GEN);                  \
    if (_pc) {                                      \
        goto *_pc;                                  \
    }                                               \
    GEN_STATE(GEN) = 1;                             \
} while (0)


// co_yield(gen_t *);
#define co_yield(GEN, ...)                                                                      \
do {                                                                                            \
    GEN_PC(GEN) = &&CO_LABEL(__LINE__); /* 1. save the restore point, at label CO_YIELD_N */    \
    goto CO_END;                        /* 2. return */                                         \
CO_LABEL(__LINE__):;                    /* 3. put label after each *return* as restore point */ \
} while (0)


// co_return();
#define co_return(...)                          \
do {                                            \
    goto CO_RETURN;         /* end coroutine */ \
} while (0)


// co_end(gen_t *);
#define co_end(GEN)                         \
do {                                        \
CO_RETURN:                                  \
    GEN_PC(GEN) = &&CO_END;                 \
    GEN_STATE(GEN) = -1;    /* finish */    \
CO_END:;                                    \
} while (0)


//
// Helper Macros
//

// Make goto label.
// e.g. CO_LABEL(13)       -> CO_YIELD_13
//      CO_LABEL(__LINE__) -> CO_YIELD_118
#define CO_LABEL(N)     CO_LABEL_(N)
#define CO_LABEL_(N)    CO_YIELD_##N

#endif //COGO_GEN_H
