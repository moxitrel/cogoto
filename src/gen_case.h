/* Use Duff's Device (Protothreads)

* Drawbacks
- Avoid using the *switch statement* in the coroutine function.
- Avoid using local variables. (Local variables become invalid after co_yield())

* Internal
** Overview
    switch (pc) {   //
    case  0:        // coroutine begin
        ...

        pc = 11;    //
        return;     // yield
    case 11:        //
        ...

        pc = N;     //
        return;     // yield
    case  N:        //
        ...

    }               // coroutine end

** Source
void f(gen_t *co)
{
    co_begin(co);       // coroutine begin

    for (co->i = 0; co->i < 9; co->i++) {
        co_yield(co);   // yield
    }

    co_end(co);         // coroutine end
}

** Expand Macro
void f(gen_t *co)
{
    switch (co->pc) {       //
    case  0:                // co_begin(co);

        for (co->i = 0; co->i < 9; co->i++) {

            co->pc = 11;    //
            return;         // co_yield(co);
    case 11:;               //

        }
    }                       // co_end(co);
}


* See Also
- Coroutines in C   (https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html)
- Protothreads      (http://dunkels.com/adam/pt/expansion.html)

*/
#ifndef COGO_GEN_H
#define COGO_GEN_H

#ifndef assert
#   define assert(...)  /* nop */
#endif

// generator context
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

// co_begin(gen_t *);
#define co_begin(GEN, ...)                              \
    switch (GEN_PC(GEN)) {                              \
    default:                /* invalid  pc      */      \
        assert(((void)"pc isn't valid.", 0));           \
        goto CO_END;                                    \
    case -1:                /* coroutine end    */      \
        goto CO_END;                                    \
    case  0:                /* coroutine begin  */      \

// co_yield(gen_t *);
#define co_yield(GEN, ...)                                                                  \
do {                                                                                        \
        GEN_PC(GEN) = __LINE__;     /* 1. save the restore point, at case __LINE__: */      \
        goto CO_END;                /* 2. return */                                         \
    case __LINE__:;                 /* 3. put case after each *return* as restore point */  \
} while (0)

// co_return();
#define co_return(...)                                  \
do {                                                    \
        goto CO_RETURN;     /* end coroutine */         \
} while (0)

// co_end(gen_t *);
#define co_end(GEN)                                     \
    CO_RETURN:                                          \
        GEN_PC(GEN) = -1;   /* finish successfully*/    \
    CO_END:;                                            \
    }

#endif // COGO_GEN_H
