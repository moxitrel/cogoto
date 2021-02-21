/* Use Duff's Device (Protothreads)

* API
CO_BEGIN      : coroutine begin label.
CO_END        : coroutine end label.
CO_YIELD      : yield from coroutine.
CO_RETURN     : return from coroutine.
CO_STATE(CO)  : get the current running state.
     0: inited
    >0: running
    -1: finished

* Example
void nat_func(nat_t* CO_THIS)
{
CO_BEGIN:

    for (CO_THIS->i = 0; ;CO_THIS->i++) {
        CO_YIELD;
    }

CO_END:;
}

* Internal
void nat_func(nat_t* CO_THIS)
{
    switch (CO_THIS->pc) {          // CO_BEGIN:
    case  0:                        //

        for (CO_THIS->i = 0; ;CO_THIS->i++) {

            CO_THIS->pc = 11;       //
            return;                 // CO_YIELD;
    case 11:;                       //

        }

    }                               // CO_END:
}

* Drawbacks
- No CO_YIELD allowed in *case* statement.

* See Also
- Coroutines in C   (https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html)
- Protothreads      (http://dunkels.com/adam/pt/expansion.html)

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
    //   0: inited
    //  >0: running
    //  -1: finished
    int cogo_pc;
} cogo_yield_t;

// cogo_yield_t.cogo_pc
// CO_THIS: point to coroutine object.
#define COGO_PC         (((cogo_yield_t*)(CO_THIS))->cogo_pc)

// get the current running state
#define CO_STATE(CO)    (((cogo_yield_t*)(CO))->cogo_pc)


#define CO_BEGIN                                        \
    switch (COGO_PC) {                                  \
    default:                /* invalid  pc      */      \
        COGO_ASSERT(((void)"cogo_pc isn't valid",0));   \
        goto cogo_exit;                                 \
    case -1:                /* coroutine end    */      \
        goto cogo_exit;                                 \
    case  0                 /* coroutine begin  */


#define CO_YIELD                                                                    \
    do {                                                                            \
        COGO_PC = __LINE__;     /* 1. save the restore point, at case __LINE__: */  \
        goto cogo_exit;         /* 2. return */                                     \
    case __LINE__:;             /* 3. restore point */                              \
    } while (0)


#define CO_RETURN                                       \
        goto cogo_return        /* end coroutine */     \


#define CO_END                                          \
    cogo_return:                                        \
        COGO_PC = -1;           /* finish */            \
    }                                                   \
    cogo_exit

#endif // MOXITREL_COGO_YIELD_IMPL_H_
